#ifndef HMS_HOSPITAL_MANAGER_H
#define HMS_HOSPITAL_MANAGER_H

#include "patient.h"
#include "doctor.h"
#include "nurse.h"
#include "user.h"

#include <string>
#include <vector>

namespace hms {

// HospitalManager owns the in-memory database (vectors of each entity type)
// and is responsible for CRUD access plus CSV-based persistence so the
// data set survives between program runs.
class HospitalManager {
public:
    HospitalManager() = default;
    ~HospitalManager() = default;

    // ---- Add / Create ----
    void addPatient(const Patient& p);
    void addDoctor(const Doctor& d);
    void addNurse(const Nurse& n);
    void addUser(const User& u);

    // ---- Lookup ----
    Patient* findPatientById(const std::string& id);
    Doctor* findDoctorById(const std::string& id);
    Nurse* findNurseById(const std::string& id);
    User* findUserByUsername(const std::string& username);

    // ---- Read-only accessors to the full collections ----
    const std::vector<Patient>& getPatients() const;
    const std::vector<Doctor>& getDoctors() const;
    const std::vector<Nurse>& getNurses() const;
    const std::vector<User>& getUsers() const;

    // ---- Persistence ----
    // Writes patients_, doctors_, nurses_ and users_ out to
    // data/patient.csv, data/doctor.csv, data/nurse.csv, data/user.csv
    // Creates the data/ directory if it does not already exist.
    void saveAllToCSV();

    // Reads the same four CSV files (if present) and repopulates the
    // in-memory vectors. Existing in-memory data is cleared first.
    void loadAllFromCSV();

    // Individual save/load entry points, exposed in case the caller
    // wants to persist/reload a single entity type instead of everything.
    void savePatientsToCSV();
    void saveDoctorsToCSV();
    void saveNursesToCSV();
    void saveUsersToCSV();

    void loadPatientsFromCSV();
    void loadDoctorsFromCSV();
    void loadNursesFromCSV();
    void loadUsersFromCSV();

private:
    std::vector<Patient> patients_;
    std::vector<Doctor> doctors_;
    std::vector<Nurse> nurses_;
    std::vector<User> users_;

    // Directory + file path constants
    static const std::string kDataDirectory;
    static const std::string kPatientFile;
    static const std::string kDoctorFile;
    static const std::string kNurseFile;
    static const std::string kUserFile;

    // Makes sure data/ exists before we try to write into it.
    static void ensureDataDirectoryExists();
};

} // namespace hms

#endif // HMS_HOSPITAL_MANAGER_H