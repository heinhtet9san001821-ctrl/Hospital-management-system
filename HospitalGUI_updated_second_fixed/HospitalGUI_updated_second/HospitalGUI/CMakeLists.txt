cmake_minimum_required(VERSION 3.16)

project(HospitalGUI VERSION 1.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    include/appointment.h
    include/blood_bank.h
    include/date_utils.h
    include/doctor.h
    include/facility.h
    include/hospital_manager.h
    include/nurse.h
    include/patient.h
    include/person.h
    include/user.h
    src/hospital_manager.cpp
    src/blood_bank.cpp
    src/appointment.cpp
    src/doctor.cpp
    src/nurse.cpp
    src/patient.cpp
    src/person.cpp
    src/user.cpp
    src/facility.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(HospitalGUI
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(HospitalGUI ${PROJECT_SOURCES})
endif()

target_include_directories(HospitalGUI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(HospitalGUI PRIVATE
    HMS_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(HospitalGUI PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.healthplusplus.HospitalGUI)
endif()

set_target_properties(HospitalGUI PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS HospitalGUI
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(HospitalGUI)
endif()
