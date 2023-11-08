set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(RES_ROOT_DIR "resources")
set(EXT_ROOT_DIR "${CMAKE_SOURCE_DIR}/external")
set(SDL_INCLUDE_DIR "${EXT_ROOT_DIR}/SDL/include")
set(GLM_INCLUDE_DIR "${EXT_ROOT_DIR}/glm/glm")

file(GLOB_RECURSE exe_Sources
	"*.cpp"
)

file(GLOB_RECURSE exe_Header
	"*.h"
)

file(TO_CMAKE_PATH ${CMAKE_SOURCE_DIR} PROJ_BASE_PATH_NORMALIZED)
add_definitions(-DPROJ_BASE_PATH="${PROJ_BASE_PATH_NORMALIZED}")

source_group(Sources FILES ${exe_Sources})
source_group(Headers FILES ${exe_Header})

add_executable(${CMAKE_PROJECT_NAME} ${exe_Sources} ${exe_Header} "commandPool.h" "commandPool.cpp")

set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

target_include_directories(${CMAKE_PROJECT_NAME}
PUBLIC
	${SDL_INCLUDE_DIR}
	${GLM_INCLUDE_DIR}
)

target_link_libraries(${CMAKE_PROJECT_NAME}
	Vulkan::Vulkan
	SDL2::SDL2
	SDL2::SDL2main
)

