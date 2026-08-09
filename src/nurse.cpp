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

