#include "user.h"

#include <QByteArray>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>

#include <sstream>
#include <stdexcept>
#include <cctype>

namespace hms
{
    namespace
    {
        struct PasswordFlags
        {
            bool upper = false;
            bool lower = false;
            bool digit = false;
            bool special = false;
        };

        PasswordFlags classifyPassword(const std::string &password,
                                       std::size_t index,
                                       PasswordFlags flags)
        {
            if (index >= password.size())
                return flags;

            const unsigned char c = static_cast<unsigned char>(password[index]);
            if (std::isupper(c)) flags.upper = true;
            else if (std::islower(c)) flags.lower = true;
            else if (std::isdigit(c)) flags.digit = true;
            else if (!std::isalnum(c)) flags.special = true;

            return classifyPassword(password, index + 1, flags);
        }

        constexpr int kIterations = 48000;
        constexpr int kSaltBytes = 16;
        constexpr int kDerivedKeyBytes = 32;

        QByteArray toHex(const QByteArray &raw)
        {
            return raw.toHex();
        }

        QByteArray fromHex(const std::string &hex)
        {
            return QByteArray::fromHex(QByteArray::fromStdString(hex));
        }

        QByteArray generateSalt()
        {
            QByteArray salt(kSaltBytes, Qt::Uninitialized);

            for (int i = 0; i < kSaltBytes; ++i)
            {
                salt[i] = static_cast<char>(
                    QRandomGenerator::global()->bounded(256)
                );
            }

            return salt;
        }

        /*
         * PBKDF2-HMAC-SHA256
         *
         * This replaces QPasswordDigestor, which is not available
         * in the Qt version being used by this project.
         */
        QByteArray deriveKeyPbkdf2(
            const QByteArray &password,
            const QByteArray &salt,
            int iterations,
            int keyLength)
        {
            if (iterations <= 0 || keyLength <= 0)
                return QByteArray();

            constexpr int hashLength = 32; // SHA-256 = 32 bytes

            const int blockCount =
                (keyLength + hashLength - 1) / hashLength;

            QByteArray derived;

            for (int block = 1; block <= blockCount; ++block)
            {
                /*
                 * INT(block) in big-endian format.
                 */
                QByteArray blockIndex(4, Qt::Uninitialized);

                blockIndex[0] =
                    static_cast<char>((block >> 24) & 0xFF);

                blockIndex[1] =
                    static_cast<char>((block >> 16) & 0xFF);

                blockIndex[2] =
                    static_cast<char>((block >> 8) & 0xFF);

                blockIndex[3] =
                    static_cast<char>(block & 0xFF);

                /*
                 * U1 = HMAC-SHA256(password, salt || INT(block))
                 */
                QByteArray u =
                    QMessageAuthenticationCode::hash(
                        salt + blockIndex,
                        password,
                        QCryptographicHash::Sha256
                    );

                /*
                 * T = U1 initially.
                 */
                QByteArray t = u;

                /*
                 * U_i = HMAC-SHA256(password, U_(i-1))
                 *
                 * T = U1 XOR U2 XOR ... XOR U_iterations
                 */
                for (int i = 1; i < iterations; ++i)
                {
                    u =
                        QMessageAuthenticationCode::hash(
                            u,
                            password,
                            QCryptographicHash::Sha256
                        );

                    for (int j = 0; j < t.size(); ++j)
                    {
                        t[j] = static_cast<char>(
                            static_cast<unsigned char>(t[j]) ^
                            static_cast<unsigned char>(u[j])
                        );
                    }
                }

                derived.append(t);
            }

            return derived.left(keyLength);
        }
    }


    // ============================================================
    // Constructor - new user
    // ============================================================

    User::User(const std::string &username,
               const std::string &password,
               const std::string &role,
               AccountStatus status)
        : username_(username),
          role_(role),
          status_(status)
    {
        if (!isPasswordStrong(password))
        {
            throw std::invalid_argument(
                passwordStrengthMessage(password)
            );
        }

        passwordHash_ = hashPassword(password);
    }


    // ============================================================
    // Constructor - existing user with stored password hash
    // ============================================================

    User::User(const std::string &username,
               const std::string &passwordHash,
               const std::string &role,
               AccountStatus status,
               StoredHashTag)
        : username_(username),
          passwordHash_(passwordHash),
          role_(role),
          status_(status)
    {
    }


    // ============================================================
    // Create User from stored password hash
    // ============================================================

    User User::fromStoredHash(
        const std::string &username,
        const std::string &passwordHash,
        const std::string &role,
        AccountStatus status)
    {
        return User(
            username,
            passwordHash,
            role,
            status,
            StoredHashTag{}
        );
    }


    // ============================================================
    // Password Hashing
    // ============================================================

    /*
     * Stored format:
     *
     *     iterations$saltHex$hashHex
     *
     * Example:
     *
     *     210000$A1B2C3...$9F8E7D...
     */
    std::string User::hashPassword(
        const std::string &password)
    {
        QByteArray salt = generateSalt();

        QByteArray derived =
            deriveKeyPbkdf2(
                QByteArray::fromStdString(password),
                salt,
                kIterations,
                kDerivedKeyBytes
            );

        std::ostringstream oss;

        oss << kIterations
            << '$'
            << toHex(salt).toStdString()
            << '$'
            << toHex(derived).toStdString();

        return oss.str();
    }


    // ============================================================
    // Verify Password
    // ============================================================

    bool User::verifyPassword(
        const std::string &password,
        const std::string &encoded)
    {
        /*
         * Expected format:
         *
         * iterations$saltHex$hashHex
         */

        const auto firstDollar =
            encoded.find('$');

        if (firstDollar == std::string::npos)
            return false;

        const auto secondDollar =
            encoded.find('$', firstDollar + 1);

        if (secondDollar == std::string::npos)
            return false;


        // --------------------------------------------------------
        // Read iteration count
        // --------------------------------------------------------

        int iterations = 0;

        try
        {
            iterations =
                std::stoi(
                    encoded.substr(
                        0,
                        firstDollar
                    )
                );
        }
        catch (...)
        {
            return false;
        }

        if (iterations <= 0)
            return false;


        // --------------------------------------------------------
        // Extract salt and hash
        // --------------------------------------------------------

        std::string saltHex =
            encoded.substr(
                firstDollar + 1,
                secondDollar - firstDollar - 1
            );

        std::string hashHex =
            encoded.substr(
                secondDollar + 1
            );


        // --------------------------------------------------------
        // Convert hexadecimal values back to bytes
        // --------------------------------------------------------

        QByteArray salt =
            fromHex(saltHex);

        QByteArray expected =
            fromHex(hashHex);


        if (salt.isEmpty() || expected.isEmpty())
            return false;


        // --------------------------------------------------------
        // Calculate hash using entered password
        // --------------------------------------------------------

        QByteArray actual =
            deriveKeyPbkdf2(
                QByteArray::fromStdString(password),
                salt,
                iterations,
                expected.size()
            );


        // --------------------------------------------------------
        // Compare calculated hash with stored hash
        // --------------------------------------------------------

        return actual == expected;
    }


    // ============================================================
    // Authenticate User
    // ============================================================

    bool User::authenticate(
        const std::string &password) const
    {
        /*
         * Only Active accounts are allowed to log in.
         *
         * Pending / Withdrawn accounts cannot authenticate.
         */

        if (status_ != AccountStatus::Active)
            return false;

        return matchesPassword(password);
    }

    bool User::matchesPassword(const std::string &password) const
    {
        return verifyPassword(password, passwordHash_);
    }


    // ============================================================
    // Password Strength Validation
    // ============================================================

    bool User::isPasswordStrong(
        const std::string &password)
    {
        /*
         * Password requirements:
         *
         * - At least 10 characters
         * - Uppercase, lowercase, number, and special character
         */

        if (password.size() < 10)
            return false;


        const PasswordFlags flags = classifyPassword(password, 0, {});
        return flags.upper && flags.lower && flags.digit && flags.special;
    }


    // ============================================================
    // Password Strength Error Message
    // ============================================================

    std::string User::passwordStrengthMessage(
        const std::string &password)
    {
        if (password.size() < 10)
        {
            return
                "Password must be at least 10 characters long.";
        }


        bool hasUpper = false;
        bool hasLower = false;
        bool hasDigit = false;
        bool hasSpecial = false;


        for (unsigned char c : password)
        {
            if (std::isupper(c))
            {
                hasUpper = true;
            }
            else if (std::islower(c))
            {
                hasLower = true;
            }
            else if (std::isdigit(c))
            {
                hasDigit = true;
            }
            else if (!std::isalnum(c))
            {
                hasSpecial = true;
            }
        }


        if (!hasUpper)
        {
            return
                "Password must contain at least one uppercase letter.";
        }

        if (!hasLower)
        {
            return
                "Password must contain at least one lowercase letter.";
        }


        if (!hasDigit)
        {
            return
                "Password must contain at least one number.";
        }


        if (!hasSpecial)
        {
            return
                "Password must contain at least one special character (e.g. ! @ # $).";
        }


        return "";
    }


    // ============================================================
    // Getters
    // ============================================================

    const std::string &User::getPasswordHash() const
    {
        return passwordHash_;
    }

    const std::string &User::getFullName() const { return fullName_; }
    int User::getAge() const { return age_; }
    const std::string &User::getGender() const { return gender_; }
    const std::string &User::getContact() const { return contact_; }
    const std::string &User::getAddress() const { return address_; }

    void User::setProfile(const std::string &fullName,
                          int age,
                          const std::string &gender,
                          const std::string &contact,
                          const std::string &address)
    {
        fullName_ = fullName;
        age_ = age;
        gender_ = gender;
        contact_ = contact;
        address_ = address;
    }


    const std::string &User::getUsername() const
    {
        return username_;
    }


    const std::string &User::getRole() const
    {
        return role_;
    }


    AccountStatus User::getStatus() const
    {
        return status_;
    }


    // ============================================================
    // Setter
    // ============================================================

    void User::setStatus(
        AccountStatus status)
    {
        status_ = status;
    }

}