#ifndef HMS_USER_H
#define HMS_USER_H

#include <string>

namespace hms
{

    class User
    {
    public:
        User(const std::string &username,
             const std::string &password,
             const std::string &role);

        // Compares the hash of the supplied password against the
        // stored hash. Returns true on match.
        bool authenticate(const std::string &password) const;

        const std::string &getUsername() const;
        const std::string &getRole() const;

        // Returns the already-hashed password. Needed so persistence
        // code (HospitalManager) can write the hash out to disk without
        // ever having access to the plaintext password.
        const std::string &getPasswordHash() const;

        // Reconstructs a User from a hash that was already produced by
        // hashPassword() (e.g. one just read back from a CSV file).
        // Use this instead of the normal constructor when reloading
        // persisted data -- passing an already-hashed value into the
        // normal constructor would hash it a second time and silently
        // invalidate every stored login.
        static User fromStoredHash(const std::string &username,
                                    const std::string &passwordHash,
                                    const std::string &role);

    private:
        // Private tag-dispatch constructor used only by fromStoredHash()
        // to set passwordHash_ directly, bypassing hashPassword().
        struct StoredHashTag {};
        User(const std::string &username,
             const std::string &passwordHash,
             const std::string &role,
             StoredHashTag);

        static std::string hashPassword(const std::string &password);

        std::string username_;
        std::string passwordHash_;
        std::string role_;
    };

}

#endif