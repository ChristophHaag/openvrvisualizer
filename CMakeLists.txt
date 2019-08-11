cmake_minimum_required(VERSION 3.0)

project(openvrvisualizer LANGUAGES C)

add_executable(openvrvisualizer main.c gl.c)

if(MSVC)
  target_compile_options(openvrvisualizer PRIVATE /W4 /WX)
else(MSVC)
  target_compile_options(openvrvisualizer PRIVATE -pedantic -Wall -Wextra -Wno-unused-parameter)
endif(MSVC)

find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(openvrvisualizer PRIVATE openhmd m ${SDL2_LIBRARY} SDL2 ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})
include_directories(/usr/include/SDL2/)

install(TARGETS openvrvisualizer RUNTIME DESTINATION bin)

