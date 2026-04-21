# NexusPilotQt（UI 改进版）

本版根据你的新要求做了以下调整：

- 左侧仅保留 5 个飞控指令：启动、停止、自检、安全锁、安全锁解锁。
- 紧急停止单独保留，并继续对接 `send_emergency_stop`。
- 已移除左下角摇杆控制、飞行模式、右下角姿态球、指南针。
- 顶部“设备连接”支持两种模式：
  - 数传串口：创建 `new SerialCommunication("COM3", 57600)`
  - WiFi / UDP：创建 `new UdpCommunication("192.168.4.1", 14550)`
- 切换连接方式时，会先销毁旧通信对象，再根据当前参数重建。
- 顶部新增 `MAVID` 配置按钮，用于设置 `setMAVID` 对应的 4 个 ID。
- 右侧实时显示：
  - `get_link_status`
  - `get_battery_status`
  - `get_battery_capacity`（用电池图标实时显示）
  - `get_battery_num`
  - `get_battery_voltage`
  - `get_current_current`
  - `get_longitude`
  - `get_latitude`
  - `get_absolute_altitude`
  - `get_relative_altitude`
- 地图区域默认允许空白，支持手动导入本地图片作为底图。

## 构建

1. 用 Qt Creator 打开根目录下 `CMakeLists.txt`
2. 选择 Qt 6 + MinGW 64-bit Kit
3. 点击 Build
4. 成功后点击 Run

## 额外修复

本版同时保留了之前为 MinGW / MAVLink 做的兼容修复：

- `CMakeLists.txt` 增加了 `address-of-packed-member` 警告抑制
- 修复了 `third_party/groundstation/source/ICommunication.cpp` 中的两个编译问题

## 文件说明

- `src/MainWindow.*`：主界面布局与交互
- `src/widgets/BatteryWidget.*`：电池图标组件
- `src/widgets/MapWidget.*`：地图/底图显示组件
- `src/backend/BackendAdapter.*`：Qt 界面与地面站后端适配层
