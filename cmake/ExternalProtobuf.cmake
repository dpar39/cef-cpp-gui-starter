function(ResolveProtobuf PKG_VERSION THIRD_PARTY_DIR)

set(PKG_NAME Protobuf)
string(TOLOWER ${CMAKE_BUILD_TYPE} BUILD_CONFIG_LOWER)
string(TOLOWER ${CMAKE_SYSTEM_NAME} SYSTEM_NAME_LOWER)
set(BUILD_NAME_SUFFIX ${SYSTEM_NAME_LOWER}_${BUILD_CONFIG_LOWER})
set(THIRD_PARTY_INSTALL_DIR ${THIRD_PARTY_DIR}/install_${BUILD_NAME_SUFFIX})

set(DOWNLOAD_URL "https://github.com/protocolbuffers/protobuf/releases/download/v${PKG_VERSION}/protobuf-all-${PKG_VERSION}.tar.gz")
set(DOWNLOAD_ARCHIVE "${THIRD_PARTY_DIR}/protobuf-all-${PKG_VERSION}.tar.gz")
set(EXTRACT_DIR "${THIRD_PARTY_DIR}/protobuf-${PKG_VERSION}")
set(SOURCE_DIR ${EXTRACT_DIR}/cmake)

set(THIRD_PARTY_BUILD_DIR ${EXTRACT_DIR}/build_${BUILD_NAME_SUFFIX})

# Download archive if it doesn't exist
if(NOT EXISTS "${DOWNLOAD_ARCHIVE}")
  message(STATUS "Downloading ${DOWNLOAD_URL} to ${DOWNLOAD_ARCHIVE}...")
  file(DOWNLOAD "${DOWNLOAD_URL}" "${DOWNLOAD_ARCHIVE}" SHOW_PROGRESS)
endif()

# Extract archive
if(NOT EXISTS "${EXTRACT_DIR}")
  message(STATUS "Extracting ${DOWNLOAD_ARCHIVE}...")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xzf "${DOWNLOAD_ARCHIVE}"
    WORKING_DIRECTORY ${THIRD_PARTY_DIR}
  )
endif()

if(NOT EXISTS "${THIRD_PARTY_INSTALL_DIR}/cmake/protobuf-config.cmake")
  # Build archive
  execute_process(
    COMMAND ${CMAKE_COMMAND} ${SOURCE_DIR} 
    -G ${CMAKE_GENERATOR}
    -B${THIRD_PARTY_BUILD_DIR}
    -DCMAKE_INSTALL_PREFIX=${THIRD_PARTY_INSTALL_DIR}
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    -Dprotobuf_BUILD_TESTS:BOOL=OFF
    -Dprotobuf_BUILD_EXAMPLES:BOOL=OFF
    -Dprotobuf_WITH_ZLIB:BOOL=OFF
  )
  execute_process(COMMAND ${CMAKE_COMMAND} --build ${THIRD_PARTY_BUILD_DIR})
  execute_process(COMMAND ${CMAKE_COMMAND} --install ${THIRD_PARTY_BUILD_DIR})
endif()

set(Protobuf_DIR ${THIRD_PARTY_INSTALL_DIR})
find_package(${PKG_NAME} REQUIRED HINTS ${THIRD_PARTY_INSTALL_DIR}/cmake)

unset(PKG_NAME)
unset(BUILD_CONFIG_LOWER)
unset(SYSTEM_NAME_LOWER)
unset(BUILD_NAME_SUFFIX)
unset(THIRD_PARTY_BUILD_DIR)
unset(SOURCE_DIR)
unset(EXTRACT_DIR)
unset(DOWNLOAD_ARCHIVE)
unset(DOWNLOAD_URL)

endfunction()

