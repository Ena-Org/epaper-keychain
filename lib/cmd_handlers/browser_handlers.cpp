#include "cmd_handlers_registry.hpp"
#include "browser_connection_state.hpp"
#include "cmd/cmd.hpp"
#include "cmd_ids.hpp"

#include <atomic>

namespace
{
	std::atomic<bool> g_browser_connected(false);
}

namespace BrowserConnectionState
{
	/**
	 * @brief 设置浏览器连接状态
	 *
	 * @param connected 连接状态标志
	 *                  - true: 浏览器已连接
	 *                  - false: 浏览器未连接
	 *
	 * @details 该函数使用原子操作将全局变量 g_browser_connected 更新为指定的连接状态。
	 *          确保在多线程环境下的线程安全性。
	 *
	 * @note 该函数是线程安全的，使用了原子存储操作。
	 */
	void set_connected(bool connected)
	{
		g_browser_connected.store(connected);
	}

	/**
	 * @brief 检查浏览器是否已连接
	 * @details 通过原子变量 g_browser_connected 的状态判断浏览器连接状态
	 * @return bool 如果浏览器已连接返回 true，否则返回 false
	 */
	bool is_connected()
	{
		return g_browser_connected.load();
	}
}

namespace CmdHandlers
{
	namespace
	{
		/**
		 * @brief 数据包类型别名
		 *
		 * 将 Protocol::Packet 定义为局部类型别名 Packet，用于简化代码中对数据包类型的引用。
		 * 这个别名主要用于浏览器处理器模块内的数据包操作。
		 *
		 * @note 这是一个类型别名，不会创建新的类型，仅是为了提高代码可读性。
		 */
		using Packet = Protocol::Packet;

		/// @brief 浏览器错误代码：无效的上下文
		/// @details 当浏览器操作在无效或不存在的上下文中执行时返回此错误代码
		/// @value 1
		constexpr uint16_t kBrowserErrorInvalidContext = 1;

		/**
		 * @brief 从路由上下文中获取命令对象指针
		 *
		 * 该函数将路由器上下文(Router::Context)转换为命令对象(Cmd)指针。
		 * 通过static_cast进行类型转换，假设Cmd是Router::Context的基类或兼容类型。
		 *
		 * @param ctx 路由器上下文对象的引用
		 * @return Cmd* 指向命令对象的指针
		 *
		 * @note 该函数为内部私有函数(以下划线后缀标记)，仅在当前模块内使用
		 * @warning 调用者需确保ctx对象的生命周期足够长，指针在使用期间保持有效
		 */
		Cmd *get_browser_cmd_context_(Router::Context &ctx)
		{
			return static_cast<Cmd *>(&ctx);
		}
	}

	/**
	 * @brief 注册浏览器相关的命令处理器
	 * 
	 * 为路由器注册浏览器连接和断开连接的命令处理器。
	 * 
	 * @param router 路由器引用，用于注册命令处理器
	 * 
	 * @details
	 * 注册以下两个处理器：
	 * - BrowserConnect: 处理浏览器连接请求，设置浏览器连接状态为已连接(true)
	 * - BrowserDisconnect: 处理浏览器断开连接请求，设置浏览器连接状态为未连接(false)
	 * 
	 * 两个处理器都会：
	 * 1. 验证数据包类型是否为请求(Request)
	 * 2. 获取浏览器命令上下文
	 * 3. 如果上下文无效，返回错误状态码 kBrowserErrorInvalidContext
	 * 4. 否则更新浏览器连接状态并回复请求
	 * 
	 * @note 处理器通过lambda函数实现，每个处理器都验证数据包类型和上下文有效性
	 */
	void registerBrowserHandlers(Router &router)
	{
		router.registerHandler(CmdId::BrowserConnect, [](const Packet &packet, Router::Context &ctx)
													 {
														 if (packet.type != Protocol::PacketType::Request)
														 {
															 return;
														 }

														 Cmd *cmd = get_browser_cmd_context_(ctx);
														 if (cmd == nullptr)
														 {
															 ctx.reply(packet.session, packet.cmdId, kBrowserErrorInvalidContext, nullptr, 0);
															 return;
														 }

														 cmd->setBrowserConnected(true);
														 const uint8_t enabled = 1;
														 ctx.reply(packet.session, packet.cmdId, 0, &enabled, 1); });

		router.registerHandler(CmdId::BrowserDisconnect, [](const Packet &packet, Router::Context &ctx)
													 {
														 if (packet.type != Protocol::PacketType::Request)
														 {
															 return;
														 }

														 Cmd *cmd = get_browser_cmd_context_(ctx);
														 if (cmd == nullptr)
														 {
															 ctx.reply(packet.session, packet.cmdId, kBrowserErrorInvalidContext, nullptr, 0);
															 return;
														 }

														 cmd->setBrowserConnected(false);
														 const uint8_t enabled = 0;
														 ctx.reply(packet.session, packet.cmdId, 0, &enabled, 1); });
	}
}