find_package(DENGINE REQUIRED)
find_package(GLFW REQUIRED)
find_package(FFMPEG REQUIRED)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
	set(RELEASE_TYPE "DEBUG")
else()
	set(RELEASE_TYPE "RELEASE")
endif()

# Include all the source code files
file(GLOB_RECURSE source_files "*.h" "*.hpp" "*.cpp")

add_library(ff7_vulkan SHARED ${source_files})
target_include_directories(ff7_vulkan PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(ff7_vulkan
	dbghelp
    DENGINE::DENGINE
    GLFW::GLFW
	FFMPEG::AVUtil
	FFMPEG::SWResample
	FFMPEG::AVCodec
	FFMPEG::AVFormat
	FFMPEG::SWScale
)
target_compile_options(ff7_vulkan
	PRIVATE /D_CRT_SECURE_NO_WARNINGS
	PRIVATE /D${RELEASE_TYPE}
)
target_link_options(ff7_vulkan PRIVATE /PDBALTPATH:ff7_vulkan.pdb)
if(_DLL_VERSION)
	target_compile_options(ff7_vulkan PRIVATE /DVERSION="${_DLL_VERSION}")
endif()
set_target_properties(ff7_vulkan PROPERTIES PREFIX "")
set_target_properties(ff7_vulkan PROPERTIES SUFFIX ".fgd")

install(TARGETS ff7_vulkan RUNTIME DESTINATION bin)
