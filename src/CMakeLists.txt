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
    KF6::I18n
    KF6::I18nQml
    KF6::CoreAddons
    KF6::IconThemes
)

install(TARGETS pikatalk ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
