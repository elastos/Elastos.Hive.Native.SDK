project(demo C)

include(HiveDefaults)
include(CheckIncludeFile)
include(CheckFunctionExists)

check_include_file(malloc.h HAVE_MALLOC_H)
if(HAVE_MALLOC_H)
    add_definitions(-DHAVE_MALLOC_H=1)
endif()

check_include_file(unistd.h HAVE_UNISTD_H)
if(HAVE_UNISTD_H)
    add_definitions(-DHAVE_UNISTD_H=1)
endif()

check_include_file(process.h HAVE_PROCESS_H)
if(HAVE_PROCESS_H)
    add_definitions(-DHAVE_PROCESS_H=1)
endif()

check_include_file(winsock2.h HAVE_WINSOCK2_H)
if(HAVE_WINSOCK2_H)
    add_definitions(-DHAVE_WINSOCK2_H=1)
endif()

check_include_file(sys/resource.h HAVE_SYS_RESOURCE_H)
if(HAVE_SYS_RESOURCE_H)
    add_definitions(-DHAVE_SYS_RESOURCE_H=1)
endif()

check_include_file(sys/select.h HAVE_SYS_SELECT_H)
if(HAVE_SYS_SELECT_H)
    add_definitions(-DHAVE_SYS_SELECT_H=1)
endif()

check_function_exists(sigaction HAVE_SIGACTION)
if(HAVE_SIGACTION)
    add_definitions(-DHAVE_SIGACTION=1)
endif()

check_include_file(libgen.h HAVE_LIBGEN_H)
if(HAVE_LIBGEN_H)
    add_definitions(-DHAVE_LIBGEN_H=1)
endif()

check_include_file(fcntl.h HAVE_FCNTL_H)
if(HAVE_FCNTL_H)
    add_definitions(-DHAVE_FCNTL_H=1)
endif()

check_include_file(getopt.h HAVE_GETOPT_H)
if(HAVE_GETOPT_H)
    add_definitions(-DHAVE_GETOPT_H=1)
endif()

check_include_file(sys/param.h HAVE_SYS_PARAM_H)
if(HAVE_SYS_PARAM_H)
    add_definitions(-DHAVE_SYS_PARAM_H=1)
endif()

set(SRC
    demo.c
    config.c)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(SYSTEM_LIBS pthread)
endif()

include_directories(
    ../../src
    ${HIVE_INT_DIST_DIR}/include)

link_directories(
    ${HIVE_INT_DIST_DIR}/lib
    ${CMAKE_CURRENT_BINARY_DIR}/../../src)
add_definitions(-DLIBCONFIG_STATIC)

if(ENABLE_SHARED)
    add_definitions(-DCRYSTAL_DYNAMIC)
else()
    add_definitions(-DCRYSTAL_STATIC)
endif()

set(LIBS
    elahive
    crystal)

if(WIN32)
    add_definitions(
        -DWIN32_LEAN_AND_MEAN
        -D_CRT_SECURE_NO_WARNINGS
        -D_CRT_NONSTDC_NO_WARNINGS)

    # Force source code encoding to utf-8
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
    set(LIBS
        ${LIBS}
        pthread
        Ws2_32
        libconfig.lib
        Shlwapi)
else()
    set(LIBS
        ${LIBS}
        config)
endif()

add_executable(hivedemo ${SRC})

target_link_libraries(hivedemo ${LIBS} ${SYSTEM_LIBS})

install(TARGETS hivedemo
    RUNTIME DESTINATION "bin"
    ARCHIVE DESTINATION "lib"
    LIBRARY DESTINATION "lib")

install(FILES demo.conf
    DESTINATION "etc/hive"
    RENAME "hivedemo.conf")
