#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

class UsbCdcTransport
{
public:
  /**
   * @brief 初始化 USB 传输接口
   * @param baud 波特率，默认值为 115200
   * @return bool 初始化是否成功
   *             true  - 初始化成功
   *             false - 初始化失败
   * @note 该函数应在使用 USB 传输其他功能前调用
   * @see end()
   */
  bool begin(uint32_t baud = 115200);

  /**
   * @brief 获取当前可用的数据大小
   * @return size_t 返回可用数据的字节数
   */
  size_t available() const;

  /**
   * @brief 从USB传输通道读取数据
   * 
   * 从USB设备读取最多 max_len 字节的数据到指定的缓冲区中。
   * 
   * @param dst 指向目标缓冲区的指针，用于存储读取的数据
   * @param max_len 最多要读取的字节数
   * 
   * @return size_t 实际读取的字节数。如果返回值小于 max_len，
   *         表示读取操作已完成或遇到错误
   * 
   * @note 调用者需要确保 dst 指针指向的缓冲区大小至少为 max_len 字节
   * @note 如果没有可读的数据，此函数可能阻塞或返回0，具体行为取决于实现
   */
  size_t read(uint8_t *dst, size_t max_len);

  /**
   * @brief 通过USB传输写入数据
   * 
   * @param data 指向要写入数据的缓冲区的指针
   * @param len 要写入的字节数
   * 
   * @return 返回实际写入的字节数，如果写入失败则返回0
   */
  size_t write(const uint8_t *data, size_t len);

  /**
   * @brief 刷新USB传输缓冲区
   * @details 将缓冲区中的待发送数据立即发送到USB设备，
   *          确保所有待处理的数据都被写入。
   * @return void
   * @note 该函数会阻塞直到缓冲区数据完全刷新
   */
  void flush();

  /**
   * @brief 从USB传输中读取一行数据
   * 
   * 该函数从USB传输接口读取一行数据，直到遇到换行符或传输结束。
   * 
   * @param outLine 输出参数，用于存储读取的一行数据（不包括换行符）
   * 
   * @return 如果成功读取一行数据返回true，如果读取失败或无数据返回false
   * 
   * @note 函数会阻塞直到读取完整的一行数据或发生错误
   */
  bool readLine(String &outLine);

  /**
   * @brief 通过USB传输写入一行数据
   * 
   * @param line 要写入的字符串数据
   * @return size_t 实际写入的字节数
   * 
   * @details 将指定的字符串通过USB传输发送出去。如果写入失败，
   *          返回值将小于预期的字节数。
   */
  size_t writeLine(const String &line);
};
