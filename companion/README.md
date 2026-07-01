# Vico Companion MVP

这个小程序负责把 Claude Code hook 写入的 `.vico/status.json` 转发给键盘。

当前链路：

```text
Claude Code hook
  -> .vico/status.json
  -> companion.py
  -> USB 串口
  -> ESP32 OLED
```

## 安装依赖

```powershell
pip install pyserial
```

## 查看串口

Windows 可以在设备管理器里看开发板串口，例如 `COM5`。

也可以运行：

```powershell
python software\vico-companion\companion.py --list
```

## 启动

把 `COM5` 换成你的实际串口：

```powershell
python software\vico-companion\companion.py --port COM5
```

如果只想发一次当前状态：

```powershell
python software\vico-companion\companion.py --port COM5 --once
```

## 手动测试

先启动 companion，然后在另一个终端运行：

```powershell
python .claude\hooks\vico-status.py THINKING "Manual test"
```

OLED 应显示：

```text
THINKING
Manual test
```
