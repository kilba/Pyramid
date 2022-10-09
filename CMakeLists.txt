cmake_minimum_required(VERSION 3.23.2)

project(bsvk)

add_executable(${PROJECT_NAME}
	src/main.c
	src/wnd.c
)

target_include_directories(${PROJECT_NAME}
	PUBLIC include/
	PUBLIC include/vulkan/
)

target_link_directories(${PROJECT_NAME}
	PRIVATE lib
)

target_link_libraries(${PROJECT_NAME}
    vulkan-1
)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
