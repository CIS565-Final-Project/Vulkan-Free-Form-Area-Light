find_package(Vulkan REQUIRED)
add_subdirectory(dds_image)
add_library(dds
    dds.cpp
)
target_link_libraries(dds
PUBLIC
    Vulkan::Vulkan
    dds_image
)
target_include_directories(dds
PUBLIC
    dds.h
)