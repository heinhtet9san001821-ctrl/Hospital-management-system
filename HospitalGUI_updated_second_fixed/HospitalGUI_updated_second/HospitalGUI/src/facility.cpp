#include "facility.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace hms {
namespace {

std::vector<std::string> split(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    std::stringstream ss(line);
    while (std::getline(ss, current, '|')) {
        fields.push_back(current);
    }
    return fields;
}

std::string clean(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (char c : value) {
        if (c == '|' || c == '\n' || c == '\r') result += ' ';
        else result += c;
    }
    return result;
}

std::vector<std::string> readLines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    std::string line;
    while (file && std::getline(file, line)) {
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

int safeInt(const std::string& value, int fallback = 0) {
    try {
        size_t parsed = 0;
        const int result = std::stoi(value, &parsed);
        return parsed == value.size() ? result : fallback;
    } catch (const std::exception&) {
        return fallback;
    }
}

} // namespace

void FacilityStore::setDataDirectory(const std::string& dir) {
    dir_ = dir;
}

std::string FacilityStore::path(const std::string& file) const {
    return dir_ + "/" + file;
}

void FacilityStore::seedIfEmpty() {
    if (rooms_.empty()) {
        const std::pair<std::string, int> wards[] = {
            {"General Ward", 20},
            {"Semi-Private", 10},
            {"Private", 8},
            {"VIP", 4},
            {"Critical Care", 6},
            {"Isolation", 4},
        };
        int n = 1;
        for (const auto& w : wards) {
            for (int i = 0; i < w.second; ++i) {
                RoomBed bed;
                bed.roomId = "R" + std::to_string(100 + n);
                bed.wardType = w.first;
                bed.status = "Vacant";
                rooms_.push_back(bed);
                ++n;
            }
        }
    }
    int nextRoomNumber = 1;
    for (const auto& room : rooms_) {
        try {
            nextRoomNumber = std::max(nextRoomNumber, std::stoi(room.roomId.substr(1)) + 1);
        } catch (const std::exception&) {
        }
    }
    while (rooms_.size() < 1000) {
        rooms_.push_back({"R" + std::to_string(nextRoomNumber++), "General Ward", "Vacant", ""});
    }
    const std::pair<std::string, int> roomTypes[] = {
        {"General Ward", 500},
        {"Semi-Private", 150},
        {"Private", 120},
        {"VIP", 80},
        {"Critical Care", 100},
        {"Isolation", 50},
    };
    int typeCounts[sizeof(roomTypes) / sizeof(roomTypes[0])] = {};
    for (const auto& room : rooms_) {
        for (size_t i = 0; i < sizeof(roomTypes) / sizeof(roomTypes[0]); ++i) {
            if (room.wardType == roomTypes[i].first) {
                ++typeCounts[i];
                break;
            }
        }
    }
    for (auto& room : rooms_) {
        if (room.status != "Vacant" || room.wardType != "General Ward") continue;
        for (size_t i = 1; i < sizeof(roomTypes) / sizeof(roomTypes[0]); ++i) {
            if (typeCounts[i] < roomTypes[i].second) {
                room.wardType = roomTypes[i].first;
                ++typeCounts[i];
                --typeCounts[0];
                break;
            }
        }
    }
    if (theatres_.size() < 20) {
        const auto existing = theatres_.size();
        for (size_t i = existing; i < 20; ++i) {
            const int roomNo = 801 + static_cast<int>(i);
            theatres_.push_back({roomNo, "Operating Theatre " + std::to_string(i + 1),
                                 true, false, ""});
        }
    }
    const auto addMedicineIfMissing = [this](const std::string& name, int quantity, const std::string& unit) {
        const auto found = std::find_if(medicines_.begin(), medicines_.end(),
                                        [&name](const MedicineStock& medicine) { return medicine.name == name; });
        if (found == medicines_.end()) medicines_.push_back({name, quantity, unit});
    };
    addMedicineIfMissing("Paracetamol 500mg", 420, "tab");
    addMedicineIfMissing("Amoxicillin 250mg", 180, "cap");
    addMedicineIfMissing("Normal Saline 500ml", 96, "bag");
    addMedicineIfMissing("Insulin (vial)", 28, "vial");
    addMedicineIfMissing("Morphine 10mg", 40, "amp");
    addMedicineIfMissing("Ibuprofen 400mg", 240, "tab");
    addMedicineIfMissing("Ceftriaxone 1g", 72, "vial");
    addMedicineIfMissing("Omeprazole 20mg", 160, "cap");
    addMedicineIfMissing("Salbutamol inhaler", 35, "inhaler");
    addMedicineIfMissing("Lidocaine 2%", 64, "vial");
    addMedicineIfMissing("Dextrose 5%", 88, "bag");
    addMedicineIfMissing("Aspirin 75mg", 210, "tab");
    addMedicineIfMissing("Metformin 500mg", 190, "tab");
    if (announcements_.empty()) {
        announcements_.push_back({"ANN-0001", "Hein Htet San", "2026-09-01 09:00",
                                  "Welcome to Health++. Please keep ward notes current and review the blood bank expiry list each morning."});
    }
}

void FacilityStore::loadAll() {
    rooms_.clear();
    theatres_.clear();
    announcements_.clear();
    reviews_.clear();
    medicines_.clear();
    reports_.clear();
    emergencyRequests_.clear();
    nurseBookings_.clear();

    for (const auto& line : readLines(path("rooms.txt"))) {
        auto f = split(line);
        if (f.size() < 4) continue;
        rooms_.push_back({f[0], f[1], f[2], f[3]});
    }
    for (const auto& line : readLines(path("theatres.txt"))) {
        auto f = split(line);
        if (f.size() < 5) continue;
        const int roomNo = safeInt(f[0], -1);
        if (roomNo < 0) continue;
        theatres_.push_back({roomNo, f[1], f[2] == "1", f[3] == "1", f[4]});
    }
    for (const auto& line : readLines(path("announcements.txt"))) {
        auto f = split(line);
        if (f.size() < 4) continue;
        announcements_.push_back({f[0], f[1], f[2], f[3]});
    }
    for (const auto& line : readLines(path("reviews.txt"))) {
        auto f = split(line);
        if (f.size() < 7) continue;
        const int stars = safeInt(f[4], 0);
        if (stars < 1 || stars > 5) continue;
        reviews_.push_back({f[0], f[1], f[2], f[3], stars, f[5], f[6]});
    }
    for (const auto& line : readLines(path("medicines.txt"))) {
        auto f = split(line);
        if (f.size() < 3) continue;
        const int quantity = safeInt(f[1], -1);
        if (quantity < 0) continue;
        medicines_.push_back({f[0], quantity, f[2]});
    }
    for (const auto& line : readLines(path("reports.txt"))) {
        auto f = split(line);
        if (f.size() < 6) continue;
        reports_.push_back({f[0], f[1], f[2], f[3], f[4], f[5]});
    }
    for (const auto& line : readLines(path("emergency_requests.txt"))) {
        auto f = split(line);
        if (f.size() < 8) continue;
        const int ambulances = safeInt(f[7], -1);
        if (ambulances < 0) continue;
        emergencyRequests_.push_back({f[0], f[1], f[2], f[3], f[4], f[5], f[6], ambulances});
    }
    for (const auto& line : readLines(path("nurse_bookings.txt"))) {
        auto f = split(line);
        if (f.size() < 8) continue;
        nurseBookings_.push_back({f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7]});
    }
    seedIfEmpty();
}

void FacilityStore::saveAll() const {
    std::ofstream rooms(path("rooms.txt"), std::ios::trunc);
    for (const auto& r : rooms_) {
        rooms << clean(r.roomId) << '|' << clean(r.wardType) << '|'
              << clean(r.status) << '|' << clean(r.occupantId) << '\n';
    }
    std::ofstream th(path("theatres.txt"), std::ios::trunc);
    for (const auto& t : theatres_) {
        th << t.roomNo << '|' << clean(t.name) << '|'
           << (t.fullyEquipped ? 1 : 0) << '|' << (t.occupied ? 1 : 0) << '|'
           << clean(t.occupiedByDoctor) << '\n';
    }
    std::ofstream an(path("announcements.txt"), std::ios::trunc);
    for (const auto& a : announcements_) {
        an << clean(a.id) << '|' << clean(a.author) << '|'
           << clean(a.createdAt) << '|' << clean(a.body) << '\n';
    }
    std::ofstream rv(path("reviews.txt"), std::ios::trunc);
    for (const auto& r : reviews_) {
        rv << clean(r.id) << '|' << clean(r.patientName) << '|'
           << clean(r.targetRole) << '|' << clean(r.targetName) << '|'
           << r.stars << '|' << clean(r.note) << '|' << clean(r.createdAt) << '\n';
    }
    std::ofstream md(path("medicines.txt"), std::ios::trunc);
    for (const auto& m : medicines_) {
        md << clean(m.name) << '|' << m.quantity << '|' << clean(m.unit) << '\n';
    }
    std::ofstream rp(path("reports.txt"), std::ios::trunc);
    for (const auto& r : reports_) {
        rp << clean(r.id) << '|' << clean(r.nurseName) << '|'
           << clean(r.doctorName) << '|' << clean(r.patientName) << '|'
           << clean(r.note) << '|' << clean(r.createdAt) << '\n';
    }
        std::ofstream er(path("emergency_requests.txt"), std::ios::trunc);
        for (const auto& request : emergencyRequests_) {
         er << clean(request.id) << '|' << clean(request.callerName) << '|'
            << clean(request.phone) << '|' << clean(request.location) << '|'
            << clean(request.severity) << '|' << clean(request.createdAt) << '|'
            << clean(request.status) << '|' << request.ambulancesSent << '\n';
        }
        std::ofstream nb(path("nurse_bookings.txt"), std::ios::trunc);
        for (const auto& booking : nurseBookings_) {
            nb << clean(booking.id) << '|' << clean(booking.patientId) << '|'
               << clean(booking.patientName) << '|' << clean(booking.nurseId) << '|'
               << clean(booking.nurseName) << '|' << clean(booking.date) << '|'
               << clean(booking.reason) << '|' << clean(booking.status) << '\n';
        }
}

bool FacilityStore::reserveRoom(const std::string& roomId, const std::string& occupantId) {
    for (auto& r : rooms_) {
        if (r.roomId == roomId && r.status == "Vacant") {
            r.status = "Reserved";
            r.occupantId = occupantId;
            saveAll();
            return true;
        }
    }
    return false;
}

bool FacilityStore::releaseRoom(const std::string& roomId, const std::string& occupantId) {
    for (auto& r : rooms_) {
        if (r.roomId == roomId && r.occupantId == occupantId) {
            r.status = "Vacant";
            r.occupantId.clear();
            saveAll();
            return true;
        }
    }
    return false;
}
bool FacilityStore::occupyRoom(const std::string& roomId, const std::string& occupantId) {
    for (auto& r : rooms_) {
        if (r.roomId == roomId && (r.status == "Vacant" || r.status == "Reserved")) {
            r.status = "Occupied";
            r.occupantId = occupantId;
            saveAll();
            return true;
        }
    }
    return false;
}

Theatre* FacilityStore::findFreeTheatre() {
    for (auto& t : theatres_) {
        if (!t.occupied && t.fullyEquipped) return &t;
    }
    return nullptr;
}

void FacilityStore::addAnnouncement(const Announcement& a) {
    announcements_.insert(announcements_.begin(), a);
    saveAll();
}

bool FacilityStore::updateAnnouncement(const std::string& id, const Announcement& updated) {
    for (auto& announcement : announcements_) {
        if (announcement.id == id) {
            announcement = updated;
            saveAll();
            return true;
        }
    }
    return false;
}

bool FacilityStore::removeAnnouncement(const std::string& id) {
    const auto oldSize = announcements_.size();
    announcements_.erase(std::remove_if(announcements_.begin(), announcements_.end(),
                                         [&id](const Announcement& announcement) {
                                             return announcement.id == id;
                                         }),
                         announcements_.end());
    if (announcements_.size() == oldSize) return false;
    saveAll();
    return true;
}

void FacilityStore::addReview(const StarReview& r) {
    reviews_.insert(reviews_.begin(), r);
    saveAll();
}

void FacilityStore::addReport(const NurseReport& r) {
    reports_.insert(reports_.begin(), r);
    saveAll();
}

void FacilityStore::addEmergencyRequest(const EmergencyRequest& request) {
    emergencyRequests_.insert(emergencyRequests_.begin(), request);
    saveAll();
}

void FacilityStore::addNurseBooking(const NurseBooking& booking) {
    nurseBookings_.insert(nurseBookings_.begin(), booking);
    saveAll();
}

} // namespace hms
