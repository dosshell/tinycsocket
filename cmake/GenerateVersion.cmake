# Generates TCS_VERSION_TXT from git tag + commit count.
# Format: {tag}.{count}

find_package(Git QUIET)
if(NOT GIT_FOUND)
    message(WARNING "Git not found, skipping version generation")
    return()
endif()

# Get latest tag
execute_process(
    COMMAND "${GIT_EXECUTABLE}" describe --tags --abbrev=0
    WORKING_DIRECTORY "${SRC_DIR}"
    RESULT_VARIABLE res
    OUTPUT_VARIABLE tag
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT res EQUAL 0)
    message(WARNING "No git tags found, skipping version generation")
    return()
endif()

# Count commits since tag
execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-list --count "${tag}..HEAD"
    WORKING_DIRECTORY "${SRC_DIR}"
    RESULT_VARIABLE res
    OUTPUT_VARIABLE count
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT res EQUAL 0)
    set(count "0")
endif()

set(version "${tag}.${count}")

# Update header
set(HEADER_FILE "${SRC_DIR}/src/tinycsocket_internal.h")
file(READ "${HEADER_FILE}" content)

string(REGEX REPLACE
    "(static const char\\* const TCS_VERSION_TXT = \")[^\"]*(\")"
    "\\1${version}\\2"
    new_content "${content}")

if(NOT "${content}" STREQUAL "${new_content}")
    file(WRITE "${HEADER_FILE}" "${new_content}")
    message(STATUS "Updated TCS_VERSION_TXT to \"${version}\"")
else()
    message(STATUS "TCS_VERSION_TXT already \"${version}\"")
endif()
