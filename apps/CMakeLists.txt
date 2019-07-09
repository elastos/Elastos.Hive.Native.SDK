include(AddSubModule)

set(DEPEND_MODULES
    libcrystal
    libconfig
    ela-hive)

add_submodule(hivecmd
    DIRECTORY cmd
    DEPENDS ${DEPEND_MODULES})
