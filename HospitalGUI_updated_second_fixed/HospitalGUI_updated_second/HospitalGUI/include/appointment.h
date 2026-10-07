#ifndef HMS_APPOINTMENT_H
#define HMS_APPOINTMENT_H

#include <string>

namespace hms {

// Lifecycle state of a booked appointment.
enum class AppointmentStatus {
    Scheduled,
    Completed,
    Cancelled
};

// Converts a status enum to its CSV/display string form.
inline std::string appointmentStatusToString(AppointmentStatus status) {
    switch (status) {
        case AppointmentStatus::Scheduled: return "Scheduled";
        case AppointmentStatus::Completed: return "Completed";
        case AppointmentStatus::Cancelled: return "Cancelled";
    }
    return "Scheduled";
}

// Parses a CSV/display string back into a status enum. Defaults to
// Scheduled on unrecognized input so a corrupt CSV field never crashes
// the loader.
inline AppointmentStatus appointmentStatusFromString(const std::string& value) {
    if (value == "Completed") return AppointmentStatus::Completed;
    if (value == "Cancelled") return AppointmentStatus::Cancelled;
    return AppointmentStatus::Scheduled;
}

// Represents a single doctor/patient appointment booking. Deliberately
// not derived from Person -- an appointment is a relationship between
// two existing Person-derived records (a Patient and a Doctor), not a
// person itself.
class Appointment {
public:
    Appointment(const std::string& appointmentId,
                const std::string& patientId,
                const std::string& doctorId,
                const std::string& dateTime,
                AppointmentStatus status = AppointmentStatus::Scheduled);

    const std::string& getAppointmentId() const;
    const std::string& getPatientId() const;
    const std::string& getDoctorId() const;
    const std::string& getDateTime() const;
    AppointmentStatus getStatus() const;

    void setStatus(AppointmentStatus status);

    void displayDetails() const;

private:
    std::string appointmentId_;
    std::string patientId_;
    std::string doctorId_;
    std::string dateTime_;
    AppointmentStatus status_;
};

} // namespace hms

#endif // HMS_APPOINTMENT_H
