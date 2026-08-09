#include "patient.h"
#include <iostream>

namespace hms {

Patient::Patient(const std::string& id,
                  const std::string& name,
                  int age,
                  const std::string& gender,
                  const std::string& phoneNumber,
                  const std::string& patientId,
                  const std::string& medicalHistory,
                  const std::string& treatmentType,
                  int assignedRoomNo,
                  bool isEmergency)
    : Person(id, name, age, gender, phoneNumber),
      patientId_(patientId),
      medicalHistory_(medicalHistory),
      treatmentType_(treatmentType),
      assignedRoomNo_(assignedRoomNo),
      isEmergency_(isEmergency) {}

const std::string& Patient::getPatientId() const { return patientId_; }
const std::string& Patient::getMedicalHistory() const { return medicalHistory_; }
const std::string& Patient::getTreatmentType() const { return treatmentType_; }
int Patient::getAssignedRoomNo() const { return assignedRoomNo_; }
bool Patient::isEmergency() const { return isEmergency_; }

void Patient::setMedicalHistory(const std::string& medicalHistory) {
    medicalHistory_ = medicalHistory;
}

void Patient::setTreatmentType(const std::string& treatmentType) {
    treatmentType_ = treatmentType;
}

void Patient::setAssignedRoomNo(int roomNo) {
    assignedRoomNo_ = roomNo;
}

void Patient::setEmergency(bool isEmergency) {
    isEmergency_ = isEmergency;
}

void Patient::displayDetails() const {
    std::cout << "----- Patient Details -----\n";
    std::cout << "Patient ID      : " << patientId_ << "\n";
    std::cout << "Person ID       : " << id_ << "\n";
    std::cout << "Name            : " << name_ << "\n";
    std::cout << "Age             : " << age_ << "\n";
    std::cout << "Gender          : " << gender_ << "\n";
    std::cout << "Phone Number    : " << phoneNumber_ << "\n";
    std::cout << "Medical History : " << medicalHistory_ << "\n";
    std::cout << "Treatment Type  : " << treatmentType_ << "\n";
    std::cout << "Assigned Room   : " << assignedRoomNo_ << "\n";
    std::cout << "Emergency Case  : " << (isEmergency_ ? "Yes" : "No") << "\n";
    std::cout << "----------------------------\n";
}

} // namespace hms