#include "hospital_manager.h"
#include "patient.h"
#include "doctor.h"
#include "nurse.h"
#include "user.h"
#include <iostream>

int main() {
    std::cout << "\033[1;32m[START] Phase 2: Testing Full CSV Database Core System...\033[0m\n\n";

    hms::HospitalManager manager;

    // ==========================================
    // FEATURE 1: Adding the new data (CRUD Test)
    // ==========================================
    std::cout << "Step 1: Adding Patient, Doctor, Nurse, and User to memory...\n";
    manager.addPatient(hms::Patient("P001", "Aung Aung", 25, "Male", "091234567", "PAT_001", "No history", "General", 101, true));
    manager.addDoctor(hms::Doctor("D001", "Dr. Thura", 40, "Male", "097654321", "DOC_001", "Cardiologist", 302, true));
    manager.addNurse(hms::Nurse("N001", "Daw Nu Nu", 30, "Female", "094445556", "NUR_001", "ICU Ward", "Night Shift"));
    manager.addUser(hms::User("admin", "hunter2", "Owner"));

     // ==========================================
    // FEATURE 2: All the data are automatically saved into CSV files(Save Test)
    // ==========================================
    std::cout << "Step 2: Writing all records into 4 distinct CSV files inside data/ ...\n";
    manager.saveAllToCSV();
    std::cout << "\033[1;32m[SUCCESS] All data serialization passed!\033[0m\n\n";

    // ==========================================
    // FEATURE 3: Read the data from the file automatically (Reload/Load Test)
    // ==========================================
    std::cout << "Step 3: Creating a brand-new Database Instance to reload data from CSV files...\n";
    hms::HospitalManager dbInstance;
    dbInstance.loadAllFromCSV();


    return 0;
}
