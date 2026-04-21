cmake_minimum_required(VERSION 3.21)
project(NexusPilotQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

if (WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_WIN32_WINNT=0x0601)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(APP_SOURCES
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/backend/BackendTypes.h
    src/backend/BackendAdapter.h
    src/backend/BackendAdapter.cpp
    src/widgets/MapWidget.h
    src/widgets/MapWidget.cpp
    src/widgets/BatteryWidget.h
    src/widgets/BatteryWidget.cpp
    src/widgets/AttitudeWidget.h
    src/widgets/AttitudeWidget.cpp
    src/widgets/CompassWidget.h
    src/widgets/CompassWidget.cpp
)

if (WIN32)
    list(APPEND APP_SOURCES
        third_party/groundstation/head/ProtocolTypes.h
        third_party/groundstation/head/ICommunication.h
        third_party/groundstation/head/SerialCommunication.h
        third_party/groundstation/head/UdpCommunication.h
        third_party/groundstation/source/ICommunication.cpp
        third_party/groundstation/source/SerialCommunication.cpp
        third_party/groundstation/source/UdpCommunication.cpp
    )
endif()

add_executable(${PROJECT_NAME} WIN32 ${APP_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/backend
    src/widgets
    third_party/groundstation/head
)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    third_party/groundstation/generated/include/mavlink/c_library_v2-master
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

if (MINGW)
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wno-address-of-packed-member
        -Wno-error=address-of-packed-member
    )
endif()

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets)
