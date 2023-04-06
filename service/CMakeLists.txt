cmake_minimum_required(VERSION 3.20)

project(tagtwo-network-discovery LANGUAGES CXX)

##########################################
##
## Title: PMM Setup
## Description: Include and configure PMM
##
##########################################
include(cmake/pmm.cmake)

set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "" FORCE)
elseif (UNIX)
    set(VCPKG_TARGET_TRIPLET x64-linux)
endif()

pmm(
        VERBOSE DEBUG VCPKG REVISION 2022.10.19
        REQUIRES
        spdlog
        amqpcpp
        libevent
        nlohmann-json
        pybind11
        TRIPLET
        ${VCPKG_TARGET_TRIPLET}
)

##########################################
##
## Title: Dependencies
## Description: Find and include required libraries
##
##########################################
find_package(Boost COMPONENTS system REQUIRED)
find_package(Threads REQUIRED)
find_package(amqpcpp CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Libevent CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(pybind11 CONFIG REQUIRED)

# Add the source files
set(SOURCE_FILES src/NetworkService.cpp  src/util/JsonBuilder.cpp src/util/DotEnv.cpp)
set(LINK_TARGETS
        amqpcpp
        spdlog::spdlog
        libevent::core
        nlohmann_json::nlohmann_json
        )

##########################################
##
## Title: Executable
## Description: Define the executable if not a Python build
##
##########################################
if(NOT DEFINED IS_PYTHON_BUILD)
    add_executable(${PROJECT_NAME} src/main.cpp ${SOURCE_FILES})

    target_include_directories(${PROJECT_NAME}
            PRIVATE
            include
            )

    target_link_libraries(${PROJECT_NAME} PUBLIC
            ${LINK_TARGETS}
            )
endif()

##########################################
##
## Title: Python bindings
## Description: Create Python bindings with pybind11
##
##########################################
set(SOURCE_FILES_PYBIND
        ${SOURCE_FILES}
        src/network_service_python_bindings.cpp
)

pybind11_add_module(tagtwo_network_discovery_python SHARED ${SOURCE_FILES_PYBIND})
target_include_directories(tagtwo_network_discovery_python
        PRIVATE
        include
        )

target_link_libraries(tagtwo_network_discovery_python
        PRIVATE
        ${LINK_TARGETS}
        pybind11::module
        )

if (WIN32)
    set_target_properties(tagtwo_network_discovery_python PROPERTIES OUTPUT_NAME "tagtwo_network_discovery_python.pyd")
elseif (APPLE)
    set_target_properties(tagtwo_network_discovery_python PROPERTIES OUTPUT_NAME "tagtwo_network_discovery_python.dylib")
else()
    set_target_properties(tagtwo_network_discovery_python PROPERTIES OUTPUT_NAME "tagtwo_network_discovery_python.so")
endif()

##########################################
##
## Title: Copy .env file
## Description: Copy .env file from source to build directory
##
##########################################
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
cmake_path(GET SOURCE_DIR PARENT_PATH PARENT_DIR)

set(ENV_INPUT_FILE "${PARENT_DIR}/.env")
set(ENV_OUTPUT_FILE "${BINARY_DIR}/.env")


add_custom_command(
        OUTPUT ${ENV_OUTPUT_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ENV_INPUT_FILE} ${ENV_OUTPUT_FILE}
        DEPENDS ${ENV_INPUT_FILE}
        COMMENT "Copying .env file to build directory"
)
if(NOT DEFINED IS_PYTHON_BUILD)
    add_custom_target(copy_env_file ALL DEPENDS ${ENV_OUTPUT_FILE})
    add_dependencies(${PROJECT_NAME} copy_env_file)
endif ()
