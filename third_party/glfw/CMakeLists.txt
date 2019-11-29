include(ExternalProject)

ExternalProject_Add(
    glfw
    GIT_REPOSITORY      "https://github.com/glfw/glfw.git"
    GIT_TAG             "3.3"
	INSTALL_DIR			"${CMAKE_BINARY_DIR}/vendor"
    CMAKE_ARGS
		"-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
		"-DCMAKE_PREFIX_PATH=<INSTALL_DIR>"
		"-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
        # GLFW flags
        "-DGLFW_BUILD_TESTS=0"
        "-DGLFW_BUILD_EXAMPLES=0"
)
