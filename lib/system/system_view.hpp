#pragma once

class SystemView
{
public:
  /**
   * @brief 初始化系统视图
   * 
   * 该函数负责初始化系统视图的各项配置和状态。
   * 包括设置初始显示参数、资源加载和事件处理器配置等。
   * 
   * @note 该函数应在系统启动时调用一次
   * @see system_view 类
   */
  void init();

  /**
   * @brief 检查系统视图是否需要重新渲染
   * 
   * @return true 表示需要重新渲染屏幕内容
   * @return false 表示无需重新渲染
   */
  bool needs_render() const;

  /**
   * @brief 渲染系统视图
   * 
   * 该函数负责更新和显示系统视图的内容。
   * 它会处理所有必要的渲染操作，将当前状态
   * 显示到屏幕或显示设备上。
   */
  void render();

  /**
   * @brief 标记视图为脏状态，需要重新绘制
   * 
   * 当视图的内容发生变化时，调用此方法将视图标记为脏状态。
   * 这将触发下一次刷新循环中的重新绘制操作。
   */
  void mark_dirty();

private:
  bool dirty_ = true;
};
