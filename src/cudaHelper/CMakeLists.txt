# CUDA Setup

find_package(CUDA 12 REQUIRED)

list(APPEND CUDA_NVCC_FLAGS ${CUDA_GENERATE_CODE})
list(APPEND CUDA_NVCC_FLAGS_DEBUG "-g -G")
set(CUDA_VERBOSE_BUILD ON)

file(GLOB_RECURSE CudaHelper_Sources
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cu"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

file(GLOB_RECURSE CudaHelper_Header
	"${CMAKE_CURRENT_SOURCE_DIR}/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/*.cuh"
)

CUDA_ADD_LIBRARY(CudaHelper ${CudaHelper_Sources})

add_library(CudaHelperLib INTERFACE)
target_link_libraries(CudaHelperLib
INTERFACE
    CudaHelper
)
target_include_directories(CudaHelperLib
INTERFACE 
	${CMAKE_CURRENT_SOURCE_DIR}
)