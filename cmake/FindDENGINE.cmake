include(FindPackageHandleStandardArgs)

if (NOT DENGINE_FOUND)
	find_library(
		DENGINE_LIBRARY
		DiligentCore
        DiligentTools
        glew-static
        glslang
        GraphicsEngineD3D11_32r
        GraphicsEngineD3D12_32r
        GraphicsEngineOpenGL_32r
        GraphicsEngineVk_32r
        HLSL
        LibJpeg
        LibPng
        LibTiff
        Lua
        OGLCompiler
        OSDependent
        SPIRV
        spirv-cross-core
        SPIRV-Tools
        SPIRV-Tools-opt
        ZLib
		PATH_SUFFIXES
        lib/DiligentCore/Debug
        lib/DiligentTools/Debug
        lib/DiligentCore/RelWithDebInfo
        lib/DiligentTools/RelWithDebInfo
		lib/DiligentCore/Release
        lib/DiligentTools/Release
	)

	find_path(
		DENGINE_INCLUDE_DIR
		DiligentCore
        DiligentTools
		PATH_SUFFIXES
		headers
	)

	add_library(DENGINE::DENGINE STATIC IMPORTED)

	set_target_properties(
		DENGINE::DENGINE
		PROPERTIES
		IMPORTED_LOCATION
		"${DENGINE_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES
		"${DENGINE_INCLUDE_DIR}"
	)

	find_package_handle_standard_args(DENGINE DEFAULT_MSG DENGINE_LIBRARY DENGINE_INCLUDE_DIR)
endif()
