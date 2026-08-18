#include "hospital_manager.h"
#include <iostream>

int main() {
    std::cout << "\033[1;32m[SUCCESS] HMS Core OOP Modules Phase 3 Linked Perfectly!\033[0m\n\n";

    hms::HospitalManager hms;

    // ---- Seed base data (Phase 1/2 style) ----
    hms.addDoctor(hms::Doctor("D001", "Dr. Thura", 40, "Male", "097654321",
                               "DOC_001", "Cardiologist", 302, true));
    hms.addDoctor(hms::Doctor("D002", "Dr. Su Su", 38, "Female", "095512345",
                               "DOC_002", "General Physician", 305, false));

    hms.addNurse(hms::Nurse("N001", "Daw Nu Nu", 30, "Female", "094445556",
                             "NUR_001", "ICU Ward", "Night Shift"));

    hms.addUser(hms::User("admin", "hunter2", "Owner"));

    // Patient designated for the General Ward -- addPatient() will
    // auto-allocate a bed for them (Phase 3.2).
    hms.addPatient(hms::Patient("P001", "Aung Aung", 25, "Male", "091234567",
                                 "PAT_001", "No history, mild asthma",
                                 "General Ward", 0, false));

    std::cout << "\n--- Phase 3.1: Appointment Scheduling ---\n";
    hms.bookAppointment("PAT_001", "DOC_001", "2026-08-20 10:00");
    // Second doctor is currently unavailable -> this should fail cleanly.
    hms.bookAppointment("PAT_001", "DOC_002", "2026-08-21 09:00");

    std::cout << "\n--- Phase 3.2: Ward Occupancy ---\n";
    hms.printRoomOccupancy();

    std::cout << "\n--- Phase 3.3: Billing ---\n";
    hms.calculateTotalBill("PAT_001", /*admittedDays=*/3);

    std::cout << "\n--- Phase 3.4: Emergency Ambulance Dispatch ---\n";
    hms.triggerEmergencyRescue("Ko Ko Lwin", "099887766", "Bogyoke Market, Yangon");
    hms.calculateTotalBill("EMG-0001");

    std::cout << "\n--- Blood Bank ---\n";
    hms.getBloodBank().donateBlood("Zin Mar Aung", "094411223", "O+", 450.0);
    hms.getBloodBank().donateBlood("Htet Aung", "098899001", "O+", 450.0, "2026-06-01"); // will be near/past expiry
    hms.getBloodBank().donateBlood("Nilar Win", "093344556", "A+", 450.0);
    hms.getBloodBank().printInventorySummary();

    hms.getBloodBank().issueBlood("O+", "EMG-0001", "Emergency Trauma Surgery");
    hms.getBloodBank().refreshExpiredUnits();
    hms.getBloodBank().printInventorySummary();

    std::cout << "\n--- Persistence ---\n";
    hms.saveAllToCSV();
    std::cout << "All data saved to data/*.csv\n";

    hms::HospitalManager reloaded;
    reloaded.loadAllFromCSV();
    std::cout << "Reloaded -> Patients: " << reloaded.getPatients().size()
              << ", Appointments: " << reloaded.getAppointments().size()
              << ", Blood Units: " << reloaded.getBloodBank().getUnits().size() << "\n";
    reloaded.printRoomOccupancy();

    return 0;
}