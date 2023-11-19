set_property(GLOBAL PROPERTY USE_FOLDERS ON)

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

