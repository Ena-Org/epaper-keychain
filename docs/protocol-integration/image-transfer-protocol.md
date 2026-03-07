# 图片传输协议规范（Browser -> ESP32）

## 1. 目的

本文档定义浏览器向 ESP32 发送图片的二进制协议细节，包含：
- 命令 ID 分配
- payload 字节布局
- ACK/重传规则
- 状态机与超时建议
- 联调与验收标准

本文档基于现有工程协议帧格式（LE 小端）与心跳机制（`PING/PONG`）。

## 2. 依赖与约束

- 传输层：串口混合流（二进制帧 + 明文心跳）。
- 字节序：小端（LE）。
- 编解码限制：
  - `maxPayloadBytes = 4096`
  - `maxBufferBytes = 8192`
- 心跳不可中断，传图期间必须持续应答。

## 3. 命令 ID 分配建议

建议将图片传输命令分配到 `0x0200` 段：

- `0x0201` `ImageBegin`
- `0x0202` `ImageChunk`
- `0x0203` `ImageEnd`
- `0x0204` `ImageApply`
- `0x0205` `ImageAbort`
- `0x0281` `ImageProgress`（可选事件，上报接收进度）

说明：
- `Packet.session`（帧头字段）用于请求-响应配对。
- 图片传输会话建议使用 payload 内 `txId`（`uint32`）区分，支持未来扩展并发/恢复。

## 4. 通用约定

1. 所有字段均为 LE。
2. 所有请求均要求响应，响应 `code=0` 代表成功。
3. 第一版采用停等协议：每个 `ImageChunk` 必须收到成功响应后再发下一片。
4. 若收到非 `0` 响应码，浏览器按规则重试或中止。

## 5. Payload 字节布局

## 5.1 `ImageBegin` (`cmdId=0x0201`)

请求 payload（固定 24 字节）：

| 偏移 | 类型     | 字段名         | 说明 |
|---|---|---|---|
| 0  | `uint32` | `txId`         | 传输会话 ID |
| 4  | `uint16` | `width`        | 图片宽 |
| 6  | `uint16` | `height`       | 图片高 |
| 8  | `uint8`  | `pixelFormat`  | 1=1bpp, 2=2bpp（预留） |
| 9  | `uint8`  | `rotation`     | 0/1/2/3 -> 0/90/180/270 |
| 10 | `uint16` | `flags`        | 预留，默认 0 |
| 12 | `uint32` | `totalBytes`   | 图片总字节数 |
| 16 | `uint16` | `chunkSize`    | 建议分片大小 |
| 18 | `uint16` | `totalChunks`  | 总分片数 |
| 20 | `uint32` | `imageCrc32`   | 整图 CRC32 |

响应 payload（建议 12 字节）：

| 偏移 | 类型     | 字段名          | 说明 |
|---|---|---|---|
| 0  | `uint32` | `txId`          | 回显 |
| 4  | `uint16` | `acceptedChunk` | 设备接受的分片大小 |
| 6  | `uint16` | `windowSize`    | 第一版固定 1 |
| 8  | `uint32` | `reserved`      | 预留 |

## 5.2 `ImageChunk` (`cmdId=0x0202`)

请求 payload（变长，头 16 字节 + data）：

| 偏移 | 类型     | 字段名        | 说明 |
|---|---|---|---|
| 0  | `uint32` | `txId`        | 传输会话 ID |
| 4  | `uint16` | `chunkIndex`  | 从 0 开始 |
| 6  | `uint16` | `dataLen`     | 本片数据长度 |
| 8  | `uint32` | `offset`      | 在整图缓冲中的偏移 |
| 12 | `uint32` | `chunkCrc32`  | 本片 CRC32 |
| 16 | `bytes`  | `data`        | 分片数据 |

响应 payload（建议 12 字节）：

| 偏移 | 类型     | 字段名         | 说明 |
|---|---|---|---|
| 0  | `uint32` | `txId`         | 回显 |
| 4  | `uint16` | `chunkIndex`   | 回显 |
| 6  | `uint16` | `nextExpected` | 期望下一片序号 |
| 8  | `uint32` | `receivedBytes`| 已确认写入字节数 |

`dataLen` 建议：
- 上限理论值：`4096 - 16 = 4080`。
- 第一版建议：`512` 或 `1024`，优先稳定性。

## 5.3 `ImageEnd` (`cmdId=0x0203`)

请求 payload（12 字节）：

| 偏移 | 类型     | 字段名        | 说明 |
|---|---|---|---|
| 0  | `uint32` | `txId`        | 传输会话 ID |
| 4  | `uint16` | `totalChunks` | 总分片数 |
| 6  | `uint16` | `reserved`    | 预留 |
| 8  | `uint32` | `imageCrc32`  | 发送端整图 CRC32 |

响应 payload（建议 12 字节）：

| 偏移 | 类型     | 字段名         | 说明 |
|---|---|---|---|
| 0  | `uint32` | `txId`         | 回显 |
| 4  | `uint32` | `computedCrc32`| 设备计算的整图 CRC32 |
| 8  | `uint32` | `receivedBytes`| 实际收到总字节数 |

## 5.4 `ImageApply` (`cmdId=0x0204`)

请求 payload（8 字节）：

| 偏移 | 类型     | 字段名       | 说明 |
|---|---|---|---|
| 0  | `uint32` | `txId`       | 传输会话 ID |
| 4  | `uint8`  | `mode`       | 0=全刷，1=快刷（预留） |
| 5  | `uint8`  | `reserved1`  | 预留 |
| 6  | `uint16` | `reserved2`  | 预留 |

响应 payload（建议 8 字节）：

| 偏移 | 类型     | 字段名       | 说明 |
|---|---|---|---|
| 0  | `uint32` | `txId`       | 回显 |
| 4  | `uint16` | `displayCode`| 显示驱动返回码 |
| 6  | `uint16` | `reserved`   | 预留 |

## 5.5 `ImageAbort` (`cmdId=0x0205`)

请求 payload（8 字节）：

| 偏移 | 类型     | 字段名      | 说明 |
|---|---|---|---|
| 0  | `uint32` | `txId`      | 传输会话 ID |
| 4  | `uint16` | `reason`    | 客户端中止原因 |
| 6  | `uint16` | `reserved`  | 预留 |

响应 payload：可空或回显 `txId`。

## 6. 响应码定义

建议统一错误码：

- `0` 成功
- `1` 参数错误
- `2` 会话不存在
- `3` 会话状态非法
- `4` 分片越界
- `5` 分片 CRC 错误
- `6` 整图 CRC 错误
- `7` 设备忙
- `8` 超时
- `9` 重复分片
- `10` 分片乱序

## 7. 时序与重传

标准时序：
1. `ImageBegin`
2. `ImageChunk(0..N-1)`（每片等 ACK）
3. `ImageEnd`
4. `ImageApply`

重传建议：
- `chunkAckTimeoutMs = 800`
- `chunkMaxRetries = 3`
- `begin/end/apply timeoutMs = 1500`
- `sessionIdleTimeoutMs = 10000`

失败策略：
- 单片超过重试上限：发送 `ImageAbort`，会话失败。
- 设备返回 `7`（忙）：指数退避后重试 `Begin`。

## 8. 状态机约束

ESP32：
- `Idle -> Receiving`：`ImageBegin` 成功
- `Receiving -> Verifying`：`ImageEnd`
- `Verifying -> ReadyToApply`：整图 CRC 成功
- `ReadyToApply -> Applying -> Done`：`ImageApply`
- 任意状态可因超时/错误 -> `Failed`

关键拒绝规则：
- 非 `Receiving` 状态收到 `ImageChunk` -> `code=3`
- 未通过校验收到 `ImageApply` -> `code=3`

## 9. 心跳共存实现要求

1. 心跳检测优先级高于业务包处理。
2. 传图期间必须稳定响应 `PING/PONG`。
3. 严禁将心跳文本干扰业务帧解码；需确保解析层能可靠分离心跳字节与二进制帧。

## 10. 联调清单

- 小图（<10KB）成功传输并显示。
- 大图（接近上限）可稳定完成。
- 人工注入错包后可触发重传。
- 整图 CRC 错误时被拒绝应用。
- 传输期间长时间运行无心跳超时断连。

## 11. 版本管理

- 协议版本建议在 `ImageBegin.flags` 高位或新增字段表示。
- 每次字段变更需更新版本号并记录兼容策略。
