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
// Getter methods to access private member variables
const std::string& Doctor::getDoctorId() const { return doctorId_; }
const std::string& Doctor::getSpecialization() const { return specialization_; }
int Doctor::getRoomNo() const { return roomNo_; }
bool Doctor::isAvailable() const { return isAvailable_; }
//Setter methods to modify private member variables
void Doctor::setSpecialization(const std::string& specialization) {
    specialization_ = specialization;
}

void Doctor::setRoomNo(int roomNo) {
    roomNo_ = roomNo;
}

void Doctor::setAvailable(bool isAvailable) {
    isAvailable_ = isAvailable;
}