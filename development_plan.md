# CyclingDataAnalyzer 开发文档

## 1. 项目定位

项目名称：

```text
CyclingDataAnalyzer
基于 Qt 的星闪骑行数据离线分析与事件复盘工具
```

项目目标：

```text
对接 OneNET 平台历史属性和事件接口，将 StarGuard 智能骑行系统的骑行数据同步到本地数据库，
实现骑行记录管理、曲线分析、GPS 离线轨迹、事件复盘、统计摘要和数据导出。
```

这个项目不是云端实时监控页面的重复。OneNET 负责在线查看，CyclingDataAnalyzer 负责离线复盘、统计分析和数据沉淀。

## 2. 技术决策

### 2.1 数据库选择

第一版使用：

```text
SQLite
```

原因：

- 项目定位是本地离线分析工具，SQLite 最贴合。
- 不需要用户安装 MySQL 服务，方便打包和演示。
- Qt 原生支持 `QSQLITE`。
- 一个 `.db` 文件即可保存历史骑行数据，适合 GitHub 项目和简历展示。
- 离线工具通常强调隐私和本地可用性，SQLite 比 MySQL 更自然。

MySQL 是否值得用：

```text
不建议第一版使用 MySQL。
```

如果你想练 MySQL，可以作为第二阶段扩展：

```text
数据库适配层：SQLite 本地模式 / MySQL 多设备共享模式
```

但 MVP 先用 SQLite。否则项目部署成本会变高，和“离线分析工具”的定位冲突。

### 2.2 GPS 轨迹图

第一版实现：

```text
无地图背景的 GPS 简易轨迹图
```

实现方式：

- 从 `latitude` / `longitude` 历史数据中取轨迹点。
- 计算经纬度最大最小范围。
- 将经纬度归一化到 QWidget 绘图区。
- 使用 `QPainter` 绘制轨迹折线。
- 标记起点、终点、跌倒/SOS 事件点。

为什么不第一版使用地图背景：

- 在线地图需要 API key、网络、坐标系转换和授权处理。
- `QWebEngineView` 会增加依赖和打包复杂度。
- 离线分析工具第一版更适合做无地图背景的轨迹复盘。

后续可选：

```text
QWebEngineView + 高德/百度/Leaflet 地图
或离线瓦片地图
```

### 2.3 是否需要多线程

需要，但不要过度设计。

建议线程模型：

```text
UI 主线程
├── 主窗口、图表、表格、轨迹绘制
│
└── SyncWorker 线程
    ├── OneNET HTTP 请求调度
    ├── 多属性/多事件分页同步
    ├── SQLite 批量写入
    └── 统计计算
```

原因：

- 同步多个属性和事件可能需要多次 HTTP 请求。
- SQLite 批量写入和统计计算可能卡 UI。
- 图表刷新、表格展示必须留在 UI 线程。

注意：

```text
SQLite 连接不能跨线程共用。
```

如果数据库写入放在 worker 线程，需要在 worker 线程内创建自己的数据库连接，或通过数据库服务类统一封装。

## 3. OneNET 数据接口

### 3.1 历史属性接口

用于查询心率、血氧、速度、温湿度、GPS、里程等历史属性。

```http
GET https://iot-api.heclouds.com/thingmodel/query-device-property-history
```

请求参数：

| 参数 | 必选 | 说明 |
| --- | --- | --- |
| `product_id` | 是 | 产品 ID |
| `device_name` | 是 | 设备名称，例如 `dev1` |
| `identifier` | 是 | 属性标识符 |
| `start_time` | 是 | 开始时间，毫秒时间戳 |
| `end_time` | 是 | 结束时间，毫秒时间戳 |
| `sort` | 否 | 排序，建议正序 |
| `offset` | 否 | 分页偏移 |
| `limit` | 否 | 每页数量 |

请求头：

```http
authorization: <OneNET token>
Accept: application/json
```

返回数据结构：

```json
{
  "code": 0,
  "data": {
    "list": [
      {
        "time": 1780466677032,
        "value": "78"
      }
    ]
  },
  "msg": "succ"
}
```

需要同步的属性：

| 属性 | 标识符 |
| --- | --- |
| 心率 | `heart_rate` |
| 血氧 | `spo2` |
| 速度 | `speed` |
| 温度 | `temperature` |
| 湿度 | `humidity` |
| 纬度 | `latitude` |
| 经度 | `longitude` |
| 单次里程 | `trip_mileage` |
| 总里程 | `Total_Mileage` |

### 3.2 历史事件接口

用于查询骑行开始、骑行结束、跌倒/SOS 等事件。

```http
GET https://iot-api.heclouds.com/device/event-log
```

请求参数：

| 参数 | 必选 | 说明 |
| --- | --- | --- |
| `product_id` | 二选一 | 产品 ID |
| `device_name` | 二选一 | 设备名称 |
| `imei` | 二选一 | NB 老平台设备使用 |
| `start_time` | 是 | 查询起始时间，毫秒时间戳 |
| `end_time` | 是 | 查询截止时间，毫秒时间戳 |
| `identifier` | 否 | 事件标识符 |
| `event_type` | 否 | 1 信息，2 告警，3 故障 |
| `offset` | 否 | 请求起始位置 |
| `limit` | 否 | 每页数量，范围 1-100 |

返回示例：

```json
{
  "code": 0,
  "data": {
    "list": [
      {
        "event_type": 1,
        "identifier": "ride_start",
        "name": "单次骑行开始事件",
        "time": 1780466677032,
        "value": "{\"longitude\":113.93456,\"latitude\":22.54321}"
      }
    ],
    "limit": 10,
    "offset": 0
  },
  "msg": "succ"
}
```

计划支持事件：

| 事件 | 标识符 | 类型 | 参数 |
| --- | --- | --- | --- |
| 单次骑行开始 | `ride_start` | 信息 | `longitude`, `latitude` |
| 单次骑行结束 | `ride_end` | 信息 | `longitude`, `latitude`, `trip_mileage` |
| 跌倒/SOS 告警 | `fall_warning` | 告警 | `longitude`, `latitude`, `heart_rate` |

## 4. MVP 功能范围

第一版必须实现：

1. OneNET 配置页：`product_id`、`device_name`、authorization token、时间范围。
2. 历史属性同步：同步心率、血氧、速度、温湿度、GPS、里程。
3. 历史事件同步：同步 `ride_start`、`ride_end`、`fall_warning`。
4. SQLite 本地存储。
5. 骑行记录列表。
6. Dashboard 骑行概要。
7. 曲线分析：心率、速度、血氧、温湿度。
8. GPS 简易轨迹图：起点、终点、事件点。
9. 事件复盘列表。
10. 原始数据表格。
11. CSV 导出。

第二版可扩展：

1. PDF 报告导出。
2. 图表导出 PNG。
3. 周/月统计报表。
4. 地图背景轨迹。
5. MySQL 共享数据库模式。

## 5. 数据库设计

MVP 使用 4 张核心表：

```text
devices
rides
sensor_data
events
```

### 5.1 devices

```sql
CREATE TABLE IF NOT EXISTS devices (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    product_id TEXT NOT NULL,
    device_name TEXT NOT NULL,
    display_name TEXT,
    remark TEXT,
    created_at TEXT NOT NULL,
    last_sync_time TEXT,
    UNIQUE(product_id, device_name)
);
```

### 5.2 rides

```sql
CREATE TABLE IF NOT EXISTS rides (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id INTEGER NOT NULL,
    title TEXT,
    start_time TEXT NOT NULL,
    end_time TEXT NOT NULL,
    duration_seconds INTEGER DEFAULT 0,
    start_latitude REAL,
    start_longitude REAL,
    end_latitude REAL,
    end_longitude REAL,
    trip_mileage REAL DEFAULT 0,
    avg_heart_rate REAL DEFAULT 0,
    max_heart_rate INTEGER DEFAULT 0,
    avg_speed REAL DEFAULT 0,
    max_speed REAL DEFAULT 0,
    avg_spo2 REAL DEFAULT 0,
    min_spo2 INTEGER DEFAULT 0,
    avg_temperature REAL DEFAULT 0,
    avg_humidity REAL DEFAULT 0,
    event_count INTEGER DEFAULT 0,
    created_at TEXT NOT NULL,
    FOREIGN KEY(device_id) REFERENCES devices(id)
);
```

### 5.3 sensor_data

宽表，方便曲线、统计和轨迹查询。

```sql
CREATE TABLE IF NOT EXISTS sensor_data (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ride_id INTEGER NOT NULL,
    device_id INTEGER NOT NULL,
    timestamp_ms INTEGER NOT NULL,
    time_text TEXT NOT NULL,
    heart_rate INTEGER,
    spo2 INTEGER,
    speed REAL,
    temperature REAL,
    humidity REAL,
    latitude REAL,
    longitude REAL,
    trip_mileage REAL,
    total_mileage REAL,
    created_at TEXT NOT NULL,
    FOREIGN KEY(ride_id) REFERENCES rides(id),
    FOREIGN KEY(device_id) REFERENCES devices(id),
    UNIQUE(ride_id, timestamp_ms)
);
```

说明：

- OneNET 属性是分别查询的，不同属性时间戳可能不完全一致。
- 入库时可以按时间戳近似合并，例如同一秒内的数据归为一条 sample。
- 如果需要保留完整原始点，第二版可增加 `raw_property_points` 表。

### 5.4 events

```sql
CREATE TABLE IF NOT EXISTS events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ride_id INTEGER,
    device_id INTEGER NOT NULL,
    timestamp_ms INTEGER NOT NULL,
    time_text TEXT NOT NULL,
    identifier TEXT NOT NULL,
    name TEXT,
    event_type INTEGER,
    level TEXT,
    value_json TEXT,
    latitude REAL,
    longitude REAL,
    heart_rate INTEGER,
    trip_mileage REAL,
    message TEXT,
    created_at TEXT NOT NULL,
    FOREIGN KEY(ride_id) REFERENCES rides(id),
    FOREIGN KEY(device_id) REFERENCES devices(id)
);
```

## 6. UI 页面设计

### 6.1 主页面结构

```text
左侧：骑行记录列表
右侧：页面内容
顶部：同步 / 导出 / 设置
底部：状态栏
```

页面：

```text
Dashboard
Charts
Track
Events
Raw Data
Settings
```

### 6.2 Dashboard

展示：

- 骑行开始/结束时间。
- 骑行时长。
- 单次里程。
- 平均/最大心率。
- 平均/最大速度。
- 平均血氧。
- 平均温湿度。
- 事件数量。

### 6.3 Charts

曲线：

- 心率曲线。
- 速度曲线。
- 血氧曲线。
- 温度曲线。
- 湿度曲线。

使用：

```text
Qt Charts
```

第一版支持：

- 缩放。
- 重置视图。
- 鼠标悬停显示数据。

### 6.4 Track

GPS 简易轨迹页面。

实现：

- 自定义 `TrackWidget : QWidget`。
- 使用 `QPainter` 画轨迹折线。
- 标记起点：绿色。
- 标记终点：蓝色。
- 标记 `fall_warning`：红色。
- 显示轨迹点数量和经纬度范围。

第一版不带地图背景。

### 6.5 Events

展示事件列表：

| 时间 | 事件 | 类型 | 心率 | 经纬度 | 里程 |
| --- | --- | --- | --- | --- | --- |

支持点击事件后：

- 显示事件详情。
- 高亮轨迹点。
- 后续可联动曲线定位到事件时间点。

### 6.6 Raw Data

展示 `sensor_data` 表。

支持：

- 按时间排序。
- 按时间范围过滤。
- 导出 CSV。

## 7. 模块设计

```text
src
├── app
│   └── AppContext
├── database
│   ├── AppDatabase
│   ├── DeviceRepository
│   ├── RideRepository
│   ├── SensorDataRepository
│   └── EventRepository
├── network
│   └── OnenetClient
├── service
│   ├── SyncWorker
│   ├── RideBuilder
│   ├── AnalysisService
│   └── ExportService
├── ui
│   ├── MainWindow
│   ├── DashboardPage
│   ├── ChartsPage
│   ├── TrackWidget
│   ├── EventsPage
│   └── SettingsDialog
└── model
    ├── Device
    ├── Ride
    ├── SensorSample
    └── RideEvent
```

### 7.1 OnenetClient

职责：

- 请求历史属性接口。
- 请求历史事件接口。
- 解析 JSON。
- 不直接操作 UI。

### 7.2 SyncWorker

职责：

- 在后台线程执行同步。
- 遍历属性和事件。
- 写入 SQLite。
- 发出进度信号。

### 7.3 RideBuilder

职责：

- 根据 `ride_start` / `ride_end` 创建骑行记录。
- 如果没有事件，则按用户选择时间范围创建 ride。
- 将 sensor_data 和 events 关联到 ride。

### 7.4 AnalysisService

职责：

- 计算概要统计。
- 更新 rides 表中的统计字段。

### 7.5 ExportService

职责：

- 导出 CSV。
- 第二版支持 PDF。

## 8. 数据同步流程

```text
用户选择时间范围
        ↓
点击同步
        ↓
SyncWorker 启动
        ↓
请求事件接口 event-log
        ↓
根据 ride_start / ride_end 切分 ride
        ↓
请求各属性历史数据
        ↓
按时间合并为 sensor_data
        ↓
写入 SQLite
        ↓
计算统计摘要
        ↓
刷新 UI
```

如果没有查询到 `ride_start` / `ride_end`：

```text
使用用户选择的 start_time / end_time 创建一条 ride
```

## 9. 开发顺序

### 阶段 1：项目骨架

- 创建 Qt Widgets + CMake 项目。
- 添加 Qt Network / Sql / Charts。
- 搭建 MainWindow。
- 搭建页面切换。

### 阶段 2：数据库

- 实现 SQLite 初始化。
- 创建 4 张表。
- 写入测试数据。
- QTableView 展示数据。

### 阶段 3：OneNET 属性同步

- 实现历史属性接口请求。
- 同步 `heart_rate`。
- 扩展到多属性。
- 写入 `sensor_data`。

### 阶段 4：OneNET 事件同步

- 实现 `device/event-log` 请求。
- 同步 `ride_start`、`ride_end`、`fall_warning`。
- 写入 `events`。
- 生成 `rides`。

### 阶段 5：Dashboard + Charts

- 展示统计卡片。
- 画心率/速度曲线。
- 画温湿度/血氧曲线。

### 阶段 6：GPS 轨迹

- 实现 `TrackWidget`。
- 绘制轨迹折线。
- 标记起点、终点、事件点。

### 阶段 7：导出和收尾

- CSV 导出。
- QSS 美化。
- README。
- 截图。
- 简历描述。

## 10. MVP 验收标准

- 能配置 OneNET 产品 ID、设备名和 token。
- 能同步至少 5 个属性历史数据。
- 能同步 `ride_start` / `ride_end` / `fall_warning` 事件。
- 能保存到 SQLite。
- 能生成骑行记录。
- 能显示 Dashboard。
- 能显示心率和速度曲线。
- 能显示 GPS 简易轨迹图。
- 能显示事件列表。
- 能导出 CSV。
