add_custom_target(export_frames)

set(SVG_TO_FRAME_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/../tools/svgToFrames.py)

function(add_svg SVG_PATH OUTPUT_BASE CONFIG_BASE IMAGE_BASE)
    set(IMAGE_PATH "${IMAGE_BASE}/${OUTPUT_BASE}.png")
    string(REPLACE / _ OUT_TARGET "export_frames_${OUTPUT_BASE}")

    add_custom_command(OUTPUT ${IMAGE_PATH}
        COMMAND python ${SVG_TO_FRAME_SCRIPT} ${SVG_PATH} ${OUTPUT_BASE} --config-path ${CONFIG_BASE} --image-path ${IMAGE_BASE}
        DEPENDS ${SVG_PATH}
    )
    
    add_custom_target(${OUT_TARGET} 
        DEPENDS ${IMAGE_PATH}
        COMMENT "Export ${OUTPUT_BASE}"
    )

    add_dependencies(export_frames ${OUT_TARGET})
endfunction()

set(ASSETS_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/../Assets)
set(DATA_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/../data)
set(EASY_CONFIG_BASE ${DATA_FOLDER}/config)
set(EASY_IMAGE_BASE ${DATA_FOLDER}/image)

function(add_svg_easy SVG_REL OUTPUT_BASE)
    add_svg("${ASSETS_FOLDER}/${SVG_REL}" common/${OUTPUT_BASE} ${EASY_CONFIG_BASE} ${EASY_IMAGE_BASE})
endfunction()

add_svg_easy(Cutscenes/C1S1Background.svg cutscenes/C1S1Background)
add_svg_easy(Cutscenes/C1S2Background.svg cutscenes/C1S2Background)
add_svg_easy(Cutscenes/C1S3Bgr.svg cutscenes/C1S3Bgr)
add_svg_easy(Cutscenes/C1S4Bgr.svg cutscenes/C1S4Bgr)
add_svg_easy(Cutscenes/C1S5Bgr.svg cutscenes/C1S5Bgr)
add_svg_easy(Cutscenes/FirstSlideObjects.svg cutscenes/FirstSlideObjects)
add_svg_easy(Cutscenes/C1Objects.svg cutscenes/C1Objects)
add_svg_easy(GUI/Icons.svg images/gameplay_icons)
add_svg_easy(Menu/Menus.svg menus/menu_graphics)
add_svg_easy(Menu/MainMenuItems.svg menus/main_menu_items)
add_svg_easy(Image/objects.svg images/object_graphics)
add_svg_easy(Characters/henchman.svg images/henchman)
add_svg_easy(Characters/father.svg images/crow_father)
add_svg_easy(Characters/dealer.svg images/dealer)
