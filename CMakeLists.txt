cmake_minimum_required(VERSION 3.10)

project(xmbshell)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog REQUIRED)
find_package(Freetype REQUIRED)

file(GLOB_RECURSE sources src/*.cpp src/*.h)
file(GLOB_RECURSE opt_sources opt/*.cpp opt/*.h)
file(GLOB_RECURSE shaders shaders/*.vert shaders/*.frag shaders/*.geom)

add_executable(xmbshell ${sources})

add_subdirectory(external/VulkanMemoryAllocator)
add_subdirectory(external/simppl)
add_subdirectory(external/libspng)
target_compile_options(spng PRIVATE -O3)

add_library(optimized_components STATIC ${opt_sources})
target_include_directories(optimized_components PRIVATE include/)
target_include_directories(optimized_components PRIVATE external/VulkanMemoryAllocator-Hpp/include/)
target_compile_options(optimized_components PRIVATE -O3)

set(GLFW_BUILD_WAYLAND ON CACHE INTERNAL "")
set(GLFW_BUILD_X11 OFF CACHE INTERNAL "")
add_subdirectory(external/glfw)

target_include_directories(xmbshell PRIVATE include/)
target_include_directories(xmbshell PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories(xmbshell PRIVATE ${GLM_INCLUDE_DIRS})
target_include_directories(xmbshell PRIVATE external/VulkanMemoryAllocator-Hpp/include/)
target_include_directories(xmbshell PRIVATE external/libspng/spng)

target_link_libraries(xmbshell PRIVATE ${Vulkan_LIBRARIES})
target_link_libraries(xmbshell PRIVATE glfw)
target_link_libraries(xmbshell PRIVATE spdlog::spdlog)
target_link_libraries(xmbshell PRIVATE dl)
target_link_libraries(xmbshell PRIVATE VulkanMemoryAllocator)
target_link_libraries(xmbshell PRIVATE Simppl::simppl)
target_link_libraries(xmbshell PRIVATE spng)
target_link_libraries(xmbshell PRIVATE Freetype::Freetype)
target_link_libraries(xmbshell PRIVATE optimized_components)

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

foreach(shader ${shaders})
	add_shader(xmbshell ${shader})
endforeach()
target_include_directories(xmbshell PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/shaders)
