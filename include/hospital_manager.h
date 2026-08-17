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

    // ==================== Phase 3.1: Appointment Scheduling ====================
    // Books an appointment for an existing patient with an existing,
    // currently-available doctor. On success: creates the Appointment
    // record, assigns the doctor's consultation room (roomNo_) to the
    // patient's assignedRoomNo_, persists both the patient and
    // appointment CSVs, and returns true. Returns false (with a
    // descriptive console message) if the patient/doctor cannot be
    // found or the doctor is not currently available.
    bool bookAppointment(const std::string& patientId,
                         const std::string& doctorId,
                         const std::string& dateTime);

    // ==================== Phase 3.2: Bed & Room Allocation ====================
    // Ward capacity constants. Room numbers are partitioned into two
    // disjoint ranges so ICU and General Ward occupancy can never
    // collide: ICU uses [kIcuRoomBase, kIcuRoomBase + kIcuCapacity),
    // General Ward uses [kGeneralWardRoomBase, kGeneralWardRoomBase +
    // kGeneralWardCapacity).
    static constexpr int kIcuRoomBase = 500;
    static constexpr int kIcuCapacity = 10;
    static constexpr int kGeneralWardRoomBase = 600;
    static constexpr int kGeneralWardCapacity = 50;
    // Sentinel assignedRoomNo_ value meaning "needs a ward bed but none
    // was free at the time" (i.e. waitlisted).
    static constexpr int kWaitlistedRoomNo = -1;

    // Inspects the patient's treatmentType_ for "ICU" or "General
    // Ward" (case-insensitive) and, if it matches one of those wards,
    // assigns the next free bed number in that ward's range to the
    // patient's assignedRoomNo_. If the ward is at capacity the
    // patient is marked waitlisted (assignedRoomNo_ = kWaitlistedRoomNo)
    // instead. Returns true if a bed was actually assigned, false
    // otherwise (patient not found, treatment type is not ward-based,
    // or the ward is full).
    bool allocateWardBed(const std::string& patientId);

    // Frees whatever ward bed the patient currently occupies (if any)
    // and resets their assignedRoomNo_ to kWaitlistedRoomNo. Use this
    // when a patient is discharged or transferred out of the ward.
    bool releaseWardBed(const std::string& patientId);

    struct WardOccupancy {
        int occupied = 0;
        int capacity = 0;
    };
    WardOccupancy getIcuOccupancy() const;
    WardOccupancy getGeneralWardOccupancy() const;
    void printRoomOccupancy() const;

   