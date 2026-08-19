#ifndef HMS_BLOOD_BANK_H
#define HMS_BLOOD_BANK_H

#include <map>
#include <string>
#include <vector>

namespace hms {

// Lifecycle state of a single donated blood unit ("bottle").
enum class BloodUnitStatus {
    Available,  // in inventory, ready to be issued
    Reserved,   // earmarked for a specific patient/operation but not yet drawn
    Used,       // transfused / consumed
    Expired,    // past its expiry date, no longer usable
    Discarded   // removed from inventory (damaged, failed screening, etc.)
};