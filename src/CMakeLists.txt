find_package( Boost COMPONENTS system filesystem regex)
rock_library(lib_config
    SOURCES
        Bundle.cpp
        Configuration.cpp
        YAMLConfiguration.cpp
    HEADERS
        Bundle.hpp
        Configuration.hpp
        YAMLConfiguration.hpp
    DEPS_PKGCONFIG
        base-types
        yaml-cpp
    DEPS_PLAIN 
        Boost_SYSTEM Boost_FILESYSTEM Boost_REGEX
    )

rock_executable(lib_config_bin Main.cpp
    DEPS lib_config)

