cmake_minimum_required(VERSION 3.12.0)

set(APP_NAME cef-gui)
project(${APP_NAME} CXX)

set(CMAKE_CONFIGURATION_TYPES Release)

set_property(GLOBAL PROPERTY OS_FOLDERS ON)
set(CEF_USE_SANDBOX OFF)
#
# CEF configuration.
#
set(CEF_VERSION "92.0.21+ga9ec100+chromium-92.0.4515.107")
# Determine the platform.
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
  if("${PROJECT_ARCH}" STREQUAL "arm64")
    set(CEF_PLATFORM "macosarm64")
  else()
    set(CEF_PLATFORM "macosx64")
  endif()
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  if(CMAKE_SIZEOF_VOID_P MATCHES 8)
    set(CEF_PLATFORM "linux64")
  else()
    set(CEF_PLATFORM "linux32")
  endif()
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
  if(CMAKE_SIZEOF_VOID_P MATCHES 8)
    set(CEF_PLATFORM "windows64")
  else()
    set(CEF_PLATFORM "windows32")
  endif()
endif()

# Add this project's cmake/ directory to the module path.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Download and extract the CEF binary distribution (executes DownloadCEF.cmake).
include(DownloadCEF)
DownloadCEF("${CEF_PLATFORM}" "${CEF_VERSION}" "${CMAKE_SOURCE_DIR}/thirdparty/cef")

# Add the CEF binary distribution's cmake/ directory to the module path.
list(APPEND CMAKE_MODULE_PATH "${CEF_ROOT}/cmake")

# Load the CEF configuration (executes FindCEF.cmake).
find_package(CEF REQUIRED)

# Support specification of the Python executable path via the command-line.
if(DEFINED ENV{PYTHON_EXECUTABLE})
  file(TO_CMAKE_PATH "$ENV{PYTHON_EXECUTABLE}" PYTHON_EXECUTABLE)
endif()

if(NOT PYTHON_EXECUTABLE)
  unset(PYTHON_EXECUTABLE)

  # Find the python interpreter.
  find_package(PythonInterp)

  if(NOT ${PYTHONINTERP_FOUND})
    message(FATAL_ERROR "A Python installation is required. Set the "
                        "PYTHON_EXECUTABLE environment variable to explicitly "
                        "specify the Python executable path.")
  endif()
endif()

message(STATUS "Using Python: ${PYTHON_EXECUTABLE}")

# Include the libcef_dll_wrapper target (executes libcef_dll/CMakeLists.txt).
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)

# Allow includes relative to the current source directory.
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Display configuration settings.
PRINT_CEF_CONFIG()

if(${CMAKE_BUILD_TYPE} MATCHES debug)
  set(CEF_LIB ${CEF_LIB_DEBUG})
else()
  set(CEF_LIB ${CEF_LIB_RELEASE})
endif()


include_directories(${CEF_INCLUDE_PATH})
file(GLOB_RECURSE LIB_CC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")
add_executable(${APP_NAME} WIN32 ${LIB_CC_FILES})
target_compile_options(${APP_NAME} PUBLIC "/EHsc" "/MT")
target_link_libraries(${APP_NAME} ${CEF_LIB} ${CEF_STANDARD_LIBS} libcef_dll_wrapper)

add_custom_command(
    TARGET ${APP_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CEF_INCLUDE_PATH}/${CMAKE_BUILD_TYPE}"
            "${CMAKE_BINARY_DIR}"
    VERBATIM
)
add_custom_command(
    TARGET ${APP_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CEF_INCLUDE_PATH}/Resources"
            "${CMAKE_BINARY_DIR}"
    VERBATIM
)


include(DownloadBoost)
DownloadBoost(1.76.0 "${CMAKE_SOURCE_DIR}/thirdparty/boost") 

set(CMAKE_CXX_STANDARD 17)
add_definitions(-DBOOST_ASIO_STANDALONE)
add_definitions(-D_WIN32_WINNT=0x0601)

find_package(Boost REQUIRED)

set(SERVER server)
file(GLOB_RECURSE LIB_CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
include_directories(${BOOST_ROOT})

if(MSVC)
    add_definitions("/EHsc")
endif()
add_executable(${SERVER}  ${LIB_CPP_FILES})
target_link_libraries(${SERVER} ${Boost_LIBRARIES})

