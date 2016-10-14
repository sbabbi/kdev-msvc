project(MSVCManager)
cmake_minimum_required(VERSION 3.0)

find_package (ECM 0.0.9 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${MSVCManager_SOURCE_DIR}/cmake/)

find_package(Qt5 5.4.0 REQUIRED COMPONENTS Core)
find_package(KF5 5.28.0 REQUIRED COMPONENTS ItemModels Parts TextEditor)
find_package(KDevPlatform 5.0.0 REQUIRED)
find_package(KDevelop 5.0.0 REQUIRED)

set(MSVCManager_SRCS
    debug.cpp
    devenvjob.cpp
    msvcbuilder.cpp
    msvcbuilderpreferences.cpp
    msvcconfig.cpp
    msvcprojectconfig.cpp
    msvcimportjob.cpp
    msvcmanager.cpp
    msvcmodelitems.cpp
    )

ki18n_wrap_ui(MSVCManager_SRCS msvcconfig.ui)
kdevplatform_add_plugin(kdevmsvcmanager JSON kdevmsvcmanager.json SOURCES ${MSVCManager_SRCS})
target_link_libraries(kdevmsvcmanager Qt5::Core KDev::Interfaces KDev::Project KDev::OutputView KF5::Parts)
