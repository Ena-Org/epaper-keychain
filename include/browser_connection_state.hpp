#pragma once

namespace BrowserConnectionState
{
  /**
   * @brief 设置浏览器全局连接状态
   *
   * @param connected true 表示已连接，false 表示已断开
   */
  void set_connected(bool connected);

  /**
   * @brief 获取浏览器全局连接状态
   *
   * @return true 浏览器已连接
   * @return false 浏览器未连接
   */
  bool is_connected();
}
