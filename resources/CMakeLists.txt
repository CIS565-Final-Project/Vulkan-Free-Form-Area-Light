file(GLOB_RECURSE Shader_Sources
	"shaders/*.vert"
	"shaders/*.frag"
	"shaders/*.comp"
	"shaders/*.mesh"
	"shaders/*.task"
)

file(GLOB_RECURSE IMAGES
    "images/*.jpg"
	"images/*.png"
	"images/*.bmp"
	"images/*.hdr"
	"images/*.dds"
)

file(GLOB_RECURSE MESHES
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
	string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" RELATIVE_PATH "${IMAGE}")
    configure_file(${IMAGE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${RELATIVE_PATH} COPYONLY)
endforeach()

#copy meshes
foreach(MESH ${MESHES})
	string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" RELATIVE_PATH "${MESH}")
    configure_file(${MESH} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${RELATIVE_PATH} COPYONLY)
endforeach()

#compile shaders
source_group("Shaders" FILES ${Shader_Sources})

foreach(shader ${Shader_Sources})
	set(Shader_Dir ${PROJECT_SOURCE_DIR}/bin/shaders)
	get_filename_component(fname ${shader} NAME)
	
	string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" RELATIVE_PATH "${shader}")
	string(REPLACE "/" "_" RELATIVE_NAME "${RELATIVE_PATH}")
	string(REPLACE "${fname}" "" RELATIVE_PATH_ONLY "${RELATIVE_PATH}")

	file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${RELATIVE_PATH_ONLY})

	add_custom_target(${RELATIVE_NAME}.spv 
		COMMAND $ENV{VK_SDK_PATH}/Bin/glslangValidator.exe --target-env spirv1.4 -V ${shader} -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${RELATIVE_PATH}.spv -g
        SOURCES ${shader}
	)
	ExternalTarget("Shaders" ${RELATIVE_NAME}.spv)
    add_dependencies(${CMAKE_PROJECT_NAME} ${RELATIVE_NAME}.spv)
endforeach()