# RemoteIDSpoofer (ESP-IDF Version)

An ESP32-based Remote ID spoofer that broadcasts fake drone identification signals. This is a port of the original Arduino-based project to ESP-IDF framework with additional features including China CAAC standard support.

**Original Project**: https://github.com/jjshoots/RemoteIDSpoofer

## ⚠️ Legal Disclaimer

**This project is for educational and research purposes only.**

- Broadcasting false Remote ID signals may be illegal in your jurisdiction
- Always comply with local aviation regulations and laws
- Do not use this device in restricted airspace or near airports
- The authors are not responsible for any misuse of this software

## 📖 Overview

This device creates a WiFi access point that you can connect to with your phone or computer. Through a web interface, you can configure GPS coordinates and the number of drones to simulate. Once started, it broadcasts Remote ID beacon frames that can be detected by Remote ID receiver apps.

### Features

- **Web-based Configuration**: Easy setup via browser
- **Multiple Drone Simulation**: Support for 1-16 simultaneous drones
- **Dynamic Flight Patterns**: Random movement simulation
- **Multi-Standard Support**:
  - ASTM F3411-19 (USA/International)
  - ASD-STAN 4709-002 (Europe)
  - CAAC Remote ID (China) - *New*
- **ESP-IDF Framework**: More stable and feature-rich than Arduino

## 🛠️ Hardware Requirements

- ESP32-S3 development board (recommended with 2MB+ Flash)
- USB cable for programming
- Computer with ESP-IDF installed

### Supported Boards
- ESP32-S3-DevKitC-1
- Any ESP32-S3 board with WiFi capability

## 📋 Installation

### Prerequisites

1. **Install ESP-IDF v6.0 or later**
   - [Windows Setup Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/windows-setup.html)
   - [Linux/macOS Setup Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/linux-macos-setup.html)

2. **Clone the repository**
   ```bash
   git clone https://github.com/your-username/RemoteIDSpoofer.git
   cd RemoteIDSpoofer
   ```

### Build and Flash

```bash
# Set target to ESP32-S3
idf.py set-target esp32s3

# Build the project
idf.py build

# Flash to device (replace COM19 with your port)
idf.py -p COM19 flash monitor
```

**For Windows PowerShell users:**
```powershell
idf.py set-target esp32s3
idf.py build
idf.py -p COM19 flash monitor
```

## 🚀 Usage

### Step 1: Connect to the Access Point

After flashing, the device will start automatically:

1. **WiFi Credentials**:
   - SSID: `ESP_RIDS`
   - Password: `makkauhijau`

2. **Connect** your phone/computer to this network

3. **Open Browser** and navigate to: `http://192.168.4.1`

### Step 2: Configure Settings

On the web interface, you can set:

#### Basic Settings
- **Latitude**: Starting latitude (e.g., `52.439100`)
- **Longitude**: Starting longitude (e.g., `-1.503900`)
- **Number of Drones**: How many drones to simulate (1-16)

#### China CAAC Settings (if enabled)
- **CAAC Registration Number**: 15-digit real-name registration code
  - Example: `123456789012345`
  - Get your code from [UOM System](https://uom.caac.gov.cn)

### Step 3: Start Spoofing

1. Verify all settings are correct
2. Click **"Start Spoofing"** button
3. The device will stop the web server and begin broadcasting
4. **Important**: You cannot reconnect to the web interface after starting. Power cycle the device to reconfigure.

### Step 4: Verify Signal

Use these apps to detect the broadcast:

#### Android
- [OpenDroneID Receiver](https://play.google.com/store/apps/details?id=org.opendroneid.android_osm)
- [DroneScanner](https://play.google.com/store/apps/details?id=com.drone.scanner)

#### iOS
- [Drone Scanner](https://apps.apple.com/app/drone-scanner/id1559816595)

#### Desktop
- Wireshark (capture WiFi beacon frames)
- Custom ODID receivers

## 🔧 Advanced Configuration

### Change WiFi Channel

Edit `main/id_open.h`:
```cpp
#define WIFI_CHANNEL      6  // Change to 1-13
```

### Enable Bluetooth Transmission

Edit `main/id_open.h`:
```cpp
#define ID_OD_BT          1  // Enable Bluetooth
#define ID_OD_WIFI_BEACON 0  // Optionally disable WiFi
```

### Adjust Transmit Power

Edit `main/id_open_esp32.cpp`:
```cpp
esp_wifi_set_max_tx_power(78);  // Max value 80 (20dBm)
```

### Customize Flight Behavior

Edit `main/spoofer.cpp`:
```cpp
const double max_speed = 25;      // m/s
const double max_height = 200;    // meters
const double max_accel = 10000;   // mm/s²
```

### Switch Regional Standards

Edit `main/id_open.h`:
```cpp
// Enable/disable regional standards
#define ID_JAPAN          0      // Japan
#define ID_CHINA          1      // China CAAC
```

## 🌍 Regional Standards

### ASTM F3411-19 (USA/International)
- Default standard
- Uses WiFi Beacon or Bluetooth
- Compliant with FAA requirements

### ASD-STAN 4709-002 (Europe)
- Similar to ASTM with EU classifications
- Requires Category and Class settings
- Compliant with EASA regulations

### CAAC Remote ID (China)
- Requires 15-digit registration number
- Obtain from UOM system
- Compliant with Chinese civil aviation rules

**How to get CAAC Registration:**
1. Visit [UOM System](https://uom.caac.gov.cn)
2. Register and complete real-name verification
3. Receive your 15-digit registration code
4. Enter it in the web interface

## 📁 Project Structure

```
RemoteIDSpoofer/
├── main/
│   ├── app_main.cpp          # Main entry point
│   ├── frontend.cpp/h        # Web server & configuration UI
│   ├── spoofer.cpp/h         # Drone simulation core
│   ├── id_open.cpp/h         # OpenDroneID wrapper
│   ├── id_open_esp32.cpp     # ESP32-specific implementation
│   ├── id_open_china.cpp     # China CAAC standard (new)
│   ├── opendroneid.c/h       # OpenDroneID library
│   ├── utm.cpp/h             # UTM utilities
│   └── CMakeLists.txt        # Component build config
├── CMakeLists.txt            # Project build config
├── sdkconfig                 # ESP-IDF configuration
├── partitions.csv            # Partition table
└── README.md                 # This file
```

## 🐛 Troubleshooting

### Issue 1: Compilation fails with "threading_alt.h not found"

**Solution**:
```bash
idf.py fullclean
idf.py build
```

### Issue 2: Cannot connect to WiFi AP

**Check**:
- Verify SSID and password are correct
- Try restarting the device
- Ensure no other device is blocking the channel

### Issue 3: Apps don't detect the signal

**Possible causes**:
- Too far away (stay within 50m for testing)
- WiFi channel mismatch
- App doesn't support Beacon mode

**Solutions**:
- Move closer to the device
- Try different receiver apps
- Use Wireshark to verify transmission

### Issue 4: "Channel number out of range" error

**Error**: `Channel number XX is out of range`

**Solution**: 
The channel has been fixed to valid range (1-13). If you still see this, check your country code settings in `main/id_open_esp32.cpp`.

### Issue 5: NVS data lost after reboot

**Solution**:
```bash
# Erase entire flash
idf.py erase-flash
# Re-flash
idf.py flash monitor
```

## 📊 Performance

| Metric | Value |
|--------|-------|
| Update Frequency | 5 Hz |
| Max Simulated Drones | 16 |
| WiFi Transmit Power | Up to 20 dBm |
| Theoretical Range | 100-200 meters |
| RAM Usage | ~250 KB |
| Flash Usage | ~1.3 MB |
| Boot Time | < 5 seconds |

## 🔬 Technical Details

### Message Types
The device broadcasts these OpenDroneID messages:
- **Basic ID**: Aircraft type and identifier
- **Location**: Position, altitude, speed, heading
- **Self ID**: Text description
- **System**: Operator location and classification
- **Operator ID**: Operator registration code

### Data Encoding
- Uses OpenDroneID reference library
- Beacon frames contain ODID message packs
- Complies with ASTM F3411-19 format

### Flight Simulation
- Random walk algorithm for trajectory
- Maximum speed: 25 m/s (90 km/h)
- Maximum altitude: 200 meters
- Automatic boundary detection

## 🤝 Contributing

Contributions are welcome! Please feel free to submit Issues and Pull Requests.

### Development Workflow
```bash
# 1. Fork and clone
git clone https://github.com/your-username/RemoteIDSpoofer.git

# 2. Create a branch
git checkout -b feature/your-feature

# 3. Make changes and test

# 4. Commit
git commit -am 'Add new feature'

# 5. Push
git push origin feature/your-feature

# 6. Create Pull Request
```

### Code Style
- Follow ESP-IDF coding conventions
- Add necessary comments
- Test all modifications
- Update documentation

## 📄 License

This project is licensed under the **MIT License**

```
MIT License

Copyright (c) 2024 RemoteIDSpoofer Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction...
```

See [LICENSE](LICENSE) for full text.

## 🙏 Acknowledgments

Special thanks to:
- **[jjshoots](https://github.com/jjshoots)** - Original Arduino implementation
- **[OpenDroneID](https://github.com/opendroneid/opendroneid-core-c)** - Core encoding library
- **[Espressif](https://www.espressif.com/)** - ESP-IDF framework
- **[Steve Jack](https://github.com/sxjack/uav_electronic_ids)** - UAV electronic IDs research

## 📚 References

1. [ASTM F3411-19 Standard](https://www.astm.org/f3411-19.html)
2. [ASD-STAN 4709-002](https://asd-stan.org/downloads/asd-stan-4709-002/)
3. [CAAC Regulations](https://www.caac.gov.cn)
4. [OpenDroneID Specification](https://github.com/opendroneid/opendroneid-core-c)
5. [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
6. [Original Project](https://github.com/jjshoots/RemoteIDSpoofer)

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/your-username/RemoteIDSpoofer/issues)
- **Discussions**: [GitHub Discussions](https://github.com/your-username/RemoteIDSpoofer/discussions)

---

**⭐ Star this repo if you find it useful!**

**⚠️ Use responsibly and comply with local regulations.**