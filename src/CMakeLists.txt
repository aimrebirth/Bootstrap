#
# Polygon-4 Bootstrapper
#

include_directories(include)

set(functional_src
    functional.cpp
    functional.h

    md5.cpp
    md5.h

    ../Bootstrap.json
)

add_library             (functional ${functional_src})

set(bootstrap_src
    bootstrap.cpp
    functional.h
)

add_executable          (Bootstrap ${bootstrap_src})
target_link_libraries   (Bootstrap functional ${Boost_LIBRARIES})

set(updater_src
    updater.cpp
    functional.h
)

add_executable          (BootstrapUpdater ${updater_src})
target_link_libraries   (BootstrapUpdater functional ${Boost_LIBRARIES})

################################################################################