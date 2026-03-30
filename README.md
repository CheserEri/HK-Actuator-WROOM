# MiC3_Test

基于 PlatformIO 的 ESP32 舵机测试项目，当前目标开发板为 `ESP32-WROOM-32E`，测试外设为 `SG90` 舵机。

当前程序功能：

- 使用 `D5`（即 `GPIO5`）作为舵机控制信号输出
- 舵机在 `0°` 和 `90°` 之间往复转动
- 每次转到目标角度后停留 `1` 秒
- 串口波特率为 `115200`

## 硬件信息

- 开发板：`ESP32-WROOM-32E`
- 舵机型号：`SG90`
- 上传串口：`COM3`

## 接线说明

按 SG90 常见线色定义接线：

- 红色线 -> `VIN`
- 棕色线 -> `GND`
- 黄色线 -> `D5` / `GPIO5`

建议接线示意：

```text
ESP32 VIN  -------- SG90 红色
ESP32 GND  -------- SG90 棕色
ESP32 D5   -------- SG90 黄色
```

## 重要注意事项

1. `SG90` 常见工作电压为 `4.8V ~ 6V`，如果舵机负载较大，直接由开发板供电可能出现抖动、复位、转动异常。
2. 如果舵机动作不稳定，建议改用独立 `5V` 电源给舵机供电。
3. 使用独立电源时，舵机电源地和 ESP32 的 `GND` 必须共地。
4. 不要把舵机信号线接到 `VIN` 或 `3.3V`。
5. `ESP32` 的 IO 逻辑电平为 `3.3V`，SG90 一般可以识别该控制信号。
6. 上传程序时如果舵机抖动属于常见现象，因为开发板复位时引脚状态会变化。

## 当前程序说明

程序文件位置：

- [src/main.cpp](D:\Code\HardWareProjects\MiC3_Test\src\main.cpp)

程序逻辑：

1. 初始化串口 `115200`
2. 初始化舵机库，绑定 `GPIO5`
3. 上电后先转到 `0°`
4. 循环在 `0°` 和 `90°` 之间切换
5. 每次切换后串口输出当前角度

## 项目配置

项目配置文件位置：

- [platformio.ini](D:\Code\HardWareProjects\MiC3_Test\platformio.ini)

当前关键配置：

- 开发板环境：`esp32dev`
- 框架：`arduino`
- 上传串口：`COM3`
- 串口监视器波特率：`115200`
- 上传速度：`921600`
- 库依赖：`ESP32Servo`

## 使用方法

### 1. 连接硬件

按上面的接线说明连接好 `ESP32` 和 `SG90`。

### 2. 打开 PowerShell

在项目目录 `D:\Code\HardWareProjects\MiC3_Test` 中打开 PowerShell。

### 3. 编译程序

```powershell
$env:PLATFORMIO_CORE_DIR="$PWD\.pio-core"
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
```

### 4. 上传程序

```powershell
$env:PLATFORMIO_CORE_DIR="$PWD\.pio-core"
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run --target upload
```

### 5. 查看串口输出

```powershell
$env:PLATFORMIO_CORE_DIR="$PWD\.pio-core"
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" device monitor
```

## 预期现象

程序上传成功后：

- 舵机会先转到 `0°`
- 然后转到 `90°`
- 再回到 `0°`
- 持续往复运动

串口会输出类似内容：

```text
SG90 servo sweep test started
Signal pin: GPIO5 (D5)
Servo will move between 0 and 90 degrees.
Servo moved to 0 degrees
Servo moved to 90 degrees
```

## 常见问题

### 1. 舵机不转

可能原因：

- 接线错误
- 电源不足
- 舵机损坏
- 信号线没有接到 `D5`

### 2. 舵机抖动但不正常转动

可能原因：

- 供电电流不足
- 地线没有共地
- 电源压降过大

建议优先检查供电。

### 3. 上传失败

请检查：

- USB 数据线是否支持传输
- 设备管理器中串口是否仍然是 `COM3`
- 开发板是否正常上电
- 是否有其他串口工具占用了串口

## 后续可扩展方向

- 修改为指定角度控制
- 改为按键触发转动
- 增加 Web 控制
- 增加串口命令控制
- 控制多个舵机
