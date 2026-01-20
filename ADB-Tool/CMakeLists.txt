# 查找线程库
find_package(Threads REQUIRED)

# 定义可执行文件
add_executable(ADB-Tool 
    "main.cpp" 
    "ADB-function.cpp"
    "include/ADB-function.h"
)

# 【这一行替代了你那三行】
# 如果你确定要用 C++20，就把下面的 17 改成 20
target_compile_features(ADB-Tool PRIVATE cxx_std_17)

# 包含路径
target_include_directories(ADB-Tool PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# 链接库
target_link_libraries(ADB-Tool PRIVATE Threads::Threads)