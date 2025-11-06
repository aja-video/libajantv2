include_guard(GLOBAL)

include (Helpers)

set(AJA_COMPANY_NAME "AJA Video Systems, Inc.")
set(AJA_WEBSITE "https://www.aja.com/")

string(TIMESTAMP DATETIME_NOW UTC)
# break appart the ISO-8601 UTC datetime
# have to do this way because cmake does not support gettting
# individual components when using UTC
string(SUBSTRING ${DATETIME_NOW} 0 10 AJA_BUILD_DATE)
string(SUBSTRING ${DATETIME_NOW} 11 8 AJA_BUILD_TIME)
string(SUBSTRING ${DATETIME_NOW} 19 -1 AJA_BUILD_TIMEZONE)
string(REGEX MATCHALL "([0-9]+)"
	AJA_BUILD_DATE_PARTS ${AJA_BUILD_DATE})
list(GET AJA_BUILD_DATE_PARTS 0 AJA_BUILD_YEAR)
list(GET AJA_BUILD_DATE_PARTS 1 AJA_BUILD_MONTH)
list(GET AJA_BUILD_DATE_PARTS 2 AJA_BUILD_DAY)

if (NOT NTV2_VERSION_STRING)
    set(NTV2_VERSION_STRING 1.2.3)
    set(AJA_NTV2_SDK_BUILD_LETTER "d") # ""=release, "a"=alpha, "b"=beta, "d"=development
endif()

# Read libajantv2 version number from VERSION file
if (EXISTS ${AJANTV2_VERSION_FILENAME})
    message(STATUS "Found libajantv2 version file: ${AJANTV2_VERSION_FILENAME}")
    file(READ ${AJANTV2_VERSION_FILENAME} NTV2_VERSION_STRING)
    string(STRIP "${NTV2_VERSION_STRING}" NTV2_VERSION_STRING)
    string(REPLACE "\n" "" NTV2_VERSION_STRING "${NTV2_VERSION_STRING}")
    string(REPLACE "\r" "" NTV2_VERSION_STRING "${NTV2_VERSION_STRING}")
    message(STATUS "File version string: ${NTV2_VERSION_STRING}")
endif()

if (DEFINED ENV{NTV2_VERSION})
    message(STATUS "NTV2 version override from env: $ENV{NTV2_VERSION}")
    set(NTV2_VERSION_STRING $ENV{NTV2_VERSION})
endif()
if (NTV2_VERSION)
    message(STATUS "NTV2 version override: ${NTV2_VERSION}")
    set(NTV2_VERSION_STRING ${NTV2_VERSION})
endif()

# NTV2 SDK version number variables. Generates an include file in `ajantv2/includes/ntv2version.h`,
# which is used throughout the SDK via `ajantv2/includes/ntv2enums.h`.
# Override the following variables to set an arbitrary NTV2 SDK version number.
if (NTV2_VERSION_MAJOR)
    message(STATUS "NTV2 major version override: ${NTV2_VERSION_MAJOR}")
    set(AJA_NTV2_SDK_VERSION_MAJOR ${NTV2_VERSION_MAJOR})
else()
    string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\1"
        AJA_NTV2_SDK_VERSION_MAJOR ${NTV2_VERSION_STRING})
endif()
if (NTV2_VERSION_MINOR)
    message(STATUS "NTV2 minor version override: ${NTV2_VERSION_MINOR}")
    set(AJA_NTV2_SDK_VERSION_MINOR ${NTV2_VERSION_MINOR})
else()
    string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\2"
        AJA_NTV2_SDK_VERSION_MINOR ${NTV2_VERSION_STRING})
endif()
if (NTV2_VERSION_POINT)
    message(STATUS "NTV2 point version override: ${NTV2_VERSION_POINT}")
    set(AJA_NTV2_SDK_VERSION_POINT ${NTV2_VERSION_POINT})
else()
    string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+)" "\\3"
        AJA_NTV2_SDK_VERSION_POINT ${NTV2_VERSION_STRING})
endif()
if(DEFINED ENV{NTV2_BUILD_LETTER})
    message(STATUS "NTV2 build letter override from env: $ENV{NTV2_BUILD_LETTER}")
    set(AJA_NTV2_SDK_BUILD_LETTER $ENV{NTV2_BUILD_LETTER})
elseif (DEFINED ENV{BUILD_LETTER})
    message(STATUS "NTV2 build letter override from TeamCity env: $ENV{BUILD_LETTER}")
    set(AJA_NTV2_SDK_BUILD_LETTER $ENV{BUILD_LETTER})
endif()
if (DEFINED ENV{NTV2_VERSION_BUILD})
    message(STATUS "NTV2 build number override from env: $ENV{NTV2_VERSION_BUILD}")
    set(AJA_NTV2_SDK_BUILD_NUMBER $ENV{NTV2_VERSION_BUILD})
elseif (DEFINED ENV{BUILD_NUMBER})
    message(STATUS "NTV2 build number override from TeamCity env: $ENV{BUILD_NUMBER}")
    set(AJA_NTV2_SDK_BUILD_NUMBER $ENV{BUILD_NUMBER})
endif()
if (NTV2_VERSION_BUILD)
    message(STATUS "NTV2 build number override: ${NTV2_VERSION_BUILD}")
    set(AJA_NTV2_SDK_BUILD_NUMBER ${NTV2_VERSION_BUILD})
endif()
if (NOT AJA_NTV2_SDK_BUILD_NUMBER)
    message(STATUS "NTV2 build number not specified. Defaulting to 0.")
    set(AJA_NTV2_SDK_BUILD_NUMBER "0")
endif()

# Try to get git commit hash from HEAD
aja_get_git_hash(AJA_GIT_COMMIT_HASH AJA_GIT_COMMIT_HASH_SHORT)
if (AJA_GIT_COMMIT_HASH)
    string(STRIP ${AJA_GIT_COMMIT_HASH} AJA_GIT_COMMIT_HASH)
else()
    message(STATUS "AJA_GIT_COMMIT_HASH not set!")
endif()
if (AJA_GIT_COMMIT_HASH_SHORT)
    string(STRIP ${AJA_GIT_COMMIT_HASH_SHORT} AJA_GIT_COMMIT_HASH_SHORT)
else()
    message(STATUS "AJA_GIT_COMMIT_HASH_SHORT not set!")
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/.git/HEAD")
    # Use configure_file to force a re-configure if the HEAD file changes. That means changing branch.
    # The re-configure will pick up a new git commit hash above, if it exists and is valid.
    # This will be slightly redundant if BUILD_VERSION_HASH is specified but that is not a case we expect to care about.
    configure_file("${CMAKE_SOURCE_DIR}/.git/HEAD" "${CMAKE_BINARY_DIR}/git_HEAD" COPYONLY)

    # in addition, if HEAD is a ref, do the same on the file it's pointing to (since HEAD won't change for commits to the current branch)
    # if we change branch then this will correspondingly
    file(READ ${CMAKE_SOURCE_DIR}/.git/HEAD HEAD_CONTENTS)
    string(STRIP "${HEAD_CONTENTS}" HEAD_CONTENTS)

    if("${HEAD_CONTENTS}" MATCHES "ref: ")
        string(REPLACE "ref: " "" REF_LOCATION "${HEAD_CONTENTS}")

        if(EXISTS "${CMAKE_SOURCE_DIR}/.git/${REF_LOCATION}")
            configure_file("${CMAKE_SOURCE_DIR}/.git/${REF_LOCATION}" "${CMAKE_BINARY_DIR}/git_ref" COPYONLY)
        endif()
    endif()
endif()

set(AJA_NTV2_SDK_BUILD_DATETIME ${DATETIME_NOW})
string(CONCAT AJA_NTV2_VER_STR
    "${AJA_NTV2_SDK_VERSION_MAJOR}"
    ".${AJA_NTV2_SDK_VERSION_MINOR}"
    ".${AJA_NTV2_SDK_VERSION_POINT}"
    ".${AJA_NTV2_SDK_BUILD_NUMBER}")
string(CONCAT AJA_NTV2_VER_SHORT_STR
    "${AJA_NTV2_SDK_VERSION_MAJOR}"
    ".${AJA_NTV2_SDK_VERSION_MINOR}"
    ".${AJA_NTV2_SDK_VERSION_POINT}")

string(REPLACE "." "," AJA_NTV2_VER_STR_COMMA
    "${AJA_NTV2_VER_STR}")

string(CONCAT AJA_NTV2_VER_STR_WITH_LETTER
    "${AJA_NTV2_SDK_VERSION_MAJOR}"
    ".${AJA_NTV2_SDK_VERSION_MINOR}"
    ".${AJA_NTV2_SDK_VERSION_POINT}${AJA_NTV2_SDK_BUILD_LETTER}${AJA_NTV2_SDK_BUILD_NUMBER}")

if (AJA_NTV2_SDK_BUILD_LETTER STREQUAL "a")
    set(AJA_NTV2_SDK_BUILD_TYPE ${AJA_NTV2_SDK_BUILD_LETTER})
    set(AJA_NTV2_SDK_BUILD_TYPE_LONG "alpha")
elseif (AJA_NTV2_SDK_BUILD_LETTER STREQUAL "b")
    set(AJA_NTV2_SDK_BUILD_TYPE_LONG "beta")
    set(AJA_NTV2_SDK_BUILD_TYPE ${AJA_NTV2_SDK_BUILD_LETTER})
else()
    set(AJA_NTV2_SDK_BUILD_TYPE_LONG "")
endif()
