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