#include "patient.h"
#include "doctor.h"
#include "nurse.h"
#include "user.h"
#include <iostream>

int main() {
    std::cout << "\033[1;32m[SUCCESS] HMS Core OOP Modules Phase 1 Linked Perfectly!\033[0m\n\n";

    // Testing the patient obj
    hms::Patient p1("P001", "Aung Aung", 25, "Male", "091234567", "PAT_001", "No history", "General", 101, true);
    p1.displayDetails();
    std::cout << "\n";

    // Testing the doctor obj
    hms::Doctor d1("D001", "Dr. Thura", 40, "Male", "097654321", "DOC_001", "Cardiologist", 302, true);
    d1.displayDetails();
    std::cout << "\n";

    // Testing the nurse obj
    // ID, Name, Age, Gender, Phone, NurseId, AssignedWard, ShiftTime
    hms::Nurse n1("N001", "Daw Nu Nu", 30, "Female", "094445556", "NUR_001", "ICU Ward", "Night Shift");
    n1.displayDetails();
    std::cout << "\n";

    // User account and encryption test
    hms::User u1("admin", "secure_password_hash_xyz", "Owner");
    std::cout << "\033[1;36m-----------------------------------------\033[0m\n";
    std::cout << "\033[1;35m          USER ACCOUNT SYSTEM            \033[0m\n";
    std::cout << "\033[1;36m-----------------------------------------\033[0m\n";
    std::cout << "Username     : " << u1.getUsername() << "\n";
    std::cout << "Access Role  : \033[1;33m[" << u1.getRole() << "]\033[0m\n";
    
    // Login Verification
    std::cout << "Authentication: ";
    if (u1.authenticate("secure_password_hash_xyz")) {
        std::cout << "\033[1;32m[ACCESS GRANTED - VERIFIED]\033[0m\n";
    } else {
        std::cout << "\033[1;31m[ACCESS DENIED]\033[0m\n";
    }
    std::cout << "\033[1;36m-----------------------------------------\033[0m\n";

    return 0;
}
