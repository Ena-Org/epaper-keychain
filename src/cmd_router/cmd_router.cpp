#include "cmd_router.hpp"

#include <cstdlib>

namespace
{
  /**
   * @brief 将字符串令牌解析为无符号32位整数
   * 
   * @param token 要解析的字符串令牌
   * @param out 输出参数，存储解析成功的结果
   * 
   * @return true 如果字符串成功解析为有效的无符号整数
   * @return false 如果字符串为空、包含非数字字符或解析失败
   * 
   * @note 函数支持多种进制格式（十进制、十六进制等），由strtoul自动识别
   * @note 如果解析的值超出uint32_t范围，结果将被截断
   */
  bool parseUint32(const String &token, uint32_t &out)
  {
    if (token.isEmpty())
    {
      return false;
    }

    const char *text = token.c_str();
    char *endptr = nullptr;
    const unsigned long value = std::strtoul(text, &endptr, 0);
    if (endptr == text || *endptr != '\0')
    {
      return false;
    }

    out = static_cast<uint32_t>(value);
    return true;
  }

  /**
   * @brief 将上传器状态枚举值转换为对应的文本描述
   * 
   * @param state 上传器的当前状态，可能的值包括：
   *              - Uploader::State::Idle: 空闲状态
   *              - Uploader::State::Receiving: 接收中状态
   *              - Uploader::State::Verifying: 验证中状态
   *              - Uploader::State::Done: 完成状态
   *              - Uploader::State::Error: 错误状态
   * 
   * @return String 返回对应状态的文本表示。如果状态未被识别，
   *                返回 "Unknown" 字符串
   * 
   * @example
   *   String result = stateText(Uploader::State::Done);  // 返回 "Done"
   */
  String stateText(Uploader::State state)
  {
    switch (state)
    {
    case Uploader::State::Idle:
      return "Idle";
    case Uploader::State::Receiving:
      return "Receiving";
    case Uploader::State::Verifying:
      return "Verifying";
    case Uploader::State::Done:
      return "Done";
    case Uploader::State::Error:
      return "Error";
    default:
      return "Unknown";
    }
  }
}

/**
 * @brief 初始化命令路由器
 * 
 * 该函数用于启动和配置命令路由器的工作。在使用CmdRouter处理命令之前，
 * 必须先调用此函数进行必要的初始化。
 * 
 * @return void
 * 
 * @note 该函数应在程序启动的早期调用
 * @todo 未来可能需要添加参数以支持更复杂的初始化需求
 * @see CmdRouter
 */
void CmdRouter::begin()
{
}

/**
 * @brief 命令路由器的主循环函数
 * @details 处理来自USB CDC传输的输入命令。如果上传会话处于二进制模式，
 *          则轮询上传会话；否则逐行读取并处理命令。
 * 
 * @param io USB CDC传输接口，用于读取用户输入和发送响应
 * @param up 上传会话对象，存储当前上传的状态和数据
 * 
 * @note 当上传会话处于二进制模式时，该函数将控制权交给上传会话处理
 * @note 在文本模式下，函数会阻塞等待新的命令行输入
 */
void CmdRouter::loop(UsbCdcTransport &io, Uploader::UploadSession &up)
{
  if (up.inBinaryMode())
  {
    up.poll(io);
    return;
  }

  String line;
  while (io.readLine(line))
  {
    handleLine(line, io, up);
  }
}

/**
 * @brief 处理来自USB CDC传输的命令行
 * 
 * 解析并执行用户发送的命令。支持的命令包括：
 * - PING: 测试连接，返回PONG
 * - ABORT: 中止当前上传会话
 * - STATE: 获取上传会话的当前状态信息
 * - PUT: 启动文件上传会话
 * 
 * @param line 待处理的命令行字符串
 * @param io USB CDC传输接口，用于发送响应
 * @param up 上传会话管理器，用于执行上传相关操作
 * 
 * @details
 * PUT命令格式: PUT <path> <size> [crc32]
 * - path: 目标文件路径
 * - size: 文件大小（字节）
 * - crc32: 可选的CRC32校验值，默认为0
 * 
 * 响应格式：
 * - 成功: "ok <msg>"
 * - 失败: "err <msg>"
 * 
 * @note 命令不区分大小写（除PUT命令的参数外）
 * @note 空行会被忽略
 */
void CmdRouter::handleLine(const String &line, UsbCdcTransport &io, Uploader::UploadSession &up)
{
  String cmd = line;
  cmd.trim();

  if (cmd.isEmpty())
  {
    return;
  }

  if (cmd.equalsIgnoreCase("PING"))
  {
    replyOk(io, "PONG");
    return;
  }

  if (cmd.equalsIgnoreCase("ABORT"))
  {
    up.abort();
    replyOk(io, "aborted");
    return;
  }

  if (cmd.equalsIgnoreCase("STATE"))
  {
    const Uploader::Stats st = up.stats();
    String msg = "state=" + stateText(up.state()) +
                 " active=" + String(up.isActive() ? 1 : 0) +
                 " err=" + String(static_cast<uint32_t>(up.lastError())) +
                 " recv=" + String(st.receivedBytes) +
                 " ack=" + String(st.ackedChunks);
    replyOk(io, msg);
    return;
  }

  if (cmd.startsWith("PUT ") || cmd.startsWith("put "))
  {
    int p1 = cmd.indexOf(' ');
    int p2 = cmd.indexOf(' ', p1 + 1);
    if (p2 < 0)
    {
      replyErr(io, "usage: PUT <path> <size> [crc32]");
      return;
    }

    int p3 = cmd.indexOf(' ', p2 + 1);
    const String path = cmd.substring(p1 + 1, p2);
    const String sizeToken = (p3 < 0) ? cmd.substring(p2 + 1) : cmd.substring(p2 + 1, p3);
    const String crcToken = (p3 < 0) ? String("0") : cmd.substring(p3 + 1);

    uint32_t size = 0;
    uint32_t crc = 0;
    if (!parseUint32(sizeToken, size) || !parseUint32(crcToken, crc))
    {
      replyErr(io, "invalid number");
      return;
    }

    Uploader::PutRequest req;
    req.path = path;
    req.size = size;
    req.crc32 = crc;

    Uploader::PutResponse resp;
    if (!up.start(req, resp))
    {
      replyErr(io, "start failed err=" + String(static_cast<uint32_t>(up.lastError())));
      return;
    }

    replyOk(io, "session=" + String(resp.session) + " chunk=" + String(resp.chunk));
    return;
  }

  replyErr(io, "unknown command");
}

/**
 * @brief 向USB CDC传输发送成功响应消息
 * 
 * 通过USB CDC传输接口发送一条以"OK"开头的成功响应消息。
 * 该方法用于向主机确认命令执行成功并可选地返回相关信息。
 * 
 * @param io USB CDC传输对象的引用，用于发送数据
 * @param msg 要附加在"OK"响应后的消息内容，默认为空字符串
 * 
 * @return void
 * 
 * @note 最终发送的格式为 "OK " 后跟消息内容，以换行符结尾
 * 
 * @see writeLine()
 */
void CmdRouter::replyOk(UsbCdcTransport &io, const String &msg)
{
  io.writeLine("OK " + msg);
}

/**
 * @brief 向USB CDC传输接口回复错误消息
 * 
 * @param io USB CDC传输接口引用，用于发送错误响应
 * @param msg 错误消息内容，将被添加到"ERR "前缀之后
 * 
 * @return void
 * 
 * @details 
 * 该函数将错误消息以"ERR "为前缀的格式通过USB CDC接口发送出去。
 * 消息格式为：ERR <msg>
 */
void CmdRouter::replyErr(UsbCdcTransport &io, const String &msg)
{
  io.writeLine("ERR " + msg);
}
