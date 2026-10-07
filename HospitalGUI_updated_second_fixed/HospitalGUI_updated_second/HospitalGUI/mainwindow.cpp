#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QApplication>
#include <QComboBox>
#include <algorithm>
#include <map>
#include <QDate>
#include <QDateEdit>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTextEdit>
#include <QTableWidget>
#include <QTabWidget>
#include <QTimeEdit>
#include <QHeaderView>
#include <QVBoxLayout>

#include <set>
#include <stdexcept>

namespace {

QPushButton *makeBtn(const QString &text, const QString &objectName = QString())
{
    auto *b = new QPushButton(text);
    if (!objectName.isEmpty()) b->setObjectName(objectName);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

QFrame *card()
{
    auto *f = new QFrame;
    f->setObjectName("card");
    return f;
}

void place(QWidget *page, QWidget *inner)
{
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    lay->addWidget(inner);
}

QWidget *scrollWrap(QWidget *inner)
{
    auto *area = new QScrollArea;
    area->setWidgetResizable(true);
    area->setWidget(inner);
    return area;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setMinimumSize(1100, 720);
    applyTheme();

#ifdef HMS_SOURCE_DIR
    QDir::setCurrent(QString::fromUtf8(HMS_SOURCE_DIR));
#endif
    QDir().mkpath("data");

    hospital_.loadAllFromCSV();
    hospital_.seedDefaultsIfNeeded();
    facilities_.setDataDirectory("data");
    facilities_.loadAll();
    facilities_.saveAll();
    int availableAmbulances = 20;
    for (const auto &request : facilities_.emergencyRequests()) {
        if (request.status == "Dispatched")
            availableAmbulances -= request.ambulancesSent;
    }
    hospital_.setAmbulancesAvailable(availableAmbulances);

    buildAllPages();
    ui->mainStackedWidget->setCurrentIndex(Welcome);
}

MainWindow::~MainWindow()
{
    hospital_.saveAllToCSV();
    facilities_.saveAll();
    delete ui;
}

void MainWindow::applyTheme()
{
    QFile file(":/theme/health.qss");
    if (!file.open(QFile::ReadOnly)) {
        file.setFileName(QDir::current().filePath("resources/health.qss"));
        if (!file.open(QFile::ReadOnly))
            return;
    }
    if (file.isOpen())
        qApp->setStyleSheet(QString::fromUtf8(file.readAll()));
}

QLabel *MainWindow::makeAvatar(const QString &initials)
{
    auto *a = new QLabel(initials);
    a->setFixedSize(54, 54);
    a->setAlignment(Qt::AlignCenter);
    a->setStyleSheet(
        "background: qradialgradient(cx:0.35, cy:0.3, radius:0.9, fx:0.3, fy:0.25,"
        " stop:0 #E8D7A8, stop:0.55 #C4A35A, stop:1 #7A5A28);"
        " color:#1C2A28; border-radius:27px; font-weight:700; font-size:16px;");
    return a;
}

QWidget *MainWindow::makeHeader(const QString &title, bool showBack, const QString &backTarget)
{
    auto *bar = new QFrame;
    bar->setStyleSheet("background:#12363A; color:#F7F1E5;");
    auto *row = new QHBoxLayout(bar);
    row->setContentsMargins(22, 14, 22, 14);
    if (showBack) {
        auto *back = makeBtn("Back", "ghostBtn");
        back->setObjectName("ghostBtn");
        connect(back, &QPushButton::clicked, this, [this, backTarget]() {
            if (backTarget == "gateway") goGateway();
            else if (backTarget == "welcome") goWelcome();
            else if (backTarget == "login") goLogin();
            else if (backTarget == "patient") ui->mainStackedWidget->setCurrentIndex(Patient);
            else logout();
        });
        row->addWidget(back);
    }
    auto *t = new QLabel(title);
    t->setStyleSheet("color:#F7F1E5; font-size:20px; font-weight:600;");
    row->addWidget(t);
    row->addStretch();
    return bar;
}

void MainWindow::refreshHeader(QLabel *nameLabel, QLabel *roleLabel, QLabel *avatar)
{
    if (nameLabel) nameLabel->setText(currentDisplayName_);
    if (roleLabel) roleLabel->setText(currentRole_);
    if (avatar) {
        QString ini = currentDisplayName_.isEmpty() ? "H+" : currentDisplayName_.left(1).toUpper();
        const auto parts = currentDisplayName_.split(' ', Qt::SkipEmptyParts);
        if (parts.size() >= 2)
            ini = parts[0].left(1).toUpper() + parts.back().left(1).toUpper();
        avatar->setText(ini);
    }
}

QString MainWindow::displayNameOf(const hms::User &user) const
{
    if (!user.getFullName().empty())
        return QString::fromStdString(user.getFullName());
    return QString::fromStdString(user.getUsername());
}

QString MainWindow::nowStamp() const
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
}

QString MainWindow::newPatientId() const
{
    int next = 1;
    for (const auto &patient : hospital_.getPatients()) {
        const QString id = QString::fromStdString(patient.getPatientId());
        if (!id.startsWith("PAT-")) continue;
        bool ok = false;
        next = std::max(next, id.mid(4).toInt(&ok) + (ok ? 1 : 0));
    }
    return QString("PAT-%1").arg(next, 4, 10, QChar('0'));
}

QString MainWindow::suggestedSpecialtyFromText(const QString &text) const
{
    const QString t = text.toLower();
    if (t.contains("heart") || t.contains("chest") || t.contains("bp") || t.contains("pressure"))
        return "Cardiologist";
    if (t.contains("head") || t.contains("seizure") || t.contains("stroke") || t.contains("migraine"))
        return "Neurologist";
    if (t.contains("bone") || t.contains("fracture") || t.contains("joint") || t.contains("knee"))
        return "Orthopedic";
    if (t.contains("child") || t.contains("baby") || (t.contains("fever") && t.contains("kid")))
        return "Pediatrician";
    if (t.contains("skin") || t.contains("rash") || t.contains("acne"))
        return "Dermatologist";
    if (t.contains("pregnan") || t.contains("period") || t.contains("womb"))
        return "Gynecologist";
    if (t.contains("ear") || t.contains("nose") || t.contains("throat") || t.contains("sinus"))
        return "ENT";
    if (t.contains("eye") || t.contains("vision") || t.contains("blur"))
        return "Ophthalmologist";
    if (t.contains("anxious") || t.contains("depress") || t.contains("sleep") || t.contains("stress"))
        return "Psychiatrist";
    if (t.contains("cancer") || t.contains("tumor") || t.contains("lump"))
        return "Oncologist";
    if (t.contains("urine") || t.contains("kidney stone") || t.contains("prostate"))
        return "Urologist";
    if (t.contains("stomach") || t.contains("acid") || t.contains("liver"))
        return "Gastroenterologist";
    if (t.contains("breath") || t.contains("cough") || t.contains("asthma") || t.contains("lung"))
        return "Pulmonologist";
    if (t.contains("kidney") || t.contains("dialysis"))
        return "Nephrologist";
    if (t.contains("sugar") || t.contains("diabet") || t.contains("thyroid") || t.contains("hormone"))
        return "Endocrinologist";
    if (t.contains("cut") || t.contains("wound") || t.contains("operate") || t.contains("appendix"))
        return "General Surgeon";
    if (t.contains("emergency") || t.contains("accident") || t.contains("bleed"))
        return "Emergency Medicine";
    return "General Surgeon";
}

void MainWindow::buildAllPages()
{
    // Welcome
    {
        auto *hero = new QFrame;
        hero->setObjectName("heroPanel");
        auto *v = new QVBoxLayout(hero);
        v->setContentsMargins(76, 48, 76, 42);
        v->setSpacing(0);
        auto *topline = new QLabel("A CALMER WAY TO CARE");
        topline->setObjectName("welcomeTopline");
        v->addWidget(topline, 0, Qt::AlignLeft);
        v->addSpacing(30);
        auto *mark = new QLabel("HEALTH++");
        mark->setObjectName("welcomeMark");
        auto *title = new QLabel("Welcome to the Health++ system");
        title->setObjectName("heroTitle");
        title->setWordWrap(true);
        title->setMinimumWidth(760);
        title->setMinimumHeight(86);
        auto *sub = new QLabel("One trusted place for patients, clinicians, and the hospital office.");
        sub->setObjectName("heroSub");
        sub->setWordWrap(true);
        sub->setMinimumWidth(720);
        sub->setMinimumHeight(42);
        v->addWidget(mark, 0, Qt::AlignLeft);
        v->addSpacing(14);
        v->addWidget(title, 0, Qt::AlignLeft);
        v->addSpacing(16);
        v->addWidget(sub, 0, Qt::AlignLeft);
        v->addStretch(1);
        auto *row = new QHBoxLayout;
        auto *cont = makeBtn("Continue", "goldBtn");
        cont->setObjectName("goldBtn");
        cont->setMinimumSize(200, 52);
        connect(cont, &QPushButton::clicked, this, &MainWindow::goGateway);
        row->addWidget(cont, 0, Qt::AlignLeft);
        v->addLayout(row);
        v->addSpacing(12);
        auto *footer = new QLabel("Private by design  ·  Ready when you are");
        footer->setObjectName("welcomeFooter");
        v->addWidget(footer, 0, Qt::AlignHCenter);
        place(ui->pageWelcome, hero);
    }

    // Gateway
    {
        auto *root = new QWidget;
        root->setObjectName("gatewayPage");
        auto *v = new QVBoxLayout(root);
        v->setContentsMargins(38, 26, 38, 30);
        v->setSpacing(14);
        v->addWidget(makeHeader("Health++  ·  Choose your path", true, "welcome"));
        auto *intro = new QLabel("Choose the care path that fits this moment.");
        intro->setObjectName("gatewayIntro");
        intro->setWordWrap(true);
        v->addWidget(intro);
        auto *grid = new QGridLayout;
        grid->setHorizontalSpacing(18);
        grid->setVerticalSpacing(18);
        const struct { const char *t; const char *d; void (MainWindow::*slot)(); } cards[] = {
            {"Sign in", "Patients, clinicians, and the hospital office.", &MainWindow::goLogin},
            {"Emergency", "No account needed. The hospital manager reviews the request.", &MainWindow::goEmergency},
            {"Create account", "Join as a patient, or request a staff desk.", &MainWindow::goSignup},
        };
        for (int i = 0; i < 3; ++i) {
            auto *c = card();
            c->setObjectName("entryCard");
            c->setProperty("entryKind", i == 0 ? "signin" : i == 1 ? "emergency" : "signup");
            c->setMinimumHeight(300);
            auto *cv = new QVBoxLayout(c);
            cv->setContentsMargins(24, 24, 24, 22);
            cv->setSpacing(12);
            auto *h = new QLabel(QString::number(i + 1) + "  ·  " + cards[i].t);
            h->setObjectName("entryTitle");
            auto *d = new QLabel(cards[i].d);
            d->setWordWrap(true);
            d->setObjectName("muted");
            auto *b = makeBtn("Open");
            connect(b, &QPushButton::clicked, this, cards[i].slot);
            h->setMinimumHeight(34);
            cv->addWidget(h);
            cv->addWidget(d);
            cv->addStretch();
            b->setMinimumHeight(48);
            cv->addWidget(b);
            grid->addWidget(c, 0, i);
            grid->setColumnStretch(i, 1);
        }
        v->addLayout(grid);
        v->addStretch();
        place(ui->pageGateway, root);
        // fix welcome back: header uses gateway; welcome back needs special
    }

    // Login
    {
        auto *root = new QWidget;
        auto *v = new QVBoxLayout(root);
        v->setContentsMargins(80, 30, 80, 40);
        v->addWidget(makeHeader("Sign in", true, "gateway"));
        auto *c = card();
        c->setObjectName("formCard");
        auto *f = new QVBoxLayout(c);
        f->setContentsMargins(32, 28, 32, 28);
        f->addWidget(new QLabel("Username"));
        loginUser_ = new QLineEdit;
        loginUser_->setPlaceholderText("Username or staff ID, e.g. DOC_001");
        f->addWidget(loginUser_);
        f->addWidget(new QLabel("Password"));
        loginPass_ = new QLineEdit;
        loginPass_->setEchoMode(QLineEdit::Password);
        f->addWidget(loginPass_);
        auto *go = makeBtn("Enter Health++", "goldBtn");
        go->setObjectName("goldBtn");
        connect(go, &QPushButton::clicked, this, &MainWindow::submitLogin);
        f->addWidget(go);
        auto *demo = new QLabel(
            "<b>Demo accounts</b><br>"
            "Manager: <b>HeinHtetSan</b> / RoyalCare#1<br>"
            "Doctor: <b>DOC_001</b> / Clinic#2026A<br>"
            "Nurse: <b>NUR_1</b> / Clinic#2026A<br>"
            "Patient: <b>patient_amy</b> / Patient#2026A");
        demo->setWordWrap(true);
        demo->setObjectName("infoPanel");
        f->addWidget(demo);
        auto *toSign = makeBtn("No account yet? Create one", "quietBtn");
        toSign->setObjectName("quietBtn");
        connect(toSign, &QPushButton::clicked, this, &MainWindow::goSignup);
        f->addWidget(toSign);
        v->addWidget(c);
        v->addStretch();
        place(ui->pageLogin, root);
    }

    // Signup
    {
        auto *root = new QWidget;
        auto *v = new QVBoxLayout(root);
        v->setContentsMargins(50, 16, 50, 20);
        v->addWidget(makeHeader("Create an account", true, "gateway"));
        auto *c = card();
        c->setObjectName("formCard");
        auto *g = new QGridLayout(c);
        g->setContentsMargins(28, 24, 28, 24);
        signName_ = new QLineEdit;
        signUser_ = new QLineEdit;
        signAge_ = new QLineEdit;
        signGender_ = new QComboBox;
        signGender_->addItems({"Female", "Male", "Other"});
        signContact_ = new QLineEdit;
        signAddress_ = new QLineEdit;
        signPass_ = new QLineEdit;
        signPass_->setEchoMode(QLineEdit::Password);
        signPass2_ = new QLineEdit;
        signPass2_->setEchoMode(QLineEdit::Password);
        signRole_ = new QComboBox;
        signRole_->addItems({"Patient", "Doctor", "Nurse"});
        signStrength_ = new QLabel("Use 10+ characters with upper, lower, number, and a symbol.");
        signStrength_->setObjectName("muted");
        signStrength_->setWordWrap(true);
        int r = 0;
        auto add = [&](const QString &lab, QWidget *w) {
            g->addWidget(new QLabel(lab), r, 0);
            g->addWidget(w, r, 1);
            ++r;
        };
        add("Full name", signName_);
        add("Username", signUser_);
        add("Age", signAge_);
        add("Gender", signGender_);
        add("Contact", signContact_);
        add("Address", signAddress_);
        add("Password", signPass_);
        add("Confirm password", signPass2_);
        add("Role", signRole_);
        g->addWidget(signStrength_, r++, 0, 1, 2);
        auto *go = makeBtn("Create account", "goldBtn");
        go->setObjectName("goldBtn");
        connect(go, &QPushButton::clicked, this, &MainWindow::submitSignup);
        g->addWidget(go, r++, 0, 1, 2);
        auto *toLogin = makeBtn("Already with us? Sign in", "quietBtn");
        toLogin->setObjectName("quietBtn");
        connect(toLogin, &QPushButton::clicked, this, &MainWindow::goLogin);
        g->addWidget(toLogin, r, 0, 1, 2);
        v->addWidget(scrollWrap(c));
        place(ui->pageSignup, root);
    }

    // Emergency
    {
        auto *root = new QWidget;
        auto *v = new QVBoxLayout(root);
        v->setContentsMargins(50, 20, 50, 24);
        v->addWidget(makeHeader("Emergency desk  ·  no sign-in", true, "gateway"));
        auto *c = card();
        c->setObjectName("emergencyForm");
        auto *f = new QVBoxLayout(c);
        f->setContentsMargins(28, 24, 28, 24);
        emName_ = new QLineEdit;
        emName_->setPlaceholderText("Caller or patient name");
        emPhone_ = new QLineEdit;
        emPhone_->setPlaceholderText("Phone we can reach");
        emLocation_ = new QLineEdit;
        emLocation_->setPlaceholderText("Street, landmark, township");
        emSeverity_ = new QComboBox;
        emSeverity_->addItems({"Severe", "Moderate", "Minor"});
        f->addWidget(new QLabel("Who needs help?"));
        f->addWidget(emName_);
        f->addWidget(new QLabel("Contact number"));
        f->addWidget(emPhone_);
        f->addWidget(new QLabel("Location"));
        f->addWidget(emLocation_);
        f->addWidget(new QLabel("How serious is this?"));
        f->addWidget(emSeverity_);
        auto *send = makeBtn("Submit emergency request", "goldBtn");
        send->setObjectName("goldBtn");
        connect(send, &QPushButton::clicked, this, &MainWindow::submitEmergency);
        f->addWidget(send);
        emResult_ = new QLabel("Submit the request with the caller, phone, location, and severity. The hospital manager reviews it and authorizes ambulance dispatch.");
        emResult_->setWordWrap(true);
        emResult_->setObjectName("muted");
        f->addWidget(emResult_);
        v->addWidget(c);
        v->addStretch();
        place(ui->pageEmergency, root);
    }

    // Patient
    {
        auto *root = new QWidget;
        auto *v = new QVBoxLayout(root);
        v->setContentsMargins(0, 0, 0, 0);
        auto *top = new QFrame;
        top->setStyleSheet("background:#12363A;");
        auto *tr = new QHBoxLayout(top);
        auto *back = makeBtn("Sign out", "ghostBtn");
        back->setObjectName("ghostBtn");
        connect(back, &QPushButton::clicked, this, &MainWindow::logout);
        tr->addWidget(back);
        auto *ttl = new QLabel("Patient portal");
        ttl->setStyleSheet("color:#F7F1E5; font-size:20px;");
        tr->addWidget(ttl);
        tr->addStretch();
        patName_ = new QLabel;
        patRole_ = new QLabel;
        patName_->setStyleSheet("color:#F7F1E5;");
        patRole_->setStyleSheet("color:#D8C7A3;");
        patAvatar_ = makeAvatar("P");
        tr->addWidget(patName_);
        tr->addWidget(patRole_);
        tr->addWidget(patAvatar_);
        v->addWidget(top);

        auto *body = new QHBoxLayout;
        auto *side = new QVBoxLayout;
        side->setContentsMargins(18, 18, 12, 18);
        side->setSpacing(10);
        auto *sideTitle = new QLabel("PATIENT SERVICES");
        sideTitle->setObjectName("muted");
        side->addWidget(sideTitle);
        auto addSide = [&](const QString &label, auto slot) {
            auto *b = makeBtn(label, "quietBtn");
            b->setObjectName("quietBtn");
            b->setCheckable(true);
            b->setMinimumHeight(44);
            connect(b, &QPushButton::clicked, this, [this, b, slot]() {
                for (auto *sibling : b->parentWidget()->findChildren<QPushButton *>(QString(), Qt::FindDirectChildrenOnly))
                    sibling->setChecked(sibling == b);
                (this->*slot)();
            });
            side->addWidget(b);
        };
        addSide("Health concern", &MainWindow::suggestSpecialist);
        addSide("Specialists", &MainWindow::openSpecialists);
        addSide("Rooms & beds", &MainWindow::openRooms);
        addSide("Maternity", &MainWindow::openPregnancy);
        addSide("Give blood", &MainWindow::openDonateBlood);
        addSide("Leave a review", &MainWindow::openReviews);
        side->addStretch();
        auto *main = card();
        auto *mv = new QVBoxLayout(main);
        mv->setContentsMargins(12, 12, 12, 12);
        patientTabs_ = new QTabWidget;
        patientTabs_->setDocumentMode(true);
        connect(patientTabs_, &QTabWidget::currentChanged, this, [this](int index) {
            if (index == 3) openRooms();
            else if (index == 4) openPregnancy();
            else if (index == 5) openDonateBlood();
            else if (index == 6) openReviews();
        });

        auto *consultPage = new QWidget;
        auto *consult = new QVBoxLayout(consultPage);
        consult->addWidget(new QLabel("<b>1. Find a doctor</b><br>Describe your concern or choose a clinic, then select one doctor."));
        patientIssue_ = new QPlainTextEdit;
        patientIssue_->setPlaceholderText("Example: tightness in my chest when I walk upstairs...");
        consult->addWidget(patientIssue_);
        auto *ai = makeBtn("Match a specialist");
        connect(ai, &QPushButton::clicked, this, &MainWindow::suggestSpecialist);
        consult->addWidget(ai);
        patientAiHint_ = new QLabel("This is a routing suggestion, not a diagnosis.");
        patientAiHint_->setWordWrap(true);
        consult->addWidget(patientAiHint_);
        patientManualSpec_ = new QComboBox;
        consult->addWidget(new QLabel("Or choose a clinic:"));
        consult->addWidget(patientManualSpec_);
        auto *openSpec = makeBtn("Show doctors in this clinic");
        connect(openSpec, &QPushButton::clicked, this, &MainWindow::openSpecialists);
        consult->addWidget(openSpec);
        specialistList_ = new QListWidget;
        specialistList_->setMinimumHeight(190);
        specialistList_->setSelectionMode(QAbstractItemView::SingleSelection);
        specialistList_->setToolTip("Select one doctor. Use Clear selection if you want to cancel.");
        connect(specialistList_, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current, QListWidgetItem *) {
            selectedDoctorId_ = current ? current->data(Qt::UserRole).toString() : QString();
            if (current) showDoctorCard();
            else if (doctorDetail_) doctorDetail_->setText("No doctor selected.");
        });
        consult->addWidget(new QLabel("Available doctors:"));
        consult->addWidget(specialistList_);
        auto *clearDoctor = makeBtn("Clear doctor selection", "quietBtn");
        clearDoctor->setObjectName("quietBtn");
        connect(clearDoctor, &QPushButton::clicked, this, [this]() {
            specialistList_->clearSelection();
            specialistList_->setCurrentRow(-1);
            selectedDoctorId_.clear();
            doctorDetail_->setText("No doctor selected. Choose a doctor to view the appointment details.");
        });
        consult->addWidget(clearDoctor);
        doctorDetail_ = new QLabel("No doctor selected. Choose a doctor to view the appointment details.");
        doctorDetail_->setWordWrap(true);
        consult->addWidget(doctorDetail_);
        apptDate_ = new QDateEdit(QDate::currentDate().addDays(1));
        apptDate_->setCalendarPopup(true);
        apptDate_->setMinimumDate(QDate::currentDate());
        consult->addWidget(new QLabel("Appointment date:"));
        consult->addWidget(apptDate_);
        apptTime_ = new QTimeEdit(QTime(9, 0));
        apptTime_->setDisplayFormat("HH:mm");
        apptTime_->setTimeRange(QTime(8, 0), QTime(17, 0));
        consult->addWidget(new QLabel("Appointment time (30-minute slots):"));
        consult->addWidget(apptTime_);
        auto *book = makeBtn("Confirm appointment", "goldBtn");
        book->setObjectName("goldBtn");
        connect(book, &QPushButton::clicked, this, &MainWindow::bookSelectedDoctor);
        consult->addWidget(book);
        patientTabs_->addTab(scrollWrap(consultPage), "Doctors & appointments");

        auto *appointmentPage = new QWidget;
        auto *appointmentLayout = new QVBoxLayout(appointmentPage);
        appointmentLayout->setContentsMargins(22, 20, 22, 20);
        appointmentLayout->addWidget(new QLabel("<b>My appointments and receipts</b><br>Select a booked appointment to view its receipt."));
        patientAppointments_ = new QTableWidget;
        patientAppointments_->setColumnCount(5);
        patientAppointments_->setHorizontalHeaderLabels({"Appointment", "Doctor", "Specialty", "Date and time", "Status"});
        patientAppointments_->setSelectionBehavior(QAbstractItemView::SelectRows);
        patientAppointments_->setSelectionMode(QAbstractItemView::SingleSelection);
        patientAppointments_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        patientAppointments_->setAlternatingRowColors(true);
        patientAppointments_->horizontalHeader()->setStretchLastSection(true);
        appointmentLayout->addWidget(patientAppointments_);
        auto *receipt = makeBtn("View selected appointment receipt", "goldBtn");
        receipt->setObjectName("goldBtn");
        connect(receipt, &QPushButton::clicked, this, [this]() {
            const int row = patientAppointments_->currentRow();
            if (row < 0 || !patientAppointments_->item(row, 0)) {
                QMessageBox::information(this, "Appointment receipt", "Select an appointment from the table first.");
                return;
            }
            const QString appointmentId = patientAppointments_->item(row, 0)->data(Qt::UserRole).toString();
            for (const auto &appointment : hospital_.getAppointments()) {
                if (QString::fromStdString(appointment.getAppointmentId()) != appointmentId) continue;
                const auto *doctor = hospital_.findDoctorById(appointment.getDoctorId());
                if (!doctor) return;
                const QString slip = QString(
                    "HEALTH++  ·  APPOINTMENT RECEIPT\n"
                    "--------------------------------\n"
                    "Patient : %1 (%2)\n"
                    "Doctor  : %3\n"
                    "Clinic  : %4\n"
                    "When    : %5\n"
                    "Room    : %6\n"
                    "Status  : %7\n"
                    "Estimate: %8 MMK\n"
                    "Office  : Hein Htet San\n"
                    "--------------------------------\n"
                    "Please arrive twenty minutes early.")
                    .arg(currentDisplayName_, currentPatientId_,
                         QString::fromStdString(doctor->getName()),
                         QString::fromStdString(doctor->getSpecialization()),
                         QString::fromStdString(appointment.getDateTime()),
                         QString::number(doctor->getRoomNo()),
                         QString::fromStdString(hms::appointmentStatusToString(appointment.getStatus())),
                         QString::number(hospital_.calculateTotalBill(currentPatientId_.toStdString(), 1), 'f', 0));
                showInvoice(slip);
                return;
            }
            QMessageBox::warning(this, "Appointment receipt", "The appointment record could not be found.");
        });
        appointmentLayout->addWidget(receipt);
        patientTabs_->addTab(scrollWrap(appointmentPage), "My appointments");

        auto *nursePage = new QWidget;
        auto *nurseLayout = new QVBoxLayout(nursePage);
        nurseLayout->setContentsMargins(22, 20, 22, 20);
        nurseLayout->addWidget(new QLabel("<b>Reserve nursing care</b><br>Choose a nurse for a ward visit, home-care request, or follow-up support."));
        patientNursePick_ = new QComboBox;
        for (const auto &n : hospital_.getNurses())
            patientNursePick_->addItem(QString::fromStdString(n.getName()) + " · " + QString::fromStdString(n.getAssignedWard()), QString::fromStdString(n.getNurseId()));
        nurseLayout->addWidget(new QLabel("Nurse:"));
        nurseLayout->addWidget(patientNursePick_);
        patientNurseDate_ = new QDateEdit(QDate::currentDate().addDays(1));
        patientNurseDate_->setCalendarPopup(true);
        patientNurseDate_->setMinimumDate(QDate::currentDate());
        nurseLayout->addWidget(new QLabel("Requested date:"));
        nurseLayout->addWidget(patientNurseDate_);
        patientNurseReason_ = new QLineEdit;
        patientNurseReason_->setPlaceholderText("Example: wound dressing, medication support, post-discharge check");
        nurseLayout->addWidget(new QLabel("Care needed:"));
        nurseLayout->addWidget(patientNurseReason_);
        auto *reserveNurseButton = makeBtn("Submit nursing request", "goldBtn");
        reserveNurseButton->setObjectName("goldBtn");
        connect(reserveNurseButton, &QPushButton::clicked, this, &MainWindow::reserveNurse);
        nurseLayout->addWidget(reserveNurseButton);
        patientNurseBookings_ = new QTableWidget;
        patientNurseBookings_->setColumnCount(5);
        patientNurseBookings_->setHorizontalHeaderLabels({"Request", "Nurse", "Date", "Care needed", "Status"});
        patientNurseBookings_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        patientNurseBookings_->setSelectionBehavior(QAbstractItemView::SelectRows);
        patientNurseBookings_->setAlternatingRowColors(true);
        nurseLayout->addWidget(new QLabel("My nursing requests:"));
        nurseLayout->addWidget(patientNurseBookings_);
        patientTabs_->addTab(scrollWrap(nursePage), "Nursing care");

        auto *roomPage = new QWidget;
        auto *rooms = new QVBoxLayout(roomPage);
        rooms->addWidget(new QLabel("<b>Rooms & beds</b><br>Every bed is shown, including vacant, reserved, and occupied beds."));
        roomTable_ = new QTableWidget;
        roomTable_->setColumnCount(4);
        roomTable_->setHorizontalHeaderLabels({"Bed / room", "Ward class", "Status", "Patient / booking"});
        roomTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
        roomTable_->setSelectionMode(QAbstractItemView::SingleSelection);
        roomTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        roomTable_->setAlternatingRowColors(true);
        roomTable_->setMinimumHeight(280);
        roomTable_->horizontalHeader()->setStretchLastSection(true);
        rooms->addWidget(roomTable_);
        roomList_ = new QComboBox;
        roomHint_ = new QLabel;
        roomHint_->setWordWrap(true);
        rooms->addWidget(roomHint_);
        auto *res = makeBtn("Reserve selected available bed", "goldBtn");
        res->setObjectName("goldBtn");
        connect(res, &QPushButton::clicked, this, &MainWindow::reserveSelectedRoom);
        rooms->addWidget(res);
        patientTabs_->addTab(scrollWrap(roomPage), "Rooms & beds");

        auto *maternityPage = new QWidget;
        auto *maternity = new QVBoxLayout(maternityPage);
        maternity->setContentsMargins(22, 20, 22, 20);
        maternity->setSpacing(14);
        auto *matTitle = new QLabel("<b>Maternity care</b>");
        matTitle->setObjectName("sectionTitle");
        maternity->addWidget(matTitle);
        auto *matHelp = new QLabel(
            "<b>How it works:</b> 1) choose an obstetrician, 2) choose the maternity stay date, "
            "3) press the booking button. The system creates the doctor appointment and reserves "
            "a vacant semi-private maternity bed together.");
        matHelp->setWordWrap(true);
        matHelp->setObjectName("infoPanel");
        maternity->addWidget(matHelp);
        auto *findMat = makeBtn("Find an obstetrician", "quietBtn");
        findMat->setObjectName("quietBtn");
        connect(findMat, &QPushButton::clicked, this, &MainWindow::openPregnancy);
        maternity->addWidget(findMat);
        maternity->addWidget(new QLabel("Selected doctor and appointment date:"));
        auto *matDate = new QDateEdit(QDate::currentDate().addDays(1));
        matDate->setCalendarPopup(true);
        matDate->setMinimumDate(QDate::currentDate());
        // Reuse the appointment date control so the booking state remains visible and consistent.
        connect(matDate, &QDateEdit::dateChanged, this, [this](const QDate &date) {
            if (apptDate_) apptDate_->setDate(date);
        });
        maternity->addWidget(matDate);
        auto *mat = makeBtn("Book maternity stay / ward space", "goldBtn");
        mat->setObjectName("goldBtn");
        connect(mat, &QPushButton::clicked, this, &MainWindow::bookMaternity);
        maternity->addWidget(mat);
        maternityHint_ = new QLabel("No maternity booking yet. Press Find an obstetrician first.");
        maternityHint_->setWordWrap(true);
        maternityHint_->setObjectName("muted");
        maternity->addWidget(maternityHint_);
        maternity->addStretch();
        patientTabs_->addTab(scrollWrap(maternityPage), "Maternity");

        auto *bloodPage = new QWidget;
        auto *bloodLayout = new QVBoxLayout(bloodPage);
        bloodLayout->setContentsMargins(22, 20, 22, 20);
        bloodLayout->setSpacing(14);
        auto *bloodTitle = new QLabel("<b>Give blood</b>");
        bloodTitle->setObjectName("sectionTitle");
        bloodLayout->addWidget(bloodTitle);
        bloodHint_ = new QLabel(
            "Choose your blood type and press Record donation. A 450 ml unit receives a tracking ID, "
            "42-day expiry date, and is visible to the hospital blood bank.");
        bloodHint_->setWordWrap(true);
        bloodHint_->setObjectName("infoPanel");
        bloodLayout->addWidget(bloodHint_);
        bloodTypePick_ = new QComboBox;
        bloodTypePick_->addItems({"A+", "A-", "B+", "B-", "AB+", "AB-", "O+", "O-"});
        bloodLayout->addWidget(new QLabel("Blood type:"));
        bloodLayout->addWidget(bloodTypePick_);
        auto *blood = makeBtn("Record donation", "goldBtn");
        blood->setObjectName("goldBtn");
        connect(blood, &QPushButton::clicked, this, &MainWindow::donateBlood);
        bloodLayout->addWidget(blood);
        bloodLayout->addWidget(new QLabel("Your donation is tracked in the manager's Blood bank table with its type, volume, expiry, and current status."));
        bloodLayout->addStretch();
        patientTabs_->addTab(scrollWrap(bloodPage), "Give blood");

        auto *reviewPage = new QWidget;
        auto *review = new QVBoxLayout(reviewPage);
        review->addWidget(new QLabel("<b>Leave a review</b><br>Your star review is sent to the hospital office and appears in My star reviews."));
        reviewTarget_ = new QComboBox;
        reviewStars_ = new QComboBox;
        reviewStars_->addItems({"5", "4", "3", "2", "1"});
        reviewNote_ = new QLineEdit;
        reviewNote_->setPlaceholderText("What went well, or what could be improved?");
        review->addWidget(new QLabel("Staff member:"));
        review->addWidget(reviewTarget_);
        review->addWidget(new QLabel("Stars:"));
        review->addWidget(reviewStars_);
        review->addWidget(new QLabel("Comment:"));
        review->addWidget(reviewNote_);
        auto *rev = makeBtn("Submit review", "goldBtn");
        rev->setObjectName("goldBtn");
        connect(rev, &QPushButton::clicked, this, &MainWindow::submitReview);
        review->addWidget(rev);
        patientTabs_->addTab(scrollWrap(reviewPage), "Leave a review");
        mv->addWidget(patientTabs_);
        body->addLayout(side, 1);
        body->addWidget(scrollWrap(main), 4);
        v->addLayout(body);
        place(ui->pagePatient, root);
    }

    // Doctor
    {
        auto *root = new QWidget;
        auto *v = new QVBoxLayout(root);
        auto *top = new QFrame;
        top->setStyleSheet("background:#12363A;");
        auto *tr = new QHBoxLayout(top);
        auto *back = makeBtn("Sign out", "ghostBtn");
        back->setObjectName("ghostBtn");
        connect(back, &QPushButton::clicked, this, &MainWindow::logout);
        tr->addWidget(back);
        auto *ttl = new QLabel("Doctor desk");
        ttl->setStyleSheet("color:#F7F1E5; font-size:20px;");
        tr->addWidget(ttl);
        doctorSpecialty_ = new QLabel("Specialty: loading");
        doctorSpecialty_->setObjectName("doctorSpecialty");
        tr->addStretch();
        tr->addWidget(doctorSpecialty_);
        docName_ = new QLabel; docRole_ = new QLabel; docAvatar_ = makeAvatar("D");
        docName_->setStyleSheet("color:#F7F1E5;");
        docRole_->setStyleSheet("color:#D8C7A3;");
        tr->addWidget(docName_); tr->addWidget(docRole_); tr->addWidget(docAvatar_);
        v->addWidget(top);
        auto *body = new QHBoxLayout;
        auto *side = new QVBoxLayout;
        btnQueue_ = makeBtn("Patient queue", "quietBtn");
        btnTheatre_ = makeBtn("Reserve operating theatre", "quietBtn");
        btnDocNews_ = makeBtn("Office notices", "quietBtn");
        auto *btnClinicalNote = makeBtn("Clinical notes", "quietBtn");
        btnQueue_->setObjectName("quietBtn");
        btnTheatre_->setObjectName("quietBtn");
        btnDocNews_->setObjectName("quietBtn");
        btnClinicalNote->setObjectName("quietBtn");
        connect(btnQueue_, &QPushButton::clicked, this, &MainWindow::doctorQueue);
        connect(btnQueue_, &QPushButton::clicked, this, [this]() {
            const auto *doctor = hospital_.findDoctorById(currentUsername_.toStdString());
            if (doctorSpecialty_)
                doctorSpecialty_->setText(doctor ? "Specialty: " + QString::fromStdString(doctor->getSpecialization()) : "Specialty: not assigned");
        });
        connect(btnTheatre_, &QPushButton::clicked, this, &MainWindow::doctorEmergencyTheatre);
        connect(btnDocNews_, &QPushButton::clicked, this, &MainWindow::doctorAnnouncements);
        connect(btnClinicalNote, &QPushButton::clicked, this, [this]() {
            if (doctorLocked_) return;
            doctorQueuePanel_->setVisible(false);
            doctorNotesPanel_->setVisible(true);
            doctorNewsPanel_->setVisible(false);
            doctorTheatrePanel_->setVisible(false);
            refreshDoctorClinicalNote();
        });
        side->addWidget(btnQueue_);
        side->addWidget(btnTheatre_);
        side->addWidget(btnDocNews_);
        side->addWidget(btnClinicalNote);
        auto *prev = makeBtn("Previous in queue", "quietBtn");
        auto *next = makeBtn("Next in queue", "quietBtn");
        prev->setToolTip("Show the previous patient in this doctor's queue");
        next->setToolTip("Show the next patient in this doctor's queue");
        prev->setObjectName("quietBtn");
        next->setObjectName("quietBtn");
        connect(prev, &QPushButton::clicked, this, &MainWindow::doctorPrevPatient);
        connect(next, &QPushButton::clicked, this, &MainWindow::doctorNextPatient);
        side->addWidget(prev);
        side->addWidget(next);
        side->addStretch();
        auto *main = card();
        auto *mv = new QVBoxLayout(main);
        doctorPatientBox_ = new QLabel("Queue is quiet.");
        doctorPatientBox_->setWordWrap(true);
        doctorNotes_ = new QTextEdit;
        doctorQueueList_ = new QListWidget;
        doctorNews_ = new QTextEdit;
        doctorNews_->setReadOnly(true);
        doctorQueuePanel_ = new QWidget;
        auto *queueLayout = new QVBoxLayout(doctorQueuePanel_);
        queueLayout->addWidget(new QLabel("Today's patient queue"));
        queueLayout->addWidget(doctorPatientBox_);
        queueLayout->addWidget(doctorQueueList_);
        doctorNotesPanel_ = new QWidget;
        auto *notesLayout = new QVBoxLayout(doctorNotesPanel_);
        doctorNotePatient_ = new QLabel("Select a patient from Patient queue first.");
        doctorNotePatient_->setWordWrap(true);
        notesLayout->addWidget(doctorNotePatient_);
        notesLayout->addWidget(doctorNotes_);
        auto *saveNote = makeBtn("Save clinical note", "goldBtn");
        saveNote->setObjectName("goldBtn");
        connect(saveNote, &QPushButton::clicked, this, [this]() {
            if (!doctorQueueList_ || doctorQueueList_->currentRow() < 0) {
                QMessageBox::warning(this, "Clinical note", "Select a patient from the queue first.");
                return;
            }
            clinicalNotes_[doctorQueueList_->currentItem()->data(Qt::UserRole).toString()]
                = doctorNotes_->toPlainText();
            QMessageBox::information(this, "Clinical note", "The note was saved for this patient.");
        });
        notesLayout->addWidget(saveNote);
        doctorNewsPanel_ = new QWidget;
        auto *newsLayout = new QVBoxLayout(doctorNewsPanel_);
        newsLayout->addWidget(new QLabel("From the hospital office"));
        newsLayout->addWidget(doctorNews_);
        doctorNotesPanel_->setVisible(false);
        doctorNewsPanel_->setVisible(false);
        doctorTheatrePanel_ = new QWidget;
        auto *theatreLayout = new QVBoxLayout(doctorTheatrePanel_);
        theatreLayout->setContentsMargins(18, 18, 18, 18);
        theatreLayout->setSpacing(12);
        theatreLayout->addWidget(new QLabel("Operating theatre reservation"));
        doctorTheatreStatus_ = new QLabel;
        doctorTheatreStatus_->setWordWrap(true);
        doctorTheatreAction_ = makeBtn("Reserve available theatre", "goldBtn");
        doctorTheatreAction_->setObjectName("goldBtn");
        doctorTheatreAction_->setMinimumWidth(220);
        connect(doctorTheatreAction_, &QPushButton::clicked, this, &MainWindow::doctorEmergencyTheatre);
        auto *theatreActionRow = new QHBoxLayout;
        theatreActionRow->setSpacing(16);
        theatreActionRow->addWidget(doctorTheatreStatus_, 1);
        theatreActionRow->addWidget(doctorTheatreAction_, 0, Qt::AlignRight | Qt::AlignVCenter);
        theatreLayout->addLayout(theatreActionRow);
        doctorTheatrePanel_->setVisible(false);
        mv->addWidget(doctorQueuePanel_);
        mv->addWidget(doctorNotesPanel_);
        mv->addWidget(doctorNewsPanel_);
        mv->addWidget(doctorTheatrePanel_);
        body->addLayout(side, 1);
        body->addWidget(main, 4);
        v->addLayout(body);
        place(ui->pageDoctor, root);
    }

    // Nurse
    {
        auto *root = new QWidget;
        auto *v = new QVBoxLayout(root);
        auto *top = new QFrame;
        top->setStyleSheet("background:#12363A;");
        auto *tr = new QHBoxLayout(top);
        auto *back = makeBtn("Sign out", "ghostBtn");
        back->setObjectName("ghostBtn");
        connect(back, &QPushButton::clicked, this, &MainWindow::logout);
        tr->addWidget(back);
        auto *ttl = new QLabel("✚  Nurse station");
        ttl->setObjectName("nurseTitle");
        ttl->setStyleSheet("color:#F7F1E5; font-size:20px;");
        tr->addWidget(ttl);
        tr->addStretch();
        nurName_ = new QLabel; nurRole_ = new QLabel; nurAvatar_ = makeAvatar("N");
        nurName_->setStyleSheet("color:#F7F1E5;");
        nurRole_->setStyleSheet("color:#D8C7A3;");
        tr->addWidget(nurName_); tr->addWidget(nurRole_); tr->addWidget(nurAvatar_);
        v->addWidget(top);
        auto *body = new QHBoxLayout;
        auto *side = new QVBoxLayout;
        btnWard_ = makeBtn("▣  Active ward", "quietBtn");
        btnMeds_ = makeBtn("◷  Scheduled medicines", "quietBtn");
        btnReport_ = makeBtn("✎  Report to doctor", "quietBtn");
        btnNurseNews_ = makeBtn("◆  Office notices", "quietBtn");
        btnNurseBookings_ = makeBtn("♧  Patient care requests", "quietBtn");
        for (auto *b : {btnWard_, btnMeds_, btnReport_, btnNurseNews_, btnNurseBookings_})
            b->setObjectName("quietBtn");
        connect(btnWard_, &QPushButton::clicked, this, &MainWindow::nurseWardList);
        connect(btnMeds_, &QPushButton::clicked, this, &MainWindow::nurseMeds);
        connect(btnReport_, &QPushButton::clicked, this, &MainWindow::nurseReport);
        connect(btnNurseNews_, &QPushButton::clicked, this, &MainWindow::nurseAnnouncements);
        connect(btnNurseBookings_, &QPushButton::clicked, this, &MainWindow::nurseBookings);
        side->addWidget(btnWard_);
        side->addWidget(btnMeds_);
        side->addWidget(btnReport_);
        side->addWidget(btnNurseNews_);
        side->addWidget(btnNurseBookings_);
        side->addStretch();
        auto *main = card();
        auto *mv = new QVBoxLayout(main);
        nursePatients_ = new QListWidget;
        nurseMedsList_ = new QListWidget;
        nurseBookingsList_ = new QListWidget;
        nurseDoctorPick_ = new QComboBox;
        nurseNote_ = new QPlainTextEdit;
        nurseNews_ = new QTextEdit;
        nurseNews_->setReadOnly(true);
        nurseWardPanel_ = new QWidget;
        nurseWardPanel_->setObjectName("nursePanel");
        auto *wardLayout = new QVBoxLayout(nurseWardPanel_);
        wardLayout->addWidget(new QLabel("<b>▣ Active ward</b><br>Patients currently assigned to a ward or waiting for a bed."));
        wardLayout->addWidget(nursePatients_);
        auto *clearWardSelection = makeBtn("Clear selected patient");
        connect(clearWardSelection, &QPushButton::clicked, this, [this]() { nursePatients_->clearSelection(); nursePatients_->setCurrentRow(-1); nurseNote_->clear(); });
        wardLayout->addWidget(clearWardSelection);
        nurseMedsPanel_ = new QWidget;
        nurseMedsPanel_->setObjectName("nursePanel");
        auto *medsLayout = new QVBoxLayout(nurseMedsPanel_);
        medsLayout->addWidget(new QLabel("<b>◷ Scheduled medicines</b><br>Medicine stock currently available for ward administration."));
        medsLayout->addWidget(nurseMedsList_);
        nurseReportPanel_ = new QWidget;
        nurseReportPanel_->setObjectName("nursePanel");
        auto *reportLayout = new QVBoxLayout(nurseReportPanel_);
        reportLayout->addWidget(new QLabel("<b>✎ Report to doctor</b><br>Send a concise patient-care note to the selected doctor."));
        reportLayout->addWidget(new QLabel("Report to:"));
        reportLayout->addWidget(nurseDoctorPick_);
        reportLayout->addWidget(nurseNote_);
        auto *send = makeBtn("Send note to doctor", "goldBtn");
        send->setObjectName("goldBtn");
        connect(send, &QPushButton::clicked, this, &MainWindow::nurseReport);
        reportLayout->addWidget(send);
        nurseNewsPanel_ = new QWidget;
        nurseNewsPanel_->setObjectName("nursePanel");
        auto *newsLayout = new QVBoxLayout(nurseNewsPanel_);
        newsLayout->addWidget(new QLabel("<b>◆ Office notices</b><br>Updates from the hospital office."));
        newsLayout->addWidget(nurseNews_);
        nurseBookingsPanel_ = new QWidget;
        nurseBookingsPanel_->setObjectName("nursePanel");
        auto *bookingsLayout = new QVBoxLayout(nurseBookingsPanel_);
        bookingsLayout->addWidget(new QLabel("<b>♧ Patient care requests</b><br>Requests assigned to your nurse account."));
        bookingsLayout->addWidget(nurseBookingsList_);
        mv->addWidget(nurseWardPanel_);
        mv->addWidget(nurseMedsPanel_);
        mv->addWidget(nurseReportPanel_);
        mv->addWidget(nurseNewsPanel_);
        mv->addWidget(nurseBookingsPanel_);
        nurseMedsPanel_->setVisible(false);
        nurseReportPanel_->setVisible(false);
        nurseNewsPanel_->setVisible(false);
        nurseBookingsPanel_->setVisible(false);
        body->addLayout(side, 1);
        body->addWidget(main, 4);
        v->addLayout(body);
        place(ui->pageNurse, root);
    }

    // Manager
    {
        auto *root = new QWidget;
        auto *v = new QVBoxLayout(root);
        auto *top = new QFrame;
        top->setStyleSheet("background:#12363A;");
        auto *tr = new QHBoxLayout(top);
        auto *back = makeBtn("Sign out", "ghostBtn");
        back->setObjectName("ghostBtn");
        connect(back, &QPushButton::clicked, this, &MainWindow::logout);
        tr->addWidget(back);
        auto *ttl = new QLabel("Hospital office");
        ttl->setStyleSheet("color:#F7F1E5; font-size:20px;");
        tr->addWidget(ttl);
        tr->addStretch();
        mgrName_ = new QLabel; mgrRole_ = new QLabel; mgrAvatar_ = makeAvatar("M");
        mgrName_->setStyleSheet("color:#F7F1E5;");
        mgrRole_->setStyleSheet("color:#D8C7A3;");
        tr->addWidget(mgrName_); tr->addWidget(mgrRole_); tr->addWidget(mgrAvatar_);
        v->addWidget(top);
        auto *body = new QHBoxLayout;
        auto *side = new QVBoxLayout;
        const struct { const char *t; void (MainWindow::*s)(); } items[] = {
            {"Overview", &MainWindow::managerOverview},
            {"Staff & access", &MainWindow::managerStaff},
            {"Rooms & theatres", &MainWindow::managerRooms},
            {"Blood bank", &MainWindow::managerBlood},
            {"Ambulances", &MainWindow::managerAmbulances},
            {"Notices", &MainWindow::managerAnnounce},
            {"Star reviews", &MainWindow::managerReviews},
            {"Monthly bonus", &MainWindow::managerAwardBonus},
            {"Medicines", &MainWindow::managerMedicines},
            {"Appointments (all)", &MainWindow::managerAppointments},
        };
        for (const auto &it : items) {
            auto *b = makeBtn(it.t, "quietBtn");
            b->setObjectName("quietBtn");
            b->setCheckable(true);
            connect(b, &QPushButton::clicked, this, [this, b, it]() {
                for (auto *sibling : b->parentWidget()->findChildren<QPushButton *>(QString(), Qt::FindDirectChildrenOnly))
                    sibling->setChecked(sibling == b);
                (this->*it.s)();
            });
            side->addWidget(b);
        }
        side->addStretch();
        auto *main = card();
        auto *mv = new QVBoxLayout(main);
        managerView_ = new QTextEdit;
        managerView_->setReadOnly(true);
        bloodTable_ = new QTableWidget;
        bloodTable_->setColumnCount(10);
        bloodTable_->setHorizontalHeaderLabels({"Bottle no.", "Type", "Amount", "Donor", "Donated", "Expires", "Status", "Patient ID", "Reason for use", "Used date"});
        bloodTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
        bloodTable_->setSelectionMode(QAbstractItemView::SingleSelection);
        bloodTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        bloodTable_->horizontalHeader()->setStretchLastSection(true);
        bloodTable_->setVisible(false);
        ambulanceRequestTable_ = new QTableWidget;
        ambulanceRequestTable_->setColumnCount(8);
        ambulanceRequestTable_->setHorizontalHeaderLabels({"Request", "Caller", "Phone", "Location", "Severity", "Created", "Status", "Sent"});
        ambulanceRequestTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
        ambulanceRequestTable_->setSelectionMode(QAbstractItemView::SingleSelection);
        ambulanceRequestTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ambulanceRequestTable_->setAlternatingRowColors(true);
        ambulanceRequestTable_->setVisible(false);
        approveEmergency_ = makeBtn("Approve and dispatch selected request", "goldBtn");
        approveEmergency_->setObjectName("goldBtn");
        approveEmergency_->setVisible(false);
        connect(approveEmergency_, &QPushButton::clicked, this, &MainWindow::managerApproveEmergency);
        returnEmergency_ = makeBtn("Return selected ambulances", "quietBtn");
        returnEmergency_->setObjectName("quietBtn");
        returnEmergency_->setVisible(false);
        connect(returnEmergency_, &QPushButton::clicked, this, &MainWindow::managerReturnEmergency);
        clearEmergencySelection_ = makeBtn("Clear request selection", "quietBtn");
        clearEmergencySelection_->setObjectName("quietBtn");
        clearEmergencySelection_->setVisible(false);
        connect(clearEmergencySelection_, &QPushButton::clicked, this, [this]() {
            ambulanceRequestTable_->clearSelection();
            ambulanceRequestTable_->setCurrentCell(-1, -1);
        });
        staffTable_ = new QTableWidget;
        staffTable_->setColumnCount(4);
        staffTable_->setHorizontalHeaderLabels({"Name", "Role", "Access status", "Username"});
        staffTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
        staffTable_->setSelectionMode(QAbstractItemView::SingleSelection);
        staffTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        staffTable_->horizontalHeader()->setStretchLastSection(true);
        staffTable_->setVisible(false);
        announceEdit_ = new QPlainTextEdit;
        announceEdit_->setPlaceholderText("Write a notice for doctors and nurses...");
        auto *row = new QHBoxLayout;
        managerStaffActions_ = new QWidget;
        managerStaffActions_->setLayout(row);
        managerStaffActions_->setVisible(false);
        staffRoleFilter_ = new QComboBox;
        staffRoleFilter_->addItems({"All roles", "Doctor", "Nurse", "Patient", "Hospital Manager"});
        connect(staffRoleFilter_, &QComboBox::currentTextChanged, this, [this]() {
            if (managerStaffActions_->isVisible()) managerStaff();
        });
        auto *ap = makeBtn("Approve pending");
        auto *wd = makeBtn("Pause access");
        auto *rs = makeBtn("Restore access");
        ap->setToolTip("Activate a pending staff account so the staff member can work");
        wd->setToolTip("Temporarily disable this staff account without deleting it");
        rs->setToolTip("Restore a withdrawn staff account");
        connect(ap, &QPushButton::clicked, this, &MainWindow::managerApproveSelected);
        connect(wd, &QPushButton::clicked, this, &MainWindow::managerWithdrawSelected);
        connect(rs, &QPushButton::clicked, this, &MainWindow::managerReinstateSelected);
        row->addWidget(staffRoleFilter_);
        auto *delPatient = makeBtn("Delete patient account", "quietBtn");
        delPatient->setObjectName("quietBtn");
        delPatient->setToolTip("Permanently removes the selected Patient account, patient record, and linked appointments.");
        connect(delPatient, &QPushButton::clicked, this, &MainWindow::managerDeletePatientSelected);
        row->addWidget(ap); row->addWidget(wd); row->addWidget(rs); row->addWidget(delPatient);
        auto *post = makeBtn("Post notice", "goldBtn");
        post->setObjectName("goldBtn");
        connect(post, &QPushButton::clicked, this, &MainWindow::managerAnnounce);
        managerNoticePanel_ = new QWidget;
        auto *noticeLayout = new QVBoxLayout(managerNoticePanel_);
        noticeLayout->addWidget(new QLabel("Write a notice for doctors and nurses"));
        announcementTable_ = new QTableWidget;
        announcementTable_->setColumnCount(3);
        announcementTable_->setHorizontalHeaderLabels({"Date", "Author", "Announcement"});
        announcementTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
        announcementTable_->setSelectionMode(QAbstractItemView::SingleSelection);
        announcementTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        announcementTable_->horizontalHeader()->setStretchLastSection(true);
        connect(announcementTable_, &QTableWidget::itemSelectionChanged, this, [this]() {
            const int row = announcementTable_->currentRow();
            if (row >= 0 && announcementTable_->item(row, 2))
                announceEdit_->setPlainText(announcementTable_->item(row, 2)->text());
        });
        noticeLayout->addWidget(announcementTable_);
        announcementPick_ = new QComboBox;
        announcementPick_->setPlaceholderText("Select an existing notice to edit or delete");
        connect(announcementPick_, &QComboBox::currentIndexChanged, this, [this]() {
            const QString id = announcementPick_->currentData().toString();
            for (const auto &a : facilities_.announcements()) {
                if (QString::fromStdString(a.id) == id) {
                    announceEdit_->setPlainText(QString::fromStdString(a.body));
                    break;
                }
            }
        });
        noticeLayout->addWidget(announcementPick_);
        noticeLayout->addWidget(announceEdit_);
        auto *noticeActions = new QHBoxLayout;
        auto *updateNotice = makeBtn("Update selected notice", "quietBtn");
        auto *deleteNotice = makeBtn("Delete selected notice", "quietBtn");
        updateNotice->setObjectName("quietBtn");
        deleteNotice->setObjectName("quietBtn");
        connect(updateNotice, &QPushButton::clicked, this, [this]() {
            const int row = announcementTable_->currentRow();
            const QString id = row >= 0 && announcementTable_->item(row, 0)
                                   ? announcementTable_->item(row, 0)->data(Qt::UserRole).toString()
                                   : QString();
            if (id.isEmpty() || announceEdit_->toPlainText().trimmed().isEmpty()) return;
            const auto existing = std::find_if(facilities_.announcements().begin(),
                                               facilities_.announcements().end(),
                                               [&id](const hms::Announcement& a) {
                                                   return QString::fromStdString(a.id) == id;
                                               });
            if (existing == facilities_.announcements().end()) return;
            hms::Announcement updated = *existing;
            updated.body = announceEdit_->toPlainText().toStdString();
            facilities_.updateAnnouncement(id.toStdString(), updated);
            announceEdit_->clear();
            managerAnnounce();
        });
        connect(deleteNotice, &QPushButton::clicked, this, [this]() {
            const int row = announcementTable_->currentRow();
            const QString id = row >= 0 && announcementTable_->item(row, 0)
                                   ? announcementTable_->item(row, 0)->data(Qt::UserRole).toString()
                                   : QString();
            if (id.isEmpty()) return;
            facilities_.removeAnnouncement(id.toStdString());
            announceEdit_->clear();
            managerAnnounce();
        });
        noticeActions->addWidget(updateNotice);
        noticeActions->addWidget(deleteNotice);
        noticeLayout->addLayout(noticeActions);
        noticeLayout->addWidget(post);
        managerNoticePanel_->setVisible(false);
        managerBloodActions_ = new QWidget;
        auto *bloodActionsLayout = new QHBoxLayout(managerBloodActions_);
        managerBloodTypePick_ = new QComboBox;
        managerBloodTypePick_->addItems({"A+", "A-", "B+", "B-", "AB+", "AB-", "O+", "O-"});
        managerBloodPatient_ = new QComboBox;
        managerBloodPatient_->setPlaceholderText("Select patient");
        managerBloodOperation_ = new QLineEdit;
        managerBloodOperation_->setPlaceholderText("Operation / reason");
        auto *issueBlood = makeBtn("Issue blood", "goldBtn");
        issueBlood->setObjectName("goldBtn");
        connect(issueBlood, &QPushButton::clicked, this, &MainWindow::managerIssueBlood);
        managerBloodUnitPick_ = new QComboBox;
        managerBloodUnitPick_->setPlaceholderText("Select a blood unit to discard");
        auto *discardBlood = makeBtn("Discard selected unit", "quietBtn");
        discardBlood->setObjectName("quietBtn");
        connect(discardBlood, &QPushButton::clicked, this, &MainWindow::managerDiscardBlood);
        bloodActionsLayout->addWidget(managerBloodTypePick_);
        bloodActionsLayout->addWidget(managerBloodPatient_);
        bloodActionsLayout->addWidget(managerBloodOperation_);
        bloodActionsLayout->addWidget(issueBlood);
        bloodActionsLayout->addWidget(managerBloodUnitPick_);
        bloodActionsLayout->addWidget(discardBlood);
        managerBloodActions_->setVisible(false);
        managerScheduleActions_ = new QWidget;
        auto *scheduleRow = new QHBoxLayout(managerScheduleActions_);
        managerDoctorFilter_ = new QComboBox;
        managerDoctorFilter_->addItem("All doctors", "");
        managerScheduleStatusFilter_ = new QComboBox;
        managerScheduleStatusFilter_->addItems({"All statuses", "Scheduled", "Completed", "Cancelled"});
        auto *refreshSchedule = makeBtn("Refresh schedule", "quietBtn");
        refreshSchedule->setObjectName("quietBtn");
        connect(refreshSchedule, &QPushButton::clicked, this, &MainWindow::managerDoctorScheduling);
        auto *allowSchedule = makeBtn("Allow selected", "goldBtn");
        allowSchedule->setObjectName("goldBtn");
        connect(allowSchedule, &QPushButton::clicked, this, &MainWindow::managerAllowAppointment);
        auto *cancelSchedule = makeBtn("Cancel selected", "quietBtn");
        cancelSchedule->setObjectName("quietBtn");
        connect(cancelSchedule, &QPushButton::clicked, this, &MainWindow::managerCancelAppointment);
        connect(managerDoctorFilter_, &QComboBox::currentTextChanged, this, [this]() {
            if (managerScheduleActions_ && managerScheduleActions_->isVisible()) managerDoctorScheduling();
        });
        connect(managerScheduleStatusFilter_, &QComboBox::currentTextChanged, this, [this]() {
            if (managerScheduleActions_ && managerScheduleActions_->isVisible()) managerDoctorScheduling();
        });
        scheduleRow->addWidget(new QLabel("Doctor:"));
        scheduleRow->addWidget(managerDoctorFilter_, 1);
        scheduleRow->addWidget(new QLabel("Status:"));
        scheduleRow->addWidget(managerScheduleStatusFilter_, 1);
        scheduleRow->addWidget(refreshSchedule);
        scheduleRow->addWidget(allowSchedule);
        scheduleRow->addWidget(cancelSchedule);
        managerScheduleActions_->setVisible(false);
        scheduleTable_ = new QTableWidget;
        scheduleTable_->setColumnCount(5);
        scheduleTable_->setHorizontalHeaderLabels({"Appointment", "Doctor", "Patient", "Date and time", "Status"});
        scheduleTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
        scheduleTable_->setSelectionMode(QAbstractItemView::SingleSelection);
        scheduleTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        scheduleTable_->horizontalHeader()->setStretchLastSection(true);
        scheduleTable_->setVisible(false);
        mv->addWidget(managerScheduleActions_);
        mv->addWidget(managerView_);
        mv->addWidget(scheduleTable_);
        mv->addWidget(staffTable_);
        mv->addWidget(bloodTable_);
        mv->addWidget(ambulanceRequestTable_);
        mv->addWidget(approveEmergency_);
        mv->addWidget(returnEmergency_);
        mv->addWidget(clearEmergencySelection_);
        mv->addWidget(managerStaffActions_);
        mv->addWidget(managerNoticePanel_);
        mv->addWidget(managerBloodActions_);
        body->addLayout(side, 1);
        body->addWidget(main, 4);
        v->addLayout(body);
        place(ui->pageManager, root);
    }

    // Invoice
    {
        auto *root = new QWidget;
        auto *v = new QVBoxLayout(root);
        v->addWidget(makeHeader("Appointment slip", true, "patient"));
        invoiceView_ = new QTextEdit;
        invoiceView_->setReadOnly(true);
        v->addWidget(invoiceView_);
        place(ui->pageInvoice, root);
    }

    // Denied / Pending
    auto makeNotice = [&](QWidget *page, const QString &title, const QString &body) {
        auto *root = new QWidget;
        auto *v = new QVBoxLayout(root);
        v->setContentsMargins(80, 60, 80, 60);
        auto *c = card();
        auto *cv = new QVBoxLayout(c);
        auto *h = new QLabel(title);
        h->setObjectName("sectionTitle");
        auto *p = new QLabel(body);
        p->setWordWrap(true);
        auto *b = makeBtn("Return");
        connect(b, &QPushButton::clicked, this, &MainWindow::logout);
        cv->addWidget(h);
        cv->addWidget(p);
        cv->addWidget(b);
        v->addWidget(c);
        place(page, root);
    };
    makeNotice(ui->pageDenied,
               "Access withdrawn",
               "This desk has been closed by the hospital office. The buttons stay pale until you are reinstated. You are not rejected as a person — only this login is paused.");
    makeNotice(ui->pagePending,
               "Waiting for the office",
               "Your clinician account is in the drawer. Hein Htet San, or another hospital manager, must approve it before the desk lights up.");
}

void MainWindow::goWelcome() { ui->mainStackedWidget->setCurrentIndex(Welcome); }
void MainWindow::goGateway() { ui->mainStackedWidget->setCurrentIndex(Gateway); }
void MainWindow::goLogin() { ui->mainStackedWidget->setCurrentIndex(Login); }
void MainWindow::goSignup() { ui->mainStackedWidget->setCurrentIndex(Signup); }
void MainWindow::goEmergency() { ui->mainStackedWidget->setCurrentIndex(Emergency); }

void MainWindow::logout()
{
    currentUsername_.clear();
    currentRole_.clear();
    currentDisplayName_.clear();
    currentPatientId_.clear();
    goGateway();
}

void MainWindow::submitLogin()
{
    const auto userName = loginUser_->text().trimmed().toStdString();
    const auto pass = loginPass_->text().toStdString();
    auto *user = hospital_.findUserByUsername(userName);
    if (!user && userName.rfind("NUR_", 0) == 0) {
        try {
            const auto compactNurseId = std::string("NUR_") +
                                        std::to_string(std::stoi(userName.substr(4)));
            user = hospital_.findUserByUsername(compactNurseId);
        } catch (const std::exception&) {
            user = nullptr;
        }
    }
    if (user == nullptr || !user->matchesPassword(pass)) {
        QMessageBox::warning(this, "Sign in", "That name and password do not match our records.");
        return;
    }
    if (user->getStatus() == hms::AccountStatus::Withdrawn) {
        QMessageBox::warning(this, "Sign in", "Access to this account has been withdrawn. Contact the hospital manager to restore access.");
        return;
    }
    currentUsername_ = QString::fromStdString(user->getUsername());
    currentRole_ = QString::fromStdString(user->getRole());
    currentDisplayName_ = displayNameOf(*user);
    if (user->getStatus() == hms::AccountStatus::Pending) {
        ui->mainStackedWidget->setCurrentIndex(Pending);
        return;
    }
    doctorLocked_ = false;
    nurseLocked_ = false;
    enterRoleHome();
}

void MainWindow::enterRoleHome()
{
    const QString role = currentRole_.toLower();
    if (role.contains("manager") || role.contains("admin") || role.contains("owner")) {
        refreshHeader(mgrName_, mgrRole_, mgrAvatar_);
        managerOverview();
        ui->mainStackedWidget->setCurrentIndex(Manager);
        return;
    }
    if (role.contains("doctor")) {
        setDoctorLocked(false);
        refreshHeader(docName_, docRole_, docAvatar_);
        const auto *doctor = hospital_.findDoctorById(currentUsername_.toStdString());
        if (!doctor)
            doctor = hospital_.findDoctorById(("DOC_NEW_" + currentUsername_).toStdString());
        if (doctorSpecialty_)
            doctorSpecialty_->setText(doctor ? "Specialty: " + QString::fromStdString(doctor->getSpecialization()) : "Specialty: not assigned");
        doctorQueue();
        ui->mainStackedWidget->setCurrentIndex(Doctor);
        return;
    }
    if (role.contains("nurse")) {
        setNurseLocked(false);
        refreshHeader(nurName_, nurRole_, nurAvatar_);
        nurseWardList();
        ui->mainStackedWidget->setCurrentIndex(Nurse);
        return;
    }
    currentPatientId_.clear();
    for (const auto &p : hospital_.getPatients()) {
        if (QString::fromStdString(p.getPatientId()) == currentUsername_) {
            currentPatientId_ = QString::fromStdString(p.getPatientId());
            break;
        }
    }
    if (currentPatientId_.isEmpty()) {
        for (const auto &p : hospital_.getPatients()) {
            if (QString::fromStdString(p.getName()).compare(currentDisplayName_, Qt::CaseInsensitive) == 0) {
                currentPatientId_ = QString::fromStdString(p.getPatientId());
                break;
            }
        }
    }
    refreshHeader(patName_, patRole_, patAvatar_);
    refreshPatientAppointments();
    refreshPatientNurseBookings();
    std::set<std::string> specs;
    patientManualSpec_->clear();
    for (const auto &d : hospital_.getDoctors())
        specs.insert(d.getSpecialization());
    for (const auto &s : specs)
        patientManualSpec_->addItem(QString::fromStdString(s));
    ui->mainStackedWidget->setCurrentIndex(Patient);
}

void MainWindow::submitSignup()
{
    const QString pass = signPass_->text();
    if (pass != signPass2_->text()) {
        QMessageBox::warning(this, "Account", "The two passwords do not match. Please type them again.");
        return;
    }
    if (!hms::User::isPasswordStrong(pass.toStdString())) {
        const auto msg = hms::User::passwordStrengthMessage(pass.toStdString());
        signStrength_->setText(QString::fromStdString(msg));
        QMessageBox::warning(this, "Password too light", QString::fromStdString(msg));
        return;
    }
    const auto username = signUser_->text().trimmed().toStdString();
    if (username.empty() || signName_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Account", "Please give both a full name and a username.");
        return;
    }
    if (hospital_.findUserByUsername(username) != nullptr) {
        QMessageBox::warning(this, "Account", "That username is already taken.");
        return;
    }
    bool ageOk = false;
    const int age = signAge_->text().trimmed().toInt(&ageOk);
    if (!ageOk || age < 1 || age > 120 || signContact_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Account", "Enter an age from 1 to 120 and a contact number.");
        return;
    }
    const QString role = signRole_->currentText();
    if (role == "Hospital Manager" && hospital_.countUsersByRole("Hospital Manager") >= 5) {
        QMessageBox::warning(this, "Account", "The hospital office already has five seats. Ask an existing manager.");
        return;
    }
    hms::AccountStatus status = hms::AccountStatus::Active;
    if (role == "Doctor" || role == "Nurse")
        status = hms::AccountStatus::Pending;
    try {
        hms::User user(username, pass.toStdString(), role.toStdString(), status);
        user.setProfile(signName_->text().trimmed().toStdString(),
                        age,
                        signGender_->currentText().toStdString(),
                        signContact_->text().toStdString(),
                        signAddress_->text().toStdString());
        hospital_.addUser(user);
        if (role == "Patient") {
            const auto pid = newPatientId().toStdString();
            hospital_.addPatient(hms::Patient(pid, signName_->text().toStdString(),
                                              age,
                                              signGender_->currentText().toStdString(),
                                              signContact_->text().toStdString(),
                                              username, "New registration", "Outpatient",
                                              hms::HospitalManager::kWaitlistedRoomNo, false));
        } else if (role == "Doctor") {
            const auto id = "DOC_NEW_" + username;
            hospital_.addDoctor(hms::Doctor(id, signName_->text().toStdString(),
                                            age,
                                            signGender_->currentText().toStdString(),
                                            signContact_->text().toStdString(),
                                            id, "General Physician", 399, false, "Resident"));
        } else if (role == "Nurse") {
            const auto id = "NUR_NEW_" + username;
            hospital_.addNurse(hms::Nurse(id, signName_->text().toStdString(),
                                          age,
                                          signGender_->currentText().toStdString(),
                                          signContact_->text().toStdString(),
                                          id, "General Ward", "Unassigned"));
        }
        hospital_.saveAllToCSV();
    } catch (const std::exception &ex) {
        QMessageBox::warning(this, "Account", ex.what());
        return;
    }
    if (status == hms::AccountStatus::Pending) {
        QMessageBox::information(this, "Account", "Your desk is waiting for the hospital office to approve it.");
        goLogin();
        return;
    }
    QMessageBox::information(this, "Account", "Welcome. You can sign in now.");
    goLogin();
}

void MainWindow::submitEmergency()
{
    if (emLocation_->text().trimmed().isEmpty() || emPhone_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Emergency", "We need a location and a phone number.");
        return;
    }
    hms::EmergencyRequest request;
    request.id = "ER-" + QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz").toStdString();
    request.callerName = emName_->text().trimmed().toStdString();
    request.phone = emPhone_->text().trimmed().toStdString();
    request.location = emLocation_->text().trimmed().toStdString();
    request.severity = emSeverity_->currentText().toStdString();
    request.createdAt = nowStamp().toStdString();
    request.status = "Pending";
    facilities_.addEmergencyRequest(request);
    emResult_->setText("Emergency request " + QString::fromStdString(request.id)
                       + " was sent to the hospital manager. No ambulance is dispatched until it is approved.");
    emName_->clear();
    emPhone_->clear();
    emLocation_->clear();
}

void MainWindow::suggestSpecialist()
{
    const auto spec = suggestedSpecialtyFromText(patientIssue_->toPlainText());
    patientAiHint_->setText("From your words, Health++ would start with a " + spec
                            + ". You may still choose any clinic below.");
    const int idx = patientManualSpec_->findText(spec);
    if (idx >= 0) patientManualSpec_->setCurrentIndex(idx);
    openSpecialists();
}

void MainWindow::openSpecialists()
{
    if (patientTabs_) patientTabs_->setCurrentIndex(0);
    fillSpecialistList();
}

void MainWindow::fillSpecialistList()
{
    specialistList_->clear();
    const QString want = patientManualSpec_->currentText();
    for (const auto &d : hospital_.getDoctors()) {
        if (!want.isEmpty() && QString::fromStdString(d.getSpecialization()) != want)
            continue;
        const QString line = QString("%1  ·  %2  ·  %3  ·  %4")
                                 .arg(QString::fromStdString(d.getName()),
                                      QString::fromStdString(d.getSpecialization()),
                                      QString::fromStdString(d.getEmploymentType()),
                                      d.isAvailable() ? "Open diary" : "Away");
        auto *item = new QListWidgetItem(line);
        item->setData(Qt::UserRole, QString::fromStdString(d.getDoctorId()));
        specialistList_->addItem(item);
    }
}

void MainWindow::showDoctorCard()
{
    auto *item = specialistList_->currentItem();
    if (!item) return;
    selectedDoctorId_ = item->data(Qt::UserRole).toString();
    auto *doc = hospital_.findDoctorById(selectedDoctorId_.toStdString());
    if (!doc) return;
    doctorDetail_->setText(
        QString("%1\n%2 · %3\nConsult room %4\nNext diary date: %5\nFee is printed on the slip after you confirm.")
            .arg(QString::fromStdString(doc->getName()),
                 QString::fromStdString(doc->getSpecialization()),
                 QString::fromStdString(doc->getEmploymentType()),
                 QString::number(doc->getRoomNo()),
                 apptDate_->date().toString("dd MMM yyyy")));
}

void MainWindow::bookSelectedDoctor()
{
    if (selectedDoctorId_.isEmpty()) {
        QMessageBox::information(this, "Appointment", "Please choose a doctor first.");
        return;
    }
    auto *doc = hospital_.findDoctorById(selectedDoctorId_.toStdString());
    if (!doc || !doc->isAvailable()) {
        QMessageBox::warning(this, "Appointment", "That diary is not open.");
        return;
    }
    if (hospital_.findPatientById(currentPatientId_.toStdString()) == nullptr) {
        const auto pid = newPatientId().toStdString();
        hospital_.addPatient(hms::Patient(pid, currentDisplayName_.toStdString(), 0, "Unknown",
                                          "", pid, patientIssue_->toPlainText().toStdString(),
                                          patientManualSpec_->currentText().toStdString(),
                                          hms::HospitalManager::kWaitlistedRoomNo, false));
        currentPatientId_ = QString::fromStdString(pid);
    }
    if (apptDate_->date() < QDate::currentDate()) {
        QMessageBox::warning(this, "Appointment", "Choose today or a future date.");
        return;
    }
    const auto when = apptDate_->date().toString("yyyy-MM-dd") + " "
        + apptTime_->time().toString("HH:mm");
    if (!hospital_.bookAppointment(currentPatientId_.toStdString(),
                                   selectedDoctorId_.toStdString(),
                                   when.toStdString())) {
        QMessageBox::warning(this, "Appointment", "Could not hold that slot.");
        return;
    }
    const double bill = hospital_.calculateTotalBill(currentPatientId_.toStdString(), 1);
    const QString slip = QString(
                             "HEALTH++  ·  APPOINTMENT SLIP\n"
                             "--------------------------------\n"
                             "Patient : %1 (%2)\n"
                             "Doctor  : %3\n"
                             "Clinic  : %4\n"
                             "When    : %5\n"
                             "Room    : %6\n"
                             "Estimate: %7 MMK\n"
                             "Office  : Hein Htet San\n"
                             "--------------------------------\n"
                             "Please arrive twenty minutes early.")
                             .arg(currentDisplayName_,
                                  currentPatientId_,
                                  QString::fromStdString(doc->getName()),
                                  QString::fromStdString(doc->getSpecialization()),
                                  when,
                                  QString::number(doc->getRoomNo()),
                                  QString::number(bill, 'f', 0));
    refreshPatientAppointments();
    showInvoice(slip);
}

void MainWindow::refreshPatientAppointments()
{
    if (!patientAppointments_) return;
    patientAppointments_->setRowCount(0);
    for (const auto &appointment : hospital_.getAppointments()) {
        if (QString::fromStdString(appointment.getPatientId()) != currentPatientId_)
            continue;
        const auto *doctor = hospital_.findDoctorById(appointment.getDoctorId());
        const int row = patientAppointments_->rowCount();
        patientAppointments_->insertRow(row);
        auto *id = new QTableWidgetItem(QString::fromStdString(appointment.getAppointmentId()));
        id->setData(Qt::UserRole, QString::fromStdString(appointment.getAppointmentId()));
        patientAppointments_->setItem(row, 0, id);
        patientAppointments_->setItem(row, 1, new QTableWidgetItem(doctor ? QString::fromStdString(doctor->getName()) : QString::fromStdString(appointment.getDoctorId())));
        patientAppointments_->setItem(row, 2, new QTableWidgetItem(doctor ? QString::fromStdString(doctor->getSpecialization()) : "Unknown"));
        patientAppointments_->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(appointment.getDateTime())));
        patientAppointments_->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(hms::appointmentStatusToString(appointment.getStatus()))));
    }
}

void MainWindow::showInvoice(const QString &text)
{
    invoiceView_->setPlainText(text);
    ui->mainStackedWidget->setCurrentIndex(Invoice);
}

void MainWindow::openRooms()
{
    if (patientTabs_) patientTabs_->setCurrentIndex(3);
    const bool needsSeed = facilities_.rooms().empty();
    facilities_.seedIfEmpty();
    if (needsSeed) facilities_.saveAll();
    roomTable_->setRowCount(0);
    roomList_->clear();
    for (const auto &r : facilities_.rooms()) {
        const int row = roomTable_->rowCount();
        roomTable_->insertRow(row);
        roomTable_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(r.roomId)));
        roomTable_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(r.wardType)));
        roomTable_->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(r.status)));
        roomTable_->setItem(row, 3, new QTableWidgetItem(
            r.occupantId.empty() ? "Available" : QString::fromStdString(r.occupantId)));
        roomList_->addItem(QString::fromStdString(r.roomId), QString::fromStdString(r.roomId));
    }
    if (roomTable_->rowCount() == 0) {
        roomHint_->setText("No rooms are available right now.");
        return;
    }
    roomHint_->setText(QString("%1 beds listed. Vacant beds can be reserved.").arg(roomTable_->rowCount()));
}

void MainWindow::reserveSelectedRoom()
{
    const auto selected = roomTable_->currentRow();
    if (selected < 0 || !roomTable_->item(selected, 0)) {
        QMessageBox::information(this, "Bed reservation", "Select an available bed from the table first.");
        return;
    }
    const auto id = roomTable_->item(selected, 0)->text();
    if (!roomTable_->item(selected, 2) || roomTable_->item(selected, 2)->text() != "Vacant") {
        QMessageBox::warning(this, "Bed reservation", "Only beds marked Vacant can be reserved.");
        return;
    }
    if (facilities_.reserveRoom(id.toStdString(), currentPatientId_.toStdString())) {
        QMessageBox::information(this, "Bed", "The bed is reserved in your name.");
        openRooms();
    } else {
        QMessageBox::warning(this, "Bed", "That bed is no longer vacant.");
    }
}

void MainWindow::reserveNurse()
{
    if (currentPatientId_.isEmpty() || !patientNursePick_ || patientNursePick_->currentData().toString().isEmpty()) {
        QMessageBox::warning(this, "Nursing care", "Please choose a nurse first.");
        return;
    }
    if (!patientNurseReason_ || patientNurseReason_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Nursing care", "Describe the care you need before submitting the request.");
        return;
    }
    const QString nurseId = patientNursePick_->currentData().toString();
    const auto *nurse = hospital_.findNurseById(nurseId.toStdString());
    if (!nurse) {
        QMessageBox::warning(this, "Nursing care", "The selected nurse is no longer available.");
        return;
    }
    hms::NurseBooking booking;
    booking.id = "NB-" + QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz").toStdString();
    booking.patientId = currentPatientId_.toStdString();
    booking.patientName = currentDisplayName_.toStdString();
    booking.nurseId = nurseId.toStdString();
    booking.nurseName = nurse->getName();
    booking.date = patientNurseDate_->date().toString("yyyy-MM-dd").toStdString();
    booking.reason = patientNurseReason_->text().trimmed().toStdString();
    booking.status = "Requested";
    facilities_.addNurseBooking(booking);
    refreshPatientNurseBookings();
    patientNurseReason_->clear();
    QMessageBox::information(this, "Nursing care", "Your nursing request was sent to the selected nurse.");
}

void MainWindow::refreshPatientNurseBookings()
{
    if (!patientNurseBookings_) return;
    patientNurseBookings_->setRowCount(0);
    for (const auto &booking : facilities_.nurseBookings()) {
        if (QString::fromStdString(booking.patientId) != currentPatientId_) continue;
        const int row = patientNurseBookings_->rowCount();
        patientNurseBookings_->insertRow(row);
        patientNurseBookings_->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(booking.id)));
        patientNurseBookings_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(booking.nurseName)));
        patientNurseBookings_->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(booking.date)));
        patientNurseBookings_->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(booking.reason)));
        patientNurseBookings_->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(booking.status)));
    }
}

void MainWindow::openPregnancy()
{
    if (patientTabs_) patientTabs_->setCurrentIndex(4);
    const int specIndex = patientManualSpec_->findText("Gynecologist");
    if (specIndex >= 0) patientManualSpec_->setCurrentIndex(specIndex);
    fillSpecialistList();
    if (specialistList_ && specialistList_->count() > 0) {
        specialistList_->setCurrentRow(0);
        showDoctorCard();
        if (maternityHint_)
            maternityHint_->setText("Obstetrician selected. Choose a date, then press Book maternity stay / ward space.");
    } else if (maternityHint_) {
        maternityHint_->setText("No obstetrician is currently available. Please contact the hospital office.");
    }
    if (patientTabs_) patientTabs_->setCurrentIndex(4);
}

void MainWindow::bookMaternity()
{
    auto *doctor = selectedDoctorId_.isEmpty()
        ? nullptr : hospital_.findDoctorById(selectedDoctorId_.toStdString());
    if (!doctor || doctor->getSpecialization() != "Gynecologist") {
        QMessageBox::information(this, "Maternity", "Please choose an available obstetrician first.");
        openPregnancy();
        return;
    }
    if (hospital_.findPatientById(currentPatientId_.toStdString()) == nullptr) {
        const auto pid = newPatientId().toStdString();
        hospital_.addPatient(hms::Patient(pid, currentDisplayName_.toStdString(), 0, "Unknown",
                                          "", pid, "Maternity consultation", "Maternity",
                                          hms::HospitalManager::kWaitlistedRoomNo, false));
        currentPatientId_ = QString::fromStdString(pid);
    }
    if (apptDate_->date() < QDate::currentDate()) {
        QMessageBox::warning(this, "Maternity", "Choose today or a future date.");
        return;
    }
    QString reservedRoom;
    for (auto &r : facilities_.rooms()) {
        if (r.wardType == "Semi-Private" && r.status == "Vacant") {
            if (facilities_.reserveRoom(r.roomId, currentPatientId_.toStdString()))
                reservedRoom = QString::fromStdString(r.roomId);
            break;
        }
    }
    if (reservedRoom.isEmpty()) {
        QMessageBox::information(this, "Maternity", "No semi-private bed is free, so the appointment was not booked.");
        return;
    }
    const auto when = apptDate_->date().toString("yyyy-MM-dd") + " "
        + apptTime_->time().toString("HH:mm");
    if (!hospital_.bookAppointment(currentPatientId_.toStdString(), selectedDoctorId_.toStdString(), when.toStdString())) {
        facilities_.releaseRoom(reservedRoom.toStdString(), currentPatientId_.toStdString());
        QMessageBox::warning(this, "Maternity", "The obstetrician appointment could not be held; the bed was released.");
        return;
    }
    facilities_.saveAll();
    hospital_.saveAllToCSV();
    if (maternityHint_)
        maternityHint_->setText("Maternity appointment and bed reserved in " + reservedRoom + ".");
    QMessageBox::information(this, "Maternity", "Your obstetrician appointment and maternity bed are reserved.");
}

void MainWindow::openDonateBlood()
{
    if (patientTabs_) patientTabs_->setCurrentIndex(5);
    if (bloodHint_)
        bloodHint_->setText("Choose your blood type and press Record donation. A 450 ml unit receives a tracking ID, 42-day expiry date, and is visible to the hospital blood bank.");
    if (bloodTypePick_) bloodTypePick_->setFocus();
}

void MainWindow::donateBlood()
{
    if (currentUsername_.isEmpty()) {
        QMessageBox::warning(this, "Blood bank", "Please sign in before recording a donation.");
        return;
    }
    const auto *user = hospital_.findUserByUsername(currentUsername_.toStdString());
    hospital_.getBloodBank().donateBlood(currentDisplayName_.toStdString(),
                                         user ? user->getContact() : std::string(),
                                         bloodTypePick_ ? bloodTypePick_->currentText().toStdString() : "O+", 450.0);
    hospital_.saveAllToCSV();
    QMessageBox::information(this, "Blood bank", "A 450 ml unit was recorded. It will remain available until it expires or is issued for a patient.");
}

void MainWindow::openReviews()
{
    if (patientTabs_) patientTabs_->setCurrentIndex(6);
    reviewTarget_->clear();
    for (const auto &d : hospital_.getDoctors())
        reviewTarget_->addItem("Doctor · " + QString::fromStdString(d.getName()));
    for (const auto &n : hospital_.getNurses())
        reviewTarget_->addItem("Nurse · " + QString::fromStdString(n.getName()));
    reviewNote_->setPlaceholderText("What went well, or what could be improved?");
    reviewTarget_->setFocus();
}

void MainWindow::submitReview()
{
    if (reviewTarget_->currentText().isEmpty()) {
        QMessageBox::information(this, "Review", "Choose a doctor or nurse before submitting a review.");
        openReviews();
        return;
    }
    if (reviewNote_->text().trimmed().isEmpty()) {
        QMessageBox::information(this, "Review", "Add a short comment so the hospital office can understand the rating.");
        return;
    }
    const auto text = reviewTarget_->currentText();
    const auto role = text.section(" · ", 0, 0);
    const auto name = text.section(" · ", 1);
    hms::StarReview r;
    r.id = "RV-" + QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz").toStdString();
    r.patientName = currentDisplayName_.toStdString();
    r.targetRole = role.toStdString();
    r.targetName = name.toStdString();
    r.stars = reviewStars_->currentText().toInt();
    r.note = reviewNote_->text().toStdString();
    r.createdAt = nowStamp().toStdString();
    facilities_.addReview(r);
    facilities_.saveAll();
    QMessageBox::information(this, "Review", "Stars were sent to the hospital office only.");
    reviewNote_->clear();
}

void MainWindow::setDoctorLocked(bool locked)
{
    doctorLocked_ = locked;
    for (auto *b : {btnQueue_, btnTheatre_, btnDocNews_}) {
        if (!b) continue;
        b->setEnabled(!locked);
        if (locked) b->setText("Rejected");
    }
    if (locked)
        doctorPatientBox_->setText("You are rejected. The hospital office has withdrawn this desk.");
}

void MainWindow::setNurseLocked(bool locked)
{
    nurseLocked_ = locked;
    for (auto *b : {btnWard_, btnMeds_, btnReport_, btnNurseNews_, btnNurseBookings_}) {
        if (!b) continue;
        b->setEnabled(!locked);
        if (locked) b->setText("Rejected");
    }
}

void MainWindow::fillDoctorQueue()
{
    doctorQueueList_->clear();
    queueIndex_ = 0;
    const auto *doctor = hospital_.findDoctorById(currentUsername_.toStdString());
    if (!doctor)
        doctor = hospital_.findDoctorById(("DOC_NEW_" + currentUsername_).toStdString());
    const QString doctorId = doctor ? QString::fromStdString(doctor->getDoctorId()) : currentUsername_;
    int ordinal = 0;
    for (const auto &a : hospital_.getAppointments()) {
        if (QString::fromStdString(a.getDoctorId()) != doctorId) continue;
        auto *p = hospital_.findPatientById(a.getPatientId());
        const QString name = p ? QString::fromStdString(p->getName()) : QString::fromStdString(a.getPatientId());
        ++ordinal;
        auto *item = new QListWidgetItem(QString("%1. %2\n   %3  ·  %4")
                                      .arg(ordinal)
                                      .arg(name)
                                      .arg(QString::fromStdString(a.getDateTime()))
                                      .arg(QString::fromStdString(hms::appointmentStatusToString(a.getStatus()))));
        item->setData(Qt::UserRole, QString::fromStdString(a.getPatientId()));
        item->setToolTip("Select this appointment to see the current patient's details.");
        doctorQueueList_->addItem(item);
    }
    if (ordinal == 0)
        doctorPatientBox_->setText("No patients are scheduled for this doctor.");
}
void MainWindow::doctorQueue()
{
    if (doctorLocked_) return;
    doctorQueuePanel_->setVisible(true);
    doctorNotesPanel_->setVisible(false);
    doctorNewsPanel_->setVisible(false);
    doctorTheatrePanel_->setVisible(false);
    fillDoctorQueue();
    doctorPrevPatient();
}

void MainWindow::refreshDoctorClinicalNote()
{
    if (!doctorQueueList_ || doctorQueueList_->currentRow() < 0) {
        doctorNotePatient_->setText("Select an appointment from the current patient's queue.");
        doctorNotes_->clear();
        return;
    }
    const auto *item = doctorQueueList_->currentItem();
    const QString patientId = item->data(Qt::UserRole).toString();
    const auto *patient = hospital_.findPatientById(patientId.toStdString());
    if (!patient) {
        doctorNotePatient_->setText("Patient record not found: " + patientId);
        return;
    }
    doctorNotePatient_->setText(QString("Current patient\nName: %1\nPatient ID: %2\nAge / gender: %3 / %4\nPhone: %5\nTreatment: %6\nRoom / bed: %7")
                                    .arg(QString::fromStdString(patient->getName()),
                                         QString::fromStdString(patient->getPatientId()),
                                         QString::number(patient->getAge()),
                                         QString::fromStdString(patient->getGender()),
                                         QString::fromStdString(patient->getPhoneNumber()),
                                         QString::fromStdString(patient->getTreatmentType()),
                                         patient->getAssignedRoomNo() < 0 ? "Waiting list" : QString::number(patient->getAssignedRoomNo())));
    doctorPatientBox_->setText(QString("Current patient: %1 (%2)")
                                   .arg(QString::fromStdString(patient->getName()), patientId));
    doctorNotes_->setPlainText(clinicalNotes_.value(patientId));
}
void MainWindow::doctorPrevPatient()
{
    if (doctorQueueList_->count() == 0) {
        doctorPatientBox_->setText("No current patient in the queue.");
        return;
    }
    queueIndex_ = (doctorQueueList_->currentRow() <= 0) ? doctorQueueList_->count() - 1 : doctorQueueList_->currentRow() - 1;
    doctorQueueList_->setCurrentRow(queueIndex_);
    refreshDoctorClinicalNote();
}
void MainWindow::doctorNextPatient()
{
    if (doctorQueueList_->count() == 0) {
        doctorPatientBox_->setText("No current patient in the queue.");
        return;
    }
    queueIndex_ = (doctorQueueList_->currentRow() + 1) % doctorQueueList_->count();
    doctorQueueList_->setCurrentRow(queueIndex_);
    refreshDoctorClinicalNote();
}
void MainWindow::doctorEmergencyTheatre()
{
    if (doctorLocked_) return;
    doctorQueuePanel_->setVisible(false);
    doctorNotesPanel_->setVisible(false);
    doctorNewsPanel_->setVisible(false);
    doctorTheatrePanel_->setVisible(true);
    bool ownTheatre = false;
    for (const auto &owned : facilities_.theatres()) {
        if (owned.occupied && owned.occupiedByDoctor == currentDisplayName_.toStdString()) {
            ownTheatre = true;
            doctorTheatreStatus_->setText(QString("Theatre %1 is reserved by you. Release it after the operation.")
                                              .arg(owned.roomNo));
            doctorTheatreAction_->setText("Release my theatre");
            break;
        }
    }
    if (!ownTheatre) {
        doctorTheatreStatus_->setText("No theatre is reserved by you. Reserve the first available theatre (801-820).");
        doctorTheatreAction_->setText("Reserve available theatre");
    }
    if (ownTheatre) {
        for (auto &owned : facilities_.theatres()) {
            if (!owned.occupied || owned.occupiedByDoctor != currentDisplayName_.toStdString())
                continue;
            owned.occupied = false;
            owned.occupiedByDoctor.clear();
            facilities_.saveAll();
            hospital_.releaseTheatre(owned.roomNo);
            doctorPatientBox_->setText(QString("Theatre %1 has been released and is available again.").arg(owned.roomNo));
            doctorTheatreStatus_->setText("No theatre is reserved by you. Reserve the first available theatre (801-820).");
            doctorTheatreAction_->setText("Reserve available theatre");
            return;
        }
    }
    auto *th = facilities_.findFreeTheatre();
    if (!th) {
        QMessageBox::warning(this, "Operating theatre",
                             "All 20 operating theatres are currently reserved. "
                             "A theatre must be released after the operation before it can be reserved again.");
        return;
    }
    th->occupied = true;
    th->occupiedByDoctor = currentDisplayName_.toStdString();
    facilities_.saveAll();
    hospital_.occupyTheatre(th->roomNo);
    doctorPatientBox_->setText(QString("Theatre %1 is yours — full lights, full tray. Call the duty anaesthetist.")
                                   .arg(th->roomNo));
    doctorTheatreStatus_->setText(QString("Theatre %1 is reserved by you. Release it after the operation.")
                                      .arg(th->roomNo));
    doctorTheatreAction_->setText("Release my theatre");
}

void MainWindow::doctorAnnouncements()
{
    if (doctorLocked_) return;
    doctorQueuePanel_->setVisible(false);
    doctorNotesPanel_->setVisible(false);
    doctorNewsPanel_->setVisible(true);
    doctorTheatrePanel_->setVisible(false);
    QString t;
    for (const auto &a : facilities_.announcements()) {
        t += QString("%1  ·  %2\n%3\n\n")
                 .arg(QString::fromStdString(a.createdAt),
                      QString::fromStdString(a.author),
                      QString::fromStdString(a.body));
    }
    doctorNews_->setPlainText(t);
}

void MainWindow::nurseWardList()
{
    if (nurseLocked_) return;
    showNursePanel(nurseWardPanel_);
    nursePatients_->clear();
    for (const auto &p : hospital_.getPatients()) {
        nursePatients_->addItem(QString("%1  ·  room %2  ·  %3")
                                    .arg(QString::fromStdString(p.getName()),
                                         QString::number(p.getAssignedRoomNo()),
                                         QString::fromStdString(p.getTreatmentType())));
    }
    nurseDoctorPick_->clear();
    for (const auto &d : hospital_.getDoctors())
        nurseDoctorPick_->addItem(QString::fromStdString(d.getName()));
}

void MainWindow::nurseMeds()
{
    if (nurseLocked_) return;
    showNursePanel(nurseMedsPanel_);
    nurseMedsList_->clear();
    for (const auto &m : facilities_.medicines()) {
        nurseMedsList_->addItem(QString("%1  ·  %2 %3")
                                    .arg(QString::fromStdString(m.name))
                                    .arg(m.quantity)
                                    .arg(QString::fromStdString(m.unit)));
    }
}

void MainWindow::nurseReport()
{
    if (nurseLocked_) return;
    if (nurseReportPanel_ && !nurseReportPanel_->isVisible()) {
        showNursePanel(nurseReportPanel_);
        return;
    }
    if (nurseNote_->toPlainText().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Report to doctor", "Select a patient and write a note before sending it.");
        return;
    }
    if (!nursePatients_->currentItem()) {
        QMessageBox::warning(this, "Report to doctor", "Select the patient this report concerns.");
        return;
    }
    hms::NurseReport r;
    r.id = "RP-" + nowStamp().toStdString();
    r.nurseName = currentDisplayName_.toStdString();
    r.doctorName = nurseDoctorPick_->currentText().toStdString();
    r.patientName = nursePatients_->currentItem() ? nursePatients_->currentItem()->text().toStdString() : "";
    r.note = nurseNote_->toPlainText().toStdString();
    r.createdAt = nowStamp().toStdString();
    facilities_.addReport(r);
    QMessageBox::information(this, "Report", "The note is on the doctor's desk.");
    nurseNote_->clear();
}

void MainWindow::nurseAnnouncements()
{
    if (nurseLocked_) return;
    showNursePanel(nurseNewsPanel_);
    QString t;
    for (const auto &a : facilities_.announcements()) {
        t += QString("%1  ·  %2\n%3\n\n")
                 .arg(QString::fromStdString(a.createdAt),
                      QString::fromStdString(a.author),
                      QString::fromStdString(a.body));
    }
    nurseNews_->setPlainText(t);
}

void MainWindow::nurseBookings()
{
    if (nurseLocked_) return;
    showNursePanel(nurseBookingsPanel_);
    nurseBookingsList_->clear();
    for (const auto &booking : facilities_.nurseBookings()) {
        if (booking.nurseId != currentUsername_.toStdString()) continue;
        nurseBookingsList_->addItem(QString("%1  ·  %2  ·  %3\n   %4  ·  %5")
                                        .arg(QString::fromStdString(booking.date),
                                             QString::fromStdString(booking.patientName),
                                             QString::fromStdString(booking.patientId),
                                             QString::fromStdString(booking.reason),
                                             QString::fromStdString(booking.status)));
    }
    if (nurseBookingsList_->count() == 0)
        nurseBookingsList_->addItem("No patient care requests are assigned to you.");
}

void MainWindow::showNursePanel(QWidget *panel)
{
    const QList<QWidget *> panels = {nurseWardPanel_, nurseMedsPanel_, nurseReportPanel_, nurseNewsPanel_, nurseBookingsPanel_};
    for (auto *candidate : panels) {
        if (candidate) candidate->setVisible(candidate == panel);
    }
}

void MainWindow::managerOverview()
{
    if (managerScheduleActions_) managerScheduleActions_->setVisible(false);
    if (scheduleTable_) scheduleTable_->setVisible(false);

    managerView_->setVisible(true);
    staffTable_->setVisible(false);
    bloodTable_->setVisible(false);
    ambulanceRequestTable_->setVisible(false);
    approveEmergency_->setVisible(false);
    returnEmergency_->setVisible(false);
    clearEmergencySelection_->setVisible(false);
    managerStaffActions_->setVisible(false);
    managerNoticePanel_->setVisible(false);
    managerBloodActions_->setVisible(false);
    const auto icu = hospital_.getIcuOccupancy();
    const auto gw = hospital_.getGeneralWardOccupancy();
    QString t;
    t += QString("Staff: %1 doctors, %2 nurses, %3 patients, %4 accounts\n")
             .arg(hospital_.getDoctors().size())
             .arg(hospital_.getNurses().size())
             .arg(hospital_.getPatients().size())
             .arg(hospital_.getUsers().size());
    t += QString("Ambulances on the bay: %1 / 20\n").arg(hospital_.remainingAmbulances());
    t += QString("ICU %1/%2   General %3/%4\n").arg(icu.occupied).arg(icu.capacity).arg(gw.occupied).arg(gw.capacity);
    t += QString("Appointments: %1\n").arg(hospital_.getAppointments().size());
    int pending = 0;
    for (const auto &u : hospital_.getUsers())
        if (u.getStatus() == hms::AccountStatus::Pending) ++pending;
    t += QString("Pending staff approvals: %1\n\n").arg(pending);
    managerView_->setPlainText(t);
}

void MainWindow::managerStaff()
{
    if (managerScheduleActions_) managerScheduleActions_->setVisible(false);
    returnEmergency_->setVisible(false);
    if (scheduleTable_) scheduleTable_->setVisible(false);

    managerView_->setVisible(true);
    staffTable_->setVisible(true);
    bloodTable_->setVisible(false);
    ambulanceRequestTable_->setVisible(false);
    approveEmergency_->setVisible(false);
    clearEmergencySelection_->setVisible(false);
    managerStaffActions_->setVisible(true);
    managerNoticePanel_->setVisible(false);
    staffTable_->setRowCount(0);
    for (const auto &u : hospital_.getUsers()) {
        if (QString::fromStdString(u.getUsername()).compare("HeinHtetSan", Qt::CaseInsensitive) == 0)
            continue;
        const QString role = QString::fromStdString(u.getRole());
        if (staffRoleFilter_->currentText() != "All roles"
            && role.compare(staffRoleFilter_->currentText(), Qt::CaseInsensitive) != 0)
            continue;
        QString st = u.getStatus() == hms::AccountStatus::Pending ? "Pending approval"
                     : u.getStatus() == hms::AccountStatus::Withdrawn ? "Access withdrawn"
                     : role.compare("Patient", Qt::CaseInsensitive) == 0 ? "Active patient"
                                                                         : "Working";
        const int row = staffTable_->rowCount();
        staffTable_->insertRow(row);
        staffTable_->setItem(row, 0, new QTableWidgetItem(displayNameOf(u)));
        staffTable_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(u.getRole())));
        staffTable_->setItem(row, 2, new QTableWidgetItem(st));
        auto *username = new QTableWidgetItem(QString::fromStdString(u.getUsername()));
        username->setData(Qt::UserRole, QString::fromStdString(u.getUsername()));
        staffTable_->setItem(row, 3, username);
    }
    managerView_->setPlainText("Staff access table: Pending staff need approval. Withdraw access temporarily disables a staff login. Reinstate access restores it.");
}

void MainWindow::managerApproveSelected()
{
    const int row = staffTable_->currentRow();
    if (row < 0 || !staffTable_->item(row, 3)) {
        QMessageBox::information(this, "Staff access", "Select a pending staff account first.");
        return;
    }
    auto *u = hospital_.findUserByUsername(staffTable_->item(row, 3)->data(Qt::UserRole).toString().toStdString());
    if (!u) {
        QMessageBox::warning(this, "Staff access", "The selected account could not be found.");
        return;
    }
    if (u->getStatus() != hms::AccountStatus::Pending) {
        QMessageBox::information(this, "Staff access", "Approve is only for accounts marked Pending approval.");
        return;
    }
    u->setStatus(hms::AccountStatus::Active);
    if (QString::fromStdString(u->getRole()).compare("Doctor", Qt::CaseInsensitive) == 0) {
        if (auto *doctor = hospital_.findDoctorById(("DOC_NEW_" + u->getUsername()).c_str()))
            doctor->setAvailable(true);
    }
    hospital_.saveAllToCSV();
    QMessageBox::information(this, "Staff access", "The account has been approved and activated.");
    managerStaff();
}

void MainWindow::managerWithdrawSelected()
{
    const int row = staffTable_->currentRow();
    if (row < 0 || !staffTable_->item(row, 3)) return;
    auto *u = hospital_.findUserByUsername(staffTable_->item(row, 3)->data(Qt::UserRole).toString().toStdString());
    if (!u) return;
    if (u->getStatus() == hms::AccountStatus::Withdrawn) {
        QMessageBox::information(this, "Staff access", "This account is already withdrawn.");
        return;
    }
    u->setStatus(hms::AccountStatus::Withdrawn);
    hospital_.saveUsersToCSV();
    managerStaff();
}

void MainWindow::managerReinstateSelected()
{
    const int row = staffTable_->currentRow();
    if (row < 0 || !staffTable_->item(row, 3)) return;
    auto *u = hospital_.findUserByUsername(staffTable_->item(row, 3)->data(Qt::UserRole).toString().toStdString());
    if (!u) return;
    if (u->getStatus() != hms::AccountStatus::Withdrawn) {
        QMessageBox::information(this, "Staff access", "Reinstate is only for accounts marked Access withdrawn.");
        return;
    }
    u->setStatus(hms::AccountStatus::Active);
    hospital_.saveUsersToCSV();
    managerStaff();
}

void MainWindow::managerDeletePatientSelected()
{
    const int row = staffTable_->currentRow();
    if (row < 0 || !staffTable_->item(row, 3)) return;
    const QString username = staffTable_->item(row, 3)->data(Qt::UserRole).toString();
    auto *user = hospital_.findUserByUsername(username.toStdString());
    if (!user || QString::fromStdString(user->getRole()).compare("Patient", Qt::CaseInsensitive) != 0) {
        QMessageBox::warning(this, "Delete patient account", "Select a row whose role is Patient. Staff accounts cannot be permanently deleted from this button.");
        return;
    }
    if (QMessageBox::warning(this, "Permanent deletion", "This permanently removes the patient account, patient record, and linked appointments. Continue?", QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    if (user) {
        for (const auto &p : hospital_.getPatients()) {
            if (p.getPatientId() == username.toStdString() || p.getId() == username.toStdString() || p.getName() == user->getFullName()) {
                for (const auto &r : facilities_.rooms())
                    facilities_.releaseRoom(r.roomId, p.getPatientId());
                break;
            }
        }
    }
    if (!hospital_.deletePatientAccount(username.toStdString())) {
        QMessageBox::warning(this, "Delete patient account", "The patient account could not be matched to a patient record.");
        return;
    }
    QMessageBox::information(this, "Patient account deleted", "The patient account and linked appointments were permanently removed.");
    managerStaff();
}
void MainWindow::managerRooms()
{
    if (managerScheduleActions_) managerScheduleActions_->setVisible(false);
    returnEmergency_->setVisible(false);
    if (scheduleTable_) scheduleTable_->setVisible(false);

    facilities_.seedIfEmpty();
    facilities_.saveAll();

    managerView_->setVisible(true);
    staffTable_->setVisible(false);
    bloodTable_->setVisible(false);
    ambulanceRequestTable_->setVisible(false);
    approveEmergency_->setVisible(false);
    clearEmergencySelection_->setVisible(false);
    managerStaffActions_->setVisible(false);
    managerNoticePanel_->setVisible(false);
    managerBloodActions_->setVisible(false);
    QMap<QString, QMap<QString, int>> wardCounts;
    for (const auto &r : facilities_.rooms()) {
        ++wardCounts[QString::fromStdString(r.wardType)][QString::fromStdString(r.status)];
    }
    QString t = "<h2>Rooms and beds</h2><p>Every room and bed is listed below. Vacant = available, Reserved = booked, Occupied = currently in use.</p><p>";
    for (auto it = wardCounts.cbegin(); it != wardCounts.cend(); ++it) {
        const auto &counts = it.value();
        const int total = counts.value("Vacant") + counts.value("Reserved") + counts.value("Occupied");
        t += QString("<b>%1:</b> %2 beds · %3 vacant · %4 reserved · %5 occupied &nbsp;&nbsp;")
                 .arg(it.key()).arg(total).arg(counts.value("Vacant"))
                 .arg(counts.value("Reserved")).arg(counts.value("Occupied"));
    }
    t += "</p><table border='1' cellspacing='0' cellpadding='6'><tr><th>Room / bed no.</th><th>Ward classification</th><th>Status</th><th>Patient / booking ID</th></tr>";
    for (const auto &r : facilities_.rooms()) {
        t += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td></tr>")
                 .arg(QString::fromStdString(r.roomId),
                      QString::fromStdString(r.wardType),
                      QString::fromStdString(r.status),
                      r.occupantId.empty() ? "Available" : QString::fromStdString(r.occupantId));
    }
    t += "</table><h2>Operating theatres</h2><p>All theatres are surgical rooms. Free theatres can be reserved by a doctor; Busy theatres show the doctor currently using them.</p><table border='1' cellspacing='0' cellpadding='6'>"
         "<tr><th>Theatre no.</th><th>Classification</th><th>Equipment</th><th>Status / doctor</th></tr>";
    for (const auto &th : facilities_.theatres()) {
        t += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td></tr>")
                 .arg(th.roomNo)
                 .arg(QString::fromStdString(th.name))
                 .arg(th.fullyEquipped ? "Fully equipped" : "Basic")
                 .arg(th.occupied ? QString("Busy · ") + QString::fromStdString(th.occupiedByDoctor)
                                  : QString("Free"));
    }
    t += "</table>";
    managerView_->setHtml(t);
}

void MainWindow::managerBlood()
{
    if (managerScheduleActions_) managerScheduleActions_->setVisible(false);
    returnEmergency_->setVisible(false);
    if (scheduleTable_) scheduleTable_->setVisible(false);

    managerView_->setVisible(true);
    staffTable_->setVisible(false);
    bloodTable_->setVisible(true);
    managerStaffActions_->setVisible(false);
    managerNoticePanel_->setVisible(false);
    managerBloodActions_->setVisible(false);
    managerBloodActions_->setVisible(true);
    ambulanceRequestTable_->setVisible(false);
    approveEmergency_->setVisible(false);
    clearEmergencySelection_->setVisible(false);
    managerBloodUnitPick_->clear();
    managerBloodPatient_->clear();
    bloodTable_->setRowCount(0);
    QString t = "<h2>Blood bank inventory and history</h2><p>Every bottle is tracked from donation through expiry or use.</p>"
                "<table border='1' cellspacing='0' cellpadding='6'><tr><th>Unit</th><th>Type</th><th>Volume</th><th>Donor</th><th>Donated</th><th>Expires</th><th>Status</th><th>Used for</th></tr>";
    for (const auto &u : hospital_.getBloodBank().getUnits()) {
        const int row = bloodTable_->rowCount();
        bloodTable_->insertRow(row);
        const QString unitId = QString::fromStdString(u.getUnitId());
        auto *unitCell = new QTableWidgetItem(unitId);
        unitCell->setData(Qt::UserRole, unitId);
        bloodTable_->setItem(row, 0, unitCell);
        bloodTable_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(u.getBloodType())));
        bloodTable_->setItem(row, 2, new QTableWidgetItem(QString::number(u.getVolumeMl()) + " ml"));
        bloodTable_->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(u.getDonorName())));
        bloodTable_->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(u.getDonationDate())));
        bloodTable_->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(u.getExpiryDate())));
        bloodTable_->setItem(row, 6, new QTableWidgetItem(QString::fromStdString(hms::bloodUnitStatusToString(u.getStatus()))));
        bloodTable_->setItem(row, 7, new QTableWidgetItem(u.getUsedForPatientId().empty() ? "Not issued" : QString::fromStdString(u.getUsedForPatientId())));
        bloodTable_->setItem(row, 8, new QTableWidgetItem(u.getUsedForOperation().empty() ? "Not issued" : QString::fromStdString(u.getUsedForOperation())));
        bloodTable_->setItem(row, 9, new QTableWidgetItem(u.getUsedDate().empty() ? "-" : QString::fromStdString(u.getUsedDate())));
        t += QString("<tr><td>%1</td><td>%2</td><td>%3 ml</td><td>%4</td><td>%5</td><td>%6</td><td>%7</td><td>%8</td></tr>")
                 .arg(QString::fromStdString(u.getUnitId()),
                      QString::fromStdString(u.getBloodType()),
                      QString::number(u.getVolumeMl()),
                      QString::fromStdString(u.getDonorName()),
                      QString::fromStdString(u.getDonationDate()),
                      QString::fromStdString(u.getExpiryDate()),
                      QString::fromStdString(hms::bloodUnitStatusToString(u.getStatus())),
                      QString::fromStdString(u.getUsedForOperation()));
    }
    for (const auto &p : hospital_.getPatients()) {
        managerBloodPatient_->addItem(
            QString("%1 · %2")
                .arg(QString::fromStdString(p.getPatientId()),
                     QString::fromStdString(p.getName())),
            QString::fromStdString(p.getPatientId()));
    }
    t += "</table>";
    managerView_->setHtml(t);
    for (const auto &u : hospital_.getBloodBank().getUnits()) {
        if (u.getStatus() != hms::BloodUnitStatus::Used)
            managerBloodUnitPick_->addItem(
                QString("%1 · %2 · %3")
                    .arg(QString::fromStdString(u.getUnitId()),
                         QString::fromStdString(u.getBloodType()),
                         QString::fromStdString(hms::bloodUnitStatusToString(u.getStatus()))),
                QString::fromStdString(u.getUnitId()));
    }
}

void MainWindow::managerDiscardBlood()
{
    const int row = bloodTable_->currentRow();
    const QString id = row >= 0 && bloodTable_->item(row, 0)
                           ? bloodTable_->item(row, 0)->data(Qt::UserRole).toString()
                           : managerBloodUnitPick_->currentData().toString();
    if (id.isEmpty()) return;
    if (!hospital_.getBloodBank().discardUnit(id.toStdString())) {
        QMessageBox::warning(this, "Blood bank", "Used units cannot be discarded from the active inventory.");
        return;
    }
    hospital_.getBloodBank().saveToCSV();
    managerBlood();
}

void MainWindow::managerIssueBlood()
{
    const QString patient = managerBloodPatient_->currentData().toString();
    const QString reason = managerBloodOperation_->text().trimmed();
    if (patient.isEmpty() || reason.isEmpty()) {
        QMessageBox::warning(this, "Blood bank", "Enter both a patient ID and an operation or treatment reason.");
        return;
    }
    const int row = bloodTable_->currentRow();
    QString selectedType = managerBloodTypePick_->currentText();
    if (row >= 0 && bloodTable_->item(row, 1))
        selectedType = bloodTable_->item(row, 1)->text();
    auto *unit = hospital_.getBloodBank().issueBlood(selectedType.toStdString(),
                                                     patient.toStdString(), reason.toStdString());
    if (!unit) {
        QMessageBox::warning(this, "Blood bank", "No available unit of that blood type was found.");
        return;
    }
    hospital_.getBloodBank().saveToCSV();
    QMessageBox::information(this, "Blood bank",
                             QString("Unit %1 was issued and recorded as Used.")
                                 .arg(QString::fromStdString(unit->getUnitId())));
    managerBlood();
}

void MainWindow::managerAnnounce()
{
    if (managerScheduleActions_) managerScheduleActions_->setVisible(false);
    returnEmergency_->setVisible(false);
    if (scheduleTable_) scheduleTable_->setVisible(false);

    managerView_->setVisible(true);
    staffTable_->setVisible(false);
    managerStaffActions_->setVisible(false);
    managerNoticePanel_->setVisible(true);
    managerBloodActions_->setVisible(false);
    bloodTable_->setVisible(false);
    ambulanceRequestTable_->setVisible(false);
    approveEmergency_->setVisible(false);
    clearEmergencySelection_->setVisible(false);
    announcementPick_->clear();
    announcementTable_->setRowCount(0);
    if (!announceEdit_->toPlainText().trimmed().isEmpty()) {
        hms::Announcement a;
        a.id = "ANN-" + nowStamp().toStdString();
        a.author = currentDisplayName_.toStdString();
        a.createdAt = nowStamp().toStdString();
        a.body = announceEdit_->toPlainText().toStdString();
        facilities_.addAnnouncement(a);
        announceEdit_->clear();
    }
    QString t;
    for (const auto &a : facilities_.announcements()) {
        t += QString("%1  %2\n%3\n\n")
                 .arg(QString::fromStdString(a.createdAt),
                      QString::fromStdString(a.author),
                      QString::fromStdString(a.body));
    }
    managerView_->setPlainText(t);
    for (const auto &a : facilities_.announcements()) {
        const int row = announcementTable_->rowCount();
        announcementTable_->insertRow(row);
        auto *date = new QTableWidgetItem(QString::fromStdString(a.createdAt));
        date->setData(Qt::UserRole, QString::fromStdString(a.id));
        announcementTable_->setItem(row, 0, date);
        announcementTable_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(a.author)));
        announcementTable_->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(a.body)));
    }
    for (const auto &a : facilities_.announcements()) {
        announcementPick_->addItem(QString("%1 · %2")
                                       .arg(QString::fromStdString(a.createdAt),
                                            QString::fromStdString(a.body).left(70)),
                                   QString::fromStdString(a.id));
    }
}

void MainWindow::managerReviews()
{
    if (managerScheduleActions_) managerScheduleActions_->setVisible(false);
    returnEmergency_->setVisible(false);
    if (scheduleTable_) scheduleTable_->setVisible(false);

    managerView_->setVisible(true);
    staffTable_->setVisible(false);
    managerStaffActions_->setVisible(false);
    managerNoticePanel_->setVisible(false);
    managerBloodActions_->setVisible(false);
    bloodTable_->setVisible(false);
    ambulanceRequestTable_->setVisible(false);
    approveEmergency_->setVisible(false);
    clearEmergencySelection_->setVisible(false);
    QString t = "<h2>Doctor and nurse star reviews</h2><table border='1' cellspacing='0' cellpadding='6'>"
                "<tr><th>Staff member</th><th>Role</th><th>Patient</th><th>Stars</th><th>Comment</th><th>Date</th></tr>";
    for (const auto &r : facilities_.reviews()) {
        t += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4 / 5</td><td>%5</td><td>%6</td></tr>")
                 .arg(QString::fromStdString(r.targetName),
                      QString::fromStdString(r.targetRole),
                      QString::fromStdString(r.patientName),
                      QString::number(r.stars),
                      QString::fromStdString(r.note),
                      QString::fromStdString(r.createdAt));
    }
    t += "</table>";
    managerView_->setHtml(t);
}

void MainWindow::managerAwardBonus()
{
    if (managerScheduleActions_) managerScheduleActions_->setVisible(false);
    returnEmergency_->setVisible(false);
    if (scheduleTable_) scheduleTable_->setVisible(false);

    managerView_->setVisible(true);
    staffTable_->setVisible(false);
    managerStaffActions_->setVisible(false);
    managerNoticePanel_->setVisible(false);
    managerBloodActions_->setVisible(false);
    bloodTable_->setVisible(false);
    ambulanceRequestTable_->setVisible(false);
    approveEmergency_->setVisible(false);
    clearEmergencySelection_->setVisible(false);
    std::map<std::string, int> score;
    for (const auto &r : facilities_.reviews())
        score[r.targetName] += r.stars;
    QString t = "<h2>Bonus recommendations</h2><table border='1' cellspacing='0' cellpadding='6'>"
                "<tr><th>Staff member</th><th>Role</th><th>Total stars</th><th>Reviews</th><th>Recommendation</th></tr>";
    int best = 0;
    std::string who;
    std::map<std::string, int> reviewCount;
    std::map<std::string, std::string> roles;
    for (const auto &r : facilities_.reviews()) {
        reviewCount[r.targetName]++;
        roles[r.targetName] = r.targetRole;
    }
    for (const auto &kv : score) {
        t += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td></tr>")
                 .arg(QString::fromStdString(kv.first),
                      QString::fromStdString(roles[kv.first]),
                      QString::number(kv.second),
                      QString::number(reviewCount[kv.first]),
                      kv.second >= 10 ? "Recommended" : "Building score");
        if (kv.second > best) {
            best = kv.second;
            who = kv.first;
        }
    }
    t += "</table>";
    if (!who.empty())
        t += "<p><b>Top recommendation:</b> " + QString::fromStdString(who) + "</p>";
    managerView_->setHtml(t);
}

void MainWindow::managerMedicines()
{
    if (managerScheduleActions_) managerScheduleActions_->setVisible(false);
    returnEmergency_->setVisible(false);
    if (scheduleTable_) scheduleTable_->setVisible(false);

    managerView_->setVisible(true);
    staffTable_->setVisible(false);
    managerStaffActions_->setVisible(false);
    managerNoticePanel_->setVisible(false);
    managerBloodActions_->setVisible(false);
    bloodTable_->setVisible(false);
    ambulanceRequestTable_->setVisible(false);
    approveEmergency_->setVisible(false);
    clearEmergencySelection_->setVisible(false);
    QString t = "<h2>Medicine stock</h2><p>Healthy: 100 or more. Monitor: 50-99. Reorder soon: below 50.</p>"
                "<table border='1' cellspacing='0' cellpadding='6'><tr><th>Medicine</th><th>Quantity</th><th>Unit</th><th>Stock level</th><th>Action</th></tr>";
    for (const auto &m : facilities_.medicines())
        t += QString("<tr><td>%1</td><td>%2</td><td>%3</td><td style='color:%4'>%5</td><td>%6</td></tr>")
                 .arg(QString::fromStdString(m.name))
                 .arg(m.quantity)
                 .arg(QString::fromStdString(m.unit))
                 .arg(m.quantity < 50 ? "#B64A4A" : "#2D7A5C")
                 .arg(m.quantity < 50 ? "Reorder soon" : (m.quantity < 100 ? "Monitor" : "Healthy"))
                 .arg(m.quantity < 50 ? "Create purchase request" : "No action");
    t += "</table>";
    managerView_->setHtml(t);
}

void MainWindow::managerDoctorScheduling()
{
    managerView_->setVisible(true);
    returnEmergency_->setVisible(false);
    staffTable_->setVisible(false);
    managerStaffActions_->setVisible(false);
    managerNoticePanel_->setVisible(false);
    managerBloodActions_->setVisible(false);
    bloodTable_->setVisible(false);
    ambulanceRequestTable_->setVisible(false);
    approveEmergency_->setVisible(false);
    clearEmergencySelection_->setVisible(false);
    if (managerScheduleActions_) managerScheduleActions_->setVisible(true);
    if (scheduleTable_) {
        scheduleTable_->setVisible(true);
        scheduleTable_->setRowCount(0);
    }

    if (managerDoctorFilter_->count() <= 1) {
        for (const auto &d : hospital_.getDoctors()) {
            managerDoctorFilter_->addItem(
                QString::fromStdString(d.getName()) + " (" + QString::fromStdString(d.getDoctorId()) + ")",
                QString::fromStdString(d.getDoctorId()));
        }
    }
    const QString doctorId = managerDoctorFilter_->currentData().toString();
    const QString status = managerScheduleStatusFilter_->currentText();
    QString t = "<h2>Doctor Scheduling</h2>"
                "<p>Use this desk to review every doctor's diary. <b>Scheduled</b> means booked, "
                "<b>Completed</b> means the visit was finished, and <b>Cancelled</b> means the slot is free again.</p>"
                "<table border='1' cellspacing='0' cellpadding='8'>"
                "<tr><th>Appointment</th><th>Doctor</th><th>Patient</th><th>Date and time</th><th>Status</th></tr>";
    int visible = 0;
    for (const auto &a : hospital_.getAppointments()) {
        const QString aDoctorId = QString::fromStdString(a.getDoctorId());
        const QString aStatus = QString::fromStdString(hms::appointmentStatusToString(a.getStatus()));
        if (!doctorId.isEmpty() && doctorId != aDoctorId) continue;
        if (status != "All statuses" && status != aStatus) continue;
        auto *p = hospital_.findPatientById(a.getPatientId());
        auto *d = hospital_.findDoctorById(a.getDoctorId());
        if (scheduleTable_) {
            const int row = scheduleTable_->rowCount();
            scheduleTable_->insertRow(row);
            auto *id = new QTableWidgetItem(QString::fromStdString(a.getAppointmentId()));
            id->setData(Qt::UserRole, QString::fromStdString(a.getAppointmentId()));
            scheduleTable_->setItem(row, 0, id);
            scheduleTable_->setItem(row, 1, new QTableWidgetItem(d ? QString::fromStdString(d->getName()) : aDoctorId));
            scheduleTable_->setItem(row, 2, new QTableWidgetItem(p ? QString::fromStdString(p->getName()) : QString::fromStdString(a.getPatientId())));
            scheduleTable_->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(a.getDateTime())));
            scheduleTable_->setItem(row, 4, new QTableWidgetItem(aStatus));
        }
        t += QString("<tr><td>%1</td><td>%2<br><small>%3</small></td><td>%4<br><small>%5</small></td><td>%6</td><td><b>%7</b></td></tr>")
                 .arg(QString::fromStdString(a.getAppointmentId()),
                      d ? QString::fromStdString(d->getName()) : aDoctorId,
                      aDoctorId,
                      p ? QString::fromStdString(p->getName()) : QString::fromStdString(a.getPatientId()),
                      QString::fromStdString(a.getPatientId()),
                      QString::fromStdString(a.getDateTime()), aStatus);
        ++visible;
    }
    if (visible == 0)
        t += "<tr><td colspan='5'>No appointments match the selected doctor and status.</td></tr>";
    t += "</table>";
    managerView_->setHtml(t);
}

void MainWindow::managerAllowAppointment()
{
    const int row = scheduleTable_ ? scheduleTable_->currentRow() : -1;
    if (row < 0 || !scheduleTable_->item(row, 0)) {
        QMessageBox::information(this, "Appointment schedule", "Select an appointment first.");
        return;
    }
    const QString id = scheduleTable_->item(row, 0)->data(Qt::UserRole).toString();
    if (hospital_.setAppointmentStatus(id.toStdString(), hms::AppointmentStatus::Scheduled)) {
        managerDoctorScheduling();
        return;
    }
    QMessageBox::warning(this, "Appointment schedule", "The selected appointment no longer exists.");
}

void MainWindow::managerCancelAppointment()
{
    const int row = scheduleTable_ ? scheduleTable_->currentRow() : -1;
    if (row < 0 || !scheduleTable_->item(row, 0)) {
        QMessageBox::information(this, "Appointment schedule", "Select an appointment first.");
        return;
    }
    const QString id = scheduleTable_->item(row, 0)->data(Qt::UserRole).toString();
    if (hospital_.setAppointmentStatus(id.toStdString(), hms::AppointmentStatus::Cancelled)) {
        managerDoctorScheduling();
        return;
    }
    QMessageBox::warning(this, "Appointment schedule", "The selected appointment no longer exists.");
}

void MainWindow::managerAppointments()
{
    if (managerScheduleStatusFilter_) managerScheduleStatusFilter_->setCurrentText("All statuses");
    if (managerDoctorFilter_) managerDoctorFilter_->setCurrentIndex(0);
    managerDoctorScheduling();
}

void MainWindow::managerAmbulances()
{
    if (managerScheduleActions_) managerScheduleActions_->setVisible(false);
    if (scheduleTable_) scheduleTable_->setVisible(false);
    managerView_->setVisible(true);
    staffTable_->setVisible(false);
    bloodTable_->setVisible(false);
    managerStaffActions_->setVisible(false);
    managerNoticePanel_->setVisible(false);
    managerBloodActions_->setVisible(false);
    ambulanceRequestTable_->setVisible(true);
    approveEmergency_->setVisible(true);
    returnEmergency_->setVisible(true);
    clearEmergencySelection_->setVisible(true);
    ambulanceRequestTable_->setRowCount(0);
    ambulanceRequestTable_->clearSelection();
    ambulanceRequestTable_->setCurrentCell(-1, -1);

    QString summary = QString("<h2>Ambulance control</h2><p><b>%1 / 20</b> ambulances are currently available at the bay. "
                              "Emergency Desk submissions remain Pending until the manager approves them.</p>")
                          .arg(hospital_.remainingAmbulances());
    managerView_->setHtml(summary);
    for (const auto &request : facilities_.emergencyRequests()) {
        const int row = ambulanceRequestTable_->rowCount();
        ambulanceRequestTable_->insertRow(row);
        auto *id = new QTableWidgetItem(QString::fromStdString(request.id));
        id->setData(Qt::UserRole, QString::fromStdString(request.id));
        ambulanceRequestTable_->setItem(row, 0, id);
        ambulanceRequestTable_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(request.callerName)));
        ambulanceRequestTable_->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(request.phone)));
        ambulanceRequestTable_->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(request.location)));
        ambulanceRequestTable_->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(request.severity)));
        ambulanceRequestTable_->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(request.createdAt)));
        ambulanceRequestTable_->setItem(row, 6, new QTableWidgetItem(QString::fromStdString(request.status)));
        ambulanceRequestTable_->setItem(row, 7, new QTableWidgetItem(QString::number(request.ambulancesSent)));
    }
}

void MainWindow::managerApproveEmergency()
{
    const int row = ambulanceRequestTable_->currentRow();
    if (row < 0 || !ambulanceRequestTable_->item(row, 0)) {
        QMessageBox::information(this, "Ambulance control", "Select a Pending request from the table first.");
        return;
    }
    const QString requestId = ambulanceRequestTable_->item(row, 0)->data(Qt::UserRole).toString();
    for (auto &request : facilities_.emergencyRequests()) {
        if (QString::fromStdString(request.id) != requestId) continue;
        if (request.status != "Pending") {
            QMessageBox::information(this, "Ambulance control", "This request has already been processed.");
            return;
        }
        const int ambulancesBefore = hospital_.remainingAmbulances();
        const std::string result = hospital_.dispatchEmergency(request.callerName, request.phone,
                                                                 request.location, request.severity);
        if (result.rfind("All 20 ambulances", 0) == 0) {
            QMessageBox::warning(this, "Ambulance control", QString::fromStdString(result));
            return;
        }
        const int sent = std::max(0, ambulancesBefore - hospital_.remainingAmbulances());
        request.status = "Dispatched";
        request.ambulancesSent = sent;
        facilities_.saveAll();
        hospital_.saveAllToCSV();
        QMessageBox::information(this, "Ambulance control", QString::fromStdString(result));
        managerAmbulances();
        return;
    }
}

void MainWindow::managerReturnEmergency()
{
    const int row = ambulanceRequestTable_->currentRow();
    if (row < 0 || !ambulanceRequestTable_->item(row, 0)) {
        QMessageBox::information(this, "Ambulance control", "Select a dispatched request first.");
        return;
    }
    const QString requestId = ambulanceRequestTable_->item(row, 0)->data(Qt::UserRole).toString();
    for (auto &request : facilities_.emergencyRequests()) {
        if (QString::fromStdString(request.id) != requestId) continue;
        if (request.status != "Dispatched" || request.ambulancesSent <= 0) {
            QMessageBox::information(this, "Ambulance control", "Only dispatched requests with active ambulances can be returned.");
            return;
        }
        hospital_.returnAmbulances(request.ambulancesSent);
        request.status = "Returned";
        facilities_.saveAll();
        QMessageBox::information(this, "Ambulance control",
                                 QString("%1 ambulance%2 returned to the hospital bay.")
                                     .arg(request.ambulancesSent)
                                     .arg(request.ambulancesSent == 1 ? " was" : "s were"));
        managerAmbulances();
        return;
    }
    QMessageBox::warning(this, "Ambulance control", "The selected request could not be found.");
}
