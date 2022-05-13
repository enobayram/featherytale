execute_process(COMMAND git describe --tags --always --dirty 
    OUTPUT_VARIABLE _output OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _result
    )

string(TIMESTAMP build_date "%Y-%m-%d")

if(_result EQUAL 0)
    set(description ${_output})

    execute_process(COMMAND git log --pretty=format:%h -n 1
        OUTPUT_VARIABLE commit OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _result
    )

else()
    set(description "unknown commit")
    set(commit ${description})
endif()

file (STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/../meta/game_version GAME_VERSION)

file(WRITE gen/build_info.ini "
[BuildInfo]
Description = ${description} : ${build_date}
Commit = \"${commit}\"
Version = \"${GAME_VERSION}\"
")
