project(hive-test C)

include(HiveDefaults)
include(CheckIncludeFile)
include(CheckFunctionExists)

set(SRC
    tests.c
    hive-new-test.c
    hive-authorize-test.c
    hive-mkdir-test.c
    hive-no-file-test.c
    hive-file-test.c
    hive-asynctest.c
    )

if(ENABLE_SHARED)
    add_definitions(-DCRYSTAL_DYNAMIC)
else()
    add_definitions(-DCRYSTAL_STATIC)
endif()

set(LIBS
    elahive
    crystal)

set(DEPS
    ela-hive
    libcrystal
    CUnit)

if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN
        -D_CRT_SECURE_NO_WARNINGS
        -D_CRT_NONSTDC_NO_WARNINGS)

    # Force source code encoding to utf-8
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
    set(LIBS
        ${LIBS}
        libcunit
        pthread
        Ws2_32
        Shlwapi)
else()
    set(LIBS
        ${LIBS}
        cunit
        pthread)
endif()

include_directories(
    .
    ../src
    ${HIVE_INT_DIST_DIR}/include)

link_directories(
    ${HIVE_INT_DIST_DIR}/lib
    ${CMAKE_CURRENT_BINARY_DIR}/../src)

add_executable(hivetest
    ${SRC})

add_dependencies(hivetest ${DEPS})
target_link_libraries(hivetest ${LIBS})
install(TARGETS hivetest
    RUNTIME DESTINATION "bin"
    ARCHIVE DESTINATION "lib"
    LIBRARY DESTINATION "lib")