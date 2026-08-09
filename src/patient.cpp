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


