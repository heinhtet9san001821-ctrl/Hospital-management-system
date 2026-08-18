cmake_minimum_required(VERSION 3.15)
project(HospitalManagementSystem LANGUAGES CXX)

# C++ Standard ကို Version 17 သတ်မှတ်ခြင်း
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Header (.h) ဖိုင်များရှိရာ include ဖိုဒါကို လမ်းညွှန်ခြင်း
include_directories(include)

# src/ ဖိုဒါအောက်က .cpp ဖိုင်အားလုံးကို စုစည်းခြင်း
file(GLOB SOURCES "src/*.cpp")

# Executable ဆော့ဖ်ဝဲလ် App ထုတ်လုပ်ခြင်း
add_executable(hms_app
    src/main.cpp
    src/hospital_manager.cpp
    src/blood_bank.cpp
    src/appointment.cpp
    src/doctor.cpp
    src/nurse.cpp
    src/patient.cpp
    src/person.cpp
    src/user.cpp
)

