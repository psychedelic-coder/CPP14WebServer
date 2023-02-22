file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cc include/*.h)
add_library(httpServer STATIC ${srcs})
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/bin/http")
target_include_directories(httpServer PUBLIC include)
target_link_libraries(httpServer PUBLIC mymuduo)
add_subdirectory(test) # 添加test子目录