find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SOURCES "*.c" "*.h")
add_library(VulkanDriver SHARED ${SOURCES})
target_include_directories(VulkanDriver PRIVATE "${CMAKE_SOURCE_DIR}/src")
set_target_properties(VulkanDriver PROPERTIES OUTPUT_NAME VulkanDriver)
target_link_libraries(Vulkan::Vulkan)
