cmake_minimum_required(VERSION 2.4.6)
include($ENV{ROS_ROOT}/core/rosbuild/rosbuild.cmake)

# Set the build type.  Options are:
#  Coverage       : w/ debug symbols, w/o optimization, w/ code-coverage
#  Debug          : w/ debug symbols, w/o optimization
#  Release        : w/o debug symbols, w/ optimization
#  RelWithDebInfo : w/ debug symbols, w/ optimization
#  MinSizeRel     : w/o debug symbols, w/ optimization, stripped binaries
set(ROS_BUILD_TYPE RelWithDebInfo)
rosbuild_init()


#set the default path for built executables to the "bin" directory
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
#set the default path for built libraries to the "lib" directory
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#uncomment if you have defined messages
rosbuild_genmsg()
#uncomment if you have defined services
#gensrv()

#common commands for building c++ executables and libraries
#rosbuild_add_library(${PROJECT_NAME} src/example.cpp)
#target_link_libraries(${PROJECT_NAME} another_library)
#rospack_add_boost_directories()
#rospack_link_boost(${PROJECT_NAME} thread)
#rospack_add_executable(example examples/example.cpp)
#target_link_libraries(example ${PROJECT_NAME})

rosbuild_add_library(irobot_create_controller src/irobot_create_controller/controller.cpp)
rosbuild_add_library(create_comms src/irobot_create_driver/create_comms.c)

rosbuild_add_executable(driver src/irobot_create_driver/driver.cpp)
target_link_libraries(driver create_comms)



rosbuild_add_executable(test_1 src/irobot_create_examples/test_1.cpp)
target_link_libraries(test_1 irobot_create_controller)
rosbuild_add_executable(listener_test src/irobot_create_examples/listener_test.cpp)
target_link_libraries(listener_test irobot_create_controller)
rosbuild_add_executable(mario_song src/irobot_create_examples/mario_song.cpp)
target_link_libraries(mario_song irobot_create_controller)


find_package(wxWidgets REQUIRED)
include(${wxWidgets_USE_FILE})
include_directories( ${wxWidgets_INCLUDE_DIRS} )

rosbuild_add_library(irobot_create_gui src/irobot_create_gui/slider_2d.cpp)
target_link_libraries(irobot_create_gui ${wxWidgets_LIBRARIES})

rosbuild_add_executable(controller_gui src/irobot_create_gui/controller_gui.cpp)
target_link_libraries(controller_gui irobot_create_gui irobot_create_controller)




#add_subdirectory(src)

