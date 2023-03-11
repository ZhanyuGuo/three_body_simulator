cmake_minimum_required( VERSION 3.0 )
project( three_body_simulator )
set( CMAKE_CXX_STANDARD 11 )

#############
# FIND GLFW #
#############
find_package( glfw3 REQUIRED )

#################
# LOCAL INCLUDE #
#################
include_directories( ${CMAKE_SOURCE_DIR}/include )

add_executable( three_body_simulator src/three_body_simulator.cpp lib/glad/glad.c )
target_link_libraries( three_body_simulator glfw3 GL -lGL -lm -ldl -lXinerama -lXrandr -lXi -lXcursor -lX11 -lXxf86vm -lpthread -lassimp )
