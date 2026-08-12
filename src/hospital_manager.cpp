#include "hospital_manager.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace hms {

namespace {

// ---------------------------------------------------------------------
// Internal (translation-unit local) helpers. None of these touch class
// state directly, so they are kept as free functions in an anonymous
// namespace rather than private static members.
// ---------------------------------------------------------------------

// Splits a single CSV line into fields on the comma delimiter.
// This is a "safe" splitter in the sense that it never throws and it
// never reads out of bounds; it does not support quoted fields that
// themselves contain commas (see sanitizeField() below for how we
// avoid that problem on the write side instead).
std::vector<std::string> splitCSVLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    std::stringstream ss(line);

    while (std::getline(ss, current, ',')) {
        fields.push_back(current);
    }

    // std::getline drops a trailing empty field after the final comma
    // (e.g. "a,b," would only yield {"a","b"}). Detect that case and
    // push the missing empty field back on so column counts stay
    // consistent with what was written.
    if (!line.empty() && line.back() == ',') {
        fields.emplace_back("");
    }

    return fields;
}

// Strips leading/trailing whitespace and carriage returns (Windows
// line endings) from a field pulled out of a CSV line.
std::string trim(const std::string& s) {
    const std::string whitespace = " \t\r\n";
    const auto start = s.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }
    const auto end = s.find_last_not_of(whitespace);
    return s.substr(start, end - start + 1);
}

// Removes characters that would break our simple comma-delimited
// format (commas and newlines) before a value is written to disk.
// This keeps the reader trivial and safe at the cost of disallowing
// literal commas inside free-text fields like medical history notes.
std::string sanitizeField(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (char c : value) {
        if (c == ',') {
            result += ';';
        } else if (c == '\n' || c == '\r') {
            result += ' ';
        } else {
            result += c;
        }
    }
    return result;
}

// Safely converts a CSV field to int. Returns fallback on any parse
// failure instead of throwing, so a malformed/missing field never
// crashes the load routine.
int safeStoi(const std::string& value, int fallback = 0) {
    const std::string trimmed = trim(value);
    if (trimmed.empty()) {
        return fallback;
    }
    try {
        size_t consumed = 0;
        int result = std::stoi(trimmed, &consumed);
        return result;
    } catch (const std::exception&) {
        return fallback;
    }
}

// Safely converts a CSV field ("1"/"0", "true"/"false", case
// insensitive) to bool. Anything unrecognized falls back to false.
bool safeStob(const std::string& value) {
    std::string trimmed = trim(value);
    std::transform(trimmed.begin(), trimmed.end(), trimmed.begin(),
                    [](unsigned char c) { return std::tolower(c); });
    return trimmed == "1" || trimmed == "true" || trimmed == "yes";
}

// Opens 'path' for reading and returns every non-empty line. Returns
// an empty vector (rather than throwing) if the file does not exist
// yet -- that is the expected state on a very first run before any
// data has ever been saved.
std::vector<std::string> readLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    if (!file.is_open()) {
        return lines;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (!trim(line).empty()) {
            lines.push_back(line);
        }
    }
    return lines;
}

} // anonymous namespace

// ---------------------------------------------------------------------
// Path constants
// ---------------------------------------------------------------------
const std::string HospitalManager::kDataDirectory = "data";
const std::string HospitalManager::kPatientFile = "data/patient.csv";
const std::string HospitalManager::kDoctorFile = "data/doctor.csv";
const std::string HospitalManager::kNurseFile = "data/nurse.csv";
const std::string HospitalManager::kUserFile = "data/user.csv";

void HospitalManager::ensureDataDirectoryExists() {
    std::error_code ec;
    if (!std::filesystem::exists(kDataDirectory, ec)) {
        std::filesystem::create_directories(kDataDirectory, ec);
        if (ec) {
            std::cerr << "[HospitalManager] Warning: could not create '"
                      << kDataDirectory << "' directory: " << ec.message()
                      << "\n";
        }
    }
}

// ---------------------------------------------------------------------
// Add / Create
// ---------------------------------------------------------------------
void HospitalManager::addPatient(const Patient& p) { patients_.push_back(p); }
void HospitalManager::addDoctor(const Doctor& d) { doctors_.push_back(d); }
void HospitalManager::addNurse(const Nurse& n) { nurses_.push_back(n); }
void HospitalManager::addUser(const User& u) { users_.push_back(u); }

// ---------------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------------
Patient* HospitalManager::findPatientById(const std::string& id) {
    for (auto& p : patients_) {
        if (p.getPatientId() == id || p.getId() == id) {
            return &p;
        }
    }
    return nullptr;
}

Doctor* HospitalManager::findDoctorById(const std::string& id) {
    for (auto& d : doctors_) {
        if (d.getDoctorId() == id || d.getId() == id) {
            return &d;
        }
    }
    return nullptr;
}

Nurse* HospitalManager::findNurseById(const std::string& id) {
    for (auto& n : nurses_) {
        if (n.getNurseId() == id || n.getId() == id) {
            return &n;
        }
    }
    return nullptr;
}

User* HospitalManager::findUserByUsername(const std::string& username) {
    for (auto& u : users_) {
        if (u.getUsername() == username) {
            return &u;
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------
// Read-only accessors
// ---------------------------------------------------------------------
const std::vector<Patient>& HospitalManager::getPatients() const { return patients_; }
const std::vector<Doctor>& HospitalManager::getDoctors() const { return doctors_; }
const std::vector<Nurse>& HospitalManager::getNurses() const { return nurses_; }
const std::vector<User>& HospitalManager::getUsers() const { return users_; }

// ---------------------------------------------------------------------
// Save
// ---------------------------------------------------------------------
void HospitalManager::saveAllToCSV() {
    ensureDataDirectoryExists();
    savePatientsToCSV();
    saveDoctorsToCSV();
    saveNursesToCSV();
    saveUsersToCSV();
}

void HospitalManager::savePatientsToCSV() {
    ensureDataDirectoryExists();
    std::ofstream file(kPatientFile, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[HospitalManager] Error: could not open "
                  << kPatientFile << " for writing.\n";
        return;
    }
    // id,name,age,gender,phone,patientId,medicalHistory,treatmentType,assignedRoomNo,isEmergency
    for (const auto& p : patients_) {
        file << sanitizeField(p.getId()) << ","
             << sanitizeField(p.getName()) << ","
             << p.getAge() << ","
             << sanitizeField(p.getGender()) << ","
             << sanitizeField(p.getPhoneNumber()) << ","
             << sanitizeField(p.getPatientId()) << ","
             << sanitizeField(p.getMedicalHistory()) << ","
             << sanitizeField(p.getTreatmentType()) << ","
             << p.getAssignedRoomNo() << ","
             << (p.isEmergency() ? 1 : 0) << "\n";
    }
}

void HospitalManager::saveDoctorsToCSV() {
    ensureDataDirectoryExists();
    std::ofstream file(kDoctorFile, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[HospitalManager] Error: could not open "
                  << kDoctorFile << " for writing.\n";
        return;
    }
    // id,name,age,gender,phone,doctorId,specialization,roomNo,isAvailable
    for (const auto& d : doctors_) {
        file << sanitizeField(d.getId()) << ","
             << sanitizeField(d.getName()) << ","
             << d.getAge() << ","
             << sanitizeField(d.getGender()) << ","
             << sanitizeField(d.getPhoneNumber()) << ","
             << sanitizeField(d.getDoctorId()) << ","
             << sanitizeField(d.getSpecialization()) << ","
             << d.getRoomNo() << ","
             << (d.isAvailable() ? 1 : 0) << "\n";
    }
}

void HospitalManager::saveNursesToCSV() {
    ensureDataDirectoryExists();
    std::ofstream file(kNurseFile, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[HospitalManager] Error: could not open "
                  << kNurseFile << " for writing.\n";
        return;
    }
    // id,name,age,gender,phone,nurseId,assignedWard,shiftTime
    for (const auto& n : nurses_) {
        file << sanitizeField(n.getId()) << ","
             << sanitizeField(n.getName()) << ","
             << n.getAge() << ","
             << sanitizeField(n.getGender()) << ","
             << sanitizeField(n.getPhoneNumber()) << ","
             << sanitizeField(n.getNurseId()) << ","
             << sanitizeField(n.getAssignedWard()) << ","
             << sanitizeField(n.getShiftTime()) << "\n";
    }
}

void HospitalManager::saveUsersToCSV() {
    ensureDataDirectoryExists();
    std::ofstream file(kUserFile, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[HospitalManager] Error: could not open "
                  << kUserFile << " for writing.\n";
        return;
    }
    // username,passwordHash,role
    // NOTE: relies on the User::passwordHash_ friend-access patch
    // described alongside this file -- see accompanying notes.
    for (const auto& u : users_) {
        file << sanitizeField(u.getUsername()) << ","
             << sanitizeField(u.getPasswordHash()) << ","
             << sanitizeField(u.getRole()) << "\n";
    }
}

// ---------------------------------------------------------------------
// Load
// ---------------------------------------------------------------------
void HospitalManager::loadAllFromCSV() {
    loadPatientsFromCSV();
    loadDoctorsFromCSV();
    loadNursesFromCSV();
    loadUsersFromCSV();
}

void HospitalManager::loadPatientsFromCSV() {
    patients_.clear();
    for (const auto& line : readLines(kPatientFile)) {
        auto f = splitCSVLine(line);
        if (f.size() < 10) {
            std::cerr << "[HospitalManager] Skipping malformed patient row: "
                      << line << "\n";
            continue;
        }
        patients_.emplace_back(
            trim(f[0]),               // id
            trim(f[1]),               // name
            safeStoi(f[2]),           // age
            trim(f[3]),               // gender
            trim(f[4]),               // phoneNumber
            trim(f[5]),               // patientId
            trim(f[6]),               // medicalHistory
            trim(f[7]),               // treatmentType
            safeStoi(f[8]),           // assignedRoomNo
            safeStob(f[9]));          // isEmergency
    }
}

void HospitalManager::loadDoctorsFromCSV() {
    doctors_.clear();
    for (const auto& line : readLines(kDoctorFile)) {
        auto f = splitCSVLine(line);
        if (f.size() < 9) {
            std::cerr << "[HospitalManager] Skipping malformed doctor row: "
                      << line << "\n";
            continue;
        }
        doctors_.emplace_back(
            trim(f[0]),               // id
            trim(f[1]),               // name
            safeStoi(f[2]),           // age
            trim(f[3]),               // gender
            trim(f[4]),               // phoneNumber
            trim(f[5]),               // doctorId
            trim(f[6]),               // specialization
            safeStoi(f[7]),           // roomNo
            safeStob(f[8]));          // isAvailable
    }
}

void HospitalManager::loadNursesFromCSV() {
    nurses_.clear();
    for (const auto& line : readLines(kNurseFile)) {
        auto f = splitCSVLine(line);
        if (f.size() < 8) {
            std::cerr << "[HospitalManager] Skipping malformed nurse row: "
                      << line << "\n";
            continue;
        }
        nurses_.emplace_back(
            trim(f[0]),               // id
            trim(f[1]),               // name
            safeStoi(f[2]),           // age
            trim(f[3]),               // gender
            trim(f[4]),               // phoneNumber
            trim(f[5]),               // nurseId
            trim(f[6]),               // assignedWard
            trim(f[7]));              // shiftTime
    }
}

void HospitalManager::loadUsersFromCSV() {
    users_.clear();
    for (const auto& line : readLines(kUserFile)) {
        auto f = splitCSVLine(line);
        if (f.size() < 3) {
            std::cerr << "[HospitalManager] Skipping malformed user row: "
                      << line << "\n";
            continue;
        }
        // Reconstructs the User with its ALREADY-HASHED password so a
        // reload does not re-hash the hash (which would silently break
        // every stored login). Requires User::fromStoredHash(); see the
        // accompanying user.h / user.cpp patch notes.
        users_.push_back(User::fromStoredHash(trim(f[0]), trim(f[1]), trim(f[2])));
    }
}

} // namespace hms