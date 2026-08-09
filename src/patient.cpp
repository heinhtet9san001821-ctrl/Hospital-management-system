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


