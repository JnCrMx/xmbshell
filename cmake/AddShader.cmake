find_program(GLSLang_Validator glslangValidator REQUIRED)
function(add_shader TARGET SHADER)
	file(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR} ${SHADER})
	set(output ${CMAKE_BINARY_DIR}/${rel}.spv)
	set(outputh ${CMAKE_BINARY_DIR}/${rel}.h)

	get_filename_component(output-dir ${output} DIRECTORY)
	file(MAKE_DIRECTORY ${output-dir})

	string(REPLACE "/" "_" shader_name ${rel})
	string(REPLACE "." "_" shader_name ${shader_name})

	add_custom_command(
		OUTPUT ${outputh}
		COMMAND ${GLSLang_Validator} -V -o ${outputh} --vn ${shader_name} ${SHADER}
		DEPENDS ${SHADER}
		VERBATIM)
	add_custom_command(
		OUTPUT ${output}
		COMMAND ${GLSLang_Validator} -V -o ${output} ${SHADER}
		DEPENDS ${SHADER}
		VERBATIM)

	set_source_files_properties(${outputh} PROPERTIES GENERATED TRUE)
	target_sources(${TARGET} PRIVATE ${outputh})

	set_source_files_properties(${output} PROPERTIES GENERATED TRUE)
	target_sources(${TARGET} PRIVATE ${output})
endfunction(add_shader)
