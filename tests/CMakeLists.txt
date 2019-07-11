project(hive-api-tests C)

include(HiveDefaults)
include(CheckIncludeFile)
include(CheckFunctionExists)

check_include_file(io.h HAVE_IO_H)
if(HAVE_IO_H)
    add_definitions(-DHAVE_IO_H=1)
endif()
check_include_file(alloca.h HAVE_ALLOCA_H)
if(HAVE_ALLOCA_H)
    add_definitions(-DHAVE_ALLOCA_H=1)
endif()
check_include_file(malloc.h HAVE_MALLOC_H)
if(HAVE_MALLOC_H)
    add_definitions(-DHAVE_MALLOC_H=1)
endif()
check_include_file(unistd.h HAVE_UNISTD_H)
if(HAVE_UNISTD_H)
    add_definitions(-DHAVE_UNISTD_H=1)
endif()
check_include_file(getopt.h HAVE_GETOPT_H)
if(HAVE_GETOPT_H)
    add_definitions(-DHAVE_GETOPT_H=1)
endif()
check_include_file(sys/resource.h HAVE_SYS_RESOURCE_H)
if(HAVE_SYS_RESOURCE_H)
    add_definitions(-DHAVE_SYS_RESOURCE_H=1)
endif()
check_include_file(direct.h HAVE_DIRECT_H)
if(HAVE_DIRECT_H)
    add_definitions(-DHAVE_DIRECT_H=1)
endif()

check_include_file(process.h HAVE_PROCESS_H)
if(HAVE_PROCESS_H)
    add_definitions(-DHAVE_PROCESS_H=1)
endif()

check_include_file(winsock2.h HAVE_WINSOCK2_H)
if(HAVE_WINSOCK2_H)
    add_definitions(-DHAVE_WINSOCK2_H=1)
endif()

check_include_file(sys/time.h HAVE_SYS_TIME_H)
if(HAVE_SYS_TIME_H)
    add_definitions(-DHAVE_SYS_TIME_H=1)
endif()

check_function_exists(sigaction HAVE_SIGACTION)
if(HAVE_SIGACTION_H)
    add_definitions(-DHAVE_SIGACTION=1)
endif()

aux_source_directory(api/client CLIENT_CASES)
aux_source_directory(api/drive DRIVE_CASES)
aux_source_directory(api/file FILE_CASES)

set(SRC
    main.c
    config.c
    launcher.c
    test_helper.c
    api/tests.c
    api/test_context.c)

add_definitions(-DLIBCONFIG_STATIC)

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
        pdcurses
        pthread
        Ws2_32
        Shlwapi)

    set(DEPS ${DEPS} PDCurses)
else()
    set(LIBS
        ${LIBS}
        cunit
        config
        ncurses
        pthread)
endif()

include_directories(
    .
    include
    api
    ../src
    ${HIVE_INT_DIST_DIR}/include)

link_directories(
    ${HIVE_INT_DIST_DIR}/lib
    ${CMAKE_CURRENT_BINARY_DIR}/../src)

add_executable(hivetests
    ${SRC}
    ${CLIENT_CASES}
    ${DRIVE_CASES}
    ${FILE_CASES})

add_dependencies(hivetests ${DEPS})
target_link_libraries(hivetests ${LIBS})

install(TARGETS hivetests
    RUNTIME DESTINATION "bin"
    ARCHIVE DESTINATION "lib"
    LIBRARY DESTINATION "lib")

install(FILES tests.conf
    DESTINATION "etc/hive")
