######################################
### ez_link_target_qt
######################################

function(ez_link_target_qt TARGET_NAME)

    # force find_package to search for Qt in the correct folder
    set (CMAKE_PREFIX_PATH ${EZ_QT_DIR})

    target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_BINARY_DIR})
    target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_BINARY_DIR}/Code)

    target_compile_definitions(${TARGET_NAME} PUBLIC EZ_USE_QT)

    foreach(module ${ARGN})
        
        find_package (Qt5${module} REQUIRED PATHS ${EZ_QT_DIR})

        target_link_libraries(${TARGET_NAME} PUBLIC "Qt5::${module}")
      
    endforeach()

endfunction()