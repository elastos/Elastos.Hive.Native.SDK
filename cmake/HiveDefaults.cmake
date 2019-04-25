if(__hive_defaults_included)
    return()
endif()
set(__hive_defaults_included TRUE)

# Global default variables defintions
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(CMAKE_CROSSCOMPILING)
    if("${CMAKE_INSTALL_PREFIX}" STREQUAL "")
        set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/outputs")
    endif()

    if(${CMAKE_INSTALL_PREFIX} STREQUAL "/usr/local")
        set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/outputs")
    endif()
endif()

# Hive Version Defintions.
set(HIVE_VERSION_MAJOR "5")
set(HIVE_VERSION_MINOR "2")
execute_process(
    COMMAND git rev-parse master
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_ID
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if("${GIT_COMMIT_ID}" STREQUAL "")
    # Not a git repository, maybe ectracted from downloaded tarball.
    set(HIVE_VERSION_PATCH "unknown")
else()
    string(SUBSTRING ${GIT_COMMIT_ID} 0 6 HIVE_VERSION_PATCH)
endif()

# Third-party dependency tarballs directory
set(HIVE_DEPS_TARBALL_DIR "${CMAKE_SOURCE_DIR}/build/.tarballs")
set(HIVE_DEPS_BUILD_PREFIX "external")

# Intermediate distribution directory
set(HIVE_INT_DIST_DIR "${CMAKE_BINARY_DIR}/intermediates")
if(WIN32)
    file(TO_NATIVE_PATH
        "${HIVE_INT_DIST_DIR}" HIVE_INT_DIST_DIR)
endif()

# Host tools directory
set(HIVE_HOST_TOOLS_DIR "${CMAKE_BINARY_DIR}/host")
if(WIN32)
    file(TO_NATIVE_PATH
         "${HIVE_HOST_TOOLS_DIR}" HIVE_HOST_TOOLS_DIR)
endif()

if(WIN32)
    set(PATCH_EXE "${HIVE_HOST_TOOLS_DIR}/usr/bin/patch.exe")
else()
    set(PATCH_EXE "patch")
endif()

if(APPLE)
    set(CMAKE_INSTALL_RPATH "@execuable_path/Frameworks;@loader_path/Frameworks;@loader_path/../lib")
else()
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif()
set(CMAKE_MACOSX_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

if (NOT WIN32)
set(CMAKE_BUILD_WITH_INSTALL_NAME_DIR TRUE)
set(CMAKE_INSTALL_NAME_DIR "@rpath")

set(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG "-Wl,-rpath,")
set(CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG_SEP ":")
endif()

##Only suport for windows.
if(WIN32)
function(set_win_build_options build_options suffix)
    # check vs platform.
    # Notice: use CMAKE_SIZEOF_VOID_P to check whether target is
    # 64bit or 32bit instead of using CMAKE_SYSTEM_PROCESSOR.
    if(${CMAKE_SIZEOF_VOID_P} STREQUAL "8")
        set(_PLATFORM "x64")
    else()
        set(_PLATFORM "Win32")
    endif()

    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(_CONFIGURATION "Debug${suffix}")
    else()
        set(_CONFIGURATION "Release${suffix}")
    endif()

    string(CONCAT _BUILD_OPTIONS
        "/p:"
        "Configuration=${_CONFIGURATION},"
        "Platform=${_PLATFORM},"
        "InstallDir=${HIVE_INT_DIST_DIR}")

    # update parent scope variable.
    set(${build_options} "${_BUILD_OPTIONS}" PARENT_SCOPE)
endfunction()
endif()