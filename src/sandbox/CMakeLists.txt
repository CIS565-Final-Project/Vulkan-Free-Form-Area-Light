set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_definitions(-DSTB_IMAGE_WRITE_IMPLEMENTATION)

set(ENGINE_INDLUDE_DIR "${CMAKE_SOURCE_DIR}/src/engine")

file(GLOB_RECURSE exe_Sources
	"*.cpp"
)

file(GLOB_RECURSE exe_Header
	"*.h"
)

source_group(Sources FILES ${exe_Sources})
source_group(Headers FILES ${exe_Header})

add_executable(${CMAKE_PROJECT_NAME} ${exe_Sources})

target_precompile_headers(${CMAKE_PROJECT_NAME} PUBLIC
	"${ENGINE_INDLUDE_DIR}/core/entryPoint.h"
)

set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

target_link_libraries(${CMAKE_PROJECT_NAME}
	Engine
)

