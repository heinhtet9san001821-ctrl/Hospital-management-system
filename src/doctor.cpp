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
// Method to display the details of the doctor
void Doctor::displayDetails() const {
    std::cout << "----- Doctor Details -----\n";
    std::cout << "Doctor ID       : " << doctorId_ << "\n";
    std::cout << "Person ID       : " << id_ << "\n";
    std::cout << "Name            : " << name_ << "\n";
    std::cout << "Age             : " << age_ << "\n";
    std::cout << "Gender          : " << gender_ << "\n";
    std::cout << "Phone Number    : " << phoneNumber_ << "\n";
    std::cout << "Specialization  : " << specialization_ << "\n";
    std::cout << "Room No.        : " << roomNo_ << "\n";
    std::cout << "Available       : " << (isAvailable_ ? "Yes" : "No") << "\n";
    std::cout << "----------------------------\n";
}

} // namespace hms