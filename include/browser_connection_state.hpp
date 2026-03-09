#pragma once

namespace BrowserConnectionState
{
  /**
   * @brief 设置连接状态
   * @param connected 连接状态标志
   *                  - true: 表示已连接
   *                  - false: 表示未连接
   */
  void set_connected(bool connected);

  /**
   * @brief 检查浏览器连接状态
   *
   * 判断设备是否与浏览器建立了连接
   *
   * @return bool 如果已连接返回 true，否则返回 false
   */
  bool is_connected();
}
