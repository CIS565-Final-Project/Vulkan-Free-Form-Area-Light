set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#set(RES_ROOT_DIR "resources")
#set(EXT_ROOT_DIR "${CMAKE_SOURCE_DIR}/external")
#set(ENGINE_INDLUDE_DIR "${CMAKE_SOURCE_DIR}/src/engine")
#set(SRC_INDLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
#set(SDL_INCLUDE_DIR "${EXT_ROOT_DIR}/SDL/include")
#set(GLM_INCLUDE_DIR "${EXT_ROOT_DIR}/glm/glm")
#set(STB_INCLUDE_DIR "${EXT_ROOT_DIR}/stb")
#set(TINYOBJ_INCLUDE_DIR "${EXT_ROOT_DIR}/tinyobjloader")
file(GLOB SOURCES
     *.cpp
)
file(GLOB HEADER
    *.h
)
add_library(LTCPrep
    ${SOURCES}
)
target_include_directories(LTCPrep PUBLIC ${HEADER})
target_link_libraries(LTCPrep
PUBLIC 
	Engine
)

