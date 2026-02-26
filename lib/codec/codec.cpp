#include "codec.hpp"
#include <cstring>
#include <algorithm>

namespace
{
	/**
	 * @brief 根据给定的错误码返回对应的错误描述文本。
	 *
	 * @param err Codec::Error 枚举类型的错误码。
	 * @return const char* 错误描述字符串。
	 *
	 * 错误码包括：
	 * - None: 正常，无错误。
	 * - InvalidArgument: 参数无效。
	 * - BufferOverflow: 缓冲区溢出。
	 * - BadMagic: 魔数错误。
	 * - BadLength: 长度错误。
	 * - BadChecksum: 校验和错误。
	 * - NeedMore: 数据不足。
	 * - EncodeFailed: 编码失败。
	 * - 其他未知错误返回 "unknown error"。
	 */
	const char *errorText(Codec::Error err)
	{
		switch (err)
		{
		case Codec::Error::None:
			return "ok";
		case Codec::Error::InvalidArgument:
			return "invalid argument";
		case Codec::Error::BufferOverflow:
			return "buffer overflow";
		case Codec::Error::BadMagic:
			return "bad magic";
		case Codec::Error::BadLength:
			return "bad length";
		case Codec::Error::BadChecksum:
			return "bad checksum";
		case Codec::Error::NeedMore:
			return "need more data";
		case Codec::Error::EncodeFailed:
			return "encode failed";
		default:
			return "unknown error";
		}
	}
}

/**
 * @brief Codec类的构造函数。
 *
 * 初始化Codec对象。当前构造函数为空，未执行任何操作。
 */
Codec::Codec() {}

/**
 * @brief Codec类的构造函数
 * 
 * 使用给定的配置参数初始化Codec对象。
 * 
 * @param cfg 用于初始化Codec的配置参数
 */
Codec::Codec(const Config &cfg) : config_(cfg) {}

/**
 * @brief 设置编解码器的配置参数。
 *
 * 此函数将传入的配置对象赋值给内部配置成员，并根据配置中的最大缓冲区字节数，
 * 调整接收缓冲区的大小。如果当前缓冲区大小超过最大字节数，则缩小缓冲区以符合配置要求。
 *
 * @param cfg 配置参数对象，包含最大缓冲区字节数等信息。
 */
void Codec::setConfig(const Config &cfg)
{
	config_ = cfg;
	if (rxBuffer_.size() > config_.maxBufferBytes)
	{
		rxBuffer_.resize(config_.maxBufferBytes);
	}
}

/**
 * @brief 获取Codec的配置对象。
 *
 * 此方法返回Codec的配置常量引用，允许只读访问配置参数。
 *
 * @return Codec::Config的常量引用，表示当前的配置。
 */
const Codec::Config &Codec::config() const
{
	return config_;
}

/**
 * @brief 编码数据包并输出到指定缓冲区。
 *
 * 此函数将输入的Packet对象编码为特定格式的字节流，并追加到输出缓冲区out中。
 * 编码格式包括帧头、有效载荷和CRC校验。若数据包大小超出配置限制，则设置错误并返回false。
 *
 * @param packet 要编码的数据包对象。
 * @param out 输出字节缓冲区，编码后的数据将追加到此缓冲区末尾。
 * @return 编码成功返回true，失败返回false（如缓冲区溢出）。
 */
bool Codec::encode(const Packet &packet, std::vector<uint8_t> &out) const
{
	if (packet.payload.size() > config_.maxPayloadBytes)
	{
		setError_(Error::BufferOverflow);
		return false;
	}
	FrameHeader hdr{};
	hdr.magic = kMagic;
	hdr.type = static_cast<uint8_t>(packet.type);
	hdr.session = packet.session;
	hdr.cmdId = packet.cmdId;
	hdr.code = packet.code;
	hdr.len = static_cast<uint16_t>(packet.payload.size());

	size_t total = sizeof(FrameHeader) + hdr.len + sizeof(uint32_t);
	if (total > config_.maxBufferBytes)
	{
		setError_(Error::BufferOverflow);
		return false;
	}

	size_t oldSize = out.size();
	out.resize(oldSize + total);
	uint8_t *buf = out.data() + oldSize;
	std::memcpy(buf, &hdr, sizeof(FrameHeader));
	if (hdr.len > 0 && !packet.payload.empty())
	{
		std::memcpy(buf + sizeof(FrameHeader), packet.payload.data(), hdr.len);
	}
	uint32_t crc = checksum32(buf, sizeof(FrameHeader) + hdr.len);
	std::memcpy(buf + sizeof(FrameHeader) + hdr.len, &crc, sizeof(crc));
	return true;
}

/**
 * @brief 向接收缓冲区添加数据。
 *
 * 此函数将指定的数据追加到内部接收缓冲区。如果数据指针为nullptr或长度为0，则不执行任何操作。
 * 若追加数据后缓冲区大小超过配置的最大字节数，则设置缓冲区溢出错误并返回。
 *
 * @param data 指向待添加数据的指针。
 * @param len  待添加数据的长度（字节数）。
 */
void Codec::feed(const uint8_t *data, size_t len)
{
	if (!data || len == 0)
		return;
	if (rxBuffer_.size() + len > config_.maxBufferBytes)
	{
		setError_(Error::BufferOverflow);
		return;
	}
	rxBuffer_.insert(rxBuffer_.end(), data, data + len);
}

/**
 * @brief 获取并解码下一个数据包。
 * 
 * 此函数调用内部的 decodeOne_ 方法，将解码后的数据包存储到 out 参数中。
 * 
 * @param[out] out 用于存储解码后的数据包。
 * @return 如果成功解码一个数据包则返回 true，否则返回 false。
 */
bool Codec::next(Packet &out)
{
	return decodeOne_(out);
}

/**
 * @brief 重置Codec对象的状态。
 *
 * 此函数会清空接收缓冲区（rxBuffer_），并将最后的错误状态（lastError_）重置为无错误（Error::None）。
 * 通常用于初始化或恢复Codec对象到初始状态。
 */
void Codec::reset()
{
	rxBuffer_.clear();
	lastError_ = Error::None;
}

/**
 * @brief 获取当前缓冲区中的字节数。
 *
 * 此函数返回接收缓冲区（rxBuffer_）中已缓冲的字节数量。
 *
 * @return 缓冲区中的字节数。
 */
size_t Codec::bufferedBytes() const
{
	return rxBuffer_.size();
}

/**
 * @brief 获取最后一次操作的错误码
 * @return Codec::Error 最后一次操作产生的错误码
 * @note 该函数为常成员函数，不会修改对象状态
 */
Codec::Error Codec::lastError() const
{
	return lastError_;
}

/**
 * @brief 获取最后一次错误的描述文本
 * 
 * @return const char* 指向最后一次错误文本的常量指针
 * 
 * @details 该函数返回与最后一次记录的错误代码对应的错误描述文本。
 *          通过调用 errorText() 方法并传入 lastError_ 成员变量来获取错误信息。
 */
const char *Codec::lastErrorText() const
{
	return errorText(lastError_);
}

/**
 * @brief 计算数据的32位CRC校验和
 * 
 * 使用多项式0xEDB88320计算给定数据的CRC32校验值。
 * 该函数采用标准的CRC32算法，通过逐位处理每个字节来生成校验和。
 * 
 * @param data 指向输入数据缓冲区的指针
 * @param len 输入数据的长度（字节数）
 * @param seed 初始种子值，用于校验和计算的起点
 * 
 * @return uint32_t 计算得到的CRC32校验值。返回值是最终CRC值按位反转后的结果
 * 
 * @note 
 * - 使用的多项式为0xEDB88320（CRC-32-IEEE 802.3标准）
 * - 该函数对输入种子值进行按位反转作为初始CRC值
 * - 最终返回值也进行了按位反转处理
 * 
 * @example
 * uint8_t data[] = {0x01, 0x02, 0x03};
 * uint32_t crc = Codec::checksum32(data, 3, 0);
 */
uint32_t Codec::checksum32(const uint8_t *data, size_t len, uint32_t seed)
{
	uint32_t crc = ~seed;
	for (size_t i = 0; i < len; ++i)
	{
		crc ^= data[i];
		for (uint8_t j = 0; j < 8; ++j)
		{
			const uint32_t mask = static_cast<uint32_t>(-(static_cast<int32_t>(crc & 1u)));
			crc = (crc >> 1) ^ (0xEDB88320u & mask);
		}
	}
	return ~crc;
}

/**
 * @brief 设置编解码器的错误状态
 * 
 * @details 该函数用于记录编解码过程中发生的错误信息。
 * 通过将错误代码保存到成员变量 lastError_ 中，
 * 以便后续可以查询和处理错误。
 * 
 * @param err 要设置的错误代码
 * 
 * @note 该函数为 const 方法，使用 mutable 修饰符修饰 lastError_
 *       以允许在 const 上下文中修改错误状态。
 * 
 * @see lastError_
 */
void Codec::setError_(Error err) const
{
	lastError_ = err;
}

/**
 * @brief 从接收缓冲区解码一个完整的数据包
 * 
 * 该函数从 rxBuffer_ 中读取并解析一个完整的数据包。数据包格式包括：
 * 帧头 + 负载数据 + CRC32校验码。
 * 
 * @param[out] out 解码后的数据包，包含类型、会话ID、命令ID、状态码和负载数据
 * 
 * @return true 成功解码一个完整的数据包，out 被填充有效数据，rxBuffer_ 中该数据包被移除
 * @return false 解码失败，具体错误原因可通过 getError() 获取
 * 
 * @details
 * - Error::NeedMore: 缓冲区数据不足，等待更多数据
 * - Error::BadMagic: 帧头魔数不匹配，丢弃一个字节并继续尝试
 * - Error::BadLength: 负载长度超过最大允许值 config_.maxPayloadBytes
 * - Error::BadChecksum: CRC32校验失败，丢弃整个损坏的数据包
 * - Error::None: 解码成功
 * 
 * @note
 * - 该函数会修改 rxBuffer_ 的内容，成功或失败都会移除已处理的数据
 * - CRC32 校验范围为：帧头 + 负载数据（不包括 CRC 字段本身）
 */
bool Codec::decodeOne_(Packet &out)
{
	out = Packet{};
	if (rxBuffer_.size() < sizeof(FrameHeader) + sizeof(uint32_t))
	{
		setError_(Error::NeedMore);
		return false;
	}
	const uint8_t *buf = rxBuffer_.data();
	FrameHeader hdr{};
	std::memcpy(&hdr, buf, sizeof(FrameHeader));
	if (hdr.magic != kMagic)
	{
		rxBuffer_.erase(rxBuffer_.begin());
		setError_(Error::BadMagic);
		return false;
	}
	if (hdr.len > config_.maxPayloadBytes)
	{
		rxBuffer_.erase(rxBuffer_.begin(), rxBuffer_.begin() + sizeof(FrameHeader));
		setError_(Error::BadLength);
		return false;
	}
	size_t total = sizeof(FrameHeader) + hdr.len + sizeof(uint32_t);
	if (rxBuffer_.size() < total)
	{
		setError_(Error::NeedMore);
		return false;
	}
	uint32_t crc = 0;
	std::memcpy(&crc, buf + sizeof(FrameHeader) + hdr.len, sizeof(crc));
	uint32_t calc_crc = checksum32(buf, sizeof(FrameHeader) + hdr.len);
	if (crc != calc_crc)
	{
		rxBuffer_.erase(rxBuffer_.begin(), rxBuffer_.begin() + total);
		setError_(Error::BadChecksum);
		return false;
	}
	out.type = static_cast<PacketType>(hdr.type);
	out.session = hdr.session;
	out.cmdId = hdr.cmdId;
	out.code = hdr.code;
	out.payload.resize(hdr.len);
	if (hdr.len > 0)
	{
		std::memcpy(out.payload.data(), buf + sizeof(FrameHeader), hdr.len);
	}
	rxBuffer_.erase(rxBuffer_.begin(), rxBuffer_.begin() + total);
	setError_(Error::None);
	return true;
}
