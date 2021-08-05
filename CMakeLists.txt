cmake_minimum_required(VERSION 3.12.0)

set(CEF_TARGET cef-gui)
project(${CEF_TARGET} CXX)

set_property(GLOBAL PROPERTY OS_FOLDERS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/thirdparty")

# Add this project's cmake/ directory to the module path.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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

# Display configuration settings.
PRINT_CEF_CONFIG()


#
# Boost configuration
#
set(BOOST_VERSION 1.76.0)
add_definitions(-DBOOST_DATE_TIME_NO_LIB)
include(DownloadBoost)
DownloadBoost(${BOOST_VERSION} "${CMAKE_SOURCE_DIR}/thirdparty/boost") 
find_package(Boost REQUIRED)
include_directories(${BOOST_ROOT})

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /bigobj")
endif()

#
# Protobuf
#
include(ExternalProtobuf)
ResolveProtobuf("3.17.3" ${THIRD_PARTY_DIR})


#
# Make a HTTP/Webasocket server library
#
set(HTTP_WS_LIB_NAME server)
set(SERVER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/server/")
file(GLOB SERVER_LIB_CC_FILES "${SERVER_SRC_DIR}/*.cc")
include_directories("${SERVER_SRC_DIR}")
add_library(${HTTP_WS_LIB_NAME} ${SERVER_LIB_CC_FILES})
SET_LIBRARY_TARGET_PROPERTIES(${HTTP_WS_LIB_NAME})
target_link_libraries(${HTTP_WS_LIB_NAME} )
#
# Add tests for HTTP/Webasocket server library
#
# include(ExternalGTest)
# ResolveGTest("1.11.0" ${THIRD_PARTY_DIR})
# set(TESTS_EXE_NAME ${HTTP_WS_LIB_NAME}_tests)
# file(GLOB TEST_CC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/test/*.cc")
# add_executable(${TESTS_EXE_NAME} ${TEST_CC_FILES})
# target_link_libraries(${TESTS_EXE_NAME} GTest::gtest GTest::gtest_main ${HTTP_WS_LIB_NAME})


# Include the libcef_dll_wrapper target (executes libcef_dll/CMakeLists.txt).
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)



if(OS_MAC)
  set(CEF_HELPER_TARGET "cefsimple_Helper")
  set(CEF_HELPER_OUTPUT_NAME "cefsimple Helper")
else()
  # Logical target used to link the libcef library.
  ADD_LOGICAL_TARGET("libcef_lib" "${CEF_LIB_DEBUG}" "${CEF_LIB_RELEASE}")
endif()

SET_CEF_TARGET_OUT_DIR()

set(CLIENT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/client/")
include_directories(${CEF_INCLUDE_PATH})
include_directories(${CLIENT_SRC_DIR})
file(GLOB_RECURSE APP_CC_FILES "${CLIENT_SRC_DIR}/*.cc")

if(OS_WINDOWS)
  set(WIN_RC_FILE "${CLIENT_SRC_DIR}/cef-gui.rc")
  set_property(SOURCE ${WIN_RC_FILE} APPEND PROPERTY 
    OBJECT_DEPENDS "${CLIENT_SRC_DIR}/res/cefsimple.ico"
  )

  add_executable(${CEF_TARGET} WIN32 ${APP_CC_FILES} ${WIN_RC_FILE})
  add_dependencies(${CEF_TARGET} libcef_dll_wrapper ${HTTP_WS_LIB_NAME})
  SET_EXECUTABLE_TARGET_PROPERTIES(${CEF_TARGET})
  target_link_libraries(${CEF_TARGET} libcef_lib libcef_dll_wrapper ${CEF_STANDARD_LIBS} ${HTTP_WS_LIB_NAME})

  if(USE_SANDBOX)
    ADD_LOGICAL_TARGET("cef_sandbox_lib" "${CEF_SANDBOX_LIB_DEBUG}" "${CEF_SANDBOX_LIB_RELEASE}")
    target_link_libraries(${CEF_TARGET} cef_sandbox_lib ${CEF_SANDBOX_STANDARD_LIBS})
  endif()
  ADD_WINDOWS_MANIFEST("${CLIENT_SRC_DIR}" "${CEF_TARGET}" "exe")
endif()

COPY_FILES("${CEF_TARGET}" "${CEF_BINARY_FILES}" "${CEF_BINARY_DIR}" "${CEF_TARGET_OUT_DIR}")
COPY_FILES("${CEF_TARGET}" "${CEF_RESOURCE_FILES}" "${CEF_RESOURCE_DIR}" "${CEF_TARGET_OUT_DIR}")

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()

