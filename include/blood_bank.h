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

// Manages the full blood-unit inventory: donation intake, issuing units
// for treatment/surgery, expiry housekeeping, inventory reporting, and
// CSV persistence. Designed to be owned as a member of HospitalManager,
// but usable standalone.
class BloodBank {
public:
    BloodBank() = default;

    // ---- Donation intake ----
    // Registers a new donated unit and computes its expiry date
    // automatically (donationDate + shelfLifeDays, default 42 days for
    // whole blood). If donationDate is left empty, today's date is used.
    // Returns the newly generated unit ID.
    std::string donateBlood(const std::string& donorName,
                             const std::string& donorPhone,
                             const std::string& bloodType,
                             double volumeMl,
                             const std::string& donationDate = "",
                             int shelfLifeDays = 42);

    // ---- Issuing blood for treatment ----
    // Finds the soonest-expiring Available unit of the requested blood
    // type (a "first-expiry-first-out" policy, standard blood-bank
    // practice for minimizing wastage), marks it Used, and records who
    // it was used for and why. Returns a pointer to the issued unit, or
    // nullptr if no matching unit is currently available.
    BloodUnit* issueBlood(const std::string& bloodType,
                          const std::string& patientId,
                          const std::string& operationReason,
                          const std::string& usedDate = "");

    // ---- Housekeeping ----
    // Scans the inventory and flips any Available/Reserved unit whose
    // expiry date has passed into Expired status. Returns how many
    // units were newly marked expired. Safe to call at any time, e.g.
    // once at startup and again before issuing a unit.
    int refreshExpiredUnits();

    // ---- Lookup ----
    BloodUnit* findUnitById(const std::string& unitId);
    std::vector<BloodUnit*> findAvailableByType(const std::string& bloodType);

    // ---- Reporting ----
    struct InventorySummaryEntry {
        int unitCount = 0;
        double totalVolumeMl = 0.0;
    };
    // Available-only inventory, grouped by blood type.
    std::map<std::string, InventorySummaryEntry> getInventorySummary() const;
    void printInventorySummary() const;

    // ---- Raw access (used by HospitalManager / CSV loader) ----
    const std::vector<BloodUnit>& getUnits() const;
    void addRawUnit(const BloodUnit& unit);

    // ---- Persistence ----
    void saveToCSV(const std::string& path = "data/bloodbank.csv");
    void loadFromCSV(const std::string& path = "data/bloodbank.csv");

private:
    std::vector<BloodUnit> units_;
    int unitCounter_ = 0;

    std::string generateUnitId();
    // Rescans units_ for the highest existing "BLD-####" id so the
    // counter never collides with data reloaded from CSV.
    void resyncCounterFromExistingUnits();
};

} // namespace hms

#endif // HMS_BLOOD_BANK_H
