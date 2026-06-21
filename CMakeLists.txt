cmake_minimum_required(VERSION 3.12)
project(gpachess VERSION 0.1.0)
set(OUTPUT_NAME "GPAChess")

add_definitions(-DVERSION=${PROJECT_VERSION}) #PROJECT_VERSION 就是指上面 VERSION 后面那个

# 自动启用 Qt 的特殊工具
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 查找 Qt6 的模块
find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network) 

# 设置自动搜索资源文件的路径
set(AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(WIN_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/assets/app.rc") # 这是打包出来的可执行文件图标（记得换）


file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

add_executable(${PROJECT_NAME} #WIN32 # WIN32 表示没有控制台窗口
    ${SRC_FILES}
    ${HEADER_FILES}
    ${WIN_RESOURCES}
    assets/resources.qrc
)

# 设置可执行文件名称
set_target_properties(${PROJECT_NAME} PROPERTIES 
    OUTPUT_NAME "${OUTPUT_NAME}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/bin")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets")

# 链接 Qt 库
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Sql Qt6::Network)

# 这样就可以直接 #include "xxx.h" 了
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/entity"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utils")