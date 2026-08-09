#include "doctor.h"
#include <iostream>

namespace hms {
// Constructor to initialize a Doctor object with personal and professional details
Doctor::Doctor(const std::string& id,
               const std::string& name,
               int age,
               const std::string& gender,
               const std::string& phoneNumber, 
               const std::string& doctorId,
               const std::string& specialization,
               int roomNo,
               bool isAvailable)
    : Person(id, name, age, gender, phoneNumber),
      doctorId_(doctorId),
      specialization_(specialization),
      roomNo_(roomNo),
      isAvailable_(isAvailable) {}