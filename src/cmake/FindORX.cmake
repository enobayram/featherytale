if(NOT ORX_DIR)
    if(DEFINED ENV{ORX_DIR})
        set(ORX_DIR $ENV{ORX_DIR})
    else()
        MESSAGE(FATAL_ERROR "The \"ORX_DIR\" variable was not set, "
                "either provide it as an environment variable or pass "
                "it to the cmake command line as -DORX_DIR=... ")
    endif()
endif()

if(IS_DIRECTORY "${ORX_DIR}/code")
    set(ORX_INCLUDE_DIR "${ORX_DIR}/code/include")
    set(ORX_LIBRARY_DIR "${ORX_DIR}/code/lib/dynamic")
else()
    if(NOT ORX_SUBDIR_NAME)
        if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
            if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
                set(ORX_SUBDIR_NAME dev-linux64)
            endif()
        elseif(${CMAKE_SYSTEM_NAME} STREQUAL Windows)
            if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
                set(ORX_SUBDIR_NAME dev-vs2013-64)
            else()
                set(ORX_SUBDIR_NAME dev-vs2013-32)
            endif()
        endif()
    endif()

    if(NOT ORX_SUBDIR_NAME)
        MESSAGE(FATAL_ERROR "FindOrx script does not recognize the current "
                "target platform, unable to generate ORX_SUBDIR_NAME")
    endif()

    if(IS_DIRECTORY "${ORX_DIR}/${ORX_SUBDIR_NAME}")
    else()
        MESSAGE(FATAL_ERROR "The Orx development package for the current "
                "target(${ORX_SUBDIR_NAME}) was not found! I was looking "
                " for the folder ${ORX_DIR}/${ORX_SUBDIR_NAME}")
    endif()
    set(ORX_INCLUDE_DIR "${ORX_DIR}/${ORX_SUBDIR_NAME}/include")
    set(ORX_LIBRARY_DIR "${ORX_DIR}/${ORX_SUBDIR_NAME}/lib")
endif()

find_library(ORX_LIBRARY_DEBUG   NAMES orxd PATHS ${ORX_LIBRARY_DIR})
find_library(ORX_LIBRARY_RELEASE NAMES orx  PATHS ${ORX_LIBRARY_DIR})
if(ORX_LIBRARY_DEBUG AND ORX_LIBRARY_RELEASE)
    set(ORX_FOUND true)
else()
    MESSAGE(FATAL_ERROR "The orx binaries were not found, I was looking "
            "for the embedded dynamic debug/release binaries, if you're "
            "compiling from source, make sure those are compiled")
endif()

set(ORX_LIBRARY debug     ${ORX_LIBRARY_DEBUG}
                optimized ${ORX_LIBRARY_RELEASE})

set(ORX_DLLS "${ORX_LIBRARY_DIR}/orx.dll" "${ORX_LIBRARY_DIR}/orxd.dll")

set(_ORX_SCROLL_EXPECTED_DIR "${ORX_DIR}/scroll")
if(";${ORX_FIND_COMPONENTS};" MATCHES ";Scroll;")
    if(IS_DIRECTORY "${_ORX_SCROLL_EXPECTED_DIR}")
        set(ORX_SCROLL_FOUND true)
        set(ORX_SCROLL_INCLUDE_DIR "${_ORX_SCROLL_EXPECTED_DIR}/include/Scroll")
    else()
        set(ORX_SCROLL_FOUND false)
        if(ORX_FIND_REQUIRED)
            message(SEND_ERROR "The Scroll library was not found at ${_ORX_SCROLL_EXPECTED_DIR}")
        endif()
    endif()
endif()
