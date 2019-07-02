INCLUDE_DIRECTORIES(../src)
rock_testsuite(test_suite suite.cpp 
                          bundle.cpp
                          yaml_configuration.cpp
               DEPS lib_config)


