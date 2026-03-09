#include "system_view.hpp"

#include "epaper.hpp"

/**
 * @brief 初始化系统视图
 * 
 * 将脏标志设置为true，标记系统视图需要进行更新/刷新。
 * 此方法应在系统视图创建或需要重新初始化时调用。
 */
void SystemView::init()
{
  dirty_ = true;
}

/**
 * @brief 检查系统视图是否需要重新渲染
 * 
 * @return bool 如果视图被标记为脏（需要更新），则返回 true；
 *             否则返回 false
 * 
 * @note 当视图的内容发生变化时，dirty_ 标志会被设置为 true，
 *       表示需要重新渲染该视图
 */
bool SystemView::needs_render() const
{
  return dirty_;
}

/**
 * @brief 渲染系统视图
 * 
 * 该函数负责显示系统视图的占位符内容。
 * 调用电子纸显示器的系统占位符显示方法，展示标题为"System"、
 * 样式为"Style placeholder"的内容。
 * 执行完成后将脏标记位设置为false，表示视图已同步。
 * 
 * @return void
 * 
 * @note 该函数会更新脏标记状态，调用者无需手动管理渲染状态
 */
void SystemView::render()
{
  EPaper::showSystemPlaceholder("System", "Style placeholder");
  dirty_ = false;
}

/**
 * @brief 标记系统视图为脏状态
 * 
 * 将系统视图的脏标志设置为真，表示视图需要重新绘制或更新。
 * 这通常在视图的内容或状态发生变化时调用。
 */
void SystemView::mark_dirty()
{
  dirty_ = true;
}
