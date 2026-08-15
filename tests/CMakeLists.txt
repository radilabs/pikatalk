find_package(Qt6 REQUIRED COMPONENTS Test Sql Network Gui Qml Quick)

add_executable(applicationpaths_test
    applicationpaths_test.cpp
    ${CMAKE_SOURCE_DIR}/src/applicationpaths.cpp
    ${CMAKE_SOURCE_DIR}/src/applicationpaths.h
)

target_include_directories(applicationpaths_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(applicationpaths_test
    PRIVATE
    Qt6::Core
    Qt6::Test
)

target_compile_definitions(applicationpaths_test
    PRIVATE
    PIKATALK_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_test(NAME applicationpaths_test COMMAND applicationpaths_test)

add_executable(applicationversion_test
    applicationversion_test.cpp
    ${CMAKE_SOURCE_DIR}/src/applicationidentity.cpp
    ${CMAKE_SOURCE_DIR}/src/applicationidentity.h
)

target_include_directories(applicationversion_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(applicationversion_test
    PRIVATE
    Qt6::Core
    Qt6::Test
)

target_compile_definitions(applicationversion_test
    PRIVATE
    PIKATALK_VERSION="${PROJECT_VERSION}"
)

add_test(NAME applicationversion_test COMMAND applicationversion_test)

add_executable(titlefilter_test
    titlefilter_test.cpp
    ${CMAKE_SOURCE_DIR}/src/titlefilter.cpp
    ${CMAKE_SOURCE_DIR}/src/titlefilter.h
    ${CMAKE_SOURCE_DIR}/src/appcontroller.cpp
    ${CMAKE_SOURCE_DIR}/src/appcontroller.h
    ${CMAKE_SOURCE_DIR}/src/database.cpp
    ${CMAKE_SOURCE_DIR}/src/database.h
    ${CMAKE_SOURCE_DIR}/src/pikaclawclient.cpp
    ${CMAKE_SOURCE_DIR}/src/pikaclawclient.h
    ${CMAKE_SOURCE_DIR}/src/pikaclawsettings.cpp
    ${CMAKE_SOURCE_DIR}/src/pikaclawsettings.h
    ${CMAKE_SOURCE_DIR}/src/messageformatting.cpp
    ${CMAKE_SOURCE_DIR}/src/messageformatting.h
    ${CMAKE_SOURCE_DIR}/src/workspaceactions.cpp
    ${CMAKE_SOURCE_DIR}/src/workspaceactions.h
    ${CMAKE_SOURCE_DIR}/src/picoclawlifecycle.cpp
    ${CMAKE_SOURCE_DIR}/src/picoclawlifecycle.h
)

target_include_directories(titlefilter_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(titlefilter_test
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Sql
    Qt6::Test
)

add_test(NAME titlefilter_test COMMAND titlefilter_test)

add_executable(shortcuts_test
    shortcuts_test.cpp
)

target_link_libraries(shortcuts_test
    PRIVATE
    Qt6::Core
    Qt6::Test
)

target_compile_definitions(shortcuts_test
    PRIVATE
    PIKATALK_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_test(NAME shortcuts_test COMMAND shortcuts_test)

add_executable(errorcopy_test
    errorcopy_test.cpp
    ${CMAKE_SOURCE_DIR}/src/errorcopy.cpp
    ${CMAKE_SOURCE_DIR}/src/errorcopy.h
)

target_include_directories(errorcopy_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(errorcopy_test
    PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME errorcopy_test COMMAND errorcopy_test)

add_executable(longchat_test
    longchat_test.cpp
    ${CMAKE_SOURCE_DIR}/src/appcontroller.cpp
    ${CMAKE_SOURCE_DIR}/src/appcontroller.h
    ${CMAKE_SOURCE_DIR}/src/database.cpp
    ${CMAKE_SOURCE_DIR}/src/database.h
    ${CMAKE_SOURCE_DIR}/src/pikaclawclient.cpp
    ${CMAKE_SOURCE_DIR}/src/pikaclawclient.h
    ${CMAKE_SOURCE_DIR}/src/pikaclawsettings.cpp
    ${CMAKE_SOURCE_DIR}/src/pikaclawsettings.h
    ${CMAKE_SOURCE_DIR}/src/messageformatting.cpp
    ${CMAKE_SOURCE_DIR}/src/messageformatting.h
    ${CMAKE_SOURCE_DIR}/src/workspaceactions.cpp
    ${CMAKE_SOURCE_DIR}/src/workspaceactions.h
    ${CMAKE_SOURCE_DIR}/src/picoclawlifecycle.cpp
    ${CMAKE_SOURCE_DIR}/src/picoclawlifecycle.h
)

target_include_directories(longchat_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(longchat_test
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    Qt6::Sql
    Qt6::Test
)

target_compile_definitions(longchat_test
    PRIVATE
    PIKATALK_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    PIKATALK_BIN="$<TARGET_FILE:pikatalk>"
)

add_dependencies(longchat_test pikatalk)
add_test(NAME longchat_test COMMAND longchat_test)
set_tests_properties(longchat_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(packaging_test
    packaging_test.cpp
)

target_link_libraries(packaging_test
    PRIVATE
    Qt6::Core
    Qt6::Test
)

target_compile_definitions(packaging_test
    PRIVATE
    PIKATALK_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_test(NAME packaging_test COMMAND packaging_test)

add_executable(uistates_test
    uistates_test.cpp
)

target_link_libraries(uistates_test
    PRIVATE
    Qt6::Core
    Qt6::Test
)

target_compile_definitions(uistates_test
    PRIVATE
    PIKATALK_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_test(NAME uistates_test COMMAND uistates_test)

add_executable(database_test
    database_test.cpp
    ${CMAKE_SOURCE_DIR}/src/database.cpp
    ${CMAKE_SOURCE_DIR}/src/database.h
)

target_include_directories(database_test PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(database_test
    PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Test
)

add_test(NAME database_test COMMAND database_test)

add_executable(appcontroller_test
    appcontroller_test.cpp
    ${CMAKE_SOURCE_DIR}/src/appcontroller.cpp
    ${CMAKE_SOURCE_DIR}/src/appcontroller.h
    ${CMAKE_SOURCE_DIR}/src/database.cpp
    ${CMAKE_SOURCE_DIR}/src/database.h
    ${CMAKE_SOURCE_DIR}/src/pikaclawclient.cpp
    ${CMAKE_SOURCE_DIR}/src/pikaclawclient.h
    ${CMAKE_SOURCE_DIR}/src/pikaclawsettings.cpp
    ${CMAKE_SOURCE_DIR}/src/pikaclawsettings.h
    ${CMAKE_SOURCE_DIR}/src/messageformatting.cpp
    ${CMAKE_SOURCE_DIR}/src/messageformatting.h
    ${CMAKE_SOURCE_DIR}/src/workspaceactions.cpp
    ${CMAKE_SOURCE_DIR}/src/workspaceactions.h
    ${CMAKE_SOURCE_DIR}/src/picoclawlifecycle.cpp
    ${CMAKE_SOURCE_DIR}/src/picoclawlifecycle.h
)

target_include_directories(appcontroller_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(appcontroller_test
    PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Sql
    Qt6::Test
)

add_test(NAME appcontroller_test COMMAND appcontroller_test)

add_executable(pikaclawlifecycle_test
    picoclawlifecycle_test.cpp
    ${CMAKE_SOURCE_DIR}/src/picoclawlifecycle.cpp
    ${CMAKE_SOURCE_DIR}/src/picoclawlifecycle.h
)

target_include_directories(pikaclawlifecycle_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(pikaclawlifecycle_test
    PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Test
)

add_test(NAME pikaclawlifecycle_test COMMAND pikaclawlifecycle_test)

add_executable(pikaclawclient_test
    pikaclawclient_test.cpp
    ${CMAKE_SOURCE_DIR}/src/pikaclawclient.cpp
    ${CMAKE_SOURCE_DIR}/src/pikaclawclient.h
    ${CMAKE_SOURCE_DIR}/src/pikaclawsettings.cpp
    ${CMAKE_SOURCE_DIR}/src/pikaclawsettings.h
)

target_include_directories(pikaclawclient_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(pikaclawclient_test
    PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Test
)

add_test(NAME pikaclawclient_test COMMAND pikaclawclient_test)
