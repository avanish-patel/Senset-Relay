# 🌅 Sunset Relay Controller

Automatic relay controller that turns ON at sunset and OFF at scheduled times - with per-day scheduling!

Perfect for outdoor lights, holiday decorations, or any automation that needs to follow sunset patterns with flexible schedules.

## ✨ Features

- 🌇 **Automatic Sunset Tracking** - Fetches daily sunset times from API
- 📅 **Per-Day Scheduling** - Different turn-off times for each day of the week
- 🌐 **Web Interface** - Easy configuration via browser (no coding required!)
- 💾 **Persistent Storage** - Saves settings permanently (survives power loss)
- 🔄 **Daily Updates** - Automatically adjusts to changing sunset times
- ⏱️ **Configurable Delay** - Turn on X minutes after sunset

## 📦 Hardware Required

- **ESP32-C3 Super Mini** (or ESP32-C3 DevKit)
- **Relay Module** (3.3V or 5V, 1-channel)
- **USB-C Cable** (for power and programming)
- **3 Jumper Wires**

### Wiring

```
ESP32-C3      Relay Module
--------      ------------
GPIO 2   -->  IN
3.3V     -->  VCC
GND      -->  GND
```

## 🚀 Quick Start

### 1. Install PlatformIO

- Install [Visual Studio Code](https://code.visualstudio.com/)
- Install PlatformIO IDE extension from VS Code marketplace

### 2. Clone & Build

```bash
git clone https://github.com/yourusername/sunset-relay-controller.git
cd sunset-relay-controller
pio run
```

### 3. Upload to ESP32-C3

```bash
pio run --target upload
```

### 4. Configure via Web Interface

1. Connect to WiFi: `SunsetRelay-Setup` (password: `12345678`)
2. Open browser: `http://192.168.4.1`
3. Enter your WiFi credentials
4. Set your location (latitude/longitude)
5. Configure sunset delay (minutes after sunset)
6. Set turn-off times for each day of the week
7. Click "Test Sunset API" to verify
8. Click "Save Configuration"

### 5. Access from Home Network

After configuration, ESP32 connects to your WiFi. Access the web interface at the IP address shown in serial monitor or found in your router.

## 📅 Example Schedules

### Church on Sunday
```
Sunday:    17:00 (5:00 PM)  - Early for evening service
Mon-Sat:   20:00 (8:00 PM)  - Regular schedule
```

### Weekend Late Nights
```
Sun-Thu:   20:00 (8:00 PM)  - Weekdays
Fri-Sat:   23:00 (11:00 PM) - Weekend fun!
```

### Energy Saving
```
Sunday:    18:00 (6:00 PM)  - Save most
Mon-Wed:   19:00 (7:00 PM)  - Moderate
Thu-Sat:   20:00 (8:00 PM)  - Normal
```

## 🌐 Web Interface

The web interface provides:

- ✅ Real-time relay status (ON/OFF with indicator)
- ✅ Current time and day of week
- ✅ Next sunset time
- ✅ Scheduled relay ON/OFF times
- ✅ Configuration for all settings
- ✅ API test functionality
- ✅ "Set All Days to Same Time" quick action

Auto-refreshes every 5 seconds!

## 🛠️ Configuration

### WiFi Settings
- **SSID**: Your WiFi network name
- **Password**: Your WiFi password (2.4 GHz only)

### Location
- **Latitude**: Your location's latitude (e.g., `41.7197`)
- **Longitude**: Your location's longitude (e.g., `-87.7479`)

Find coordinates: [Google Maps](https://maps.google.com) (right-click location) or [LatLong.net](https://www.latlong.net/)

### Timing
- **Sunset Delay**: Minutes after sunset to turn ON (0-240)
- **Turn-Off Schedule**: Hour and minute for each day (24-hour format)

## 📖 How It Works

1. **Midnight (00:00)**: Fetches today's sunset time from [sunrise-sunset.org API](https://sunrise-sunset.org/api)
2. **Sunset + Delay**: Relay turns **ON** (GPIO 2 HIGH)
3. **Scheduled Time**: Relay turns **OFF** based on current day's schedule (GPIO 2 LOW)
4. **Repeat**: Process repeats daily with updated sunset times

## 🔧 Troubleshooting

### Can't Upload Code
- Hold **BOOT button** on ESP32-C3 while uploading
- Try different USB cable (must be data cable)
- Install [CH340 drivers](https://www.wch.cn/downloads/CH341SER_EXE.html)

### WiFi Not Connecting
- Ensure WiFi is **2.4 GHz** (ESP32-C3 doesn't support 5 GHz)
- Check credentials are exact (case-sensitive)
- Move closer to router

### Wrong Times
- Wait 60 seconds for NTP time sync after WiFi connection
- Timezone is set to CST (UTC-6) - modify in code if needed
- Verify location coordinates are correct

### Relay Not Switching
- Check wiring: GPIO2 → IN, 3.3V → VCC, GND → GND
- Some relay modules need 5V instead of 3.3V
- Verify relay module LED lights up
- Listen for relay "click" sound

## 📚 Dependencies

All dependencies are automatically installed via PlatformIO:

- **ArduinoJson** (^6.21.3) - JSON parsing for API and web interface

Built with:
- Arduino framework
- ESP32 Arduino Core
- Standard ESP32 libraries (WiFi, WebServer, HTTPClient, Preferences)

## 🔒 Storage

Configuration is stored in ESP32's NVS (Non-Volatile Storage):
- Survives power loss and reboots
- Automatically saved when you click "Save Configuration"
- Includes WiFi credentials, location, and all 7-day schedule

## ⚙️ Advanced Configuration

### Change Timezone

Edit this line in `main.cpp`:
```cpp
configTime(-6 * 3600, 0, "pool.ntp.org", "time.nist.gov");
```

Change `-6` to your timezone offset:
- EST: `-5`
- MST: `-7`
- PST: `-8`
- UTC: `0`

### Change Relay Pin

Edit this line:
```cpp
#define RELAY_PIN 2  // Change to desired GPIO
```

### Change Access Point Credentials

Edit this line:
```cpp
WiFi.softAP("SunsetRelay-Setup", "12345678");
```

## 📝 PlatformIO Commands

```bash
# Build project
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor

# Clean build
pio run --target clean

# Erase flash (factory reset)
pio run --target erase
```

## 🎯 Use Cases

- 🏠 **Outdoor Lighting** - Automatic dusk-to-dawn with custom schedules
- 🎄 **Holiday Decorations** - Set and forget seasonal lighting
- 🔒 **Security Lighting** - Make home look occupied with varied schedules
- ⛪ **Religious Observance** - Early turn-off on specific days
- 🏢 **Business Hours** - Different schedules for weekdays/weekends
- 💡 **Smart Home** - Integrate with existing automation

## ⚠️ Safety Warning

**When connecting to mains voltage (110V/220V AC):**
- Always disconnect power before wiring
- Use proper insulation and wire ratings
- Relay must be rated for your load
- Consider hiring a qualified electrician
- Include proper fuses in circuit

## 📄 License

MIT License - feel free to use, modify, and distribute!

## 🤝 Contributing

Contributions welcome! Please feel free to submit a Pull Request.

## 💬 Support

- **Issues**: Open an issue on GitHub
- **Discussions**: Use GitHub Discussions for questions

## 🌟 Star This Project

If you find this useful, please give it a star! ⭐

---

**Built with ❤️ for home automation enthusiasts**

*Automatic sunset tracking meets flexible scheduling - set it once, enjoy forever!*