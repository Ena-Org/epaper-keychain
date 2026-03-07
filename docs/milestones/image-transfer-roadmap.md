# 浏览器到 ESP32 图片传输逻辑与里程碑

配套协议细节文档：`docs/protocol-integration/image-transfer-protocol.md`

## 1. 目标与范围

目标：在现有“浏览器 <-> ESP32”连接与心跳能力基础上，实现浏览器向 ESP32 稳定传输图片，并最终驱动电子纸显示。

当前范围：
- 先完成协议、分片、校验、状态机与异常处理设计。
- 先以全量图片传输为主，后续再考虑增量刷新与更高吞吐。

## 2. 现有能力与约束

基于当前工程实现：
- 传输链路是混合流：二进制协议帧 + 明文心跳 `PING/PONG`。
- 编解码器限制：`maxPayloadBytes = 4096`，`maxBufferBytes = 8192`。
- 业务入口流程：`Transport -> Codec -> Router -> Handler`。
- 命令路由已具备扩展能力，可按 `cmdId` 增加图片相关命令。

设计约束：
- 传图期间必须持续维持心跳，不允许阻塞心跳响应。
- 每个分片需可校验、可确认、可重传。
- 图片最终刷新需在“完整性校验通过”后触发。

## 3. 端到端传输逻辑

### 3.1 浏览器侧职责

1. 用户选择图片。
2. 预处理：缩放到屏幕分辨率、旋转、灰度/二值化、可选抖动。
3. 编码为设备目标位图格式（如 1bpp）。
4. 计算整图 `CRC32`。
5. 按约定分片大小切片。
6. 按会话发送：`ImageBegin -> ImageChunk* -> ImageEnd -> ImageApply`。

### 3.2 ESP32 侧职责

1. 接收 `ImageBegin`，校验参数并创建传输会话。
2. 接收 `ImageChunk`，按 `offset/chunkIndex` 写入缓冲区并校验分片 CRC。
3. 接收 `ImageEnd`，执行整图 CRC 校验。
4. 校验通过后进入可应用状态。
5. 接收 `ImageApply` 后调用电子纸模块刷新显示。

### 3.3 ACK 与重传

建议第一版采用“停等协议”：
- 浏览器发送一个分片后等待 ACK。
- ACK 成功再发下一片。
- 超时或失败码触发重发（最大重试次数可配置）。

后续可升级为滑动窗口以提升吞吐。

## 4. 协议建议（草案）

## 4.1 命令建议

- `ImageBegin`：开始一次图片传输会话。
- `ImageChunk`：发送图片分片。
- `ImageEnd`：声明分片发送结束并请求总校验。
- `ImageApply`：请求设备将已校验图片应用到电子纸。
- `ImageAbort`：取消当前会话并清理状态。

## 4.2 关键字段建议

`ImageBegin` payload：
- `sessionId`
- `width`
- `height`
- `pixelFormat`（例如 1bpp）
- `rotation`
- `totalBytes`
- `chunkSize`
- `imageCrc32`

`ImageChunk` payload：
- `sessionId`
- `chunkIndex`
- `offset`
- `chunkLen`
- `chunkCrc32`
- `data`

`ImageEnd` payload：
- `sessionId`
- `totalChunks`
- `imageCrc32`（可与 Begin 一致，用于双端一致性校验）

## 4.3 响应码建议

- `0`: 成功
- `1`: 参数错误
- `2`: 会话不存在
- `3`: 会话状态非法
- `4`: 分片越界
- `5`: 分片 CRC 错误
- `6`: 整图 CRC 错误
- `7`: 设备忙
- `8`: 超时

## 5. 状态机建议

### 5.1 ESP32 状态机

- `Idle`
- `Receiving`
- `Verifying`
- `ReadyToApply`
- `Applying`
- `Done`
- `Failed`

典型流转：
- `Idle -> Receiving`（收到 Begin）
- `Receiving -> Verifying`（收到 End）
- `Verifying -> ReadyToApply`（校验通过）
- `ReadyToApply -> Applying -> Done`（收到 Apply 并刷新成功）
- 任意状态可 `-> Failed`（超时/校验失败/异常）

### 5.2 浏览器状态机

- `Selected`
- `Preprocessed`
- `Sending`
- `WaitingAck`
- `Completed`
- `Retrying`
- `Failed`

## 6. 分片策略与流控

第一版建议：
- 分片大小从 `512` 或 `1024` 字节起步。
- 每片必须 ACK。
- ACK 超时后重试，超过上限则会话失败。
- 会话超时自动清理，避免占用内存。

后续优化方向：
- 滑动窗口（如窗口大小 4/8）。
- 自适应分片大小（依据错误率动态调整）。

## 7. 心跳共存策略

重点原则：
- 心跳处理优先级高于业务分片。
- 传图过程中保持 `PING/PONG` 正常往返。
- 解析层必须严格区分心跳文本与二进制帧，避免相互污染。

## 8. 异常场景清单

必须覆盖以下场景：
- Begin 后长时间无 Chunk。
- Chunk 丢失、重复、乱序。
- Chunk CRC 正确但整图 CRC 失败。
- 传输中断开连接后重连。
- 正在 Applying 时收到新的 Begin。
- 用户主动取消（Abort）。

## 9. 实施里程碑

### M1: 协议冻结

输出：
- 图片传输命令表。
- payload 二进制字段定义。
- 响应码表。
- 状态机图与超时参数。

验收标准：
- 浏览器与固件双方对字段含义、字节序、错误码达成一致。

### M2: 最小传输闭环（不刷屏）

输出：
- 支持 `Begin -> Chunk -> End`。
- 设备能累计接收并返回结果。

验收标准：
- 小图片可稳定传完并返回成功。
- 错误参数能返回对应错误码。

### M3: 完整性与重传

输出：
- 分片 CRC 校验。
- 整图 CRC 校验。
- ACK 超时重传。

验收标准：
- 注入丢包/错包后可重传恢复或明确失败。

### M4: 接入电子纸刷新

输出：
- 支持 `ImageApply`。
- 校验通过后刷新屏幕。

验收标准：
- 屏幕显示结果与浏览器预览一致。

### M5: 稳定性与性能优化

输出：
- 压测与长稳测试记录。
- 分片大小与超时参数调优。
- 可选滑动窗口方案。

验收标准：
- 长时间运行无异常断传。
- 典型图片传输时间可接受。

## 10. 联调与验收建议

联调检查项：
- 心跳连续稳定。
- 图片会话日志可追踪（sessionId、chunkIndex、offset、错误码）。
- 失败场景具备可复现与可定位信息。

建议指标：
- 成功率。
- 平均传输时长。
- 重传率。
- 校验失败率。
