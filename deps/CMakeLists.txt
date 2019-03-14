include(AddSubModule)

add_submodule(platform-specific)
add_submodule(libcrystal
    DEPENDS platform-specific)
add_submodule(cJSON
    DEPENDS platform-specific)
add_submodule(zlib
    DEPENDS platform-specific)
add_submodule(libressl
    DEPENDS platform-specific)
add_submodule(curl
    DEPENDS platform-specific zlib libressl)

if(WIN32)
    add_submodule(pthreads-w32
        DEPENDS platform-specific)
    add_dependencies(libcrystal pthreads-w32)
endif()
