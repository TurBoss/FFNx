include(FindPackageHandleStandardArgs)

if (NOT GLFW_FOUND)
	find_library(
		GLFW_LIBRARY
		glfw32 
        glfw32s 
        glfw
        glfw3
		PATH_SUFFIXES
		lib
	)

	find_path(
		GLFW_INCLUDE_DIR
		GLFW
		PATH_SUFFIXES
		include
	)

	add_library(GLFW::GLFW STATIC IMPORTED)

	set_target_properties(
		GLFW::GLFW
		PROPERTIES
		IMPORTED_LOCATION
		"${GLFW_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES
		"${GLFW_INCLUDE_DIR}"
	)

	find_package_handle_standard_args(GLFW DEFAULT_MSG GLFW_LIBRARY GLFW_INCLUDE_DIR)
endif()
