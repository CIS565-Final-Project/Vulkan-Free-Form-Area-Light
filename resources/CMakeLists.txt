file(GLOB_RECURSE Shader_Sources
	"shaders/*.vert"
	"shaders/*.frag"
	"shaders/*.comp"
	"shaders/*.mesh"
	"shaders/*.task"
)

file(GLOB IMAGES
    "images/*.jpg"
	"images/*.png"
	"images/*.bmp"
	"images/*.hdr"
	"images/*.dds"
)

file(GLOB MESHES
    "meshes/*.obj"
    "meshes/*.mtl"
	"meshes/*.fbx"
	"meshes/*.gltf"
)

#message("Shaders: ${Shaders_Sources}")
#message("Images: ${IMAGES}")
#message("Meshes: ${MESHES}")

#copy images
foreach(IMAGE ${IMAGES})
    get_filename_component(fname ${IMAGE} NAME)
    configure_file(${IMAGE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/images/${fname} COPYONLY)
endforeach()

#copy meshes
foreach(MESH ${MESHES})
    get_filename_component(fname ${MESH} NAME)
    configure_file(${MESH} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/meshes/${fname} COPYONLY)
endforeach()

#compile shaders
source_group("Shaders" FILES ${Shader_Sources})

foreach(shader ${Shader_Sources})
	set(Shader_Dir ${PROJECT_SOURCE_DIR}/bin/shaders)

	get_filename_component(fname ${shader} NAME)
	add_custom_target(${fname}.spv 
		COMMAND ${CMAKE_COMMAND} -E make_directory ${Shader_Dir} &&
		$ENV{VK_SDK_PATH}/Bin/glslangValidator.exe --target-env spirv1.4 -V ${shader} -o ${Shader_Dir}/${fname}.spv -g
        SOURCES ${shader}
	)
	ExternalTarget("Shaders" ${fname}.spv)
    add_dependencies(${CMAKE_PROJECT_NAME} ${fname}.spv)
endforeach()