
######################################
### ez_ci_add_to_targets_list
######################################

function(ez_ci_add_to_targets_list TARGET_NAME LANGUAGE)

	file(RELATIVE_PATH REL_PATH_TO_FOLDER ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

	get_target_property(LINK_LIBS ${TARGET_NAME} LINK_LIBRARIES)
	message(STATUS "Target Link Libs: ${LINK_LIBS}")

	file(APPEND ${CMAKE_BINARY_DIR}/Targets.txt "${TARGET_NAME}|${REL_PATH_TO_FOLDER}|0|${LANGUAGE}|${LINK_LIBS}\n")

endfunction()

######################################
### ez_ci_add_test
######################################

function(ez_ci_add_test TARGET_NAME NEEDS_HW_ACCESS)

	if (${NEEDS_HW_ACCESS})
		set (HWA_VALUE 1)
	else ()
		set (HWA_VALUE 0)
	endif ()

	file(APPEND ${CMAKE_BINARY_DIR}/Tests.txt "${TARGET_NAME}|${HWA_VALUE}|0\n")

endfunction()

