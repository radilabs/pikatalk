cmake_minimum_required(VERSION 3.20)
project(pikatalk VERSION 1.0.0 LANGUAGES CXX)

find_package(ECM 6.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMFindQmlModule)
include(ECMQmlModule)
include(FeatureSummary)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Network
    Quick
    Gui
    QuickControls2
    Widgets
    Sql
)

find_package(KF6 REQUIRED COMPONENTS
    Kirigami
    I18n
    CoreAddons
    QQC2DesktopStyle
    IconThemes
)

ecm_find_qmlmodule(org.kde.kirigami REQUIRED)

add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

install(PROGRAMS org.radilabs.pikatalk.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.radilabs.pikatalk.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES icons/org.radilabs.pikatalk.svg DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
