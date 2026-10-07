#include "person.h"

namespace hms {

Person::Person(const std::string& id,
               const std::string& name,
               int age,
               const std::string& gender,
               const std::string& phoneNumber)
    : id_(id), name_(name), age_(age), gender_(gender), phoneNumber_(phoneNumber) {}

const std::string& Person::getId() const { return id_; }
const std::string& Person::getName() const { return name_; }
int Person::getAge() const { return age_; }
const std::string& Person::getGender() const { return gender_; }
const std::string& Person::getPhoneNumber() const { return phoneNumber_; }

} // namespace hms

//Our c++ project