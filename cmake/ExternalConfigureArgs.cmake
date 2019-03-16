if(__external_configure_args_included)
    return()
endif()
set(__external_configure_args_included TRUE)

if(WIN32)
    string(APPEND CMAKE_C_FLAGS_INIT " -D_CRT_SECURE_NO_WARNINGS ")
else()
    string(APPEND CMAKE_C_FLAGS_INIT " -fPIC -fvisibility=hidden ")
endif()

if(${CMAKE_CROSSCOMPILING})
    if(RASPBERRYPI)
        # Cross compilation toolchains
        set(XDK_HOST "arm-linux-gnueabihf")
        set(XDK_SYSROOT ${CMAKE_SYSROOT})
        set(XDK_CC  ${CMAKE_C_COMPILER})
        set(XDK_CXX ${CMAKE_CXX_COMPILER})
        set(XDK_CPP "${CMAKE_C_COMPILER} -E")
        set(XDK_AR  ${CMAKE_AR})
        set(XDK_RANLIB ${CMAKE_RANLIB})
        set(XDK_STRIP ${CMAKE_STRIP})

        # Cross compilation flags
        set(XDK_C_FLAGS "${CMAKE_C_FLAGS_INIT} ${CMAKE_C_FLAGS}")
        set(XDK_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
        set(XDK_C_LINK_FLAGS "${CMAKE_C_FLAGS}")
    endif()

#[[
    message("======== Cross compiling environment for autoconf 3rd party =========")
    message("XDK_HOST=${XDK_HOST}")
    message("XDK_SYSROOT=${XDK_SYSROOT}")
    message("XDK_CC=${XDK_CC}")
    message("XDK_CXX=${XDK_CXX}")
    message("XDK_CPP=${XDK_CPP}")
    message("XDK_LD=${XDK_LD}")
    message("XDK_AR=${XDK_AR}")
    message("XDK_RANLIB=${XDK_RANLIB}")
    message("XDK_STRIP=${XDK_STRIP}")
    message("XDK_C_FLAGS=${XDK_C_FLAGS}")
    message("XDK_CXX_FLAGS=${XDK_CXX_FLAGS}")
    message("XDK_C_LINK_FLAGS=${XDK_C_LINK_FLAGS}")
    message("=====================================================================")
]]
    set(CONFIGURE_ARGS_INIT
        "--host=${XDK_HOST}"
        "CC=${XDK_CC}"
        "CXX=${XDK_CXX}"
        "CPP=${XDK_CPP}"
        "LD=${XDK_LD}"
        "AR=${XDK_AR}"
        "RANLIB=${XDK_RANLIB}"
        "STRIP=${XDK_STRIP}"
        "CFLAGS=${XDK_C_FLAGS}"
        "CXXFLAGS=${XDK_CXX_FLAGS}"
        "LDFLAGS=${XDK_C_LINK_FLAGS}")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_INIT} ${CMAKE_C_FLAGS}")

    set(CONFIGURE_ARGS_INIT
        "CFLAGS=${CMAKE_C_FLAGS}"
        "CXXFLAGS=${CMAKE_C_FLAGS} -fexceptions"
        "LDFLAGS=${CMAKE_C_FLAGS}")
endif()
