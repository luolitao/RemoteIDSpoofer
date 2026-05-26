# Remote ID Spoofer for ESP32

基于 ESP32-S3 的无人机远程识别（Remote ID）信号模拟器，支持 ASTM F3411-19、ASD-STAN 4709-002 以及中国 CAAC 标准。

## ⚠️ 免责声明

**本项目仅用于教育、研究和测试目的。**

- 在某些国家和地区，发送虚假的 Remote ID 信号可能是非法的
- 请遵守当地法律法规和民航管理规定
- 不要在真实空域或禁飞区使用此设备
- 作者不对任何滥用行为承担责任

## 📋 功能特性

### 核心功能
- ✅ **WiFi Beacon 帧传输** - 符合 ASTM F3411-19 标准
- ✅ **多无人机模拟** - 支持同时模拟最多 16 架无人机
- ✅ **Web 配置界面** - 通过浏览器轻松配置参数
- ✅ **实时位置更新** - 动态生成飞行轨迹
- ✅ **多标准支持**：
  - ASTM F3411-19 (美国/国际)
  - ASD-STAN 4709-002 (欧洲)
  - CAAC Remote ID (中国)

### 技术规格
- **硬件平台**: ESP32-S3 (推荐 2MB+ Flash)
- **开发框架**: ESP-IDF v6.0
- **传输方式**: WiFi Beacon 帧 (2.4GHz, Channel 6)
- **更新频率**: 5Hz (200ms 周期)
- **覆盖范围**: 约 100-200 米（取决于环境）

## 🛠️ 硬件要求

### 必需组件
- ESP32-S3 开发板（推荐带 USB-C 的版本）
- Micro USB 或 USB-C 数据线
- 电脑（Windows/macOS/Linux）

### 推荐开发板
- ESP32-S3-DevKitC-1
- ESP32-S3-WROOM-1 模块开发板
- 任何带有 ESP32-S3 的开发板

## 📦 安装步骤

### 1. 安装 ESP-IDF

#### Windows
```powershell
# 下载并安装 ESP-IDF Tools Installer
# https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/windows-setup.html

# 推荐使用 ESP-IDF v6.0 或更高版本
```

#### Linux/macOS
```bash
# 克隆 ESP-IDF
git clone -b v6.0 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32s3
. ./export.sh
```

### 2. 克隆项目

```bash
git clone https://github.com/your-username/RemoteIDSpoofer.git
cd RemoteIDSpoofer
```

### 3. 配置项目

```bash
# 设置目标芯片
idf.py set-target esp32s3

# 可选：自定义配置
idf.py menuconfig
```

### 4. 编译和烧录

```bash
# 编译项目
idf.py build

# 烧录到设备（替换 COM19 为你的端口号）
idf.py -p COM19 flash

# 查看串口输出
idf.py -p COM19 monitor
```

**Windows PowerShell 用户：**
```powershell
idf.py build
idf.py -p COM19 flash monitor
```

## 🚀 使用方法

### 第一步：连接到配置界面

1. **上电启动** - 将 ESP32 连接电源
2. **连接 WiFi** - 在手机/电脑上连接到：
   - SSID: `ESP_RIDS`
   - 密码: `makkauhijau`
3. **打开浏览器** - 访问 `http://192.168.4.1`

### 第二步：配置参数

在 Web 界面中设置：

#### 基本参数
- **Latitude** (纬度): 例如 `52.439100`
- **Longitude** (经度): 例如 `-1.503900`
- **No. of Drones** (无人机数量): 1-16

#### 中国用户额外参数
- **CAAC Registration** (实名登记码): 15位数字
  - 格式示例: `123456789012345`
  - 可在 UOM 系统查询你的登记码

### 第三步：启动模拟

1. 确认所有参数正确
2. 点击 **"Start Spoofing"** 按钮
3. 设备将关闭 Web 服务器并开始发送 Remote ID 信号
4. **注意**: 启动后无法再次连接 Web 界面，需要重启设备才能重新配置

### 第四步：验证信号

使用以下工具验证信号是否正常发送：

#### Android
- [OpenDroneID Receiver](https://play.google.com/store/apps/details?id=org.opendroneid.android_osm)
- [DroneScanner](https://play.google.com/store/apps/details?id=com.drone.scanner)

#### iOS
- [Drone Scanner](https://apps.apple.com/app/drone-scanner/id1559816595)

#### 桌面端
- Wireshark（捕获 WiFi beacon 帧）
- 自定义 ODID 接收器

## 🔧 高级配置

### 修改 WiFi 信道

编辑 `main/id_open.h`:
```cpp
#define WIFI_CHANNEL      6  // 改为 1-13 之间的值
```

### 启用蓝牙传输

编辑 `main/id_open.h`:
```cpp
#define ID_OD_BT          1  // 启用蓝牙
#define ID_OD_WIFI_BEACON 0  // 可选：禁用 WiFi
```

### 调整发射功率

编辑 `main/id_open_esp32.cpp`:
```cpp
esp_wifi_set_max_tx_power(78);  // 最大值 80 (20dBm)
```

### 自定义飞行轨迹

编辑 `main/spoofer.cpp` 中的 `update()` 函数：
```cpp
// 修改最大速度、高度等参数
const double max_speed = 25;      // m/s
const double max_height = 200;    // 米
const double max_accel = 10000;   // mm/s²
```

## 📁 项目结构

```
RemoteIDSpoofer/
├── main/
│   ├── app_main.cpp          # 主程序入口
│   ├── frontend.cpp/h        # Web 服务器和配置界面
│   ├── spoofer.cpp/h         # 无人机模拟器核心
│   ├── id_open.cpp/h         # OpenDroneID 封装
│   ├── id_open_esp32.cpp     # ESP32 特定实现
│   ├── id_open_china.cpp     # 中国 CAAC 标准实现
│   ├── opendroneid.c/h       # OpenDroneID 库
│   ├── utm.cpp/h             # UTM 工具函数
│   └── CMakeLists.txt        # 组件构建配置
├── CMakeLists.txt            # 项目构建配置
├── sdkconfig                 # ESP-IDF 配置
├── partitions.csv            # 分区表
└── README.md                 # 本文档
```

## 🌍 各地区标准说明

### ASTM F3411-19 (美国/国际)
- **默认标准**
- 使用 WiFi Beacon 或 Bluetooth
- 消息格式符合 FAA 要求

### ASD-STAN 4709-002 (欧洲)
- 与 ASTM 类似，但包含 EU 分类
- 需要设置 Category 和 Class
- 符合 EASA 规定

### CAAC Remote ID (中国)
- 需要 15 位实名登记码
- 从 UOM 系统获取登记码
- 符合中国民航局要求

**获取 CAAC 登记码：**
1. 访问 [UOM 系统](https://uom.caac.gov.cn)
2. 注册并实名登记你的无人机
3. 获取 15 位登记码
4. 在 Web 界面中输入该代码

## 🐛 故障排除

### 问题 1: 编译失败

**错误**: `threading_alt.h: No such file or directory`

**解决**:
```bash
idf.py fullclean
idf.py build
```

### 问题 2: 无法连接 WiFi AP

**检查**:
- 确认 SSID 和密码正确
- 尝试重启设备
- 检查是否有其他设备占用信道

### 问题 3: 手机 App 检测不到信号

**可能原因**:
- 距离太远（保持在 50 米内测试）
- WiFi 信道不匹配
- App 不支持 Beacon 帧模式

**解决**:
- 靠近设备测试
- 尝试不同的接收 App
- 使用 Wireshark 验证信号是否发送

### 问题 4: 频道超出范围错误

**错误**: `Channel number XX is out of range`

**解决**:
编辑 `main/id_open_esp32.cpp`，确保频道在 1-13 范围内：
```cpp
if (channel < 1 || channel > 13) {
    channel = 6;
}
```

### 问题 5: NVS 数据丢失

**解决**:
```bash
# 擦除整个 flash
idf.py erase-flash
# 重新烧录
idf.py flash monitor
```

## 📊 性能指标

| 指标 | 数值 |
|------|------|
| 信号更新频率 | 5 Hz |
| 最大模拟无人机数 | 16 |
| WiFi 发射功率 | 最高 20 dBm |
| 理论覆盖范围 | 100-200 米 |
| 内存占用 | ~250 KB RAM |
| Flash 占用 | ~1.3 MB |
| 启动时间 | < 5 秒 |

## 🔬 技术细节

### 消息类型
项目发送以下 OpenDroneID 消息：
- **Basic ID**: 无人机类型和标识符
- **Location**: 位置、高度、速度、航向
- **Self ID**: 自我描述文本
- **System**: 操作员位置和分类
- **Operator ID**: 操作员注册码

### 数据编码
- 使用 OpenDroneID 参考库进行编码
- Beacon 帧包含 ODID 消息包
- 符合 ASTM F3411-19 消息格式

### 飞行模拟
- 随机游走算法生成轨迹
- 最大速度: 25 m/s (90 km/h)
- 最大高度: 200 米
- 自动边界检测

## 🤝 贡献指南

欢迎提交 Issue 和 Pull Request！

### 开发环境设置
```bash
# 1. Fork 并克隆仓库
git clone https://github.com/your-username/RemoteIDSpoofer.git

# 2. 创建分支
git checkout -b feature/your-feature

# 3. 进行修改和测试

# 4. 提交更改
git commit -am 'Add new feature'

# 5. 推送到分支
git push origin feature/your-feature

# 6. 创建 Pull Request
```

### 代码规范
- 遵循 ESP-IDF 编码风格
- 添加必要的注释
- 测试所有修改
- 更新文档

## 📄 许可证

本项目采用 **MIT License**

```
Copyright (c) 2024 RemoteIDSpoofer Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction...
```

完整许可证文本请查看 [LICENSE](LICENSE) 文件。

## 🙏 致谢

感谢以下开源项目：
- [OpenDroneID](https://github.com/opendroneid/opendroneid-core-c) - 核心编码库
- [ESP-IDF](https://github.com/espressif/esp-idf) - ESP32 开发框架
- [Steve Jack](https://github.com/sxjack/uav_electronic_ids) - 原始 Arduino 实现

## 📞 联系方式

- **Issues**: [GitHub Issues](https://github.com/your-username/RemoteIDSpoofer/issues)
- **Discussions**: [GitHub Discussions](https://github.com/your-username/RemoteIDSpoofer/discussions)

## 📚 参考资料

1. [ASTM F3411-19 Standard](https://www.astm.org/f3411-19.html)
2. [ASD-STAN 4709-002](https://asd-stan.org/downloads/asd-stan-4709-002/)
3. [CAAC Remote ID Regulations](https://www.caac.gov.cn)
4. [OpenDroneID Specification](https://github.com/opendroneid/opendroneid-core-c)
5. [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)

---

**⭐ 如果这个项目对你有帮助，请给个 Star！**

**⚠️ 请负责任地使用本工具，遵守当地法律法规。**