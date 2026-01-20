cmake_minimum_required(VERSION 3.10)
project(ADB-Tool-Workspace) # 整个工作区的名字

# 1. 全局标准设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. 查找全局依赖
find_package(Threads REQUIRED)

# 3. 包含子目录 (这是关键！)

add_subdirectory(ADB-Tool)
