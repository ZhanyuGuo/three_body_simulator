cmake_minimum_required( VERSION 3.0 )
project( Three-Body )
set( CMAKE_CXX_STANDARD 11 )

#############
# FIND GLFW #
#############
find_package( glfw3 REQUIRED )

#################
# LOCAL INCLUDE #
#################
include_directories( ${CMAKE_SOURCE_DIR}/include )

add_executable( Three-Body_Simulator src/Three-Body_Simulator.cpp lib/glad/glad.c )
target_link_libraries( Three-Body_Simulator glfw3 GL -lGL -lm -ldl -lXinerama -lXrandr -lXi -lXcursor -lX11 -lXxf86vm -lpthread -lassimp )
