# 这里的 project 名字可以和父级不同
project(ADB-Tool-App)

# 定义可执行文件 (注意：此时路径是相对于当前这个 CMakeLists.txt 的)
add_executable(ADB-Tool 
    "maintest.cpp" 
    "ADB-function.cpp"
    "include/ADB-function.h"
)

# 包含路径 (使用 CMAKE_CURRENT_SOURCE_DIR 确保路径正确)
target_include_directories(ADB-Tool PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# 链接库 (父级找过的 Threads 在这里可以直接用)
target_link_libraries(ADB-Tool PRIVATE Threads::Threads)
