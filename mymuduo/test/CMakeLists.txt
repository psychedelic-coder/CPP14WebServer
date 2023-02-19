#INCLUDE_DIRECTORIES("${PROJECT_SOURCE_DIR}/include")

add_executable(test01 test01.cc)
target_link_libraries(test01 mymuduo)
add_test(NAME mytest COMMAND test01)

add_executable(test02 muduo_server.cpp)
target_link_libraries(test02 mymuduo)
add_test(NAME mytest2 COMMAND test02)