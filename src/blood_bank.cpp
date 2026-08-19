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