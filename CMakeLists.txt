cmake_minimum_required(VERSION 2.8.12)
set(CMAKE_MOUDULE_PATH ${CMAKE_MOUDULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

project(vulkan_free_form_area_light)

OPTION(USE_D2D_WSI "Use Direct to Display Swapchain" OFF)

add_definitions(-DVULKAN_HPP_NO_CONSTRUCTORS)
add_definitions(-DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
remove_definitions(-DVULKAN_HPP_NO_UNION_SETTERS)

find_package(Vulkan REQUIRED)

set(CMAKE_CXX_STANDARD 20)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

function(ExternalTarget folder target)
	set_property(TARGET ${target} PROPERTY FOLDER ${folder})
endfunction(ExternalTarget)

add_subdirectory(external)

foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

add_subdirectory(src)
add_subdirectory(resources)
