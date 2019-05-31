find_package(Boost COMPONENTS python REQUIRED)
execute_process ( COMMAND python -c "import sys; version=sys.version_info[:3]; print('{0}.{1}'.format(*version))" OUTPUT_VARIABLE PYTHON_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
set(PYTHON_INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/lib/python${PYTHON_VERSION}/site-packages)
message("Installing Python packages to: ${PYTHION_INSTALL_PATH}")
message("Include dirs of boost: " ${Boost_INCLUDE_DIRS} )
message("Libs of boost: " ${Boost_LIBRARIES} )

link_directories(
    ${PYTHON_LIBRARY_DIRS}
)
include_directories(${PYTHON_INCLUDE_DIRS})

add_library(bundle SHARED bundle.cpp)
set_target_properties(bundle PROPERTIES PREFIX "")
target_link_libraries(bundle
    ${Boost_LIBRARIES}
    lib_config
    ${PYTHON_LIBRARIES}
)

add_custom_target(rockpy
    COMMAND if(NOT EXISTS ${PYTHON_INSTALL_PATH}/rock/__init__.py)
        file(WRITE ${PYTHON_INSTALL_PATH}/rock/__init__.py)
    endif()
)

install(TARGETS bundle
        DESTINATION ${PYTHON_INSTALL_PATH}/rock)
install(FILES __init__.py
        DESTINATION ${PYTHON_INSTALL_PATH}/rock)

