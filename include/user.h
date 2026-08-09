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

    private:
        static std::string hashPassword(const std::string &password);

        std::string username_;
        std::string passwordHash_;
        std::string role_;
    };

}

#endif