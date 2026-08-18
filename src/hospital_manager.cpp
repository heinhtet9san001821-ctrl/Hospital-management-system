#include "hospital_manager.h"
#include "date_utils.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

namespace hms {

namespace {

// ---------------------------------------------------------------------
// Generic CSV helpers (shared by patient/doctor/nurse/user/appointment
// persistence). See Phase 2 notes: this is a deliberately simple
// split-by-comma format, so free-text fields are sanitized on write
// (commas -> semicolons) rather than quoted, keeping the reader dead
// simple and crash-proof.
// ---------------------------------------------------------------------
std::vector<std::string> splitCSVLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    std::stringstream ss(line);

    while (std::getline(ss, current, ',')) {
        fields.push_back(current);
    }
    if (!line.empty() && line.back() == ',') {
        fields.emplace_back("");
    }
    return fields;
}

std::string trim(const std::string& s) {
    const std::string whitespace = " \t\r\n";
    const auto start = s.find_first_not_of(whitespace);
    if (start == std::string::npos) return "";
    const auto end = s.find_last_not_of(whitespace);
    return s.substr(start, end - start + 1);
}

std::string sanitizeField(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (char c : value) {
        if (c == ',') result += ';';
        else if (c == '\n' || c == '\r') result += ' ';
        else result += c;
    }
    return result;
}

int safeStoi(const std::string& value, int fallback = 0) {
    const std::string trimmed = trim(value);
    if (trimmed.empty()) return fallback;
    try {
        return std::stoi(trimmed);
    } catch (const std::exception&) {
        return fallback;
    }
}

bool safeStob(const std::string& value) {
    std::string trimmed = trim(value);
    std::transform(trimmed.begin(), trimmed.end(), trimmed.begin(),
                    [](unsigned char c) { return std::tolower(c); });
    return trimmed == "1" || trimmed == "true" || trimmed == "yes";
}

std::vector<std::string> readLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    if (!file.is_open()) return lines;
    std::string line;
    while (std::getline(file, line)) {
        if (!trim(line).empty()) lines.push_back(line);
    }
    return lines;
}

std::string toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                    [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool containsCI(const std::string& haystack, const std::string& needleLower) {
    return toLower(haystack).find(needleLower) != std::string::npos;
}

// ---------------------------------------------------------------------
// Billing fee tables. In a real deployment these would live in a
// pricing/config table (or the database); they are kept here as static
// lookup tables so calculateTotalBill() stays self-contained and
// auditable. All figures are in MMK (Myanmar Kyat).
// ---------------------------------------------------------------------
double consultationFeeForSpecialization(const std::string& specialization) {
    static const std::map<std::string, double> kFeeTable = {
        {"cardiologist",       50000.0},
        {"neurologist",        55000.0},
        {"orthopedic",         45000.0},
        {"pediatrician",       30000.0},
        {"general physician",  15000.0},
        {"surgeon",            60000.0},
        {"dermatologist",      25000.0},
    };
    const auto it = kFeeTable.find(toLower(specialization));
    return (it != kFeeTable.end()) ? it->second : 20000.0; // default consultation fee
}

double roomRatePerDayForRoomNo(int roomNo) {
    if (roomNo >= HospitalManager::kIcuRoomBase &&
        roomNo < HospitalManager::kIcuRoomBase + HospitalManager::kIcuCapacity) {
        return 120000.0; // ICU per-day rate
    }
    if (roomNo >= HospitalManager::kGeneralWardRoomBase &&
        roomNo < HospitalManager::kGeneralWardRoomBase + HospitalManager::kGeneralWardCapacity) {
        return 45000.0; // General Ward per-day rate
    }
    return 0.0; // not admitted to a tracked ward (e.g. outpatient consult room only)
}

double treatmentChargeForType(const std::string& treatmentType) {
    if (containsCI(treatmentType, "emergency"))  return 100000.0;
    if (containsCI(treatmentType, "surgery"))    return 300000.0;
    if (containsCI(treatmentType, "icu"))        return 80000.0;
    if (containsCI(treatmentType, "general"))    return 20000.0;
    return 25000.0; // default basic treatment charge
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
const std::string HospitalManager::kAppointmentFile = "data/appointment.csv";
const std::string HospitalManager::kBloodBankFile = "data/bloodbank.csv";

void HospitalManager::ensureDataDirectoryExists() {
    std::error_code ec;
    if (!std::filesystem::exists(kDataDirectory, ec)) {
        std::filesystem::create_directories(kDataDirectory, ec);
        if (ec) {
            std::cerr << "[HospitalManager] Warning: could not create '"
                      << kDataDirectory << "' directory: " << ec.message() << "\n";
        }
    }
}

// ---------------------------------------------------------------------
// Add / Create
// ---------------------------------------------------------------------
void HospitalManager::addPatient(const Patient& p) {
    patients_.push_back(p);
    // Phase 3.2: if this patient's treatment type designates them for
    // the ICU or the General Ward, automatically try to give them a bed.
    const std::string tt = toLower(patients_.back().getTreatmentType());
    if (tt.find("icu") != std::string::npos || tt.find("general") != std::string::npos) {
        allocateWardBed(patients_.back().getPatientId());
    }
}

void HospitalManager::addDoctor(const Doctor& d) { doctors_.push_back(d); }
void HospitalManager::addNurse(const Nurse& n) { nurses_.push_back(n); }
void HospitalManager::addUser(const User& u) { users_.push_back(u); }

// ---------------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------------
Patient* HospitalManager::findPatientById(const std::string& id) {
    for (auto& p : patients_) {
        if (p.getPatientId() == id || p.getId() == id) return &p;
    }
    return nullptr;
}

Doctor* HospitalManager::findDoctorById(const std::string& id) {
    for (auto& d : doctors_) {
        if (d.getDoctorId() == id || d.getId() == id) return &d;
    }
    return nullptr;
}

Nurse* HospitalManager::findNurseById(const std::string& id) {
    for (auto& n : nurses_) {
        if (n.getNurseId() == id || n.getId() == id) return &n;
    }
    return nullptr;
}

User* HospitalManager::findUserByUsername(const std::string& username) {
    for (auto& u : users_) {
        if (u.getUsername() == username) return &u;
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
const std::vector<Appointment>& HospitalManager::getAppointments() const { return appointments_; }

BloodBank& HospitalManager::getBloodBank() { return bloodBank_; }
const BloodBank& HospitalManager::getBloodBank() const { return bloodBank_; }

// ---------------------------------------------------------------------
// Save
// ---------------------------------------------------------------------
void HospitalManager::saveAllToCSV() {
    ensureDataDirectoryExists();
    savePatientsToCSV();
    saveDoctorsToCSV();
    saveNursesToCSV();
    saveUsersToCSV();
    saveAppointmentsToCSV();
    bloodBank_.saveToCSV(kBloodBankFile);
}

void HospitalManager::savePatientsToCSV() {
    ensureDataDirectoryExists();
    std::ofstream file(kPatientFile, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[HospitalManager] Error: could not open " << kPatientFile << " for writing.\n";
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
        std::cerr << "[HospitalManager] Error: could not open " << kDoctorFile << " for writing.\n";
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
        std::cerr << "[HospitalManager] Error: could not open " << kNurseFile << " for writing.\n";
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
        std::cerr << "[HospitalManager] Error: could not open " << kUserFile << " for writing.\n";
        return;
    }
    // username,passwordHash,role
    for (const auto& u : users_) {
        file << sanitizeField(u.getUsername()) << ","
             << sanitizeField(u.getPasswordHash()) << ","
             << sanitizeField(u.getRole()) << "\n";
    }
}

void HospitalManager::saveAppointmentsToCSV() {
    ensureDataDirectoryExists();
    std::ofstream file(kAppointmentFile, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[HospitalManager] Error: could not open " << kAppointmentFile << " for writing.\n";
        return;
    }
    // appointmentId,patientId,doctorId,dateTime,status
    for (const auto& a : appointments_) {
        file << sanitizeField(a.getAppointmentId()) << ","
             << sanitizeField(a.getPatientId()) << ","
             << sanitizeField(a.getDoctorId()) << ","
             << sanitizeField(a.getDateTime()) << ","
             << appointmentStatusToString(a.getStatus()) << "\n";
    }
}

void HospitalManager::appendPatientToCSV(const Patient& p) {
    ensureDataDirectoryExists();
    std::ofstream file(kPatientFile, std::ios::app);
    if (!file.is_open()) {
        std::cerr << "[HospitalManager] Error: could not open " << kPatientFile << " for appending.\n";
        return;
    }
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

// ---------------------------------------------------------------------
// Load
// ---------------------------------------------------------------------
void HospitalManager::loadAllFromCSV() {
    loadPatientsFromCSV();
    loadDoctorsFromCSV();
    loadNursesFromCSV();
    loadUsersFromCSV();
    loadAppointmentsFromCSV();
    bloodBank_.loadFromCSV(kBloodBankFile);

    rebuildRoomOccupancyFromPatients();
    resyncEmergencyCounterFromPatients();
    resyncAppointmentCounter();
    // Housekeeping: any blood unit whose expiry date has already
    // passed while the system was offline gets flagged immediately.
    bloodBank_.refreshExpiredUnits();
}

void HospitalManager::loadPatientsFromCSV() {
    patients_.clear();
    for (const auto& line : readLines(kPatientFile)) {
        auto f = splitCSVLine(line);
        if (f.size() < 10) {
            std::cerr << "[HospitalManager] Skipping malformed patient row: " << line << "\n";
            continue;
        }
        patients_.emplace_back(
            trim(f[0]), trim(f[1]), safeStoi(f[2]), trim(f[3]), trim(f[4]),
            trim(f[5]), trim(f[6]), trim(f[7]), safeStoi(f[8]), safeStob(f[9]));
    }
}

void HospitalManager::loadDoctorsFromCSV() {
    doctors_.clear();
    for (const auto& line : readLines(kDoctorFile)) {
        auto f = splitCSVLine(line);
        if (f.size() < 9) {
            std::cerr << "[HospitalManager] Skipping malformed doctor row: " << line << "\n";
            continue;
        }
        doctors_.emplace_back(
            trim(f[0]), trim(f[1]), safeStoi(f[2]), trim(f[3]), trim(f[4]),
            trim(f[5]), trim(f[6]), safeStoi(f[7]), safeStob(f[8]));
    }
}

void HospitalManager::loadNursesFromCSV() {
    nurses_.clear();
    for (const auto& line : readLines(kNurseFile)) {
        auto f = splitCSVLine(line);
        if (f.size() < 8) {
            std::cerr << "[HospitalManager] Skipping malformed nurse row: " << line << "\n";
            continue;
        }
        nurses_.emplace_back(
            trim(f[0]), trim(f[1]), safeStoi(f[2]), trim(f[3]), trim(f[4]),
            trim(f[5]), trim(f[6]), trim(f[7]));
    }
}

void HospitalManager::loadUsersFromCSV() {
    users_.clear();
    for (const auto& line : readLines(kUserFile)) {
        auto f = splitCSVLine(line);
        if (f.size() < 3) {
            std::cerr << "[HospitalManager] Skipping malformed user row: " << line << "\n";
            continue;
        }
        // Reconstructed with the already-hashed password so a reload
        // never re-hashes the hash. See User::fromStoredHash().
        users_.push_back(User::fromStoredHash(trim(f[0]), trim(f[1]), trim(f[2])));
    }
}

void HospitalManager::loadAppointmentsFromCSV() {
    appointments_.clear();
    for (const auto& line : readLines(kAppointmentFile)) {
        auto f = splitCSVLine(line);
        if (f.size() < 5) {
            std::cerr << "[HospitalManager] Skipping malformed appointment row: " << line << "\n";
            continue;
        }
        appointments_.emplace_back(
            trim(f[0]), trim(f[1]), trim(f[2]), trim(f[3]),
            appointmentStatusFromString(trim(f[4])));
    }
}

// ---------------------------------------------------------------------
// Phase 3.1: Appointment Scheduling
// ---------------------------------------------------------------------
std::string HospitalManager::generateAppointmentId() {
    ++appointmentCounter_;
    std::ostringstream oss;
    oss << "APT-" << std::setfill('0') << std::setw(4) << appointmentCounter_;
    return oss.str();
}

void HospitalManager::resyncAppointmentCounter() {
    const std::string prefix = "APT-";
    for (const auto& a : appointments_) {
        const std::string& id = a.getAppointmentId();
        if (id.size() > prefix.size() && id.compare(0, prefix.size(), prefix) == 0) {
            try {
                int n = std::stoi(id.substr(prefix.size()));
                appointmentCounter_ = std::max(appointmentCounter_, n);
            } catch (const std::exception&) {
                // Non-numeric suffix on a manually-entered ID; ignore.
            }
        }
    }
}

bool HospitalManager::bookAppointment(const std::string& patientId,
                                      const std::string& doctorId,
                                      const std::string& dateTime) {
    Patient* patient = findPatientById(patientId);
    if (patient == nullptr) {
        std::cerr << "[Appointment] Failed: no patient with ID '" << patientId << "'.\n";
        return false;
    }

    Doctor* doctor = findDoctorById(doctorId);
    if (doctor == nullptr) {
        std::cerr << "[Appointment] Failed: no doctor with ID '" << doctorId << "'.\n";
        return false;
    }

    if (!doctor->isAvailable()) {
        std::cerr << "[Appointment] Failed: Dr. " << doctor->getName()
                  << " (" << doctor->getDoctorId() << ") is not currently available.\n";
        return false;
    }

    const std::string appointmentId = generateAppointmentId();
    appointments_.emplace_back(appointmentId, patient->getPatientId(),
                                doctor->getDoctorId(), dateTime,
                                AppointmentStatus::Scheduled);

    // assignedRoomNo_ is a single field shared between "ward bed" and
    // "consultation room" concerns. If the patient currently holds a
    // tracked ward bed, it must be released here -- otherwise the ward
    // occupancy tracker would keep counting that bed as occupied even
    // though the patient's own record no longer points at it once we
    // overwrite it below, leaking capacity that can never be reclaimed.
    const int previousRoom = patient->getAssignedRoomNo();
    if (previousRoom >= kIcuRoomBase && previousRoom < kIcuRoomBase + kIcuCapacity) {
        occupiedIcuRooms_.erase(previousRoom);
        std::cout << "[WardAllocation] ICU bed " << previousRoom
                  << " released (patient moved to consultation).\n";
    } else if (previousRoom >= kGeneralWardRoomBase &&
               previousRoom < kGeneralWardRoomBase + kGeneralWardCapacity) {
        occupiedGeneralWardRooms_.erase(previousRoom);
        std::cout << "[WardAllocation] General Ward bed " << previousRoom
                  << " released (patient moved to consultation).\n";
    }

    // Assign the doctor's consultation room to the patient for this visit.
    patient->setAssignedRoomNo(doctor->getRoomNo());

    std::cout << "[Appointment] " << appointmentId << " booked: patient "
              << patient->getPatientId() << " with Dr. " << doctor->getName()
              << " (" << doctor->getSpecialization() << ") at " << dateTime
              << ". Consultation room " << doctor->getRoomNo() << " assigned.\n";

    savePatientsToCSV();
    saveAppointmentsToCSV();
    return true;
}

// ---------------------------------------------------------------------
// Phase 3.2: Bed & Room Allocation
// ---------------------------------------------------------------------
void HospitalManager::rebuildRoomOccupancyFromPatients() {
    occupiedIcuRooms_.clear();
    occupiedGeneralWardRooms_.clear();
    for (const auto& p : patients_) {
        const int room = p.getAssignedRoomNo();
        if (room >= kIcuRoomBase && room < kIcuRoomBase + kIcuCapacity) {
            occupiedIcuRooms_.insert(room);
        } else if (room >= kGeneralWardRoomBase && room < kGeneralWardRoomBase + kGeneralWardCapacity) {
            occupiedGeneralWardRooms_.insert(room);
        }
    }
}

bool HospitalManager::allocateWardBed(const std::string& patientId) {
    Patient* patient = findPatientById(patientId);
    if (patient == nullptr) {
        std::cerr << "[WardAllocation] Failed: no patient with ID '" << patientId << "'.\n";
        return false;
    }

    const std::string tt = toLower(patient->getTreatmentType());
    const bool wantsIcu = tt.find("icu") != std::string::npos;
    const bool wantsGeneral = !wantsIcu && tt.find("general") != std::string::npos;

    if (!wantsIcu && !wantsGeneral) {
        // Not a ward-based treatment type (e.g. outpatient) -- nothing to do.
        return false;
    }

    const int base = wantsIcu ? kIcuRoomBase : kGeneralWardRoomBase;
    const int capacity = wantsIcu ? kIcuCapacity : kGeneralWardCapacity;
    std::set<int>& occupied = wantsIcu ? occupiedIcuRooms_ : occupiedGeneralWardRooms_;
    const std::string wardName = wantsIcu ? "ICU" : "General Ward";

    for (int room = base; room < base + capacity; ++room) {
        if (occupied.find(room) == occupied.end()) {
            occupied.insert(room);
            patient->setAssignedRoomNo(room);
            std::cout << "[WardAllocation] " << wardName << " bed " << room
                      << " assigned to patient " << patient->getPatientId() << ".\n";
            return true;
        }
    }

    // No free bed in this ward: mark the patient waitlisted rather than
    // silently leaving a stale/incorrect room number in place.
    patient->setAssignedRoomNo(kWaitlistedRoomNo);
    std::cout << "[WardAllocation] " << wardName << " is FULL (" << capacity
              << "/" << capacity << "). Patient " << patient->getPatientId()
              << " is WAITLISTED.\n";
    return false;
}

bool HospitalManager::releaseWardBed(const std::string& patientId) {
    Patient* patient = findPatientById(patientId);
    if (patient == nullptr) {
        std::cerr << "[WardAllocation] Failed: no patient with ID '" << patientId << "'.\n";
        return false;
    }

    const int room = patient->getAssignedRoomNo();
    bool released = false;
    if (room >= kIcuRoomBase && room < kIcuRoomBase + kIcuCapacity) {
        released = occupiedIcuRooms_.erase(room) > 0;
    } else if (room >= kGeneralWardRoomBase && room < kGeneralWardRoomBase + kGeneralWardCapacity) {
        released = occupiedGeneralWardRooms_.erase(room) > 0;
    }

    patient->setAssignedRoomNo(kWaitlistedRoomNo);

    if (released) {
        std::cout << "[WardAllocation] Room " << room << " released by patient "
                  << patient->getPatientId() << ".\n";
    }
    return released;
}

HospitalManager::WardOccupancy HospitalManager::getIcuOccupancy() const {
    return WardOccupancy{static_cast<int>(occupiedIcuRooms_.size()), kIcuCapacity};
}

HospitalManager::WardOccupancy HospitalManager::getGeneralWardOccupancy() const {
    return WardOccupancy{static_cast<int>(occupiedGeneralWardRooms_.size()), kGeneralWardCapacity};
}

void HospitalManager::printRoomOccupancy() const {
    const auto icu = getIcuOccupancy();
    const auto gw = getGeneralWardOccupancy();
    std::cout << "===== Ward Occupancy =====\n";
    std::cout << "  ICU           : " << icu.occupied << " / " << icu.capacity << " beds occupied\n";
    std::cout << "  General Ward  : " << gw.occupied << " / " << gw.capacity << " beds occupied\n";
    std::cout << "===========================\n";
}

// ---------------------------------------------------------------------
// Phase 3.3: Billing
// ---------------------------------------------------------------------
double HospitalManager::calculateTotalBill(const std::string& patientId, int admittedDays) {
    Patient* patient = findPatientById(patientId);
    if (patient == nullptr) {
        std::cerr << "[Billing] Failed: no patient with ID '" << patientId << "'.\n";
        return -1.0;
    }
    if (admittedDays < 1) admittedDays = 1;

    // 1. Doctor consultation fees: sum across every appointment booked
    //    for this patient, priced by each doctor's specialization.
    double consultationTotal = 0.0;
    int consultationCount = 0;
    for (const auto& appt : appointments_) {
        if (appt.getPatientId() != patient->getPatientId()) continue;
        // Cancelled appointments were never actually rendered, so they
        // are not billed.
        if (appt.getStatus() == AppointmentStatus::Cancelled) continue;

        const Doctor* doctor = nullptr;
        for (const auto& d : doctors_) {
            if (d.getDoctorId() == appt.getDoctorId()) { doctor = &d; break; }
        }
        const double fee = doctor != nullptr
            ? consultationFeeForSpecialization(doctor->getSpecialization())
            : consultationFeeForSpecialization(""); // default fee if doctor record missing
        consultationTotal += fee;
        ++consultationCount;
    }
    // Emergency intakes with no booked appointment still receive an
    // initial ER examination, billed at the default consultation rate.
    if (consultationCount == 0 && patient->isEmergency()) {
        consultationTotal += consultationFeeForSpecialization("");
        consultationCount = 1;
    }

    // 2. Room charges: ward rate (if currently in a tracked ward) * days.
    const double roomRate = roomRatePerDayForRoomNo(patient->getAssignedRoomNo());
    const double roomTotal = roomRate * admittedDays;

    // 3. Base treatment charge, priced by treatment type.
    const double treatmentTotal = treatmentChargeForType(patient->getTreatmentType());

    const double grandTotal = consultationTotal + roomTotal + treatmentTotal;

    std::cout << "===== Invoice: " << patient->getPatientId() << " (" << patient->getName() << ") =====\n";
    std::cout << "  Consultations (" << consultationCount << ") : " << consultationTotal << " MMK\n";
    std::cout << "  Room (" << admittedDays << " day(s) @ " << roomRate << ")  : " << roomTotal << " MMK\n";
    std::cout << "  Treatment (" << patient->getTreatmentType() << ")     : " << treatmentTotal << " MMK\n";
    std::cout << "  ------------------------------------------\n";
    std::cout << "  TOTAL                            : " << grandTotal << " MMK\n";
    std::cout << "=============================================\n";

    return grandTotal;
}

// ---------------------------------------------------------------------
// Phase 3.4: Emergency Dispatch
// ---------------------------------------------------------------------
std::string HospitalManager::generateEmergencyPatientId() {
    ++emergencyPatientCounter_;
    std::ostringstream oss;
    oss << "EMG-" << std::setfill('0') << std::setw(4) << emergencyPatientCounter_;
    return oss.str();
}

void HospitalManager::resyncEmergencyCounterFromPatients() {
    const std::string prefix = "EMG-";
    for (const auto& p : patients_) {
        const std::string& id = p.getPatientId();
        if (id.size() > prefix.size() && id.compare(0, prefix.size(), prefix) == 0) {
            try {
                int n = std::stoi(id.substr(prefix.size()));
                emergencyPatientCounter_ = std::max(emergencyPatientCounter_, n);
            } catch (const std::exception&) {
                // Non-numeric suffix on a manually-entered ID; ignore.
            }
        }
    }
}

void HospitalManager::triggerEmergencyRescue(const std::string& patientName,
                                             const std::string& contactPhone,
                                             const std::string& emergencyLocation) {
    const std::string emergencyId = generateEmergencyPatientId();

    // Age and gender are unknown at the point of dispatch (the call
    // center rarely has this before the ambulance arrives) -- they are
    // captured properly during in-hospital intake once the patient is
    // stabilized. Placeholder values make that explicit rather than
    // silently guessing.
    Patient emergencyPatient(
        /*id=*/emergencyId,
        /*name=*/patientName,
        /*age=*/0,
        /*gender=*/"Unknown",
        /*phoneNumber=*/contactPhone,
        /*patientId=*/emergencyId,
        /*medicalHistory=*/"Unknown - Emergency Intake, pending full assessment",
        /*treatmentType=*/"Emergency ICU",
        /*assignedRoomNo=*/kWaitlistedRoomNo,
        /*isEmergency=*/true);

    // Financial screens are intentionally skipped entirely: the patient
    // is registered and dispatched to before any payment step exists.
    addPatient(emergencyPatient); // also triggers automatic ICU bed allocation

    Patient* registered = findPatientById(emergencyId);
    const int assignedRoom = (registered != nullptr) ? registered->getAssignedRoomNo() : kWaitlistedRoomNo;

    std::cout << "\n\U0001F6A8 [EMERGENCY ALIVE DISPATCH] -> Deploying Ambulance immediately to "
              << emergencyLocation << ". Upfront Cost: 0.0 MMK. Priority Level: CRITICAL. "
              << "Initial treatment type set to Emergency ICU.\n";
    std::cout << "    Patient ID       : " << emergencyId << "\n";
    std::cout << "    Contact Phone    : " << contactPhone << "\n";
    if (assignedRoom == kWaitlistedRoomNo) {
        std::cout << "    ICU Bed          : WAITLISTED (ICU currently at full capacity)\n";
    } else {
        std::cout << "    ICU Bed Assigned : " << assignedRoom << "\n";
    }
    std::cout << std::endl;

    // Immediate durability: this record is appended to disk right away
    // rather than waiting for the next explicit saveAllToCSV() call.
    if (registered != nullptr) {
        appendPatientToCSV(*registered);
    } else {
        appendPatientToCSV(emergencyPatient);
    }
}

} // namespace hms

