if(__external_cmake_args_included)
    return()
endif()
set(__external_cmake_args_included TRUE)

cmake_policy(PUSH)
cmake_policy(SET CMP0054 NEW)

include(HiveDefaults)

if(MSVC)
    set(CMAKE_ARGS_INIT "-DCMAKE_C_FLAGS_INIT=-D_CRT_SECURE_NO_WARNINGS")
else()
    set(CMAKE_ARGS_INIT "-DCMAKE_C_FLAGS_INIT=-fPIC -fvisibility=hidden")
endif()

cmake_policy(POP)

set(CMAKE_ARGS_INIT ${CMAKE_ARGS_INIT}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

if(RASPBERRYPI)
    set(XDK_SYSROOT ${CMAKE_SYSROOT})
    set(CMAKE_ARGS_INIT ${CMAKE_ARGS_INIT}
        -DRPI_TOOLCHAIN_HOME=${RPI_TOOLCHAIN_HOME})
endif()

if(CMAKE_CROSSCOMPILING)
    set(CMAKE_ARGS_INIT ${CMAKE_ARGS_INIT}
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        -DCMAKE_SYSROOT=${XDK_SYSROOT}
        -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER
        -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY
        -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY
        -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY
        -DCMAKE_FIND_ROOT_PATH=${HIVE_INT_DIST_DIR})

#[[
    message("========== Cross compiling environment for CMake 3rd party ==========")
    message("${CMAKE_ARGS_INIT}")
    message("=====================================================================")
]]
endif()
