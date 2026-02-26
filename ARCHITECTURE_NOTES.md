# 项目架构与接口关键信息整理

---

## 1. 分层设计与职责
- **transport**：只管字节收发与连接状态。
- **codec**：只管字节流 <-> 协议包（Packet）编解码。
- **router**：只管 Packet -> handler 分发，不做 I/O。
- **cmd**：聚合业务，注册 handler，调用 router。

---

## 2. 共享协议包类型
- 文件：include/protocol_packet.hpp
```cpp
namespace Protocol {
  enum class PacketType : uint8_t { Request = 1, Response = 2, Event = 3 };
  struct Packet {
    PacketType type = PacketType::Request;
    uint16_t session = 0;
    uint16_t cmdId = 0;
    uint16_t code = 0;
    std::vector<uint8_t> payload{};
  };
}
```
- router 和 codec 都用 `using Packet = Protocol::Packet;`，避免重复定义。

---

## 3. codec模块接口与实现
- 文件：lib/codec/codec.hpp
  - `encode(const Packet&, std::vector<uint8_t>&)`
  - `feed(const uint8_t*, size_t)`
  - `next(Packet&)`
  - `reset()`
  - `bufferedBytes()`
  - `lastError()/lastErrorText()`
- 文件：lib/codec/codec.cpp
  - 已实现，支持流式解包、校验、错误处理。

---

## 4. router模块接口
- 文件：lib/router/router.hpp
  - 只做分发，不依赖 transport/codec。
  - 通过 `registerHandler(cmdId, handler)` 注册业务处理器。
  - 通过 `dispatch(packet, ctx)` 分发已解包 Packet。

---

## 5. cmd_ids用法
- 文件：include/cmd_ids.hpp
```cpp
namespace CmdId { static constexpr uint16_t Info = 0x0001; }
```
- 注册 handler 时用：`router.registerHandler(CmdId::Info, handler);`

---

## 6. 典型调用链
- transport 收到字节流 → codec.feed → codec.next 得到 Packet → router.dispatch → handler → ctx.reply/publish

---

## 7. 其它建议
- logger 可重构为环形缓冲+订阅，支持历史回溯和实时推送。
- 业务 handler 不应写在 router 内部，而是注册到 router。

---

如需迁移到新会话，建议复制本整理内容，并补充你后续的新增代码或问题。
