#ifndef HMS_NURSE_H
#define HMS_NURSE_H

#include "person.h"
#include <string>

namespace hms {

class Nurse : public Person {
public:
    // Constructor & Destructor //
    Nurse(const std::string& id,
          const std::string& name,
          int age,
          const std::string& gender,
          const std::string& phoneNumber,
          const std::string& nurseId,
          const std::string& assignedWard,
          const std::string& shiftTime);

    ~Nurse() override = default;
    // Getter Functions //
    const std::string& getNurseId() const;
    const std::string& getAssignedWard() const;
    const std::string& getShiftTime() const;

    //Setter Functions//
    void setAssignedWard(const std::string& assignedWard);
    void setShiftTime(const std::string& shiftTime);
    
    // Display Functions //
    void displayDetails() const override;

private:
    std::string nurseId_;
    std::string assignedWard_;
    std::string shiftTime_;
};

} // namespace hms

#endif // HMS_NURSE_H