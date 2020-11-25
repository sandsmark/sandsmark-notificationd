cmake_minimum_required(VERSION 3.5)

project(sandsmark-notificationd LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets DBus REQUIRED)
add_executable(sandsmark-notificationd
    main.cpp
    Manager.cpp
    Widget.cpp
    Widget.h
    icons.qrc
    )

target_link_libraries(sandsmark-notificationd PRIVATE Qt5::Widgets Qt5::DBus)
install(TARGETS sandsmark-notificationd DESTINATION bin)
install(FILES sandsmark-notificationd.desktop DESTINATION /etc/xdg/autostart/)
