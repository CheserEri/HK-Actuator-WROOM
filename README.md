# HK-Actuator-WROOM

基于 PlatformIO 的 ESP32 舵机测试项目，当前适配开发板为 `ESP32-WROOM-32E`，测试舵机为 `SG90`。

当前程序功能：

- 使用 `D5`（`GPIO5`）输出舵机控制信号
- 舵机在 `0°` 和 `90°` 之间往复转动
- 每次转到目标角度后停留 `1` 秒
- 串口输出当前动作日志，波特率为 `115200`

## 项目文件

- `src/main.cpp`：当前舵机测试程序
- `platformio.ini`：PlatformIO 配置
- `README.md`：项目说明

## 硬件信息

- 开发板：`ESP32-WROOM-32E`
- 舵机：`SG90`
- 上传串口：`COM3`

## 接线说明

按 SG90 常见线色定义接线：

- 红色线 -> `VIN`
- 棕色线 -> `GND`
- 黄色线 -> `D5` / `GPIO5`

接线示意：

```text
ESP32 VIN  -------- SG90 红色
ESP32 GND  -------- SG90 棕色
ESP32 D5   -------- SG90 黄色
```

## 当前程序说明

程序启动后会先初始化串口和舵机，然后执行以下循环：

1. 舵机转到 `90°`
2. 等待 `1` 秒
3. 舵机回到 `0°`
4. 等待 `1` 秒
5. 持续循环

串口输出示例：

```text
SG90 servo sweep test started
Signal pin: GPIO5 (D5)
Servo will move between 0 and 90 degrees.
Servo moved to 0 degrees
Servo moved to 90 degrees
```

## 使用方法

在项目目录中打开 PowerShell：

```powershell
cd D:\Code\HardWareProjects\MiC3_Test
```

编译：

```powershell
$env:PLATFORMIO_CORE_DIR="$PWD\.pio-core"
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
```

上传：

```powershell
$env:PLATFORMIO_CORE_DIR="$PWD\.pio-core"
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run --target upload
```

打开串口监视器：

```powershell
$env:PLATFORMIO_CORE_DIR="$PWD\.pio-core"
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" device monitor
```

## 关键配置

`platformio.ini` 当前关键配置如下：

- 环境：`esp32dev`
- 框架：`arduino`
- 上传串口：`COM3`
- 串口波特率：`115200`
- 上传速度：`921600`
- 依赖库：`ESP32Servo`

## 注意事项

1. `SG90` 常见工作电压为 `4.8V ~ 6V`，负载稍大时可能超过开发板稳定供电能力。
2. 如果出现抖动、乱转、ESP32 重启，优先怀疑供电不足。
3. 更稳妥的做法是给舵机使用独立 `5V` 电源，但舵机电源地必须和 ESP32 的 `GND` 共地。
4. 不要把舵机信号线接到 `VIN` 或 `3.3V`。
5. ESP32 的 IO 是 `3.3V` 逻辑，SG90 一般可以正常识别。
6. 开发板复位和上传程序时，舵机偶发抖动属于常见现象。

## 常见问题

### 舵机不转

可能原因：

- 接线错误
- 供电不足
- 舵机损坏
- 信号线未接到 `D5`

### 舵机抖动但不正常转动

可能原因：

- 电流不足
- 没有共地
- 电源压降过大

### 上传失败

请检查：

- USB 数据线是否支持数据传输
- 设备管理器中的串口是否仍为 `COM3`
- 开发板是否正常上电
- 是否有其他串口工具占用了串口

## 仓库整理说明

当前仓库已做精简处理：

- `.pio` 和 `.pio-core` 不纳入版本控制
- `.vscode` 中自动生成的调试和 C/C++ 索引文件不纳入版本控制
- 保留 `.vscode/extensions.json` 作为推荐扩展配置
