find_package(Vulkan REQUIRED)

add_subdirectory(SDL EXCLUDE_FROM_ALL)
add_subdirectory(dds EXCLUDE_FROM_ALL)
add_subdirectory(glm EXCLUDE_FROM_ALL)
add_subdirectory(tinyobjloader EXCLUDE_FROM_ALL)

#set(RES_ROOT_DIR "resources")
set(EXT_ROOT_DIR "${CMAKE_SOURCE_DIR}/external")
set(GLM_INCLUDE_DIR "${EXT_ROOT_DIR}/glm/glm")
set(STB_INCLUDE_DIR "${EXT_ROOT_DIR}/stb")
set(IMGUI_INCLUDE_DIR 
	"${EXT_ROOT_DIR}/imgui"
	"${EXT_ROOT_DIR}/imgui/backends/"
)

file(GLOB IMGUI_SRC 
	"${EXT_ROOT_DIR}/imgui/*.cpp"
	"${EXT_ROOT_DIR}/imgui/backends/imgui_impl_vulkan.cpp"
	"${EXT_ROOT_DIR}/imgui/backends/imgui_impl_sdl2.cpp"
)

add_library(ImGui ${IMGUI_SRC})
target_include_directories(ImGui
PUBLIC
	${IMGUI_INCLUDE_DIR}
)
target_link_libraries(ImGui
PUBLIC
	Vulkan::Vulkan
	SDL2::SDL2
	SDL2::SDL2main
)

add_library(ExternalLibs INTERFACE)
target_link_libraries(ExternalLibs
INTERFACE
    Vulkan::Vulkan
	SDL2::SDL2
	SDL2::SDL2main
	tinyobjloader
	glm
	dds
	ImGui
)
target_include_directories(ExternalLibs
INTERFACE 
	${GLM_INCLUDE_DIR}
	${STB_INCLUDE_DIR}
	${IMGUI_INCLUDE_DIR}
)
