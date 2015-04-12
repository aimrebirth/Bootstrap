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
    bootstrap.cpp
    functional.h
)

add_executable          (Bootstrap ${bootstrap_src})
target_link_libraries   (Bootstrap functional ${Boost_LIBRARIES})


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

################################################################################