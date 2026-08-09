#ifndef HMS_PERSON_H
#define HMS_PERSON_H

#include <string>

namespace hms {

class Person {
public:
    Person(const std::string& id,
           const std::string& name,
           int age,
           const std::string& gender,
           const std::string& phoneNumber);

    virtual ~Person() = default;

    const std::string& getId() const;
    const std::string& getName() const;
    int getAge() const;
    const std::string& getGender() const;
    const std::string& getPhoneNumber() const;

    // Pure virtual: forces every derived class to define its own
    // presentation of details. Makes Person a non-instantiable ABC.
    virtual void displayDetails() const = 0;

protected:
    std::string id_;
    std::string name_;
    int age_;
    std::string gender_;
    std::string phoneNumber_;
};

} // namespace hms

#endif // HMS_PERSON_H