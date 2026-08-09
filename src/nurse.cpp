#include "nurse.h"
#include <iostream>

namespace hms {

Nurse::Nurse(const std::string& id,
             const std::string& name,
             int age,
             const std::string& gender,
             const std::string& phoneNumber,
             const std::string& nurseId,
             const std::string& assignedWard,
             const std::string& shiftTime)
        : Person(id, name, age, gender, phoneNumber),
      nurseId_(nurseId),
      assignedWard_(assignedWard),
      shiftTime_(shiftTime) {}

const std::string& Nurse::getNurseId() const { return nurseId_; }
const std::string& Nurse::getAssignedWard() const { return assignedWard_; }
const std::string& Nurse::getShiftTime() const { return shiftTime_; }

void Nurse::setAssignedWard(const std::string& assignedWard) {
    assignedWard_ = assignedWard;
}

void Nurse::setShiftTime(const std::string& shiftTime) {
    shiftTime_ = shiftTime;
}

void Nurse::displayDetails() const {
    std::cout << "----- Nurse Details -----\n";
    std::cout << "Nurse ID        : " << nurseId_ << "\n";
    std::cout << "Person ID       : " << id_ << "\n";
    std::cout << "Name            : " << name_ << "\n";
    std::cout << "Age             : " << age_ << "\n";
    std::cout << "Gender          : " << gender_ << "\n";
    std::cout << "Phone Number    : " << phoneNumber_ << "\n";
    std::cout << "Assigned Ward   : " << assignedWard_ << "\n";
    std::cout << "Shift Time      : " << shiftTime_ << "\n";
    std::cout << "----------------------------\n";
}

} // namespace hms