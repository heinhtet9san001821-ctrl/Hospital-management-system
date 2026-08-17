#include "appointment.h"
#include <iostream>

namespace hms {

// Constructor to initialize new appointment details
Appointment::Appointment(const std::string& appointmentId,
                          const std::string& patientId,
                          const std::string& doctorId,
                          const std::string& dateTime,
                          AppointmentStatus status)
    : appointmentId_(appointmentId),
      patientId_(patientId),
      doctorId_(doctorId),
      dateTime_(dateTime),
      status_(status) {}
      //Function to return the Appointment ID
const std::string& Appointment::getAppointmentId() const { return appointmentId_; }
//Function to return the Patient ID
const std::string& Appointment::getPatientId() const { return patientId_; }
//Function to return the Doctor ID
const std::string& Appointment::getDoctorId() const { return doctorId_; }
//Function to return
const std::string& Appointment::getDateTime() const { return dateTime_; }
AppointmentStatus Appointment::getStatus() const { return status_; }

void Appointment::setStatus(AppointmentStatus status) { status_ = status; }

//Function to display all appointment details 
void Appointment::displayDetails() const {
    std::cout << "----- Appointment Details -----\n";
    std::cout << "Appointment ID  : " << appointmentId_ << "\n";
    std::cout << "Patient ID      : " << patientId_ << "\n";
    std::cout << "Doctor ID       : " << doctorId_ << "\n";
    std::cout << "Date/Time       : " << dateTime_ << "\n";
    std::cout << "Status          : " << appointmentStatusToString(status_) << "\n";
    std::cout << "----------------------------\n";
}

} // namespace hms
