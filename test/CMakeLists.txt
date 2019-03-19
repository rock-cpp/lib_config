INCLUDE_DIRECTORIES(../src)
rock_testsuite(test_suite suite.cpp 
                          lib_config.cpp
               DEPS lib_config)


