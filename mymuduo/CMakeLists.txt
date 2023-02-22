#cmake_minimum_required(VERSION 3.12)

#if(NOT CMAKE_BUILD_TYPE)
#    set(CMAKE_BUILD_TYPE Release)
#endif()

#set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

#project(mymuduo)
file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cc include/*.h)
add_library(mymuduo SHARED ${srcs})
target_include_directories(mymuduo PUBLIC include)
# mymuduo最终编译成so动态库，设置动态库的路径,放在根目录的lib文件夹下
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/bin/mymuduo")

find_package(Threads REQUIRED)
#add_subdirectory(${PROJECT_SOURCE_DIR}/test)

# 定义参与编译的源代码文件 . 指的是当前目录的所有文件 src指的是参与编译的源代码文件是src文件夹
#aux_source_directory(src SRC_LIST) 
# 编译生成动态库mymuduo
#add_library(mymuduo SHARED ${SRC_LIST})
#add_library(mymuduo STATIC ${SRC_LIST})
# 生成对象库，对象库类似于静态库，但不生成.a文件，只由CMake记住生成了哪些对象文件。
#add_library(mymuduo OBJECT ${SRC_LIST})
# target 由add_library或add_executable构建，所以target_include_directories应该在之后。
#target_include_directories(mymuduo PUBLIC include)
target_link_libraries(mymuduo PUBLIC Threads::Threads)

enable_testing() # 打开测试

add_subdirectory(test) # 添加test子目录