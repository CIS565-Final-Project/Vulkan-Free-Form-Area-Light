set(ENGINE_INDLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/")

# Engine Lib
add_definitions(-DSTB_IMAGE_IMPLEMENTATION)
add_definitions(-DSTB_IMAGE_WRITE_IMPLEMENTATION)

file(GLOB_RECURSE Engine_Sources
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE Engine_Header
	"${CMAKE_CURRENT_SOURCE_DIR}/*.h"
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
