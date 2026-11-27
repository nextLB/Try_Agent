
cmake_minimum_required(VERSION 3.21)

project(QtFirstDemo VERSION 1.0 LANGUAGES CXX)

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==================================================
# Qt6 安装路径
# ==================================================
list(PREPEND CMAKE_PREFIX_PATH
    "D:/Qt6/6.12.1/mingw_64"
)

# Qt 自动化处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC ON)

# 查找 Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

qt_wrap_cpp(MAIN_MOC_SOURCES
    backend_controller.h
)

# 创建可执行程序
add_executable(main
    main.cpp

    # Backend Controller
    backend_controller.cpp
    backend_controller.h
    ${MAIN_MOC_SOURCES}

    # UI Theme
    ui_theme.cpp
    ui_theme.h

    # Python Event Handler
    python_event_handler.cpp
    python_event_handler.h

    # Python Stdout Parser
    python_stdout_parser.cpp
    python_stdout_parser.h
)

# 链接 Qt6
target_link_libraries(main
    PRIVATE Qt6::Widgets Qt6::Network
)
