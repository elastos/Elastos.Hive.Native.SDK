project(hive-test C)

include(HiveDefaults)
include(CheckIncludeFile)
include(CheckFunctionExists)

check_include_file(unistd.h HAVE_UNISTD_H)
if(HAVE_UNISTD_H)
    add_definitions(-DHAVE_UNISTD_H=1)
endif()

check_include_file(getopt.h HAVE_GETOPT_H)
if(HAVE_GETOPT_H)
    add_definitions(-DHAVE_GETOPT_H=1)
endif()

check_include_file(sys/time.h HAVE_SYS_TIME_H)
if(HAVE_SYS_TIME_H)
    add_definitions(-DHAVE_SYS_TIME_H=1)
endif()

check_include_file(libgen.h HAVE_LIBGEN_H)
if(HAVE_LIBGEN_H)
    add_definitions(-DHAVE_LIBGEN_H=1)
endif()

set(SRC
    config.c
    tests.c
    hive-client-new-test.c
    hive-client-getinfo-test.c
    hive-client-login-test.c
    hive-client-logining-test.c
    hive-client-opendrive-test.c
    hive-client-drivelist-test.c
    hive-drive-getinfo-test.c
    hive-drive_mkdir-test.c
    hive-drive-filestat-test.c
    hive-drive-filelist-test.c
    hive-drive-copy-test.c
    hive-drive-move-test.c
    hive-expired-token-test.c
    )

add_definitions(-DLIBCONFIG_STATIC)

if(ENABLE_SHARED)
    add_definitions(-DCRYSTAL_DYNAMIC)
else()
    add_definitions(-DCRYSTAL_STATIC)
endif()

set(LIBS
    elahive-shared
    crystal)

set(DEPS
    ela-hive
    libcrystal
    libconfig
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
        libconfig.lib
        pthread
        Ws2_32
        Shlwapi)
else()
    set(LIBS
        ${LIBS}
        config
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

install(FILES tests.conf
    DESTINATION "etc/hive")