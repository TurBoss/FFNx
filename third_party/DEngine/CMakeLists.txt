include(ExternalProject)

ExternalProject_Add(
    DEngine
    GIT_REPOSITORY      "https://github.com/DiligentGraphics/DiligentEngine.git"
    GIT_TAG             "b45c86c25950da660a25c1daf1f0a40bac530366"
	INSTALL_DIR			"${CMAKE_BINARY_DIR}/vendor"
    CMAKE_ARGS
		"-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
		"-DCMAKE_PREFIX_PATH=<INSTALL_DIR>"
		"-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>"
        # DiligentEngine flags
        "-DDILIGENT_BUILD_FX=0"
        "-DDILIGENT_BUILD_SAMPLES=0"
        "-DDILIGENT_BUILD_DEMOS=0"
        "-DDILIGENT_BUILD_UNITY_PLUGIN=0"
)
