

function(DownloadBoost BOOST_VERSION BOOST_DOWNLOAD_DIR)

string(REPLACE "." "_" VERSION_UNDERSCORE ${BOOST_VERSION})

set(BOOST_DIR_NAME "boost_${VERSION_UNDERSCORE}")
set(BOOST_ROOT "${BOOST_DOWNLOAD_DIR}/${BOOST_DIR_NAME}" CACHE INTERNAL "BOOST_ROOT")

# Download and/or extract the binary distribution if necessary.
if(NOT IS_DIRECTORY "${BOOST_ROOT}")
  set(BOOST_DOWNLOAD_FILENAME "${BOOST_DIR_NAME}.tar.bz2")
  set(BOOST_DOWNLOAD_PATH "${BOOST_DOWNLOAD_DIR}/${BOOST_DOWNLOAD_FILENAME}")
  if(NOT EXISTS "${BOOST_DOWNLOAD_PATH}")

    set(BOOST_DOWNLOAD_URL "https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/${BOOST_DOWNLOAD_FILENAME}")

    # Download the binary distribution and verify the hash.
    message(STATUS "Downloading ${BOOST_DOWNLOAD_URL} to ${BOOST_DOWNLOAD_PATH}...")
    file(DOWNLOAD "${BOOST_DOWNLOAD_URL}" "${BOOST_DOWNLOAD_PATH}" SHOW_PROGRESS)
  endif()

  # Extract the binary distribution.
  message(STATUS "Extracting ${BOOST_DOWNLOAD_PATH}...")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xzf "${BOOST_DOWNLOAD_DIR}/${BOOST_DOWNLOAD_FILENAME}"
    WORKING_DIRECTORY ${BOOST_DOWNLOAD_DIR}
    )
endif()
endfunction()