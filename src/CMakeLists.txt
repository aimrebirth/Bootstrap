#
# Polygon-4 Bootstrapper
#

include_directories(include)

########################################
# LIBRARY functional
########################################

set(functional_src
    functional.cpp
    functional.h

    md5.cpp
    md5.h

    ../Bootstrap.json
)

add_library             (functional ${functional_src})


########################################
# EXECUTABLE Bootstrap
########################################

set(bootstrap_src
    bootstrap_developer.cpp
    functional.h
)

add_executable          (BootstrapDeveloper ${bootstrap_src})
target_link_libraries   (BootstrapDeveloper functional ${Boost_LIBRARIES})


########################################
# EXECUTABLE BootstrapRelease
########################################

set(bootstrap_src
    bootstrap_release.cpp
    functional.h
)

add_executable          (BootstrapRelease ${bootstrap_src})
target_link_libraries   (BootstrapRelease functional ${Boost_LIBRARIES})


########################################
# EXECUTABLE BootstrapUpdater
########################################

set(updater_src
    updater.cpp
    functional.h
)

add_executable          (BootstrapUpdater ${updater_src})
target_link_libraries   (BootstrapUpdater functional ${Boost_LIBRARIES})


########################################
# EXECUTABLE RemoveUntrackedContent
########################################

set(remove_u_c_src
    remove_untracked_content.cpp
    functional.h
)

add_executable          (RemoveUntrackedContent ${remove_u_c_src})
target_link_libraries   (RemoveUntrackedContent functional ${Boost_LIBRARIES})
target_compile_definitions(RemoveUntrackedContent PUBLIC -DBOOTSTRAP_PREFIX=L"../")

################################################################################