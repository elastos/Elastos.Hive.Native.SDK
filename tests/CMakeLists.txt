project(test C)

include(HiveDefaults)
include(CheckIncludeFile)
include(CheckFunctionExists)

set(SRC
    onedrv_auth.c)

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
    libcrystal)

if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN
        -D_CRT_SECURE_NO_WARNINGS
        -D_CRT_NONSTDC_NO_WARNINGS)

    # Force source code encoding to utf-8
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
    set(LIBS
        ${LIBS}
        pthread
        Ws2_32
        Shlwapi)
else()
    set(LIBS
        ${LIBS}
        pthread)
endif()

include_directories(
    .
    ../src
    ${HIVE_INT_DIST_DIR}/include)

link_directories(
    ${HIVE_INT_DIST_DIR}/lib
    ${CMAKE_CURRENT_BINARY_DIR}/../src)

add_executable(test
    ${SRC})

add_dependencies(test ${DEPS})
target_link_libraries(test ${LIBS})
