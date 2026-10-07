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

bool appointmentTimesOverlap(const std::string& first, const std::string& second) {
    const auto firstSeparator = first.find(' ');
    const auto secondSeparator = second.find(' ');
    if (firstSeparator == std::string::npos || secondSeparator == std::string::npos)
        return false;
    if (first.substr(0, firstSeparator) != second.substr(0, secondSeparator))
        return false;

    try {
        const auto parseMinutes = [](const std::string& value) {
            const auto separator = value.find(':');
            if (separator == std::string::npos) return -1;
            const int hour = std::stoi(value.substr(0, separator));
            const int minute = std::stoi(value.substr(separator + 1, 2));
            if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return -1;
            return hour * 60 + minute;
        };
        const int firstMinutes = parseMinutes(first.substr(firstSeparator + 1));
        const int secondMinutes = parseMinutes(second.substr(secondSeparator + 1));
        return firstMinutes >= 0 && secondMinutes >= 0
            && std::abs(firstMinutes - secondMinutes) < 45;
    } catch (const std::exception&) {
        return false;
    }
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
        {"obstetrician",       40000.0},
        {"ent",                28000.0},
        {"ophthalmologist",    32000.0},
        {"psychiatrist",       35000.0},
        {"radiologist",        30000.0},
        {"anesthesiologist",   45000.0},
        {"emergency medicine", 38000.0},
        {"oncologist",         58000.0},
        {"urologist",          42000.0},
        {"gastroenterologist", 48000.0},
        {"pulmonologist",      46000.0},
        {"nephrologist",       47000.0},
        {"endocrinologist",    40000.0},
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

bool HospitalManager::deletePatientAccount(const std::string& username) {
    auto userIt = std::find_if(users_.begin(), users_.end(), [&username](const User& u) {
        return u.getUsername() == username && toLower(u.getRole()) == "patient";
    });
    if (userIt == users_.end()) return false;
    const std::string fullName = userIt->getFullName();
    auto patientIt = std::find_if(patients_.begin(), patients_.end(), [&username, &fullName](const Patient& p) {
        return p.getPatientId() == username || p.getId() == username || (!fullName.empty() && p.getName() == fullName);
    });
    if (patientIt == patients_.end()) return false;
    const std::string patientId = patientIt->getPatientId();
    appointments_.erase(std::remove_if(appointments_.begin(), appointments_.end(), [&patientId](const Appointment& a) {
        return a.getPatientId() == patientId;
    }), appointments_.end());
    patients_.erase(patientIt);
    users_.erase(userIt);
    savePatientsToCSV();
    saveUsersToCSV();
    saveAppointmentsToCSV();
    return true;
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

bool HospitalManager::setAppointmentStatus(const std::string& appointmentId, AppointmentStatus status) {
    for (auto& appointment : appointments_) {
        if (appointment.getAppointmentId() != appointmentId) continue;
        appointment.setStatus(status);
        saveAppointmentsToCSV();
        return true;
    }
    return false;
}

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
             << (d.isAvailable() ? 1 : 0) << ","
             << sanitizeField(d.getEmploymentType()) << "\n";
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
        std::string employment = (f.size() >= 10) ? trim(f[9]) : "Resident";
        doctors_.emplace_back(
            trim(f[0]), trim(f[1]), safeStoi(f[2]), trim(f[3]), trim(f[4]),
            trim(f[5]), trim(f[6]), safeStoi(f[7]), safeStob(f[8]), employment);
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
    if (dateTime.size() < 10 || dateutils::isPast(dateTime.substr(0, 10))) {
        std::cerr << "[Appointment] Failed: appointment date must be today or later.\n";
        return false;
    }
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

    for (const auto& existing : appointments_) {
        if (existing.getStatus() == AppointmentStatus::Cancelled) continue;
        if (!appointmentTimesOverlap(existing.getDateTime(), dateTime)) continue;
        if (existing.getDoctorId() == doctor->getDoctorId()
            || existing.getPatientId() == patient->getPatientId()) {
            std::cerr << "[Appointment] Failed: the doctor or patient has an overlapping 45-minute time slot.\n";
            return false;
        }
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

void HospitalManager::saveUsersToCSV() {
    ensureDataDirectoryExists();
    std::ofstream file(kUserFile, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[HospitalManager] Error: could not open " << kUserFile << " for writing.\n";
        return;
    }
    // username,passwordHash,role,status
    for (const auto& u : users_) {
        std::string statusStr =
            u.getStatus() == AccountStatus::Pending   ? "Pending" :
            u.getStatus() == AccountStatus::Withdrawn ? "Withdrawn" : "Active";
        file << sanitizeField(u.getUsername()) << ","
             << sanitizeField(u.getPasswordHash()) << ","
             << sanitizeField(u.getRole()) << ","
             << statusStr << ","
             << sanitizeField(u.getFullName()) << ","
             << u.getAge() << ","
             << sanitizeField(u.getGender()) << ","
             << sanitizeField(u.getContact()) << ","
             << sanitizeField(u.getAddress()) << "\n";
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
        AccountStatus status = AccountStatus::Active; // default for old rows without a status column
        if (f.size() >= 4) {
            std::string s = trim(f[3]);
            if (s == "Pending") status = AccountStatus::Pending;
            else if (s == "Withdrawn") status = AccountStatus::Withdrawn;
        }
        users_.push_back(User::fromStoredHash(trim(f[0]), trim(f[1]), trim(f[2]), status));
        if (f.size() >= 9) {
            users_.back().setProfile(trim(f[4]), safeStoi(f[5]), trim(f[6]), trim(f[7]), trim(f[8]));
        }
    }
}

int HospitalManager::countUsersByRole(const std::string& role) const {
    int count = 0;
    for (const auto& u : users_) {
        if (toLower(u.getRole()) == toLower(role)) ++count;
    }
    return count;
}

int HospitalManager::remainingAmbulances() const {
    return ambulancesAvailable_;
}

void HospitalManager::setAmbulancesAvailable(int count) {
    ambulancesAvailable_ = std::clamp(count, 0, 20);
}

void HospitalManager::returnAmbulances(int count) {
    ambulancesAvailable_ = std::min(20, ambulancesAvailable_ + std::max(0, count));
}

int HospitalManager::findFreeTheatre() const {
    for (int room = 801; room < 821; ++room) {
        if (occupiedTheatres_.find(room) == occupiedTheatres_.end()) return room;
    }
    return 0;
}

void HospitalManager::occupyTheatre(int roomNo) {
    occupiedTheatres_.insert(roomNo);
}

void HospitalManager::releaseTheatre(int roomNo) {
    occupiedTheatres_.erase(roomNo);
}

std::string HospitalManager::dispatchEmergency(const std::string& callerName,
                                               const std::string& contactPhone,
                                               const std::string& location,
                                               const std::string& severity) {
    int needed = 4;
    const std::string sev = toLower(severity);
    if (sev.find("severe") != std::string::npos) needed = 20;
    else if (sev.find("moderate") != std::string::npos) needed = 10;

    if (ambulancesAvailable_ <= 0) {
        return "All 20 ambulances are currently deployed. Stay on the line — the duty officer will redirect the nearest returning unit.";
    }
    const int sent = std::min(needed, ambulancesAvailable_);
    ambulancesAvailable_ -= sent;

    std::string name = callerName.empty() ? "Unknown caller" : callerName;
    triggerEmergencyRescue(name, contactPhone, location + " [" + severity + "]");

    std::ostringstream oss;
    oss << sent << " ambulance" << (sent == 1 ? "" : "s")
        << " dispatched to " << location
        << " for a " << severity << " case. "
        << ambulancesAvailable_ << " unit(s) remain at the bay. "
        << "Care begins before paperwork — please keep the phone reachable.";
    return oss.str();
}

void HospitalManager::seedDefaultsIfNeeded() {
    struct DocSeed {
        const char* id;
        const char* name;
        int age;
        const char* gender;
        const char* phone;
        const char* docId;
        const char* spec;
        int room;
        const char* employment;
    };
    const DocSeed seeds[] = {
        {"D001","Dr. Nyan Lin Htet",40,"Male","09-765-4321","DOC_001","Anesthesiologist",302,"Resident"},
        {"D002","Dr. Phyo Sithu Kyaw",38,"Male","09-111-2002","DOC_002","Neurologist",305,"Resident"},
        {"D003","Dr. Zune Sandi Wint",42,"Female","09-111-2003","DOC_003","Cardiologist",310,"Resident"},
        {"D004","Dr. Htoo Kaung Kyaw",36,"Male","09-111-2004","DOC_004","General Surgeon",220,"Resident"},
        {"D005","Dr. Aung Kaung Myat",45,"Male","09-111-2005","DOC_005","Radiologist",201,"Resident"},
        {"D006","Dr. Kaung Myat Ko Ko",48,"Male","09-111-2006","DOC_006","Gynecologist",801,"Resident"},
        {"D007","Dr. Kaung Htet Kyaw",34,"Male","09-111-2007","DOC_007","Proctologist",215,"Part-time"},
        {"D008","Dr. Moe Thura Aung",39,"Male","09-111-2008","DOC_008","Andrologist",240,"Resident"},
        {"D009","Dr. Ye Hein Thuta Aung",41,"Male","09-111-2009","DOC_009","Oncologist",218,"Part-time"},
        {"D010","Dr. Htet Naing Lin",37,"Male","09-111-2010","DOC_010","Ophthalmologist",219,"Part-time"},
        {"D011","Dr. Arkar Chan Myae",43,"Male","09-111-2011","DOC_011","Psychiatrist",230,"Part-time"},
        {"D012","Dr. Kyi Phyu Khin",40,"Female","09-111-2012","DOC_012","Dermatologist",410,"Resident"},
        {"D013","Dr. Han Thi Thi Htun",44,"Female","09-111-2013","DOC_013","Pediatrician",802,"Resident"},
        {"D014","Dr. May Thu Aung",35,"Female","09-111-2014","DOC_014","Orthopedic Surgeon",101,"Resident"},
        {"D015","Dr. Pa Pa",46,"Female","09-111-2015","DOC_015","Otolaryngologist",330,"Resident"},
        {"D016","Dr. Lae Yi Phyo",41,"Female","09-111-2016","DOC_016","Gastroenterologist",318,"Part-time"},
        {"D017","Dr. May Myat Chel",47,"Female","09-111-2017","DOC_017","Plastic & Reconstructive Surgeon",322,"Resident"},
        {"D018","Dr. Khine Hsu Zar Thin",39,"Female","09-111-2018","DOC_018","Pulmonologist",325,"Resident"},
        {"D019","Dr. Yone Phyu Phyu Aung",38,"Female","09-111-2019","DOC_019","Pathologist",328,"Part-time"},
        {"D020","Dr. Myat Thin Nwe",36,"Female","09-111-2020","DOC_020","Rheumatologist",335,"Resident"},
        {"D021","Dr. Yone Yamone Phoo",36,"Female","09-111-2021","DOC_021","Geriatrician",336,"Part-time"},
        {"D023","Nay Chi",33,"Female","09-111-2023","DOC_023","Gynecologist",803,"Resident"},
        {"D024","Aye Thiri Aung",35,"Female","09-111-2024","DOC_024","Gynecologist",804,"Resident"},
        {"D025","Ei Zin Htet",31,"Female","09-111-2025","DOC_025","Gynecologist",805,"Part-time"},
    };

    for (const auto& s : seeds) {
        if (findDoctorById(s.docId) == nullptr) {
            addDoctor(Doctor(s.id, s.name, s.age, s.gender, s.phone, s.docId, s.spec, s.room, true, s.employment));
        }
    }
    if (auto *poe = findDoctorById("DOC_022")) {
        poe->setSpecialization("Gynecologist");
        poe->setEmploymentType("Resident");
    }
    saveDoctorsToCSV();

    if (nurses_.size() < 50) {
        static const char* nurseNames[] = {
            "Daw Nu Nu", "Daw Moe Moe", "U Kyaw Swa", "Daw Khin Hnin", "Daw Ei Ei",
            "Daw Su Mon", "U Min Zaw", "Daw Hla Hla", "Daw Thiri Win", "Daw May Zin",
            "U Aung Kyaw", "Daw Nwe Nwe", "Daw Phyu Phyu", "U Hein Htet", "Daw San San",
            "Daw Mya Mya", "U Zaw Lin", "Daw Khin Yu", "Daw Wai Wai", "U Nay Lin",
            "Daw Htet Htet", "Daw Mi Mi", "U Kaung Myat", "Daw Yadanar", "Daw Nilar",
            "U Phone Myint", "Daw Cherry", "Daw Pwint Pwint", "U Sithu", "Daw Hnin Hnin",
            "Daw Su Su", "U Thant Zin", "Daw Shwe Sin", "Daw Kyal Sin", "U Ye Min",
            "Daw Thazin", "Daw Khin Khin", "U Myo Min", "Daw May May", "Daw Zar Zar",
            "U Lin Htet", "Daw Ei Mon", "Daw Wut Yi", "U Sai Aung", "Daw Nandar",
            "Daw Hsu Hsu", "U Kyaw Zin", "Daw Mone Mone", "Daw Thet Thet", "U Aung Min"
        };
        for (int i = static_cast<int>(nurses_.size()); i < 50; ++i) {
            const std::string id = "NUR_" + std::to_string(i + 1);
            const std::string ward = (i % 4 == 0) ? "Critical Care" : (i % 4 == 1) ? "General Ward" : (i % 4 == 2) ? "Isolation" : "Maternity";
            const std::string shift = (i % 3 == 0) ? "Morning" : (i % 3 == 1) ? "Evening" : "Night";
            addNurse(Nurse("N" + std::to_string(i + 1), nurseNames[i], 28 + (i % 12), i % 5 == 0 ? "Male" : "Female", "09-444-" + std::to_string(5556 + i), id, ward, shift));
        }
        saveNursesToCSV();
    }

    if (findUserByUsername("HeinHtetSan") == nullptr) {
        User manager("HeinHtetSan", "RoyalCare#1", "Hospital Manager", AccountStatus::Active);
        manager.setProfile("Hein Htet San", 42, "Male", "09-900-1000", "Health++ Headquarters");
        addUser(manager);
        saveUsersToCSV();
    }

    bool usersChanged = false;
    for (const auto& s : seeds) {
        if (findUserByUsername(s.docId) != nullptr) continue;
        User doc(s.docId, "Clinic#2026A", "Doctor", AccountStatus::Active);
        doc.setProfile(s.name, s.age, s.gender, s.phone, "Health++ Clinic");
        addUser(doc);
        usersChanged = true;
    }
    for (const auto& n : nurses_) {
        if (findUserByUsername(n.getNurseId()) != nullptr) continue;
        User nurse(n.getNurseId(), "Clinic#2026A", "Nurse", AccountStatus::Active);
        nurse.setProfile(n.getName(), n.getAge(), n.getGender(), n.getPhoneNumber(), n.getAssignedWard());
        addUser(nurse);
        usersChanged = true;
    }
    if (usersChanged) saveUsersToCSV();

    struct PatientSeed {
        const char* username;
        const char* name;
        int age;
        const char* gender;
        const char* phone;
        const char* address;
        const char* patientId;
        const char* history;
    };
    const PatientSeed patientSeeds[] = {
        {"patient_amy", "Amy Win", 27, "Female", "09-700-1001", "Bahan", "PAT-DEMO-01", "Recurring migraine"},
        {"patient_ko", "Ko Min Thu", 34, "Male", "09-700-1002", "Sanchaung", "PAT-DEMO-02", "Knee pain after sports"},
        {"patient_thiri", "Thiri Aung", 31, "Female", "09-700-1003", "Kamayut", "PAT-DEMO-03", "Seasonal asthma"},
        {"patient_nay", "Nay Lin", 52, "Male", "09-700-1004", "Insein", "PAT-DEMO-04", "Blood pressure review"},
        {"patient_may", "May Zin", 29, "Female", "09-700-1005", "Tamwe", "PAT-DEMO-05", "Routine check-up"},
        {"patient_hla", "Hla Hla", 41, "Female", "09-700-1006", "Thingangyun", "PAT-DEMO-06", "Skin irritation"}
    };
    bool patientsChanged = false;
    bool demoUsersChanged = false;
    for (const auto& s : patientSeeds) {
        if (findUserByUsername(s.username) == nullptr) {
            User patient(s.username, "Patient#2026A", "Patient", AccountStatus::Active);
            patient.setProfile(s.name, s.age, s.gender, s.phone, s.address);
            addUser(patient);
            demoUsersChanged = true;
        }
        if (findPatientById(s.patientId) == nullptr) {
            addPatient(Patient(s.patientId, s.name, s.age, s.gender, s.phone,
                               s.patientId, s.history, "Outpatient", kWaitlistedRoomNo, false));
            patientsChanged = true;
        }
    }
    if (demoUsersChanged) saveUsersToCSV();
    if (patientsChanged) savePatientsToCSV();
}

} // namespace hms