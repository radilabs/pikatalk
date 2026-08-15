find_package(Qt6 REQUIRED COMPONENTS Test Sql Network)

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
)

target_include_directories(appcontroller_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_link_libraries(appcontroller_test
    PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    Qt6::Test
)

add_test(NAME appcontroller_test COMMAND appcontroller_test)

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
