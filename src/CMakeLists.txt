project(elastos_hive C)
cmake_policy(SET CMP0054 NEW)

include(HiveDefaults)
include(CheckIncludeFile)

check_include_file(arpa/inet.h HAVE_ARPA_INET_H)
if(HAVE_ARPA_INET_H)
    add_definitions(-DHAVE_ARPA_INET_H=1)
endif()

check_include_file(unistd.h HAVE_UNISTD_H)
if(HAVE_UNISTD_H)
    add_definitions(-DHAVE_UNISTD_H=1)
endif()

check_include_file(stdlib.h HAVE_STDLIB_H)
if(HAVE_STDLIB_H)
    add_definitions(-DHAVE_STDLIB_H=1)
endif()

check_include_file(malloc.h HAVE_MALLOC_H)
if(HAVE_MALLOC_H)
    add_definitions(-DHAVE_MALLOC_H=1)
endif()

check_include_file(alloca.h HAVE_ALLOCA_H)
if(HAVE_ALLOCA_H)
    add_definitions(-DHAVE_ALLOCA_H=1)
endif()

check_include_file(libgen.h HAVE_LIBGEN_H)
if(HAVE_LIBGEN_H)
    add_definitions(-DHAVE_LIBGEN_H=1)
endif()

check_include_file(io.h HAVE_IO_H)
if(HAVE_IO_H)
    add_definitions(-DHAVE_IO_H=1)
endif()

check_include_file(sys/time.h HAVE_SYS_TIME_H)
if(HAVE_SYS_TIME_H)
    add_definitions(-DHAVE_SYS_TIME_H=1)
endif()

check_include_file(sys/param.h HAVE_SYS_PARAM_H)
if(HAVE_SYS_PARAM_H)
    add_definitions(-DHAVE_SYS_PARAM_H=1)
endif()

set(SRC
    hive_error.c
    hive_file.c
    hive_drive.c
    hive_client.c
    http_status.c
    mkdirs.c
    sandbird/sandbird.c
    http/http_client.c
    oauth/oauth_token.c
    vendors/ipfs/ipfs_client.c
    vendors/ipfs/ipfs_drive.c
    vendors/ipfs/ipfs_file.c
    vendors/ipfs/ipfs_rpc.c
    vendors/ipfs/ipfs_utils.c
    vendors/onedrive/onedrive_client.c
    vendors/onedrive/onedrive_drive.c
    vendors/onedrive/onedrive_file.c
    vendors/native/native_client.c
    vendors/owncloud/owncloud.c)

set(HEADERS
    ela_hive.h)

if(WIN32)
    set(SYSTEM_LIBS Ws2_32 Iphlpapi)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(SYSTEM_LIBS pthread m)
endif()

include_directories(
    BEFORE
    .
    http
    oauth
    sandbird
    vendors/native
    vendors/ipfs
    vendors/onedrive
    vendors/owncloud
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
    set_target_properties(elahive-shared PROPERTIES
        OUTPUT_NAME elahive)
    if(WIN32)
        set_target_properties(elahive-shared PROPERTIES LINK_FLAGS /FORCE:MULTIPLE)
    endif()
    if(DARWIN OR IOS)
        set_property(SOURCE ${SRC} APPEND PROPERTY
            OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/symbols.txt")
        set_property(TARGET elahive-shared APPEND_STRING PROPERTY
            LINK_FLAGS " -Wl,-exported_symbols_list,\"${CMAKE_CURRENT_SOURCE_DIR}/symbols.txt\"")
    endif()
    target_link_libraries(elahive-shared ${LIBS} ${SYSTEM_LIBS})

    add_dependencies(ela-hive elahive-shared)

    install(TARGETS elahive-shared
        RUNTIME DESTINATION "bin"
        ARCHIVE DESTINATION "lib"
        LIBRARY DESTINATION "lib")
endif()

install(FILES ${HEADERS} DESTINATION "include")