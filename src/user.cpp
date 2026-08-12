#include "user.h"
#include <functional>
#include <sstream>

namespace hms
{

    User::User(const std::string &username,
               const std::string &password,
               const std::string &role)
        : username_(username), passwordHash_(hashPassword(password)), role_(role) {}

    // Tag-dispatch constructor: passwordHash is stored as-is, never
    // re-hashed. Only reachable via fromStoredHash().
    User::User(const std::string &username,
               const std::string &passwordHash,
               const std::string &role,
               StoredHashTag)
        : username_(username), passwordHash_(passwordHash), role_(role) {}

    User User::fromStoredHash(const std::string &username,
                               const std::string &passwordHash,
                               const std::string &role)
    {
        return User(username, passwordHash, role, StoredHashTag{});
    }

    const std::string &User::getPasswordHash() const { return passwordHash_; }

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