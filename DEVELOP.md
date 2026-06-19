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
│   ├── resources.qrc              # Qt 资源文件
│   └── logo_frames/               # 开屏序列帧
├── build/                         # CMake 构建输出目录
│   ├── bin/                       # ---编译输出在这里！---
├── design/                        # 设计稿/临时设计资源
├── CMakeLists.txt                 # CMake 项目配置（记得按照自己的需求改名字、图标、版本之类的）
├── DEVELOP.md                     # 开发文档（用于解释项目如何构建，以及告诉ai项目结构）
├── README.md                      # 项目文档
└── src/                           # 源代码目录
  ├── main.cpp                   # 主程序入口
  ├── entry.cpp                  # 主窗口逻辑
  ├── logo_player.cpp            # 开屏动画播放器
  ├── print.cpp                  # 全局日志输出
  ├── entry.ui                   # 游戏主 UI
    ├── include/                   # 头文件目录
    │   ├── mainwindow.h           # 主窗口头文件
  │   ├── logo_player.h          # 开屏动画头文件
  │   └── print.h                # 全局日志头文件
    
```
## 构建项目
1. 安装最新 Qt
2. 安装最新 CMake
3. 运行 CMake（vscode可以一键运行，不同平台可能不一样，总之拿着输出问ai就行）

## 如何开发
- 最一针见血、最直截了当的回答：**用Qt Designer设计ui，对照Qt文档在src目录写代码，不会的不懂的Vibe**
- 日志用全局`print(const QString&)`
- 所有资源文件放到`assets/`，编译后会把整个 `assets/` 目录复制到 `build/bin/assets/`

## 注意事项
- 核心业务代码建议都放在 src/ 中，资源文件放在 assets/ 中
- 建议用Git进行版本管理
- *.ui 文件就是图形界面
  - 建议使用Qt Designer进行可视化界面设计，直接输出ui文件
  - Qt Designer是Qt自带的，vscode也有插件之类的一键启动

