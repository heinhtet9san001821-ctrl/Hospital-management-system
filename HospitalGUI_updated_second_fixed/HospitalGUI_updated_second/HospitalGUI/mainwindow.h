#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "facility.h"
#include "hospital_manager.h"

#include <QMainWindow>
#include <QHash>
#include <QString>

class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QTabWidget;
class QTextEdit;
class QTimeEdit;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void goWelcome();
    void goGateway();
    void goLogin();
    void goSignup();
    void goEmergency();
    void submitLogin();
    void submitSignup();
    void submitEmergency();
    void suggestSpecialist();
    void openSpecialists();
    void showDoctorCard();
    void bookSelectedDoctor();
    void openRooms();
    void reserveSelectedRoom();
    void reserveNurse();
    void openPregnancy();
    void bookMaternity();
    void openDonateBlood();
    void donateBlood();
    void openReviews();
    void submitReview();
    void doctorQueue();
    void doctorPrevPatient();
    void doctorNextPatient();
    void doctorEmergencyTheatre();
    void doctorAnnouncements();
    void nurseWardList();
    void nurseMeds();
    void nurseReport();
    void nurseAnnouncements();
    void nurseBookings();
    void showNursePanel(QWidget *panel);
    void managerOverview();
    void managerStaff();
    void managerApproveSelected();
    void managerWithdrawSelected();
    void managerReinstateSelected();
    void managerDeletePatientSelected();
    void managerRooms();
    void managerBlood();
    void managerDiscardBlood();
    void managerIssueBlood();
    void managerAnnounce();
    void managerReviews();
    void managerAwardBonus();
    void managerMedicines();
    void managerAppointments();
    void managerAllowAppointment();
    void managerCancelAppointment();
    void managerAmbulances();
    void managerApproveEmergency();
    void managerReturnEmergency();
    void managerDoctorScheduling();
    void logout();
    void refreshPatientAppointments();
    void refreshPatientNurseBookings();

private:
    enum Page {
        Welcome = 0,
        Gateway,
        Login,
        Signup,
        Emergency,
        Patient,
        Doctor,
        Nurse,
        Manager,
        Invoice,
        Denied,
        Pending
    };

    void buildAllPages();
    void applyTheme();
    QWidget *makeHeader(const QString &title, bool showBack, const QString &backTarget);
    QLabel *makeAvatar(const QString &initials);
    void refreshHeader(QLabel *nameLabel, QLabel *roleLabel, QLabel *avatar);
    void enterRoleHome();
    void setDoctorLocked(bool locked);
    void setNurseLocked(bool locked);
    void fillSpecialistList();
    void fillDoctorQueue();
    void refreshDoctorClinicalNote();
    void showInvoice(const QString &text);
    QString suggestedSpecialtyFromText(const QString &text) const;
    QString displayNameOf(const hms::User &user) const;
    QString newPatientId() const;
    QString nowStamp() const;

    Ui::MainWindow *ui;
    hms::HospitalManager hospital_;
    hms::FacilityStore facilities_;

    QString currentUsername_;
    QString currentRole_;
    QString currentDisplayName_;
    QString currentPatientId_;
    QString selectedDoctorId_;
    int queueIndex_ = 0;
    bool doctorLocked_ = false;
    bool nurseLocked_ = false;
    QTabWidget *patientTabs_ = nullptr;

    QLineEdit *loginUser_ = nullptr;
    QLineEdit *loginPass_ = nullptr;

    QLineEdit *signName_ = nullptr;
    QLineEdit *signUser_ = nullptr;
    QLineEdit *signAge_ = nullptr;
    QComboBox *signGender_ = nullptr;
    QLineEdit *signContact_ = nullptr;
    QLineEdit *signAddress_ = nullptr;
    QLineEdit *signPass_ = nullptr;
    QLineEdit *signPass2_ = nullptr;
    QComboBox *signRole_ = nullptr;
    QLabel *signStrength_ = nullptr;

    QLineEdit *emName_ = nullptr;
    QLineEdit *emPhone_ = nullptr;
    QLineEdit *emLocation_ = nullptr;
    QComboBox *emSeverity_ = nullptr;
    QLabel *emResult_ = nullptr;

    QPlainTextEdit *patientIssue_ = nullptr;
    QLabel *patientAiHint_ = nullptr;
    QComboBox *patientManualSpec_ = nullptr;
    QListWidget *specialistList_ = nullptr;
    QLabel *doctorDetail_ = nullptr;
    QDateEdit *apptDate_ = nullptr;
    QTimeEdit *apptTime_ = nullptr;
    QComboBox *roomList_ = nullptr;
    QTableWidget *roomTable_ = nullptr;
    QTableWidget *patientAppointments_ = nullptr;
    QComboBox *patientNursePick_ = nullptr;
    QDateEdit *patientNurseDate_ = nullptr;
    QLineEdit *patientNurseReason_ = nullptr;
    QTableWidget *patientNurseBookings_ = nullptr;
    QLabel *roomHint_ = nullptr;
    QLabel *maternityHint_ = nullptr;
    QComboBox *bloodTypePick_ = nullptr;
    QLabel *bloodHint_ = nullptr;
    QComboBox *reviewTarget_ = nullptr;
    QComboBox *reviewStars_ = nullptr;
    QLineEdit *reviewNote_ = nullptr;

    QLabel *doctorPatientBox_ = nullptr;
    QLabel *doctorSpecialty_ = nullptr;
    QTextEdit *doctorNotes_ = nullptr;
    QListWidget *doctorQueueList_ = nullptr;
    QTextEdit *doctorNews_ = nullptr;
    QPushButton *btnQueue_ = nullptr;
    QPushButton *btnTheatre_ = nullptr;
    QPushButton *btnDocNews_ = nullptr;
    QWidget *doctorQueuePanel_ = nullptr;
    QWidget *doctorNotesPanel_ = nullptr;
    QWidget *doctorNewsPanel_ = nullptr;
    QWidget *doctorTheatrePanel_ = nullptr;
    QLabel *doctorTheatreStatus_ = nullptr;
    QPushButton *doctorTheatreAction_ = nullptr;
    QLabel *doctorNotePatient_ = nullptr;
    QHash<QString, QString> clinicalNotes_;

    QListWidget *nursePatients_ = nullptr;
    QListWidget *nurseMedsList_ = nullptr;
    QListWidget *nurseBookingsList_ = nullptr;
    QComboBox *nurseDoctorPick_ = nullptr;
    QPlainTextEdit *nurseNote_ = nullptr;
    QTextEdit *nurseNews_ = nullptr;
    QWidget *nurseWardPanel_ = nullptr;
    QWidget *nurseMedsPanel_ = nullptr;
    QWidget *nurseReportPanel_ = nullptr;
    QWidget *nurseNewsPanel_ = nullptr;
    QWidget *nurseBookingsPanel_ = nullptr;
    QPushButton *btnWard_ = nullptr;
    QPushButton *btnMeds_ = nullptr;
    QPushButton *btnReport_ = nullptr;
    QPushButton *btnNurseNews_ = nullptr;
    QPushButton *btnNurseBookings_ = nullptr;

    QTextEdit *managerView_ = nullptr;
    QListWidget *staffList_ = nullptr;
    QTableWidget *staffTable_ = nullptr;
    QComboBox *staffRoleFilter_ = nullptr;
    QTableWidget *bloodTable_ = nullptr;
    QTableWidget *ambulanceRequestTable_ = nullptr;
    QPushButton *approveEmergency_ = nullptr;
    QPushButton *returnEmergency_ = nullptr;
    QPushButton *clearEmergencySelection_ = nullptr;
    QPlainTextEdit *announceEdit_ = nullptr;
    QTableWidget *announcementTable_ = nullptr;
    QComboBox *announcementPick_ = nullptr;
    QComboBox *managerBloodUnitPick_ = nullptr;
    QComboBox *managerBloodTypePick_ = nullptr;
    QComboBox *managerBloodPatient_ = nullptr;
    QLineEdit *managerBloodOperation_ = nullptr;
    QWidget *managerBloodActions_ = nullptr;
    QWidget *managerStaffActions_ = nullptr;
    QWidget *managerNoticePanel_ = nullptr;
    QWidget *managerScheduleActions_ = nullptr;
    QComboBox *managerDoctorFilter_ = nullptr;
    QComboBox *managerScheduleStatusFilter_ = nullptr;
    QTableWidget *scheduleTable_ = nullptr;

    QTextEdit *invoiceView_ = nullptr;

    QLabel *patName_ = nullptr;
    QLabel *patRole_ = nullptr;
    QLabel *patAvatar_ = nullptr;
    QLabel *docName_ = nullptr;
    QLabel *docRole_ = nullptr;
    QLabel *docAvatar_ = nullptr;
    QLabel *nurName_ = nullptr;
    QLabel *nurRole_ = nullptr;
    QLabel *nurAvatar_ = nullptr;
    QLabel *mgrName_ = nullptr;
    QLabel *mgrRole_ = nullptr;
    QLabel *mgrAvatar_ = nullptr;
};

#endif
