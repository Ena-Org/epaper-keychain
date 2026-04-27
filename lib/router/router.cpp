#include "router.hpp"
#include <algorithm>

/**
 * @brief 注册或更新命令处理器
 *
 * 将指定的命令ID与其对应的处理器函数进行绑定。如果该命令ID已存在，
 * 则更新其处理器；如果不存在，则创建新的路由条目。
 *
 * @param cmdId 命令ID，用于标识该处理器对应的命令类型
 * @param handler 处理器函数，用来处理指定命令ID的请求。
 *                如果传入nullptr或无效的处理器，函数将返回false
 *
 * @return true 如果注册或更新成功；false 如果处理器无效（为nullptr）
 *
 * @note 该函数使用std::find_if遍历现有的路由表来查找是否已存在该命令ID
 * @note 处理器在注册时将通过std::move进行移动语义处理，以提高性能
 */
bool Router::registerHandler(uint16_t cmdId, Handler handler)
{
  if (!handler)
  {
    return false;
  }

  auto it = std::find_if(routes_.begin(), routes_.end(), [cmdId](const RouteEntry &entry)
                         { return entry.cmdId == cmdId; });
  if (it != routes_.end())
  {
    it->handler = std::move(handler);
    return true;
  }

  RouteEntry entry;
  entry.cmdId = cmdId;
  entry.handler = std::move(handler);
  routes_.push_back(std::move(entry));
  return true;
}

/**
 * @brief 注销指定命令ID对应的处理器
 *
 * @param cmdId 要注销的命令ID
 *
 * @return true 如果成功注销了至少一个处理器，false 如果没有找到对应的处理器
 *
 * @details 该函数会遍历路由表，移除所有命令ID匹配的路由条目。
 *          使用erase-remove惯用法来高效地删除多个元素。
 */
bool Router::unregisterHandler(uint16_t cmdId)
{
  const auto oldSize = routes_.size();
  routes_.erase(
      std::remove_if(routes_.begin(), routes_.end(), [cmdId](const RouteEntry &entry)
                     { return entry.cmdId == cmdId; }),
      routes_.end());

  return routes_.size() != oldSize;
}

/**
 * @brief 清除所有路由处理器
 *
 * 清空路由器中所有已注册的路由处理器。
 * 调用此函数后，路由器将不再能够处理任何已注册的路由请求。
 *
 * @note 此操作为破坏性操作，清除后的路由无法恢复
 *
 * @see Router::addRoute
 */
void Router::clearHandlers()
{
  routes_.clear();
}

/**
 * @brief 检查路由器中是否存在指定命令ID的处理器
 *
 * @param cmdId 要查询的命令ID
 *
 * @return true 如果存在该命令ID对应的有效处理器，false 否则
 *
 * @details 此函数通过遍历路由表，查找与给定cmdId匹配且处理器指针非空的路由项。
 *          只有当路由项的命令ID匹配且处理器有效时，才返回true。
 */
bool Router::hasHandler(uint16_t cmdId) const
{
  const auto it = std::find_if(routes_.begin(), routes_.end(), [cmdId](const RouteEntry &entry)
                               { return entry.cmdId == cmdId && static_cast<bool>(entry.handler); });
  return it != routes_.end();
}

/**
 * @brief 获取路由处理器的数量
 *
 * @return size_t 当前注册的路由处理器数量
 */
size_t Router::handlerCount() const
{
  return routes_.size();
}

/**
 * @brief 根据数据包的命令ID分发到对应的处理器
 *
 * 在已注册的路由表中查找与数据包命令ID匹配的路由项，
 * 如果找到且处理器有效，则调用相应的处理器进行处理。
 *
 * @param packet 待分发的数据包，包含命令ID和其他数据
 * @param ctx 执行上下文，传递给路由处理器
 *
 * @return true 如果成功找到匹配的路由并执行处理器
 * @return false 如果未找到匹配的路由或处理器无效
 *
 * @note 处理器的执行结果不会直接反映到返回值中，
 *       仅表示是否成功找到并执行了对应的处理器
 */
bool Router::dispatch(const Packet &packet, Context &ctx) const
{
  const auto it = std::find_if(routes_.begin(), routes_.end(), [&packet](const RouteEntry &entry)
                               { return entry.cmdId == packet.cmdId; });
  if (it == routes_.end() || !it->handler)
  {
    return false;
  }

  it->handler(packet, ctx);
  return true;
}
