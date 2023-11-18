find_package(Vulkan REQUIRED)

add_subdirectory(SDL EXCLUDE_FROM_ALL)
add_subdirectory(dds)
add_subdirectory(glm)
add_subdirectory(tinyobjloader)

#set(RES_ROOT_DIR "resources")
set(EXT_ROOT_DIR "${CMAKE_SOURCE_DIR}/external")
set(GLM_INCLUDE_DIR "${EXT_ROOT_DIR}/glm/glm")
set(STB_INCLUDE_DIR "${EXT_ROOT_DIR}/stb")


add_library(ExternalLibs INTERFACE)
target_link_libraries(ExternalLibs
INTERFACE
    Vulkan::Vulkan
	SDL2::SDL2
	SDL2::SDL2main
	tinyobjloader
	glm
	dds
)
target_include_directories(ExternalLibs
INTERFACE 
	${GLM_INCLUDE_DIR}
	${STB_INCLUDE_DIR}
)
