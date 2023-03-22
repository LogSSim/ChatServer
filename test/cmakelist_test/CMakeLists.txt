# bin 放置可执行文件， lib 放置库文件， include 放置头文件， sec 放置源文件， build 放置cmake文件
cmake_minimum_required(VERSION 3.0)
project(main)

# 配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 配置头文件搜索路径
# include_directories()

# 配置库文件搜索路径
# link_directories()

# 设置需要编译的源文件列表
set(SRC_LIST muduo_server.cpp)

# 设置可执行文件放在指定的目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)


# 把指定路径下的源文件名字放入变量名SRC_LIST里面
# aux_source_directory(. SRC_LIST)

# 生成可执行文件server，由SRC_LIST变量所定义的源文件编译而来
add_executable(server ${SRC_LIST})
# 表示server目标程序，需要连接muduo_net muduo_base pthread三个库文件
target_link_libraries(server muduo_net muduo_base pthread)

