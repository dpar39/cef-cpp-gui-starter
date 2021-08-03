cmake_minimum_required(VERSION 3.12.0)

set(APP_NAME cef-gui)
project(${APP_NAME} CXX)

set_property(GLOBAL PROPERTY OS_FOLDERS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_definitions(-DBOOST_ASIO_STANDALONE)
if(MSVC)
    add_definitions("/EHsc /bigobj")
endif()

# Add this project's cmake/ directory to the module path.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#
# Boost configuration.
#
set(BOOST_VERSION 1.76.0)
include(DownloadBoost)
DownloadBoost(${BOOST_VERSION} "${CMAKE_SOURCE_DIR}/thirdparty/boost") 
find_package(Boost REQUIRED)

#
# CEF configuration.
#
set(CEF_RUNTIME_LIBRARY_FLAG "/MT")
set(CEF_VERSION "92.0.21+ga9ec100+chromium-92.0.4515.107")
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
include(DownloadCEF)
DownloadCEF("${CEF_PLATFORM}" "${CEF_VERSION}" "${CMAKE_SOURCE_DIR}/thirdparty/cef")
list(APPEND CMAKE_MODULE_PATH "${CEF_ROOT}/cmake")
find_package(CEF REQUIRED)

# Support specification of the Python executable path via the command-line.
if(DEFINED ENV{PYTHON_EXECUTABLE})
  file(TO_CMAKE_PATH "$ENV{PYTHON_EXECUTABLE}" PYTHON_EXECUTABLE)
endif()

if(NOT PYTHON_EXECUTABLE)
  unset(PYTHON_EXECUTABLE)
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


# Display configuration settings.
PRINT_CEF_CONFIG()

string( TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_LOWER)
if(${BUILD_TYPE_LOWER} MATCHES release OR ${BUILD_TYPE_LOWER} MATCHES relwithdebuginfo)
  set(CEF_DLL_DIR "${CEF_INCLUDE_PATH}/Release")
  set(CEF_LIB ${CEF_LIB_RELEASE})
else()
  set(CEF_DLL_DIR "${CEF_INCLUDE_PATH}/Debug")
  set(CEF_LIB ${CEF_LIB_DEBUG})
  endif() 
message(STATUS CEF_LIB=${CEF_LIB})

include_directories(${BOOST_ROOT})
include_directories(${CEF_INCLUDE_PATH})
file(GLOB_RECURSE LIB_CC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")
add_executable(${APP_NAME} WIN32 ${LIB_CC_FILES})
SET_LIBRARY_TARGET_PROPERTIES(${APP_NAME})
target_link_libraries(${APP_NAME} ${CEF_LIB} ${CEF_STANDARD_LIBS} ${Boost_LIBRARIES} libcef_dll_wrapper)

add_custom_command(
    TARGET ${APP_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CEF_DLL_DIR}" "${CMAKE_BINARY_DIR}" VERBATIM
)
add_custom_command(
    TARGET ${APP_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CEF_INCLUDE_PATH}/Resources"
            "${CMAKE_BINARY_DIR}"
    VERBATIM
)

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()


# set(SERVER server)
# file(GLOB_RECURSE LIB_CPP_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
# include_directories(${BOOST_ROOT})


# add_executable(${SERVER}  ${LIB_CPP_FILES})
# target_link_libraries(${SERVER} ${Boost_LIBRARIES})

