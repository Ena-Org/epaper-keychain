# epaper-keychain 指令与心跳对接文档

本文档用于 PC/前端（如 Vue3）与设备串口协议对接，覆盖：
- 二进制指令帧格式
- 命令 ID 与 payload 约定
- 心跳机制（`PING\n` / `PONG`）
- 最小交互时序

---

## 1. 传输参数

- 物理链路：串口（`Serial`）
- 建议波特率：`115200`（固件 `Serial.begin(115200)`）
- 数据流类型：**混合流**
  - 二进制协议帧（命令/响应/事件）
  - 明文心跳：`PING\n`（设备发出），`PONG`（主机应答）

---

## 2. 二进制帧格式

设备使用 `Codec::FrameHeader`（`#pragma pack(1)`）+ payload + CRC32：

```
+----------------+------+---------+-------+------+-----+----------+
| magic (u32 LE) | type | session | cmdId | code | len | payload  |
+----------------+------+---------+-------+------+-----+----------+
                                                  + crc32 (u32 LE)
```

- 字节序：**小端（LE）**
- `magic` 固定值：`0x31434443`
  - 在线路上的字节序（LE）通常是：`43 44 43 31`
- 固定头长度：`13` 字节
- 总帧长：`13 + len + 4`

字段定义：

- `type` (`uint8`)
  - `1`: Request
  - `2`: Response
  - `3`: Event
- `session` (`uint16`)：请求/响应配对 ID
- `cmdId` (`uint16`)：命令号
- `code` (`uint16`)：结果码（响应时有效）
  - `0` = 成功
  - 非 0 = 失败（当前代码中常用 `1` 表示参数错误）
- `len` (`uint16`)：payload 字节长度

CRC32 规则：

- 多项式：`0xEDB88320`
- 计算范围：`FrameHeader + payload`（不含尾部 crc 字段）
- 算法与固件 `Codec::checksum32` 一致

---

## 3. 命令定义

当前已注册命令（见 `include/cmd_ids.hpp` 与 handlers）：

1) `0x0001` Info
- 请求：`type=Request`，payload 可空
- 响应：`code=0`，payload 为 UTF-8 文本 `"ok"`

2) `0x0101` LogHistory
- 请求：payload 可空
- 响应：
  - `code=0`
  - payload 为日志历史字节流（可按 UTF-8 文本展示）
  - 长度上限受设备 `maxPayloadBytes` 限制

3) `0x0102` LogLevelSet
- 请求 payload：1 字节日志级别
  - `0..5`（Verbose）或 `255`（Off）
- 响应：
  - 参数合法：`code=0`，payload 回显 1 字节等级
  - 参数非法：`code=1`，payload 空

4) `0x0103` LogClear
- 请求：payload 可空
- 响应：`code=0`，payload 空

---

## 4. 心跳机制

设备在连接后会周期发送：

- `PING\n`（默认每 3000ms）

主机必须应答：

- `PONG`（建议发送 `PONG\n`，设备按子串 `PONG` 匹配）

超时策略（设备侧默认）：

- 心跳超时：10000ms
- 若等待 ACK 超时，设备进入错误并断开连接（随后可能自动重连）

### 对接注意

- 心跳是**明文**，不是二进制帧。
- 主机解析串口流时，必须先识别并剥离 `PING\n`，避免把它喂给二进制解码器。
- 设备端会在接收中识别 `PONG`，但仍可能保留原始字节给上层；主机侧建议严格区分心跳与协议帧，保证解析稳定。

---

## 5. 典型交互时序

### 5.1 Info 请求

1. 主机发送 `Request(cmd=0x0001, session=N)`
2. 设备返回 `Response(cmd=0x0001, session=N, code=0, payload="ok")`

### 5.2 心跳

1. 设备发送 `PING\n`
2. 主机立即回复 `PONG\n`
3. 双方继续业务帧收发

---

## 6. 主机实现建议

- 使用“流式解析”：
  - 先处理心跳明文
  - 再进行二进制帧解码（按 magic/len/crc）
- 对响应按 `(session, cmdId)` 关联，设置超时（如 3~5 秒）
- 收到 CRC 或 magic 异常时，按字节滑窗重同步
- 发送 payload 前校验长度（不超过设备允许上限）

---

## 7. 快速联调清单

- 打开串口后是否能看到 `PING`
- 是否每次都应答 `PONG`
- `Info` 命令是否返回 `ok`
- `LogLevelSet` 传非法等级时是否返回 `code=1`
- 长时间运行是否无心跳超时断连
