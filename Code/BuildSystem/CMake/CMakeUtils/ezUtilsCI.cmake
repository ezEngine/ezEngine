# #####################################
# ## ez_ci_add_to_targets_list(<target> [C++])
# #####################################

function(ez_ci_add_to_targets_list TARGET_NAME LANGUAGE)
	if(EZ_NO_TXT_FILES)
		return()
	endif()
	
	file(RELATIVE_PATH REL_PATH_TO_FOLDER ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

	get_target_property(LINK_LIBS ${TARGET_NAME} LINK_LIBRARIES)

	file(APPEND ${CMAKE_BINARY_DIR}/Targets.txt "${TARGET_NAME}|${REL_PATH_TO_FOLDER}|0|${LANGUAGE}|${LINK_LIBS}\n")
endfunction()

# #####################################
# ## ez_ci_add_test(<target> [NEEDS_HW_ACCESS])
# #####################################
function(ez_ci_add_test TARGET_NAME)
	set(ARG_OPTIONS NEEDS_HW_ACCESS)
	set(ARG_ONEVALUEARGS "")
	set(ARG_MULTIVALUEARGS "")
	cmake_parse_arguments(ARG "${ARG_OPTIONS}" "${ARG_ONEVALUEARGS}" "${ARG_MULTIVALUEARGS}" ${ARGN})

	if(ARG_UNPARSED_ARGUMENTS)
		message(FATAL_ERROR "ez_ci_add_test: Invalid arguments '${ARG_UNPARSED_ARGUMENTS}'")
	endif()
	
	if(EZ_NO_TXT_FILES)
		return()
	endif()

	if(${ARG_NEEDS_HW_ACCESS})
		set(HWA_VALUE 1)
	else()
		set(HWA_VALUE 0)
	endif()

	file(APPEND ${CMAKE_BINARY_DIR}/Tests.txt "${TARGET_NAME}|${HWA_VALUE}|0\n")

	# Add CTest integration
	get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
	if(TARGET_TYPE STREQUAL "EXECUTABLE")
		add_test(
			NAME ${TARGET_NAME}
			COMMAND ${TARGET_NAME} -noGui -run -close -console
		)
		# Set test properties
		set_tests_properties(${TARGET_NAME} PROPERTIES
			TIMEOUT 300  # 5 minutes timeout
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		)
	endif()
endfunction()
