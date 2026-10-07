#ifndef HMS_PATIENT_H
#define HMS_PATIENT_H

#include "person.h"
#include <string>

namespace hms {

class Patient : public Person {
public:
    Patient(const std::string& id,
            const std::string& name,
            int age,
            const std::string& gender,
            const std::string& phoneNumber,
            const std::string& patientId,
            const std::string& medicalHistory,
            const std::string& treatmentType,
            int assignedRoomNo,
            bool isEmergency);

    ~Patient() override = default;

    const std::string& getPatientId() const;
    const std::string& getMedicalHistory() const;
    const std::string& getTreatmentType() const;
    int getAssignedRoomNo() const;
    bool isEmergency() const;

    void setMedicalHistory(const std::string& medicalHistory);
    void setTreatmentType(const std::string& treatmentType);
    void setAssignedRoomNo(int roomNo);
    void setEmergency(bool isEmergency);

    void displayDetails() const override;

private:
    std::string patientId_;
    std::string medicalHistory_;
    std::string treatmentType_;
    int assignedRoomNo_;
    bool isEmergency_;
};

} // namespace hms

#endif // HMS_PATIENT_H