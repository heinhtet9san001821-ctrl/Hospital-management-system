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

std::string bloodUnitStatusToString(BloodUnitStatus status);
BloodUnitStatus bloodUnitStatusFromString(const std::string& value);

// A single donated "bottle" of blood, tracked from donation through to
// use or expiry. Whole blood is conventionally usable for ~42 days after
// donation when refrigerated -- that shelf life is applied automatically
// when a donation is recorded (see BloodBank::donateBlood).
class BloodUnit {
public:
    BloodUnit(const std::string& unitId,
              const std::string& bloodType,
              double volumeMl,
              const std::string& donorName,
              const std::string& donorPhone,
              const std::string& donationDate,
              const std::string& expiryDate,
              BloodUnitStatus status,
              const std::string& usedForPatientId,
              const std::string& usedForOperation,
              const std::string& usedDate);

    // Getters
    const std::string& getUnitId() const;
    const std::string& getBloodType() const;
    double getVolumeMl() const;
    const std::string& getDonorName() const;
    const std::string& getDonorPhone() const;
    const std::string& getDonationDate() const;
    const std::string& getExpiryDate() const;
    BloodUnitStatus getStatus() const;
    const std::string& getUsedForPatientId() const;
    const std::string& getUsedForOperation() const;
    const std::string& getUsedDate() const;

    // Mutators (used when issuing a unit or marking it expired/discarded)
    void setStatus(BloodUnitStatus status);
    void setUsedForPatientId(const std::string& patientId);
    void setUsedForOperation(const std::string& operationReason);
    void setUsedDate(const std::string& usedDate);

    void displayDetails() const;

private:
    std::string unitId_;
    std::string bloodType_;
    double volumeMl_;
    std::string donorName_;
    std::string donorPhone_;
    std::string donationDate_;
    std::string expiryDate_;
    BloodUnitStatus status_;
    std::string usedForPatientId_;   // empty until issued
    std::string usedForOperation_;   // e.g. "Emergency Surgery - RTA", empty until issued
    std::string usedDate_;           // empty until issued
};