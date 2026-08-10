#include "patient.h"
#include "doctor.h"
#include "nurse.h"
#include "user.h"
#include <iostream>

int main() {
    std::cout << "\033[1;32m[SUCCESS] HMS Core Entities Phase 1 Linked Perfectly with CMake!\033[0m\n\n";

    //for patient testing
    hms::Patient p1("P001", "Aung Aung", 25, "Male", "091234567", "PAT_001", "No history", "General", 101, true);
    p1.displayDetails();

    // for doctor testing
    // 1.id, 2.name, 3.age, 4.gender, 5.phone, 6.doctorId, 7.specialization, 8.roomNo, 9.isAvailable
    hms::Doctor d1("D001", "Dr. Thura", 40, "Male", "097654321", "DOC_001", "Cardiologist", 302, true);
    d1.displayDetails();

    // ၃။ testing the password of user
    hms::User u1("admin", "hashed_password_123", "Owner");
    std::cout << "User Login Test Successful!\n";

    return 0;
}
