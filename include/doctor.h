#ifndef HMS_DOCTOR_H
#define HMS_DOCTOR_H

#include "person.h"
#include <string>

namespace hms {
// Class representing a Doctor, inheriting from the Person class
class Doctor : public Person {
public:
    Doctor(const std::string& id,
           const std::string& name,
           int age,
           const std::string& gender,
           const std::string& phoneNumber,
           const std::string& doctorId,
           const std::string& specialization,
           int roomNo,
           bool isAvailable);

    ~Doctor() override = default;
    // Getter function
    const std::string& getDoctorId() const;
    const std::string& getSpecialization() const;
    int getRoomNo() const;
    bool isAvailable() const;
    //setter functions
    void setSpecialization(const std::string& specialization);
    void setRoomNo(int roomNo);
    void setAvailable(bool isAvailable);
    //Method to display the details of the doctor
    void displayDetails() const override;
    // Private member variables specific to the Doctor class
private:
    std::string doctorId_;
    std::string specialization_;
    int roomNo_;
    bool isAvailable_;
};

} // namespace hms

#endif // HMS_DOCTOR_H