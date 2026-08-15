add_executable(pikatalk)

ecm_add_qml_module(pikatalk
    URI
    org.radilabs.pikatalk
)

target_sources(pikatalk
    PRIVATE
    main.cpp
    applicationpaths.cpp
    applicationpaths.h
    database.cpp
    database.h
    appcontroller.cpp
    appcontroller.h
    pikaclawclient.cpp
    pikaclawclient.h
    pikaclawsettings.cpp
    pikaclawsettings.h
    messageformatting.cpp
    messageformatting.h
    workspaceactions.cpp
    workspaceactions.h
)

ecm_target_qml_sources(pikatalk
    SOURCES
    Main.qml
)

target_link_libraries(pikatalk
    PRIVATE
    Qt6::Quick
    Qt6::Qml
    Qt6::Gui
    Qt6::QuickControls2
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    KF6::I18n
    KF6::I18nQml
    KF6::CoreAddons
    KF6::IconThemes
)

install(TARGETS pikatalk ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
