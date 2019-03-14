project(elastos-hive C)
cmake_policy(SET CMP0054 NEW)

include(HiveDefaults)
include(CheckIncludeFile)

check_include_file(unistd.h HAVE_UNISTD_H)
if(HAVE_UNISTD_H)
    add_definitions(-DHAVE_UNISTD_H=1)
endif()

check_include_file(io.h HAVE_IO_H)
if(HAVE_IO_H)
    add_definitions(-DHAVE_IO_H=1)
endif()

check_include_file(sys/time.h HAVE_SYS_TIME_H)
if(HAVE_SYS_TIME_H)
    add_definitions(-DHAVE_SYS_TIME_H=1)
endif()

configure_file(
    version.h.in
    version.h
    "@ONLY")

set(SRC
    http_cli.c
    hive.c
    oauth_cli.c
    onedrive.c
    sandbird.c)

set(HEADERS
    hive.h)

if(WIN32)
    set(SYSTEM_LIBS Ws2_32 Iphlpapi)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(SYSTEM_LIBS pthread m)
endif()

include_directories(
    BEFORE
    .
    ${CMAKE_CURRENT_BINARY_DIR}
    ${HIVE_INT_DIST_DIR}/include)

link_directories(
    ${HIVE_INT_DIST_DIR}/lib)

if(WIN32)
    add_definitions(
        -DWIN32_LEAN_AND_MEAN
        -D_CRT_SECURE_NO_WARNINGS
        -D_CRT_NONSTDC_NO_WARNINGS)

    # Force source code encoding to utf-8
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
endif()

set(LIBS
    crystal
    libcurl
    cjson)

if(WIN32)
set(LIBS
    ${LIBS}
    pthread)
else()
set(LIBS
    ${LIBS})
endif()

add_definitions(-DHIVE_BUILD)

add_custom_target(ela-hive)

set(ELAHIVE_DEPENDS curl libcrystal cJSON)
if(ENABLE_STATIC)
    add_library(elahive-static STATIC ${SRC})
    add_dependencies(elahive-static ${ELAHIVE_DEPENDS})

    target_compile_definitions(elahive-static PRIVATE HIVE_STATIC CRYSTAL_STATIC)
    set_target_properties(elahive-static PROPERTIES OUTPUT_NAME elahive)
    if(WIN32)
        set_target_properties(elahive-static PROPERTIES LINK_FLAGS /FORCE:MULTIPLE)
    endif()

    add_dependencies(ela-hive elahive-static)

    install(TARGETS elahive-static
        RUNTIME DESTINATION "bin"
        ARCHIVE DESTINATION "lib"
        LIBRARY DESTINATION "lib")
endif()

if(ENABLE_SHARED)
    add_library(elahive-shared SHARED ${SRC})
    add_dependencies(elahive-shared ${ELAHIVE_DEPENDS})

    target_compile_definitions(elahive-shared PRIVATE HIVE_DYNAMIC CRYSTAL_DYNAMIC)
    set_target_properties(elahive-shared PROPERTIES OUTPUT_NAME elahive)
    if(WIN32)
        set_target_properties(elahive-shared PROPERTIES LINK_FLAGS /FORCE:MULTIPLE)
    endif()
    target_link_libraries(elahive-shared ${LIBS} ${SYSTEM_LIBS})

    add_dependencies(ela-hive elahive-shared)

    install(TARGETS elahive-shared
        RUNTIME DESTINATION "bin"
        ARCHIVE DESTINATION "lib"
        LIBRARY DESTINATION "lib")
endif()

install(FILES ${HEADERS} DESTINATION "include")