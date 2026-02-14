#include "transport_usb.hpp"

/**
 * @brief 初始化USB CDC串口通信
 * 
 * @param baud 波特率，用于配置串口通信速度
 * 
 * @return true 初始化完成（无论串口是否真正连接）
 * 
 * @details 
 * 该函数初始化Serial对象并等待USB CDC连接建立。
 * 如果在2000ms内Serial连接成功，则立即返回；
 * 如果在2000ms后Serial仍未连接，也返回true。
 * 每次检查之间延迟10ms以降低CPU占用率。
 */
bool UsbCdcTransport::begin(uint32_t baud)
{
	Serial.begin(baud);

	const unsigned long start = millis();
	while (!Serial && (millis() - start < 2000))
	{
		delay(10);
	}

	return true;
}

/**
 * @brief 获取USB CDC串口可用的字节数
 * 
 * 查询Serial对象中当前可用的待读取字节数。此方法用于检查
 * 是否有数据可从USB CDC传输接口读取。
 * 
 * @return size_t 返回Serial缓冲区中可用的字节数。如果没有
 *         可用数据，则返回0。
 * 
 * @note 该函数将Serial.available()的返回值从整数类型
 *       强制转换为size_t类型。
 */
size_t UsbCdcTransport::available() const
{
	return static_cast<size_t>(Serial.available());
}

/**
 * @brief 从USB CDC串口读取数据
 * 
 * 从串口缓冲区中读取最多指定长度的数据字节。该函数会持续读取
 * 直到达到最大长度限制或没有更多数据可读。
 * 
 * @param dst 指向目标缓冲区的指针，用于存储读取的数据。
 *            不能为nullptr。
 * @param max_len 要读取的最大字节数。必须大于0。
 * 
 * @return 实际读取的字节数。如果dst为nullptr或max_len为0，
 *         返回0。如果串口缓冲区中没有数据，返回0。
 * 
 * @note 此函数为非阻塞操作。如果串口缓冲区中没有足够的数据
 *       达到max_len指定的长度，会返回实际可用的字节数。
 * 
 * @warning 调用者需确保dst指向的缓冲区至少有max_len字节的空间。
 */
size_t UsbCdcTransport::read(uint8_t *dst, size_t max_len)
{
	if (dst == nullptr || max_len == 0)
	{
		return 0;
	}

	size_t total = 0;
	while (total < max_len)
	{
		const int v = Serial.read();
		if (v < 0)
		{
			break;
		}
		dst[total++] = static_cast<uint8_t>(v);
	}

	return total;
}

/**
 * @brief 通过USB CDC串口发送数据
 * 
 * 将指定长度的数据写入USB CDC串口。如果数据指针为空或长度为0，
 * 则不执行任何操作。
 * 
 * @param data 指向要写入数据的指针，不能为nullptr
 * @param len 要写入的数据长度（字节数），不能为0
 * 
 * @return 实际写入的字节数。如果data为nullptr或len为0，返回0；
 *         否则返回Serial.write()的返回值
 * 
 * @note 如果写入失败，返回值可能小于len
 * @see Serial.write()
 */
size_t UsbCdcTransport::write(const uint8_t *data, size_t len)
{
	if (data == nullptr || len == 0)
	{
		return 0;
	}

	return Serial.write(data, len);
}

/**
 * @brief 刷新USB CDC传输缓冲区
 * 
 * 将USB CDC串口的发送缓冲区中的所有数据立即发送出去。
 * 此函数会阻塞直到所有待发送的数据都被写入到硬件缓冲区。
 * 
 * @return void
 * 
 * @note 这是一个同步操作，可能会导致短暂的程序暂停。
 * 
 * @see Serial.flush()
 */
void UsbCdcTransport::flush()
{
	Serial.flush();
}

/**
 * @brief 从USB CDC串口读取一行数据
 * 
 * 该函数尝试从串口读取一行数据，直到遇到换行符('\n')为止。
 * 读取后会自动去除首尾的空白字符。
 * 
 * @param outLine [out] 输出参数，存储读取到的一行数据（不包含换行符）
 * 
 * @return true 如果成功读取到非空的一行数据，返回true
 * @return false 如果串口无可用数据或读取到的为空行，返回false
 * 
 * @note 该函数会阻塞等待换行符，直到收到'\n'才会返回
 * @note 返回的字符串已进行trim()处理，前后空白字符已被移除
 */
bool UsbCdcTransport::readLine(String &outLine)
{
	if (Serial.available() <= 0)
	{
		return false;
	}

	outLine = Serial.readStringUntil('\n');
	outLine.trim();
	return outLine.length() > 0;
}

/**
 * @brief 通过USB CDC接口写入一行数据
 * 
 * 将指定的字符串写入Serial对象，并在末尾追加换行符。
 * 
 * @param line 要写入的字符串内容
 * 
 * @return size_t 实际写入的字符总数，包括字符串和换行符
 * 
 * @note 此函数用于USB CDC串口通讯，可用于日志输出或数据传输
 */
size_t UsbCdcTransport::writeLine(const String &line)
{
	size_t written = Serial.print(line);
	written += Serial.print('\n');
	return written;
}
