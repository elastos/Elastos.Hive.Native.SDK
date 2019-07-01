include(AddSubModule)

set(DEPEND_MODULES
    libcrystal
    libconfig
    ela-hive)

add_submodule(hivedemo
    DIRECTORY demo
    DEPENDS ${DEPEND_MODULES})
