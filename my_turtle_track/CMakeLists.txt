cmake_minimum_required(VERSION 2.8.3)
project(my_turtle_track)

find_package(catkin_simple REQUIRED)

#uncomment next line to use OpenCV library
#find_package(OpenCV REQUIRED)

#uncomment the next 2 lines to use the point-cloud library
#find_package(PCL 1.7 REQUIRED)
#include_directories(${PCL_INCLUDE_DIRS})


#uncomment the following 4 lines to use the Eigen library
#find_package(cmake_modules REQUIRED)
#find_package(Eigen3 REQUIRED)
#include_directories(${EIGEN3_INCLUDE_DIR})
#add_definitions(${EIGEN_DEFINITIONS})

catkin_simple()

cs_add_executable(turtle_track src/ApplyIPM.cpp)
cs_add_executable(turtle_track src/CollissionFinder.cpp)
cs_add_executable(turtle_track src/IPM.cpp)
cs_add_executable(turtle_track src/LaneFinder.cpp)
cs_add_executable(turtle_track src/Movement.cpp)

#set the default path for built executables to the "bin" directory
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
#set the default path for built libraries to the "lib" directory
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

find_package(OpenCV)
include_directories(${OpenCV_INCLUDE_DIRS})
rosbuild_add_library(${PROJECT_NAME} src/LaneFinder.cpp)
rosbuild_add_library(${PROJECT_NAME} src/CollissionFinder.cpp)
rosbuild_add_library(${PROJECT_NAME} src/ApplyIPM.cpp)
rosbuild_add_library(${PROJECT_NAME} src/Movement.cpp)
target_link_libraries(main ${OpenCV_LIBRARIES})
target_link_libraries(ipm ${OpenCV_LIBRARIES})

# example boost usage
# find_package(Boost REQUIRED COMPONENTS system thread)

# C++0x support - not quite the same as final C++11!
# use carefully;  can interfere with point-cloud library
# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

# Libraries: uncomment the following and edit arguments to create a new library
# cs_add_library(my_lib src/my_lib.cpp)   

# Executables: uncomment the following and edit arguments to compile new nodes
# may add more of these lines for more nodes from the same package
# cs_add_executable(example src/example.cpp)

#the following is required, if desire to link a node in this package with a library created in this same package
# edit the arguments to reference the named node and named library within this package
# target_link_library(example my_lib)

cs_install()
cs_export()
    
