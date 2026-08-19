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