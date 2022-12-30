set(AJA_COMPANY_NAME "AJA Video Systems, Inc.")
set(AJA_WEBSITE "https://www.aja.com/")

# NTV2 SDK version number variables. Generates an include file in `ajantv2/includes/ntv2version.h`,
# which is used throughout the SDK via `ajantv2/includes/ntv2enums.h`.
# Override the following variables to set an arbitrary NTV2 SDK version number.
string(TIMESTAMP AJA_BUILD_MONTH "%m")
string(TIMESTAMP AJA_BUILD_DAY "%d")
string(TIMESTAMP AJA_BUILD_YEAR "%Y")
string(TIMESTAMP DATETIME_NOW "%m/%d/%Y +8:%H:%M:%S")
if (NOT DEFINED AJA_NTV2_SDK_VERSION_MAJOR)
    set(AJA_NTV2_SDK_VERSION_MAJOR "16")
endif()
if (NOT DEFINED AJA_NTV2_SDK_VERSION_MINOR)
    set(AJA_NTV2_SDK_VERSION_MINOR "3")
endif()
if (NOT DEFINED AJA_NTV2_SDK_VERSION_POINT)
    set(AJA_NTV2_SDK_VERSION_POINT "1")
endif()

# Try to get git commit hash from HEAD
get_git_hash(AJA_GIT_COMMIT_HASH AJA_GIT_COMMIT_HASH_SHORT)
if (AJA_GIT_COMMIT_HASH)
    string(STRIP ${AJA_GIT_COMMIT_HASH} AJA_GIT_COMMIT_HASH)
else()
    aja_message(STATUS "AJA_GIT_COMMIT_HASH not set!")
endif()
if (AJA_GIT_COMMIT_HASH_SHORT)
    string(STRIP ${AJA_GIT_COMMIT_HASH_SHORT} AJA_GIT_COMMIT_HASH_SHORT)
else()
    aja_message(STATUS "AJA_GIT_COMMIT_HASH_SHORT not set!")
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
    ".${AJA_NTV2_SDK_VERSION_POINT}")
string(CONCAT AJA_NTV2_VER_STR_LONG
    "${AJA_NTV2_VER_STR}"
    ".${AJA_NTV2_SDK_BUILD_NUMBER}")
string(REPLACE "." "," AJA_NTV2_VER_STR_COMMA
    "${AJA_NTV2_VER_STR_LONG}")
set(AJA_NTV2_SDK_BUILD_TYPE "d") # ""=release, "a"=alpha, "b"=beta, "d"=development
if (AJA_NTV2_SDK_BUILD_TYPE STREQUAL "a")
    set(AJA_NTV2_SDK_BUILD_TYPE_LONG "alpha")
elseif (AJA_NTV2_SDK_BUILD_TYPE STREQUAL "b")
    set(AJA_NTV2_SDK_BUILD_TYPE_LONG "beta")
else()
    set(AJA_NTV2_SDK_BUILD_TYPE_LONG "")
endif()
