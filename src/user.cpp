#include "user.h"
#include <functional>
#include <sstream>

namespace hms
{

    User::User(const std::string &username,
               const std::string &password,
               const std::string &role)
        : username_(username), passwordHash_(hashPassword(password)), role_(role) {}
    std::string User::hashPassword(const std::string &password)
    {

        std::hash<std::string> hasher;
        std::size_t hashed = hasher(password);

        std::ostringstream oss;
        oss << hashed;
        return oss.str();
    }

    bool User::authenticate(const std::string &password) const
    {
        return passwordHash_ == hashPassword(password);
    }
    const std::string &User::getUsername() const { return username_; }
    const std::string &User::getRole() const { return role_; }

}