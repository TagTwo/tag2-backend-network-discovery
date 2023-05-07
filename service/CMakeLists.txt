cmake_minimum_required(VERSION 3.22.1)

project(tagtwo_network_discovery LANGUAGES CXX)

##########################################
##
## Title: PMM Setup
## Description: Include and configure PMM
##
##########################################
include(cmake/pmm.cmake)
include(cmake/CPM.cmake)
include(cmake/util.cmake)

if(WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "" FORCE)
elseif (UNIX)
    set(VCPKG_TARGET_TRIPLET x64-linux)
endif()

# # THIS MUST BE BEFORE PMM (else it will use python from vcpkg)
find_package(pybind11 CONFIG REQUIRED)

set(vcpkg_dependencies)
check_package("spdlog" spdlog vcpkg_dependencies)
check_package("amqpcpp" amqpcpp vcpkg_dependencies)
check_package("libevent" Libevent vcpkg_dependencies)
check_package("nlohmann-json" nlohmann_json vcpkg_dependencies)
check_package("nng" nng vcpkg_dependencies)
check_package("nngpp" nngpp vcpkg_dependencies)

pmm(
        VERBOSE DEBUG VCPKG REVISION 2022.10.19
        REQUIRES
        spdlog
        amqpcpp
        libevent
        nlohmann-json
        nng  # NNGClient
        nngpp  # NNGClient
        TRIPLET
        ${VCPKG_TARGET_TRIPLET}
)

#############################################################################################
#####
##### CPM - Install Dependencies
#####
#############################################################################################
CPMAddPackage(
        NAME pybind11_json  # has target: pybind11_json
        GIT_TAG master
        GIT_REPOSITORY https://github.com/pybind/pybind11_json.git
        OPTIONS
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # optional, ensure standard is supported
set(CMAKE_CXX_EXTENSIONS ON)  # optional, keep compiler extensions off
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -ffast-math -funroll-loops -march=native -fomit-frame-pointer -flto")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math -funroll-loops -march=native -fomit-frame-pointer -flto")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -ffast-math -funroll-loops -march=native -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ffast-math -funroll-loops -march=native -fomit-frame-pointer")


##########################################
##
## Title: Dependencies
## Description: Find and include required libraries
##
##########################################
find_package(Threads REQUIRED)
find_package(amqpcpp CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Libevent CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(nng CONFIG REQUIRED)
find_package(nngpp CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)




# Add the source files
set(SOURCE_FILES
        src/ServiceDiscoveryClient.cpp
        src/ServiceDiscoveryRecord.cpp
        src/util/JsonBuilder.cpp
        src/util/DotEnv.cpp
)
set(LINK_TARGETS
        amqpcpp
        spdlog::spdlog
        libevent::core
        pybind11_json
        nlohmann_json::nlohmann_json
        nng::nng
        nng::nngpp
        OpenSSL::SSL
)


add_compile_definitions(TAGTWO_NETWORKING_VERSION=1) #  SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})
target_compile_definitions(${PROJECT_NAME}
        PUBLIC
        TAGTWO_NETWORKING_VERSION=1
)
target_include_directories(${PROJECT_NAME}
        PUBLIC include
)
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        ${LINK_TARGETS}
)
##########################################
##
## Title: Executable
## Description: Define the executable if not a Python build
##
##########################################
if(NOT DEFINED IS_PYTHON_BUILD)
    add_executable(${PROJECT_NAME}-bin src/main.cpp ${SOURCE_FILES})

    target_include_directories(${PROJECT_NAME}-bin
            PRIVATE
            include
    )

    target_link_libraries(${PROJECT_NAME}-bin PUBLIC
            ${LINK_TARGETS}
    )


endif()



##########################################
##
## Title: Python bindings
## Description: Create Python bindings with pybind11
##
##########################################
if(DEFINED IS_PYTHON_BUILD)
    set(SOURCE_FILES_PYBIND
            src/service_discovery_client_python_bindings.cpp
    )

    pybind11_add_module(tagtwo_network_discovery_python ${SOURCE_FILES_PYBIND})

    target_link_libraries(tagtwo_network_discovery_python
            PRIVATE
            ${PROJECT_NAME}
            ${LINK_TARGETS}
            -static-libgcc
            -static-libstdc++

    )

    if (WIN32)
        set_target_properties(tagtwo_network_discovery_python PROPERTIES SUFFIX ".pyd")
    elseif (APPLE)
        set_target_properties(tagtwo_network_discovery_python PROPERTIES SUFFIX ".dylib")
    else()
        set_target_properties(tagtwo_network_discovery_python PROPERTIES SUFFIX ".so")
    endif()
endif ()
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
    add_dependencies(${PROJECT_NAME}-bin copy_env_file)
endif ()
