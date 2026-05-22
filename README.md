# qt-visa-controller

基于 Qt 的仪器控制软件，提供一个统一的桌面 GUI 来：

- 控制 **Rigol DP 系列直流稳压源**（LAN，SCPI Raw Socket 5025 端口）
- 控制 **SmartUSBHub 4 端口可编程 USB 集线器**（USB CDC 虚拟串口）

> 历史版本依赖 NI-VISA / VXI-11。当前版本**完全脱离 NI-VISA**，仅依赖 Qt
> 自带的 `QTcpSocket` 与 `QSerialPort`，可在 Windows / Linux / macOS 上原生
> 编译运行。

---

## 一、功能

### DC Power（标签页 "DC Power"）
- LAN 自动扫描（UDP portmap 广播 + 异步 `*IDN?` 探测，扫描期间 UI 不卡）。
- 通道 1/2/3 各自支持：
  - 输出 ON/OFF
  - 设置 / 读取 电压、电流、过压保护、过流保护
  - 过压 / 过流保护开关
  - 周期性测量电压 / 电流 / 功率（250 ms）
- 仪器断网自动恢复 UI 到 “未连接” 状态。

### USB Hub（标签页 "USB Hub"）
- 串口枚举 + 一键刷新，若设置了 `kHubVid/kHubPid` 会高亮匹配项。
- 4 个通道每个支持：
  - 电源 ON/OFF（按钮颜色 = 状态）
  - 数据线 通/断（按钮颜色 = 状态）
  - 电压 (mV) / 电流 (mA) 实时刷新（500 ms）
- 工作模式：普通 / 互锁（互锁下电源切换自动调用 CMD 0x02）。
- “刷新状态”按钮可一次性同步全部通道与模式。

---

## 二、依赖

| 项 | 要求 |
|---|---|
| Qt | Qt 5.12+ 或 Qt 6（推荐 Qt 6） |
| 模块 | `core` `gui` `widgets` `network` `serialport` |
| 编译器 | 支持 C++17（GCC 9+ / MSVC 2019+ / Clang 10+） |
| 操作系统 | Windows / Linux / macOS |

Linux 上额外安装：
```
sudo apt install qtbase5-dev libqt5serialport5-dev qt5-qmake
```

> 不再需要 NI-VISA、VXI-11 或任何 IVI / IO 库。

---

## 三、编译与运行

```bash
qmake
make -j$(nproc)
./Visactl
```

Windows（Qt MaintenanceTool 安装 Qt + MinGW 或 MSVC 后）：

```
qmake
mingw32-make            # 或 nmake / jom
Visactl.exe
```

Qt Creator 直接打开 `Visactl.pro` 也可以构建运行。

---

## 四、设备使用提示

### DC Power（Rigol DP 系列）
1. 将仪器接入与本机同子网的 LAN（建议同一交换机）。
2. 点击 **SCAN LAN**，几秒内可用 IP 会被填进下拉框，仪器型号包含 `DP` 字样。
3. 选择条目 → **连接** → 自动读取当前参数。
4. 修改参数后按 **设置参数** 写回；周期测量自动开始。
5. **断开** 或拔掉网线均会让 UI 自动复位。

### SmartUSBHub
1. 用 USB 线连接设备到 PC，操作系统自动识别为 CDC 串口（Windows: `COMx`，Linux: `/dev/ttyACMx`，macOS: `/dev/cu.usbmodemx`）。
2. 点击 **刷新串口**，选择对应端口，**连接**。
3. 切换 “普通 / 互锁” 模式；通道按钮控制电源 / 数据；电压、电流自动每 500 ms 刷新。
4. 互锁模式下：CMD 0x01 无效，软件已自动改用 CMD 0x02，保证同一时刻只有一个通道开启。

> **VID/PID 自动识别**：若你的设备有固定 VID/PID，请在 `mainwindow.cpp` 顶部修改
> `kHubVid` / `kHubPid` 后重新编译，匹配的串口会自动选中并标注 `[SmartUSBHub]`。

---

## 五、协议参考

- DC Power：**SCPI Raw Socket（TCP 端口 5025）**，命令直接用 Rigol DP 手册中的 SCPI 字符串，例如 `:INST CH1`、`:VOLT 5`、`:MEAS:ALL? CH1`。
- USB Hub：见 `../SmartUSBHub.md`（项目仓库外的设备厂商规格）。简要：
  - 帧格式 `0x55 0x5A CMD DATA... SUM8`，6 或 7 字节。
  - 通道位掩码 CH1=0x01 / CH2=0x02 / CH3=0x04 / CH4=0x08。
  - 设备按键按下会**主动上报**当前状态（CMD 0x00 帧），本程序的读取路径会自动丢弃 CMD 不匹配的帧，避免错位。

---

## 六、目录结构

```
Visactl.pro                 # qmake 工程
main.cpp                    # 入口
mainwindow.h / .cpp / .ui   # 主窗口（DC Power + USB Hub 两个 tab）
scpiclient.h / .cpp         # SCPI 长连接客户端（QTcpSocket + 流式读取）
smartusbhubclient.h / .cpp  # SmartUSBHub 串口客户端（QSerialPort）
```

---

## 七、License

待定（项目作者自行选择，例如 MIT / Apache-2.0）。
