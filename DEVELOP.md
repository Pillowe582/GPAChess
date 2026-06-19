# 开发指南
## 项目文件结构
```
GPAChess/
├── .git/                          
├── .gitignore                     
├── .vscode/                       
├── assets/                        # 资源文件
│   ├── app.rc                     
│   ├── MainIcon.ico               # 应用图标
│   └── resources.qrc              # Qt 资源文件
├── build/                         # CMake 构建输出目录
│   ├── bin/                       # ---编译输出在这里！---
├── CMakeLists.txt                 # CMake 项目配置（记得按照自己的需求改名字、图标、版本之类的）
├── DEVELOP.md                     # 开发文档（用于解释项目如何构建，以及告诉ai项目结构）
├── README.md                      # 项目文档
└── src/                           # 源代码目录
    ├── *.cpp                      # 其他源文件    
    ├── entry.ui                   # 游戏主UI
    ├── *.ui                       # Qt UI 文件
    ├── include/                   # 头文件目录
    │   ├── mainwindow.h           # 主窗口头文件
    │   └── *.h                    # 其他头文件
    └── main.cpp                   # 主程序文件
    └── entry.cpp                  # 游戏入口文件
    
```
## 构建项目
1. 安装最新 Qt
2. 安装最新 CMake
3. 运行 CMake（vscode可以一键运行，不同平台可能不一样，总之拿着输出问ai就行）

## 如何开发
- 最一针见血、最直截了当的回答：**用Qt Designer设计ui，对照Qt文档在src目录写代码，不会的不懂的Vibe**

## 注意事项
- 核心业务代码建议都放在 src/ 中，资源文件放在 assets/ 中
- 建议用Git进行版本管理
- *.ui 文件就是图形界面
  - 建议使用Qt Designer进行可视化界面设计，直接输出ui文件
  - Qt Designer是Qt自带的，vscode也有插件之类的一键启动

## 项目设计
### 游戏启动
- 启动后，首先
