# Hospital Management System (HMS) with GUI

A professional, desktop-based Hospital Management System (HMS) developed with a graphical user interface (GUI) using C++ and the Qt framework. This project aims to streamline hospital workflows, manage patient records, and improve administrative efficiency.

* **Hein Htet San (Project Leader & Lead UI Developer)**
  * Formulated overall project architecture, milestones, and task delegation.
  * Designed and developed the main Graphical User Interface (GUI) using Qt Designer.
  * Managed team coordination and integration of all backend-frontend modules.
 
* **Hsu Yati Htet (Backend Developer - Patient & Staff Management)**
  * Implemented core logic for patient registration and medical record storage.
  * Developed modules for managing doctor profiles, departments, and scheduling.
 
* **Akari (Backend Developer - Appointment & Billing Systems)**
  * Created the appointment booking system and automated queuing logic.
  * Designed the hospital billing and invoicing calculation engine.

* **April Win (Database Administrator & File I/O Specialist)**
  * Configured data structures and handled file handling / database mechanisms for data persistence.
  * Ensured data security, recovery, and seamless read/write operations for hospital records.

* **Chaw Nyein Tun (Quality Assurance (QA) & Integration Section)**
  * Conducted system-wide debugging, testing, and memory leak checks using Qt tools.
  * Assisted in connecting Qt GUI components with the backend logic.


##  Tech Stack & Extensions
Programming Language: C++
GUI Framework: Qt Framework (Qt Creator & Qt Designer)
Extension/Tools: Qt Widgets, Qt Signals and Slots mechanism for event handling

##  Key Features
🏥 Role-Based Hospital Management System

A comprehensive role-based Hospital Management System designed with four distinct user roles — HMS Manager, Doctor, Patient, and Nurse — with customized permissions and features for each role.

👨‍💼 HMS Manager

The HMS Manager has centralized access to hospital records and system activities. The manager can monitor hospital information, publish important announcements, and manage the doctor bonus system based on patient star reviews and ratings, helping encourage quality healthcare services.

👨‍⚕️ Doctor

Doctors can efficiently manage their appointments, access relevant patient medical histories, and respond to emergency cases. Doctors can also manage their availability and handle patients who require immediate attention in the Emergency Room.

🧑‍🦽 Patient

Patients can search for doctors and diseases using keywords, view detailed doctor information, make appointments, book hospital rooms, and request the services of doctors or nurses. After receiving healthcare services, patients can also provide star ratings and reviews for doctors.

👩‍⚕️ Nurse

Nurses can access relevant patient information, assist with patient care, and report important patient updates and observations directly to doctors, improving communication between nurses and doctors.

🚨 Emergency Access Without Sign-In

The system provides a dedicated Emergency option directly from the initial screen, allowing users to request urgent assistance without signing in first. This helps ensure that emergency situations are not delayed by the normal authentication process.

🚑 Ambulance Request

Users can submit an ambulance request to the HMS Manager during emergency situations, allowing the hospital management team to receive and respond to urgent transportation requests.

## 🚀 How to Run the Project
To run this application locally, you need to have **Qt Creator** installed.
1. Clone this repository:
   ```bash
   git clone https://github.com/heinhtet9san001821-ctrl/Hospital-management-system.git
   ```
2. Open the `.pro` file in **Qt Creator**.
3. Build and Run the project (Ctrl + R).

## 📄 License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
