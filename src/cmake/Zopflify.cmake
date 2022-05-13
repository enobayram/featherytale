add_custom_target(zopflify)

function(add_zopflify_image BASE_DIR REL_IMG_PATH)
    set(IMG_PATH ${BASE_DIR}/${REL_IMG_PATH})
    set(OUT_REL_PATH zopflify/${REL_IMG_PATH}.zop)
    set(OUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/${OUT_REL_PATH})
    string(REPLACE / _ OUT_TARGET ${OUT_REL_PATH})
    
    get_filename_component(OUT_DIR ${OUT_PATH} DIRECTORY)
    
    add_custom_command(OUTPUT ${OUT_PATH}
        COMMAND zopflipng ${IMG_PATH} ${IMG_PATH} -y
        COMMAND mkdir -p ${OUT_DIR} && touch ${OUT_PATH}
        DEPENDS ${IMG_PATH}
    )
    
    add_custom_target(${OUT_TARGET} 
        DEPENDS ${OUT_PATH}
        COMMENT "Zopflify ${IMG_PATH}"
    )
    
    add_dependencies(zopflify ${OUT_TARGET})
endfunction()

function(zopflify_all BASE_DIR)
    file (GLOB_RECURSE IMGS RELATIVE ${BASE_DIR} ${BASE_DIR}/*.png)
    foreach(IMG ${IMGS})
        add_zopflify_image(${BASE_DIR} ${IMG})
    endforeach()
endfunction()
