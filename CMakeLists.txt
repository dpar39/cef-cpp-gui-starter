cmake_minimum_required(VERSION 3.12.0)

message (STATUS "Configuring with cmake ${CMAKE_VERSION}")

set(CEF_TARGET cef-gui)
project(${CEF_TARGET} CXX)

set_property(GLOBAL PROPERTY OS_FOLDERS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/thirdparty")

if (MSVC)
  foreach(FLAG_VAR CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
    if(${FLAG_VAR} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${FLAG_VAR} "${${FLAG_VAR}}")
    endif()
  endforeach()
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /bigobj")
  add_compile_options(/w44101)
  add_definitions(-D_WIN32_WINNT=0x0601)
endif()

# Add this project's cmake/ directory to the module path.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#
# Boost configuration
#
set(BOOST_VERSION 1.76.0)
add_definitions(-DBOOST_DATE_TIME_NO_LIB)
include(DownloadBoost)
DownloadBoost(${BOOST_VERSION} "${CMAKE_SOURCE_DIR}/thirdparty/boost") 
find_package(Boost REQUIRED)
include_directories(${BOOST_ROOT})


set(CPP_SRC_DIR "${CMAKE_SOURCE_DIR}/src")

#
# Protobuf
#
include(ExternalProtobuf)
ResolveProtobuf("3.17.3" ${THIRD_PARTY_DIR})
set(Protobuf_USE_STATIC_LIBS ON)

string(TOLOWER ${CMAKE_BUILD_TYPE} BUILD_CONFIG_LOWER)
string(TOLOWER ${CMAKE_SYSTEM_NAME} SYSTEM_NAME_LOWER)
set(BUILD_NAME_SUFFIX ${SYSTEM_NAME_LOWER}_${BUILD_CONFIG_LOWER})
set(THIRD_PARTY_INSTALL_DIR ${THIRD_PARTY_DIR}/install_${BUILD_NAME_SUFFIX})

list(APPEND CMAKE_PREFIX_PATH "${THIRD_PARTY_INSTALL_DIR}")
find_package(Protobuf REQUIRED)

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()

# generate c++ bindings
set(PROTOS_GEN_DIR "${CPP_SRC_DIR}/outpb")
execute_process(COMMAND ${Protobuf_PROTOC_EXECUTABLE} 
  "-I=${CMAKE_CURRENT_SOURCE_DIR}/proto"
  "--cpp_out=${PROTOS_GEN_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto"
  RESULT_VARIABLE ret
)
if(NOT ret EQUAL "0")
    message(FATAL_ERROR "Failed to generate protobuf bindings")
endif()

#
# Make a HTTP/Websocket server library/application
#
set(HTTP_WS_LIB_NAME libserver)
set(SERVER_SRC_DIR "${CPP_SRC_DIR}/server")
include_directories("${SERVER_SRC_DIR}")
include_directories("${PROTOS_GEN_DIR}")
include_directories("${PROTOBUF_INCLUDE_DIR}")
# server lib
file(GLOB SERVER_LIB_CC_FILES "${SERVER_SRC_DIR}/*.cc")
file(GLOB PROTOS_LIB_CC_FILES "${PROTOS_GEN_DIR}/*.cc")
add_library(${HTTP_WS_LIB_NAME} ${SERVER_LIB_CC_FILES} ${PROTOS_LIB_CC_FILES})
target_link_libraries(${HTTP_WS_LIB_NAME} ${PROTOBUF_LIBRARY})
# sever app
set(SERVE_EXE_NAME server)
add_executable(${SERVE_EXE_NAME} "${CPP_SRC_DIR}/server/main.cpp")
target_link_libraries(${SERVE_EXE_NAME} ${HTTP_WS_LIB_NAME})
# server tests
include(ExternalGTest)
ResolveGTest("1.11.0" ${THIRD_PARTY_DIR})
find_package(GTest REQUIRED)
set(TESTS_EXE_NAME server_tests)
file(GLOB TEST_CC_FILES "${CPP_SRC_DIR}/test/*.cc")
add_executable(${TESTS_EXE_NAME} ${TEST_CC_FILES})
target_link_libraries(${TESTS_EXE_NAME} GTest::gtest GTest::gtest_main ${HTTP_WS_LIB_NAME})



