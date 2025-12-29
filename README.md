# Qt6 Multithreaded Ping Tool

基于 Qt6 和 C++ 开发的高性能多线程 Ping 工具。

## 主要功能

*   **多线程架构**：每个 Ping 目标由独立线程管理，互不干扰，支持高并发。
*   **Windows 原生优化**：使用 `IcmpSendEcho` API，高效且无需管理员权限（通常情况）。
*   **实时统计**：实时计算并显示 RTT 的最小值、最大值、平均值和 TTL。
*   **历史记录数据库**：使用 SQLite 自动记录所有 Ping 结果，支持事务和批量写入以提高性能。
*   **交互式图表**：
    *   双击目标即可查看历史 RTT 趋势图。
    *   支持方波显示（Start -> Return），精准展示耗时段。
    *   超时记录以红色单独显示。
    *   支持缩放和平移时间轴。
    *   支持自动/手动调整纵轴范围。
*   **数据持久化**：自动保存和加载监控目标列表。

## 系统要求

*   **操作系统**：Windows 10/11 (依赖 Windows IPHLPAPI)
*   **开发环境**：
    *   Qt 6.x (Core, Gui, Widgets, Sql, Charts)
    *   C++17 兼容编译器 (推荐 MSVC 2019+)

## 构建说明

1.  确保已安装 Qt6 和对应的编译器。
2.  使用 Qt Creator 打开 `PingTool.pro`。
3.  配置项目（选择 Release 或 Debug 构建）。
4.  运行 `qmake`。
5.  构建项目。

或者在命令行中使用：
```powershell
qmake
nmake  # 或者 jom
```

## 使用说明

1.  **添加目标**：在上方输入框输入 IP 地址或域名，点击 "Add"。
2.  **设置超时**：在 "Timeout" 输入框设置超时时间（毫秒）。
3.  **开始/停止**：
    *   选中列表中的目标，点击 "Start All" 开始所有任务。
    *   点击 "Stop" 停止选中目标，或 "Stop All" 停止所有。
    *   点击 "Remove" 移除选中目标。
4.  **查看图表**：
    *   在主界面的目标列表（上半部分）中，**双击**某一行。
    *   在弹出的图表窗口中，点击 "Query Database" 查询历史数据。
    *   使用鼠标滚轮缩放时间轴，左键拖拽平移。
    *   勾选 "Auto Y" 自动调整高度，或取消勾选手动设置最大值。

## 目录结构

*   `src/`: 源代码目录
    *   `PingWorker`: 负责执行 Ping 操作的线程类。
    *   `PingManager`: 管理 PingWorker 的生命周期。
    *   `DatabaseThread`: 负责数据库异步写入的线程类。
    *   `ChartWindow`: 基于 Qt Charts 的图表显示窗口。
    *   `MainWindow`: 主界面逻辑。
*   `PingTool.pro`: qmake 项目文件。
