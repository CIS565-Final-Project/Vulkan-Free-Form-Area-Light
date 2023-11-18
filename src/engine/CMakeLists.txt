
set(ENGINE_INDLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/")

# Engine Lib
file(TO_CMAKE_PATH ${CMAKE_SOURCE_DIR} PROJ_BASE_PATH_NORMALIZED)
add_definitions(-DPROJ_BASE_PATH="${PROJ_BASE_PATH_NORMALIZED}")
add_definitions(-DSTB_IMAGE_IMPLEMENTATION)

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
)
target_link_libraries(Engine
    ExternalLibs
)
target_precompile_headers(Engine PUBLIC
  "pch.h"
)
