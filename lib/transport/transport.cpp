#include "transport.hpp"

#include <algorithm>
#include <cstring>
#include "logger.hpp"

namespace
{
  /**
   * @brief 传输模块的日志标签
   * 
   * 用于标识来自传输层(transport layer)的日志输出，
   * 便于在日志系统中筛选和跟踪与传输相关的调试信息。
   * 
   * @note 此标签在整个传输模块中使用，与日志宏(如ESP_LOGI等)配合使用
   */
	constexpr const char *TAG = "TRANSPORT";

	constexpr size_t kSerialRxChunkSize = 256;

  /**
   * @brief 将传输事件枚举值转换为对应的文本描述
   * 
   * @param e Transport::Event 枚举值，表示传输层发生的事件类型
   * 
   * @return const char* 事件对应的文本描述字符串
   *         - "Connected": 传输连接已建立
   *         - "Disconnected": 传输连接已断开
   *         - "Error": 传输发生错误
   *         - "HeartbeatTimeout": 心跳超时
   *         - "Unknown": 未知事件类型（默认情况）
   * 
   * @note 返回值为内部字符串常量，无需手动释放内存
   */
	const char *eventText(Transport::Event e)
	{
		switch (e)
		{
		case Transport::Event::Connected:
			return "Connected";
		case Transport::Event::Disconnected:
			return "Disconnected";
		case Transport::Event::Error:
			return "Error";
		case Transport::Event::HeartbeatTimeout:
			return "HeartbeatTimeout";
		default:
			return "Unknown";
		}
	}
}

/**
 * @brief Transport 类的构造函数
 * 
 * 初始化 Transport 对象，将其状态设置为断开连接状态，
 * 并重置心跳状态。
 * 
 * @details
 * 构造函数执行以下操作：
 * - 调用 resetHeartbeatState_() 重置心跳状态
 * - 将 state_ 成员变量初始化为 State::Disconnected
 */
Transport::Transport()
{
	resetHeartbeatState_();
	state_ = State::Disconnected;
}

/**
 * @brief 初始化传输对象
 * @details 根据提供的配置初始化传输对象，重置所有相关的状态变量，
 *          包括连接状态、重连时间戳和心跳状态
 * @param cfg 传输配置对象，包含所有必要的配置参数
 * @return void
 * @note 调用此方法后，传输对象的状态将被重置为断开连接状态
 */
void Transport::init(const Config &cfg)
{
	config_ = cfg;
	state_ = State::Disconnected;
	lastReconnectTryAtMs_ = 0;
	resetHeartbeatState_();
}

/**
 * @brief 设置传输层配置
 * 
 * 将提供的配置对象赋值给内部配置成员变量，用于更新传输层的工作参数。
 * 
 * @param cfg 常引用类型的Config对象，包含要设置的配置参数
 * 
 * @return void
 * 
 * @note 该函数会直接覆盖之前的配置，请确保传入的配置参数有效
 */
void Transport::setConfig(const Config &cfg)
{
	config_ = cfg;
}

/**
 * @brief 连接到传输层
 * @details 建立传输连接。如果已经处于连接状态，则直接返回true。
 *          否则将状态设置为连接中，然后设置为已连接，重置心跳状态，
 *          并发出连接成功事件。
 * @return bool 连接是否成功。当已连接或连接操作完成时返回true。
 * @note 该函数会修改内部状态machine，并触发Connected事件。
 */
bool Transport::connect()
{
	if (state_ == State::Connected)
	{
		return true;
	}

	state_ = State::Connecting;

	state_ = State::Connected;
	resetHeartbeatState_();
	emit_(Event::Connected);
	return true;
}

/**
 * @brief 同步连接到传输层
 * @details 该函数执行同步连接操作，阻塞调用直到连接完成或失败
 * @return bool 连接成功返回 true，失败返回 false
 */
bool Transport::syncConnect()
{
	return connect();
}

/**
 * @brief 重新连接传输层
 * 
 * 断开当前连接，然后建立新的连接。这是一个便利函数，
 * 用于快速重置传输连接状态。
 * 
 * @return bool 如果成功连接返回 true，否则返回 false
 * 
 * @note 该函数会先调用 disconnect() 断开现有连接，
 *       然后调用 connect() 建立新连接。
 * 
 * @see connect()
 * @see disconnect()
 */
bool Transport::reconnect()
{
	disconnect();
	return connect();
}

/**
 * @brief 断开连接
 * 
 * 将传输层状态设置为断开连接，并重置心跳状态，最后发出断开连接事件。
 * 如果当前状态已经是断开连接状态，则不进行任何操作。
 * 
 * @return void
 * 
 * @note 该函数会触发 Event::Disconnected 事件
 * 
 * @see resetHeartbeatState_()
 * @see emit_(Event)
 */
void Transport::disconnect()
{
	if (state_ != State::Disconnected)
	{
		state_ = State::Disconnected;
		resetHeartbeatState_();
		emit_(Event::Disconnected);
	}
}

/**
 * @brief 传输层主循环函数
 * 
 * 该函数执行传输层的主要循环任务，包括：
 * 1. 心跳循环 - 定期发送心跳信号以保持连接活跃
 * 2. 重连循环 - 监测和处理连接状态，在连接断开时尝试重新连接
 * 
 * 应定期调用此函数以确保传输层的正常运行和连接的稳定性。
 */
void Transport::loop()
{
	uint8_t rxChunk[kSerialRxChunkSize];
	while (Serial.available() > 0)
	{
		const size_t availableBytes = static_cast<size_t>(Serial.available());
		const size_t toRead = std::min(availableBytes, kSerialRxChunkSize);
		const size_t readBytes = Serial.readBytes(reinterpret_cast<char *>(rxChunk), toRead);
		if (readBytes == 0)
		{
			break;
		}

		onBytesReceived(rxChunk, readBytes);
	}

	heartbeatLoop_();
	reconnectLoop_();
}

/**
 * @brief 通过传输层发送数据
 * 
 * 将指定长度的数据发送到已连接的设备。如果数据指针为空或长度为0，
 * 函数将直接返回。如果设备未连接，操作将被忽略并记录警告日志。
 * 
 * @param data 指向待发送数据的指针
 * @param len 待发送数据的长度（字节数）
 * 
 * @return void
 * 
 * @note 如果 data 为 nullptr 或 len 为 0，函数不执行任何操作
 * @note 如果设备未连接，发送操作将被忽略，并记录警告级别的日志
 * @warning 调用者需确保在调用此函数前数据已正确初始化
 * 
 * @see isConnected()
 */
void Transport::send(const uint8_t *data, size_t len)
{
	if (data == nullptr || len == 0)
	{
		return;
	}

	if (!isConnected())
	{
		LOGW(TAG, "send ignored: not connected (len=%u)", static_cast<unsigned>(len));
		return;
	}

	size_t totalWritten = 0;
	uint8_t idleRounds = 0;
	while (totalWritten < len)
	{
		const size_t written = Serial.write(data + totalWritten, len - totalWritten);
		if (written == 0)
		{
			++idleRounds;
			if (idleRounds >= 5)
			{
				break;
			}
			delay(1);
			continue;
		}

		totalWritten += written;
		idleRounds = 0;
	}

	if (totalWritten != len)
	{
		LOGW(TAG, "send partial: %u/%u bytes", static_cast<unsigned>(totalWritten), static_cast<unsigned>(len));
		return;
	}

	LOGV(TAG, "send ok: %u bytes", static_cast<unsigned>(len));
}

/**
 * @brief 注入接收到的原始字节数据
 *
 * 该函数由底层驱动在收到数据后调用，用于完成三件事：
 * 1. 记录接收活动时间（用于连接存活判断）；
 * 2. 在字节流中增量识别心跳 ACK（"PONG"）；
 * 3. 将原始数据追加到内部接收缓冲区，供上层读取。
 *
 * @param data 接收数据指针
 * @param len 接收数据长度（字节）
 *
 * @note 当 data 为 nullptr 或 len 为 0 时，函数直接返回
 */
void Transport::onBytesReceived(const uint8_t *data, size_t len)
{
	if (data == nullptr || len == 0)
	{
		return;
	}

	notifyRxActivity();
	processHeartbeatAckBytes_(data, len);
	rxBuffer_.insert(rxBuffer_.end(), data, data + len);
}

/**
 * @brief 获取接收缓冲区当前可读字节数
 *
 * @return size_t 内部接收缓冲区中尚未被读取的数据长度
 */
size_t Transport::available() const
{
	return rxBuffer_.size();
}

/**
 * @brief 从接收缓冲区读取数据
 *
 * 将内部接收缓冲区头部数据拷贝到输出缓冲，并消费对应字节数。
 *
 * @param out 输出缓冲区指针
 * @param maxLen 允许读取的最大字节数
 * @return size_t 实际读取的字节数
 *
 * @note 当 out 为 nullptr、maxLen 为 0 或缓冲区为空时返回 0
 */
size_t Transport::receive(uint8_t *out, size_t maxLen)
{
	if (out == nullptr || maxLen == 0 || rxBuffer_.empty())
	{
		return 0;
	}

	const size_t n = std::min(maxLen, rxBuffer_.size());
	std::memcpy(out, rxBuffer_.data(), n);
	rxBuffer_.erase(rxBuffer_.begin(), rxBuffer_.begin() + n);
	return n;
}

/**
 * @brief 通知接收到活动
 * 
 * 更新最后一次接收数据的时间戳。如果当前未在等待心跳确认，
 * 则同时更新最后一次接收到心跳确认的时间戳。
 * 
 * @note 此函数应在接收到任何数据时调用，用于维护通信活动的时间记录。
 */
void Transport::notifyRxActivity()
{
	const uint32_t now = millis();
	lastRxAtMs_ = now;

	if (!awaitingHeartbeatAck_)
	{
		lastHeartbeatAckAtMs_ = now;
	}
}

/**
 * @brief 通知心跳确认已收到
 * 
 * 当接收到心跳确认响应时调用此函数。更新相关时间戳，
 * 标记不再等待心跳确认。
 * 
 * @details
 * - 将 awaitingHeartbeatAck_ 设置为 false，表示心跳确认已收到
 * - 更新 lastHeartbeatAckAtMs_ 为当前时间戳
 * - 更新 lastRxAtMs_ 为当前时间戳，指示最后接收数据的时间
 * 
 * @return void
 * 
 * @note 此函数应在成功接收到心跳确认时调用
 * 
 * @see awaitingHeartbeatAck_, lastHeartbeatAckAtMs_, lastRxAtMs_
 */
void Transport::notifyHeartbeatAck()
{
	const uint32_t now = millis();
	awaitingHeartbeatAck_ = false;
	lastHeartbeatAckAtMs_ = now;
	lastRxAtMs_ = now;
}

/**
 * @brief 获取传输状态
 * 
 * 返回当前传输对象的状态。
 * 
 * @return Transport::State 传输的当前状态
 */
Transport::State Transport::state() const
{
	return state_;
}

/**
 * @brief 判断传输层是否处于已连接状态
 *
 * @return true 当前状态为 State::Connected
 * @return false 当前状态不是 State::Connected
 */
bool Transport::isConnected() const
{
	return state_ == State::Connected;
}

/**
 * @brief 心跳维护循环
 *
 * 在已连接且启用心跳时执行：
 * - 到达发送周期则发送心跳 PING；
 * - 若等待 ACK 超时则触发心跳超时事件并断开连接。
 *
 * @note 该函数通常由 loop() 周期性调用
 */
void Transport::heartbeatLoop_()
{
	if (!config_.heartbeatEnabled || !isConnected())
	{
		return;
	}

	const uint32_t now = millis();
	if (shouldSendHeartbeat_(now))
	{
		if (!sendHeartbeatPing_())
		{
			state_ = State::Error;
			emit_(Event::Error);
			disconnect();
			return;
		}
	}

	if (isHeartbeatTimeout_(now))
	{
		state_ = State::Error;
		emit_(Event::HeartbeatTimeout);
		disconnect();
	}
}

/**
 * @brief 自动重连循环
 *
 * 当启用自动重连且当前非 Connected/Connecting 时，按配置间隔尝试 reconnect。
 * 若连接失败，转入 Error 状态并发出错误事件。
 */
void Transport::reconnectLoop_()
{
	if (!config_.autoReconnect)
	{
		return;
	}

	if (state_ == State::Connected || state_ == State::Connecting)
	{
		return;
	}

	const uint32_t now = millis();
	if ((now - lastReconnectTryAtMs_) < config_.reconnectIntervalMs)
	{
		return;
	}

	lastReconnectTryAtMs_ = now;
	if (!connect())
	{
		state_ = State::Error;
		emit_(Event::Error);
	}
}

/**
 * @brief 判断当前时刻是否应发送心跳
 *
 * @param now 当前毫秒时间戳
 * @return true 已达到心跳发送间隔
 * @return false 尚未达到发送间隔
 */
bool Transport::shouldSendHeartbeat_(uint32_t now) const
{
	return (now - lastHeartbeatSentAtMs_) >= config_.heartbeatIntervalMs;
}

/**
 * @brief 判断等待心跳 ACK 是否超时
 *
 * @param now 当前毫秒时间戳
 * @return true 正在等待 ACK 且超过超时时间
 * @return false 未等待 ACK 或尚未超时
 */
bool Transport::isHeartbeatTimeout_(uint32_t now) const
{
	if (!awaitingHeartbeatAck_)
	{
		return false;
	}

	const uint32_t lastAliveAt = std::max(lastHeartbeatAckAtMs_, lastRxAtMs_);
	return (now - lastAliveAt) >= config_.heartbeatTimeoutMs;
}

/**
 * @brief 重置心跳相关状态
 *
 * 将发送时间、ACK 时间、接收活动时间重置为当前时刻，
 * 并清空“等待 ACK”标记和 ACK 匹配进度。
 */
void Transport::resetHeartbeatState_()
{
	const uint32_t now = millis();
	lastHeartbeatSentAtMs_ = now;
	lastHeartbeatAckAtMs_ = now;
	lastRxAtMs_ = now;
	awaitingHeartbeatAck_ = false;
	heartbeatAckMatchPos_ = 0;
}

/**
 * @brief 发射内部事件（当前以日志形式输出）
 *
 * @param e 事件类型
 *
 * @note 后续可在此处扩展为事件回调或消息总线分发
 */
void Transport::emit_(Event e)
{
	LOGD(TAG, "event=%s state=%u", eventText(e), static_cast<unsigned>(state_));
}

/**
 * @brief 发送心跳 PING
 *
 * 发送固定内容 "PING\n"，并更新心跳发送时间与 ACK 等待标记。
 *
 * @return true 发送流程已执行
 * @return false 当前未连接，发送被拒绝
 */
bool Transport::sendHeartbeatPing_()
{
	if (!isConnected())
	{
		return false;
	}

	static constexpr uint8_t kPing[] = {'P', 'I', 'N', 'G', '\n'};
	send(kPing, sizeof(kPing));
	lastHeartbeatSentAtMs_ = millis();
	awaitingHeartbeatAck_ = true;
	return true;
}

/**
 * @brief 在接收字节流中增量识别心跳 ACK（"PONG"）
 *
 * 支持跨包匹配：即使 "PONG" 被拆分在多次接收中，也能正确识别。
 * 每次完整匹配成功后会调用 notifyHeartbeatAck()。
 *
 * @param data 输入字节流指针
 * @param len 输入字节长度
 */
void Transport::processHeartbeatAckBytes_(const uint8_t *data, size_t len)
{
	static constexpr char kAck[] = "PONG";
	static constexpr size_t kAckLen = sizeof(kAck) - 1;

	for (size_t i = 0; i < len; ++i)
	{
		const char ch = static_cast<char>(data[i]);

		if (ch == kAck[heartbeatAckMatchPos_])
		{
			++heartbeatAckMatchPos_;
			if (heartbeatAckMatchPos_ == kAckLen)
			{
				notifyHeartbeatAck();
				heartbeatAckMatchPos_ = 0;
			}
			continue;
		}

		heartbeatAckMatchPos_ = (ch == kAck[0]) ? 1 : 0;
	}
}
