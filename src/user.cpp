#include "user.h"
#include <functional>
#include <sstream>

namespace hms
{

    User::User(const std::string &username,
               const std::string &password,
               const std::string &role)
        : username_(username), passwordHash_(hashPassword(password)), role_(role) {}
