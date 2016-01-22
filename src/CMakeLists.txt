rock_library(lib_config
    SOURCES Dummy.cpp
    HEADERS Dummy.hpp)

rock_executable(lib_config_bin Main.cpp
    DEPS lib_config)

