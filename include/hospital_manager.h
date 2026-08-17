#ifndef HMS_HOSPITAL_MANAGER_H
#define HMS_HOSPITAL_MANAGER_H

#include "patient.h"
#include "doctor.h"
#include "nurse.h"
#include "user.h"
#include "appointment.h"
#include "blood_bank.h"

#include <set>
#include <string>
#include <vector>

namespace hms {

// HospitalManager owns the in-memory database (vectors of each entity
// type), the appointment book, the ward/bed occupancy tracker, and the
// blood bank. It is the single point of contact for CRUD access,
// business logic (booking, billing, admissions, emergency dispatch),
// and CSV-based persistence so the whole data set survives between
// program runs.
class HospitalManager {
public:
    HospitalManager() = default;
    ~HospitalManager() = default;

    // ==================== Phase 2: Add / Create ====================
    // NOTE: addPatient() now also runs automatic ward-bed allocation
    // (see Phase 3 section 2) whenever the patient's treatment type
    // designates them for the ICU or the General Ward.
    void addPatient(const Patient& p);
    void addDoctor(const Doctor& d);
    void addNurse(const Nurse& n);
    void addUser(const User& u);

    // ==================== Phase 2: Lookup ====================
    Patient* findPatientById(const std::string& id);
    Doctor* findDoctorById(const std::string& id);
    Nurse* findNurseById(const std::string& id);
    User* findUserByUsername(const std::string& username);

    // ==================== Phase 2: Read-only accessors ====================
    const std::vector<Patient>& getPatients() const;
    const std::vector<Doctor>& getDoctors() const;
    const std::vector<Nurse>& getNurses() const;
    const std::vector<User>& getUsers() const;
    const std::vector<Appointment>& getAppointments() const;

    // Direct access to the blood bank subsystem for donation intake,
    // issuing, and inventory reporting (see BloodBank in blood_bank.h).
    BloodBank& getBloodBank();
    const BloodBank& getBloodBank() const;

    // ==================== Phase 2: Persistence ====================
    void saveAllToCSV();
    void loadAllFromCSV();

    void savePatientsToCSV();
    void saveDoctorsToCSV();
    void saveNursesToCSV();
    void saveUsersToCSV();
    void saveAppointmentsToCSV();

    void loadPatientsFromCSV();
    void loadDoctorsFromCSV();
    void loadNursesFromCSV();
    void loadUsersFromCSV();
    void loadAppointmentsFromCSV();

    