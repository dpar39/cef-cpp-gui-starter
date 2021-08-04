cmake_minimum_required(VERSION 3.12.0)

set(CEF_TARGET cef-gui)
project(${CEF_TARGET} CXX)

set_property(GLOBAL PROPERTY OS_FOLDERS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add this project's cmake/ directory to the module path.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#
# Boost configuration.
#
set(BOOST_VERSION 1.76.0)
include(DownloadBoost)
DownloadBoost(${BOOST_VERSION} "${CMAKE_SOURCE_DIR}/thirdparty/boost") 
add_definitions(-DBOOST_ASIO_STANDALONE)
find_package(Boost REQUIRED)

#
# CEF configuration.
#
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


if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /bigobj")
endif()
# Include the libcef_dll_wrapper target (executes libcef_dll/CMakeLists.txt).
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)

# Display configuration settings.
PRINT_CEF_CONFIG()

if(OS_MAC)
  set(CEF_HELPER_TARGET "cefsimple_Helper")
  set(CEF_HELPER_OUTPUT_NAME "cefsimple Helper")
else()
  # Logical target used to link the libcef library.
  ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")
endif()

SET_CEF_TARGET_OUT_DIR()

include_directories(${BOOST_ROOT})
include_directories(${CEF_INCLUDE_PATH})
file(GLOB_RECURSE LIB_CC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc")

if(OS_WINDOWS)
  set(WIN_RC_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/cef-gui.rc")
  set_property(SOURCE ${WIN_RC_FILE} APPEND PROPERTY
        OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/res/cefsimple.ico
        )

  add_executable(${CEF_TARGET} WIN32 ${LIB_CC_FILES} ${WIN_RC_FILE})
  add_dependencies(${CEF_TARGET} libcef_dll_wrapper)
  SET_EXECUTABLE_TARGET_PROPERTIES(${CEF_TARGET})
  #target_link_libraries(${CEF_TARGET} ${CEF_LIB} ${CEF_STANDARD_LIBS} ${Boost_LIBRARIES} libcef_dll_wrapper)
  target_link_libraries(${CEF_TARGET} libcef_lib libcef_dll_wrapper ${CEF_STANDARD_LIBS})

  if(USE_SANDBOX)
    ADD_LOGICAL_TARGET("cef_sandbox_lib" "${CEF_SANDBOX_LIB_DEBUG}" "${CEF_SANDBOX_LIB_RELEASE}")
    target_link_libraries(${CEF_TARGET} cef_sandbox_lib ${CEF_SANDBOX_STANDARD_LIBS})
  endif()

  ADD_WINDOWS_MANIFEST("${CMAKE_CURRENT_SOURCE_DIR}/src" "${CEF_TARGET}" "exe")
endif()

COPY_FILES("${CEF_TARGET}" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
COPY_FILES("${CEF_TARGET}" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()

