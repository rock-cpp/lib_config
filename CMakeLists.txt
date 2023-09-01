# CMakeLists.txt has to be located in the project folder and cmake has to be
# executed from 'project/build' with 'cmake ../'.
cmake_minimum_required(VERSION 3.0)
find_package(Rock)
project(lib_config VERSION 0.1)
rock_init()
include_directories(src)
rock_standard_layout()

