file(GLOB_RECURSE Shader_Sources
	"shaders/*.vert"
	"shaders/*.frag"
)

#message("Shaders: ${Shaders_Sources}")

source_group("Shaders" FILES ${Shader_Sources})

foreach(shader ${Shader_Sources})
	set(Shader_Dir ${PROJECT_SOURCE_DIR}/bin/shaders)

	get_filename_component(fname ${shader} NAME)
	add_custom_target(${fname}.spv 
		COMMAND ${CMAKE_COMMAND} -E make_directory ${Shader_Dir} &&
		$ENV{VK_SDK_PATH}/Bin/glslangValidator.exe -V ${shader} -o ${Shader_Dir}/${fname}.spv -g
        SOURCES ${shader}
	)
	ExternalTarget("Shaders" ${fname}.spv)
    add_dependencies(${CMAKE_PROJECT_NAME} ${fname}.spv)
endforeach()