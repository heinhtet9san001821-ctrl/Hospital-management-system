#ifndef HMS_FACILITY_H
#define HMS_FACILITY_H

#include <string>
#include <vector>

namespace hms {

struct RoomBed {
    std::string roomId;
    std::string wardType;
    std::string status; // Vacant, Occupied, Reserved
    std::string occupantId;
};

struct Theatre {
    int roomNo = 0;
    std::string name;
    bool fullyEquipped = true;
    bool occupied = false;
    std::string occupiedByDoctor;
};

struct Announcement {
    std::string id;
    std::string author;
    std::string createdAt;
    std::string body;
};

struct StarReview {
    std::string id;
    std::string patientName;
    std::string targetRole;
    std::string targetName;
    int stars = 5;
    std::string note;
    std::string createdAt;
};

struct MedicineStock {
    std::string name;
    int quantity = 0;
    std::string unit;
};

struct NurseReport {
    std::string id;
    std::string nurseName;
    std::string doctorName;
    std::string patientName;
    std::string note;
    std::string createdAt;
};

struct EmergencyRequest {
    std::string id;
    std::string callerName;
    std::string phone;
    std::string location;
    std::string severity;
    std::string createdAt;
    std::string status; // Pending, Dispatched, Declined
    int ambulancesSent = 0;
};

struct NurseBooking {
    std::string id;
    std::string patientId;
    std::string patientName;
    std::string nurseId;
    std::string nurseName;
    std::string date;
    std::string reason;
    std::string status; // Requested, Confirmed, Completed, Cancelled
};

class FacilityStore {
public:
    void setDataDirectory(const std::string& dir);
    void loadAll();
    void saveAll() const;

    void seedIfEmpty();

    std::vector<RoomBed>& rooms() { return rooms_; }
    const std::vector<RoomBed>& rooms() const { return rooms_; }
    std::vector<Theatre>& theatres() { return theatres_; }
    const std::vector<Theatre>& theatres() const { return theatres_; }
    std::vector<Announcement>& announcements() { return announcements_; }
    const std::vector<Announcement>& announcements() const { return announcements_; }
    std::vector<StarReview>& reviews() { return reviews_; }
    const std::vector<StarReview>& reviews() const { return reviews_; }
    std::vector<MedicineStock>& medicines() { return medicines_; }
    const std::vector<MedicineStock>& medicines() const { return medicines_; }
    std::vector<NurseReport>& reports() { return reports_; }
    const std::vector<NurseReport>& reports() const { return reports_; }
    std::vector<EmergencyRequest>& emergencyRequests() { return emergencyRequests_; }
    const std::vector<EmergencyRequest>& emergencyRequests() const { return emergencyRequests_; }
    std::vector<NurseBooking>& nurseBookings() { return nurseBookings_; }
    const std::vector<NurseBooking>& nurseBookings() const { return nurseBookings_; }

    bool reserveRoom(const std::string& roomId, const std::string& occupantId);
    bool releaseRoom(const std::string& roomId, const std::string& occupantId);
    bool occupyRoom(const std::string& roomId, const std::string& occupantId);
    Theatre* findFreeTheatre();

    void addAnnouncement(const Announcement& a);
    bool updateAnnouncement(const std::string& id, const Announcement& updated);
    bool removeAnnouncement(const std::string& id);
    void addReview(const StarReview& r);
    void addReport(const NurseReport& r);
    void addEmergencyRequest(const EmergencyRequest& request);
    void addNurseBooking(const NurseBooking& booking);

private:
    std::string dir_ = "data";
    std::vector<RoomBed> rooms_;
    std::vector<Theatre> theatres_;
    std::vector<Announcement> announcements_;
    std::vector<StarReview> reviews_;
    std::vector<MedicineStock> medicines_;
    std::vector<NurseReport> reports_;
    std::vector<EmergencyRequest> emergencyRequests_;
    std::vector<NurseBooking> nurseBookings_;

    std::string path(const std::string& file) const;
};

} // namespace hms

#endif
