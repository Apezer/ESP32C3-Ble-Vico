# ESP32C3 BLE Vico Keyboard

Vico Keyboard 是一个基于 ESP32-C3 的五键 BLE 小键盘原型，用于 vibe coding / AI agent 控制场景。

当前版本功能：

- 5 个按键：↑、↓、←、→、Enter
- BLE HID Keyboard：通过蓝牙向电脑发送方向键和 Enter
- SSD1306 128×32 I2C OLED：显示 Vico / Claude Code 状态
- USB CDC Serial：电脑可以通过串口发送状态到 OLED
- Claude Code Hooks：将 Claude Code 生命周期事件映射为 OLED 状态
- Python companion：监听 `.vico/status.json` 并转发到 ESP32-C3 串口
- Claude Code logo 启动画面与状态界面小图标
- `THINKING` 状态下，Claude 小图标外框有流动线条动画

## Hardware

测试硬件：

- ESP32-C3 DevKitM-1
- SSD1306 128×32 I2C OLED
- 5 个按键，使用内部上拉，按下接 GND

默认引脚：

| 功能 | GPIO |
|---|---:|
| OLED SDA | GPIO6 |
| OLED SCL | GPIO7 |
| ↑ | GPIO2 |
| ↓ | GPIO3 |
| ← | GPIO4 |
| → | GPIO5 |
| Enter | GPIO8 |

## Firmware stack

- PlatformIO
- Arduino framework
- Adafruit SSD1306
- Adafruit GFX
- ESP32 BLE Keyboard

`platformio.ini` 中启用了 ESP32-C3 USB CDC：

```ini
build_flags =
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
```

这两个宏用于让 `Serial` 绑定到 ESP32-C3 的 USB Serial/JTAG 通道，否则电脑虽然能看到 COM 口，但向设备写入状态时可能会超时。

## Build

安装 PlatformIO 后，在仓库根目录运行：

```powershell
pio run
```

## Upload

将 `COM7` 替换为你的实际串口：

```powershell
pio run -t upload --upload-port COM7
```

查看串口：

```powershell
pio device list
```

注意：烧录时要先关闭 `companion.py`、`pio device monitor` 等占用串口的程序。

## BLE pairing

烧录后，电脑蓝牙中会看到设备：

```text
Vico Keyboard
```

配对后，五个按键会发送：

```text
↑ ↓ ← → Enter
```

## OLED status protocol

电脑可以通过 USB CDC 串口向 ESP32-C3 发送文本协议，控制 OLED 状态显示。

串口参数：

```text
115200 8-N-1
```

支持命令：

```text
STATE THINKING
TEXT Running Claude
SHOW DONE Build finished
```

说明：

| 命令 | 作用 |
|---|---|
| `STATE <state>` | 设置主状态，例如 `READY`、`THINKING`、`DONE`、`ERROR` |
| `TEXT <message>` | 设置底部消息 |
| `SHOW <state> <message>` | 同时设置状态和消息 |

常用状态：

```text
READY
THINKING
CONFIRM
WAITING
DONE
ERROR
```

`THINKING` 状态会启用流动边框动画。

## Manual serial test

可以用 Python / pyserial 向设备发送状态：

```python
import serial
import time

ser = serial.Serial("COM7", 115200, timeout=1)
time.sleep(2)
ser.write(b"STATE THINKING\n")
ser.write(b"TEXT Manual test\n")
ser.close()
```

OLED 应显示：

```text
THINKING
Manual test
```

## Companion

本仓库包含一个电脑端 companion：

```text
companion/companion.py
```

它会监听：

```text
.vico/status.json
```

当状态文件变化时，companion 会通过串口向 ESP32-C3 发送：

```text
STATE <state>
TEXT <message>
```

安装依赖：

```powershell
pip install pyserial
```

查看串口：

```powershell
python companion\companion.py --list
```

启动持续监听：

```powershell
python companion\companion.py --port COM7
```

只发送一次当前状态：

```powershell
python companion\companion.py --port COM7 --once
```

## Claude Code integration

本固件不直接读取 Claude Code。完整链路是：

```text
Claude Code Hook
  -> .vico/status.json
  -> companion/companion.py
  -> USB CDC Serial
  -> ESP32-C3 OLED
```

本仓库已包含 Claude Code 配置示例：

```text
.claude/
├── settings.json
└── hooks/
    └── vico-status.py
```

### 1. 让 Claude Code 读取 settings

把本仓库作为 Claude Code 项目打开：

```powershell
cd path\to\ESP32C3-MiniKeyboard
claude
```

在 Claude Code 里运行：

```text
/hooks
```

确认能看到 `SessionStart`、`UserPromptSubmit`、`Notification`、`Stop` 等 hooks。

### 2. 手动测试 hook

在仓库根目录运行：

```powershell
python .claude\hooks\vico-status.py THINKING "Manual test"
```

它会生成或更新：

```text
.vico/status.json
```

然后运行：

```powershell
python companion\companion.py --port COM7 --once
```

OLED 应显示：

```text
THINKING
Manual test
```

### 3. 自动显示 Claude Code 状态

保持 companion 运行：

```powershell
python companion\companion.py --port COM7
```

然后在 Claude Code 中提交 prompt。OLED 会根据 hooks 自动切换状态。

示例映射：

| Claude Code 事件 | OLED 状态 |
|---|---|
| SessionStart | READY |
| UserPromptSubmit | THINKING |
| Notification / permission_prompt | CONFIRM |
| Notification / idle_prompt | WAITING |
| Stop | DONE |
| PostToolUseFailure | ERROR |

## Project structure

```text
.
├── .claude/
│   ├── settings.json          # Claude Code hooks 配置
│   └── hooks/
│       └── vico-status.py     # 将 hook 事件写入 .vico/status.json
├── companion/
│   ├── companion.py           # 监听状态文件并转发到串口
│   └── README.md
├── include/
│   └── font.h      # OLED 图标声明与尺寸定义
├── lib/
├── src/
│   ├── font.c      # 蓝牙图标、Claude logo 位图
│   └── main.cpp    # 键盘、OLED、串口协议主逻辑
├── test/
├── platformio.ini
└── README.md
```

## Notes

- ESP32-C3 不支持原生 USB HID；当前键盘输入使用 BLE HID。
- USB 线只用于烧录、调试和接收 OLED 状态。
- 如果后续切换到 ESP32-S3，可以升级为 USB Composite：HID Keyboard + CDC Serial。
