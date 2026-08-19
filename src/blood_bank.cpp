#include "blood_bank.h"
#include "date_utils.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace hms {

// ---------------------------------------------------------------------
// Status <-> string
// ---------------------------------------------------------------------
std::string bloodUnitStatusToString(BloodUnitStatus status) {
    switch (status) {
        case BloodUnitStatus::Available: return "Available";
        case BloodUnitStatus::Reserved:  return "Reserved";
        case BloodUnitStatus::Used:      return "Used";
        case BloodUnitStatus::Expired:   return "Expired";
        case BloodUnitStatus::Discarded: return "Discarded";
    }
    return "Available";
}

BloodUnitStatus bloodUnitStatusFromString(const std::string& value) {
    if (value == "Reserved")  return BloodUnitStatus::Reserved;
    if (value == "Used")      return BloodUnitStatus::Used;
    if (value == "Expired")   return BloodUnitStatus::Expired;
    if (value == "Discarded") return BloodUnitStatus::Discarded;
    return BloodUnitStatus::Available;
}

// ---------------------------------------------------------------------
// BloodUnit
// ---------------------------------------------------------------------
BloodUnit::BloodUnit(const std::string& unitId,
                     const std::string& bloodType,
                     double volumeMl,
                     const std::string& donorName,
                     const std::string& donorPhone,
                     const std::string& donationDate,
                     const std::string& expiryDate,
                     BloodUnitStatus status,
                     const std::string& usedForPatientId,
                     const std::string& usedForOperation,
                     const std::string& usedDate)
    : unitId_(unitId),
      bloodType_(bloodType),
      volumeMl_(volumeMl),
      donorName_(donorName),
      donorPhone_(donorPhone),
      donationDate_(donationDate),
      expiryDate_(expiryDate),
      status_(status),
      usedForPatientId_(usedForPatientId),
      usedForOperation_(usedForOperation),
      usedDate_(usedDate) {}

const std::string& BloodUnit::getUnitId() const { return unitId_; }
const std::string& BloodUnit::getBloodType() const { return bloodType_; }
double BloodUnit::getVolumeMl() const { return volumeMl_; }
const std::string& BloodUnit::getDonorName() const { return donorName_; }
const std::string& BloodUnit::getDonorPhone() const { return donorPhone_; }
const std::string& BloodUnit::getDonationDate() const { return donationDate_; }
const std::string& BloodUnit::getExpiryDate() const { return expiryDate_; }
BloodUnitStatus BloodUnit::getStatus() const { return status_; }
const std::string& BloodUnit::getUsedForPatientId() const { return usedForPatientId_; }
const std::string& BloodUnit::getUsedForOperation() const { return usedForOperation_; }
const std::string& BloodUnit::getUsedDate() const { return usedDate_; }

void BloodUnit::setStatus(BloodUnitStatus status) { status_ = status; }
void BloodUnit::setUsedForPatientId(const std::string& patientId) { usedForPatientId_ = patientId; }
void BloodUnit::setUsedForOperation(const std::string& operationReason) { usedForOperation_ = operationReason; }
void BloodUnit::setUsedDate(const std::string& usedDate) { usedDate_ = usedDate; }

void BloodUnit::displayDetails() const {
    std::cout << "----- Blood Unit Details -----\n";
    std::cout << "Unit ID         : " << unitId_ << "\n";
    std::cout << "Blood Type      : " << bloodType_ << "\n";
    std::cout << "Volume (ml)     : " << volumeMl_ << "\n";
    std::cout << "Donor Name      : " << donorName_ << "\n";
    std::cout << "Donor Phone     : " << donorPhone_ << "\n";
    std::cout << "Donation Date   : " << donationDate_ << "\n";
    std::cout << "Expiry Date     : " << expiryDate_ << "\n";
    std::cout << "Status          : " << bloodUnitStatusToString(status_) << "\n";
    if (status_ == BloodUnitStatus::Used) {
        std::cout << "Used For Patient: " << usedForPatientId_ << "\n";
        std::cout << "Used For Op.    : " << usedForOperation_ << "\n";
        std::cout << "Used Date       : " << usedDate_ << "\n";
    }
    std::cout << "----------------------------\n";
}

// ---------------------------------------------------------------------
// BloodBank - internal helpers
// ---------------------------------------------------------------------
namespace {

std::string trimField(const std::string& s) {
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

double safeStod(const std::string& value, double fallback = 0.0) {
    const std::string trimmed = trimField(value);
    if (trimmed.empty()) return fallback;
    try {
        return std::stod(trimmed);
    } catch (const std::exception&) {
        return fallback;
    }
}

std::vector<std::string> readLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    if (!file.is_open()) return lines;
    std::string line;
    while (std::getline(file, line)) {
        if (!trimField(line).empty()) lines.push_back(line);
    }
    return lines;
}

} // anonymous namespace

// ---------------------------------------------------------------------
// BloodBank - ID generation
// ---------------------------------------------------------------------
std::string BloodBank::generateUnitId() {
    ++unitCounter_;
    std::ostringstream oss;
    oss << "BLD-" << std::setfill('0') << std::setw(4) << unitCounter_;
    return oss.str();
}

void BloodBank::resyncCounterFromExistingUnits() {
    for (const auto& u : units_) {
        const std::string& id = u.getUnitId();
        const std::string prefix = "BLD-";
        if (id.size() > prefix.size() && id.compare(0, prefix.size(), prefix) == 0) {
            try {
                int n = std::stoi(id.substr(prefix.size()));
                unitCounter_ = std::max(unitCounter_, n);
            } catch (const std::exception&) {
                // Non-numeric suffix on a manually-entered ID; ignore.
            }
        }
    }
}

// ---------------------------------------------------------------------
// BloodBank - donation intake
// ---------------------------------------------------------------------
std::string BloodBank::donateBlood(const std::string& donorName,
                                    const std::string& donorPhone,
                                    const std::string& bloodType,
                                    double volumeMl,
                                    const std::string& donationDate,
                                    int shelfLifeDays) {
    const std::string effectiveDonationDate =
        donationDate.empty() ? dateutils::today() : donationDate;
    const std::string expiryDate = dateutils::addDays(effectiveDonationDate, shelfLifeDays);

    const std::string unitId = generateUnitId();
    units_.emplace_back(unitId, bloodType, volumeMl, donorName, donorPhone,
                         effectiveDonationDate, expiryDate,
                         BloodUnitStatus::Available,
                         /*usedForPatientId=*/"", /*usedForOperation=*/"", /*usedDate=*/"");

    std::cout << "[BloodBank] Donation registered -> " << unitId << " ("
              << bloodType << ", " << volumeMl << "ml from " << donorName
              << "). Expires " << expiryDate << ".\n";

    return unitId;
}

// ---------------------------------------------------------------------
// BloodBank - issuing
// ---------------------------------------------------------------------
BloodUnit* BloodBank::issueBlood(const std::string& bloodType,
                                  const std::string& patientId,
                                  const std::string& operationReason,
                                  const std::string& usedDate) {
    refreshExpiredUnits();

    BloodUnit* best = nullptr;
    for (auto& u : units_) {
        if (u.getBloodType() != bloodType) continue;
        if (u.getStatus() != BloodUnitStatus::Available) continue;
        // First-expiry-first-out: prefer the unit that expires soonest,
        // which is standard blood-bank practice to minimize wastage.
        if (best == nullptr || u.getExpiryDate() < best->getExpiryDate()) {
            best = &u;
        }
    }

    if (best == nullptr) {
        std::cout << "[BloodBank] No available unit of type " << bloodType
                  << " for patient " << patientId << ".\n";
        return nullptr;
    }

    best->setStatus(BloodUnitStatus::Used);
    best->setUsedForPatientId(patientId);
    best->setUsedForOperation(operationReason);
    best->setUsedDate(usedDate.empty() ? dateutils::today() : usedDate);

    std::cout << "[BloodBank] Issued unit " << best->getUnitId() << " ("
              << bloodType << ") to patient " << patientId << " for \""
              << operationReason << "\".\n";

    return best;
}

// ---------------------------------------------------------------------
// BloodBank - housekeeping
// ---------------------------------------------------------------------
int BloodBank::refreshExpiredUnits() {
    int expiredCount = 0;
    const std::string todayStr = dateutils::today();
    for (auto& u : units_) {
        const bool isActive = (u.getStatus() == BloodUnitStatus::Available ||
                                u.getStatus() == BloodUnitStatus::Reserved);
        if (isActive && u.getExpiryDate() < todayStr) {
            u.setStatus(BloodUnitStatus::Expired);
            ++expiredCount;
        }
    }
    if (expiredCount > 0) {
        std::cout << "[BloodBank] " << expiredCount
                  << " unit(s) newly marked Expired.\n";
    }
    return expiredCount;
}

// ---------------------------------------------------------------------
// BloodBank - lookup
// ---------------------------------------------------------------------
BloodUnit* BloodBank::findUnitById(const std::string& unitId) {
    for (auto& u : units_) {
        if (u.getUnitId() == unitId) return &u;
    }
    return nullptr;
}

std::vector<BloodUnit*> BloodBank::findAvailableByType(const std::string& bloodType) {
    std::vector<BloodUnit*> result;
    for (auto& u : units_) {
        if (u.getBloodType() == bloodType && u.getStatus() == BloodUnitStatus::Available) {
            result.push_back(&u);
        }
    }
    std::sort(result.begin(), result.end(), [](const BloodUnit* a, const BloodUnit* b) {
        return a->getExpiryDate() < b->getExpiryDate();
    });
    return result;
}

// ---------------------------------------------------------------------
// BloodBank - reporting
// ---------------------------------------------------------------------
std::map<std::string, BloodBank::InventorySummaryEntry> BloodBank::getInventorySummary() const {
    std::map<std::string, InventorySummaryEntry> summary;
    for (const auto& u : units_) {
        if (u.getStatus() != BloodUnitStatus::Available) continue;
        auto& entry = summary[u.getBloodType()];
        entry.unitCount += 1;
        entry.totalVolumeMl += u.getVolumeMl();
    }
    return summary;
}

void BloodBank::printInventorySummary() const {
    const auto summary = getInventorySummary();
    std::cout << "===== Blood Bank Inventory (Available) =====\n";
    if (summary.empty()) {
        std::cout << "  (no available units)\n";
    } else {
        for (const auto& [type, entry] : summary) {
            std::cout << "  " << std::left << std::setw(5) << type
                      << " : " << entry.unitCount << " unit(s), "
                      << entry.totalVolumeMl << " ml total\n";
        }
    }
    std::cout << "==============================================\n";
}

// ---------------------------------------------------------------------
// BloodBank - raw access
// ---------------------------------------------------------------------
const std::vector<BloodUnit>& BloodBank::getUnits() const { return units_; }

void BloodBank::addRawUnit(const BloodUnit& unit) {
    units_.push_back(unit);
}

// ---------------------------------------------------------------------
// BloodBank - persistence
// ---------------------------------------------------------------------
void BloodBank::saveToCSV(const std::string& path) {
    std::error_code ec;
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path(), ec);
    }

    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[BloodBank] Error: could not open " << path << " for writing.\n";
        return;
    }
    // unitId,bloodType,volumeMl,donorName,donorPhone,donationDate,expiryDate,
    // status,usedForPatientId,usedForOperation,usedDate
    for (const auto& u : units_) {
        file << sanitizeField(u.getUnitId()) << ","
             << sanitizeField(u.getBloodType()) << ","
             << u.getVolumeMl() << ","
             << sanitizeField(u.getDonorName()) << ","
             << sanitizeField(u.getDonorPhone()) << ","
             << sanitizeField(u.getDonationDate()) << ","
             << sanitizeField(u.getExpiryDate()) << ","
             << bloodUnitStatusToString(u.getStatus()) << ","
             << sanitizeField(u.getUsedForPatientId()) << ","
             << sanitizeField(u.getUsedForOperation()) << ","
             << sanitizeField(u.getUsedDate()) << "\n";
    }
}

void BloodBank::loadFromCSV(const std::string& path) {
    units_.clear();
    for (const auto& line : readLines(path)) {
        auto f = splitCSVLine(line);
        if (f.size() < 11) {
            std::cerr << "[BloodBank] Skipping malformed unit row: " << line << "\n";
            continue;
        }
        units_.emplace_back(
            trimField(f[0]),                              // unitId
            trimField(f[1]),                              // bloodType
            safeStod(f[2]),                                // volumeMl
            trimField(f[3]),                              // donorName
            trimField(f[4]),                              // donorPhone
            trimField(f[5]),                              // donationDate
            trimField(f[6]),                              // expiryDate
            bloodUnitStatusFromString(trimField(f[7])),   // status
            trimField(f[8]),                              // usedForPatientId
            trimField(f[9]),                              // usedForOperation
            trimField(f[10]));                            // usedDate
    }
    resyncCounterFromExistingUnits();
}

} // namespace hms
