#ifndef HMS_USER_H
#define HMS_USER_H

#include <string>

namespace hms
{
    enum class AccountStatus
    {
        Pending,
        Active,
        Withdrawn
    };

    class User
    {
    public:
        User(const std::string &username,
             const std::string &password,
             const std::string &role,
             AccountStatus status = AccountStatus::Active);

        bool authenticate(const std::string &password) const;
        bool matchesPassword(const std::string &password) const;

        const std::string &getUsername() const;
        const std::string &getRole() const;
        AccountStatus getStatus() const;
        void setStatus(AccountStatus status);

        const std::string &getPasswordHash() const;
        const std::string &getFullName() const;
        int getAge() const;
        const std::string &getGender() const;
        const std::string &getContact() const;
        const std::string &getAddress() const;

        void setProfile(const std::string &fullName,
                        int age,
                        const std::string &gender,
                        const std::string &contact,
                        const std::string &address);

        static User fromStoredHash(const std::string &username,
                                    const std::string &passwordHash,
                                    const std::string &role,
                                    AccountStatus status);

        static bool isPasswordStrong(const std::string &password);
        static std::string passwordStrengthMessage(const std::string &password);

    private:
        struct StoredHashTag
        {
        };
        User(const std::string &username,
             const std::string &passwordHash,
             const std::string &role,
             AccountStatus status,
             StoredHashTag);

        static std::string hashPassword(const std::string &password);
        static bool verifyPassword(const std::string &password, const std::string &encoded);

        std::string username_;
        std::string passwordHash_;
        std::string role_;
        AccountStatus status_;
        std::string fullName_;
        int age_ = 0;
        std::string gender_;
        std::string contact_;
        std::string address_;
    };

}

#endif