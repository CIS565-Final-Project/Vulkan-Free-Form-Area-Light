set(RES_ROOT_DIR "resources")
set(EXT_ROOT_DIR "${CMAKE_SOURCE_DIR}/external")
set(ENGINE_INDLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/")
set(SDL_INCLUDE_DIR "${EXT_ROOT_DIR}/SDL/include")
set(GLM_INCLUDE_DIR "${EXT_ROOT_DIR}/glm/glm")
set(STB_INCLUDE_DIR "${EXT_ROOT_DIR}/stb")
set(TINYOBJ_INCLUDE_DIR "${EXT_ROOT_DIR}/tinyobjloader")

# TinyObjLoader Lib
add_library(TinyObjLoader 
	"${EXT_ROOT_DIR}/tinyobjloader/tiny_obj_loader.cc"
)

# Engine Lib
file(TO_CMAKE_PATH ${CMAKE_SOURCE_DIR} PROJ_BASE_PATH_NORMALIZED)
add_definitions(-DPROJ_BASE_PATH="${PROJ_BASE_PATH_NORMALIZED}")

source_group(Sources FILES ${exe_Sources})
source_group(Headers FILES ${exe_Header})

file(GLOB_RECURSE Engine_Sources
	"core/*.cpp"
	"renderEngine/*.cpp"
	"scene/*.cpp"
)

file(GLOB_RECURSE Engine_Header
	"core/*.h"
	"renderEngine/*.h"
	"scene/*.h"
)

source_group(Sources FILES ${Engine_Sources})
source_group(Headers FILES ${Engine_Header})

add_library(Engine ${Engine_Sources})

target_include_directories(Engine
PUBLIC
	${ENGINE_INDLUDE_DIR}
	${SDL_INCLUDE_DIR}
	${GLM_INCLUDE_DIR}
	${STB_INCLUDE_DIR}
	${TINYOBJ_INCLUDE_DIR}
)

target_precompile_headers(Engine PUBLIC
  "pch.h"
)

target_link_libraries(Engine
	Vulkan::Vulkan
	SDL2::SDL2
	SDL2::SDL2main
	TinyObjLoader
)
