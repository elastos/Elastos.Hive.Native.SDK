project(libcrystal)

include(HiveDefaults)
include(ExternalProject)
include(ExternalCMakeArgs)

ExternalProject_Add(
    libcrystal

    PREFIX ${HIVE_DEPS_BUILD_PREFIX}
    URL "https://github.com/iwhisperio/libcrystal/archive/master.zip"
    URL_HASH SHA256=70cf1b4f5fc1a59af45881d78749357245e1cf53c73019e34d7eb95b17110b7a
    DOWNLOAD_NAME "libcrystal-master.zip"
    DOWNLOAD_DIR ${HIVE_DEPS_TARBALL_DIR}
    DOWNLOAD_NO_PROGRESS 1

    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${HIVE_INT_DIST_DIR}
        -DENABLE_STATIC=${ENABLE_STATIC}
        -DENABLE_SHARED=${ENABLE_SHARED}
        -DENABLE_CRYPTO=FALSE
        -DENABLE_TESTS=FALSE
        ${CMAKE_ARGS_INIT}
)

if(ENABLE_SHARED)
    if(WIN32)
        set(INSTALL_DESTINATION bin)
    else()
        set(INSTALL_DESTINATION lib)
    endif()

    string(CONCAT SHARED_LIBRARY_NAME
        "${HIVE_INT_DIST_DIR}/${INSTALL_DESTINATION}/"
        "${CMAKE_SHARED_LIBRARY_PREFIX}"
        "crystal"
        "${CMAKE_SHARED_LIBRARY_SUFFIX}")

    file(RELATIVE_PATH SHARED_LIBRARY_NAME ${CMAKE_CURRENT_LIST_DIR}
        ${SHARED_LIBRARY_NAME})

    install(PROGRAMS "${SHARED_LIBRARY_NAME}"
        DESTINATION ${INSTALL_DESTINATION})
endif()
