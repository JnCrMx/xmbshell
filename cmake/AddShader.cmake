find_program(GLSLang_Validator glslangValidator)
function(add_shader TARGET SHADER)
	file(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR} ${SHADER})
	set(output ${CMAKE_BINARY_DIR}/${rel}.spv)
	set(outputh ${CMAKE_BINARY_DIR}/${rel}.h)

	get_filename_component(output-dir ${output} DIRECTORY)
	file(MAKE_DIRECTORY ${output-dir})

	add_custom_command(
		OUTPUT ${output}
		COMMAND ${GLSLang_Validator} -V -o ${output} ${SHADER}
		DEPENDS ${SHADER}
		VERBATIM)
	add_custom_command(
		OUTPUT ${outputh}
		COMMAND xxd -i ${rel}.spv > ${outputh}
		DEPENDS ${output}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		VERBATIM)

	set_source_files_properties(${outputh} PROPERTIES GENERATED TRUE)
	target_sources(${TARGET} PRIVATE ${outputh})
endfunction(add_shader)
