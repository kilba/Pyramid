cmake_minimum_required(VERSION 3.23.2)

option(GLFW_VULKAN_STATIC ON)

project(bsvk)

add_executable(${PROJECT_NAME}
	src/main.c
	src/bs/bs_ini.c
	src/bs/bs_math.c
	src/bs/bs_mem.c
	src/bs/bs_core.c
	src/bs/bs_shaders.c
)

target_include_directories(${PROJECT_NAME}
	PUBLIC external/include/
	PUBLIC include/
	PUBLIC include/vulkan/
	PUBLIC include/basilisk/
)

target_link_directories(${PROJECT_NAME}
	PRIVATE external
)

target_link_libraries(${PROJECT_NAME}
    vulkan-1 glfw3
)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
