#include "epaper_render.hpp"

#include <Arduino.h>

#include "browser_connection_state.hpp"
#include "epaper.hpp"
#include "log_streaming.hpp"
#include "system_view.hpp"

namespace
{
	bool g_initialized = false;
	EpaperRender::DisplayMode g_mode = EpaperRender::DisplayMode::SystemView;
	LogStreaming g_log_streaming;
	SystemView g_system_view;
}

void EpaperRender::init()
{
	EPaper::init();
	g_log_streaming.init();
	g_system_view.init();

	const bool connected = BrowserConnectionState::is_connected();
	g_mode = connected ? DisplayMode::LogStreaming : DisplayMode::SystemView;
	switch_mode_(g_mode);
	g_initialized = true;
}

void EpaperRender::loop()
{
	if (!g_initialized)
	{
		return;
	}

	const bool connected = BrowserConnectionState::is_connected();
	const DisplayMode target_mode = connected ? DisplayMode::LogStreaming : DisplayMode::SystemView;
	if (target_mode != g_mode)
	{
		switch_mode_(target_mode);
	}

	const uint32_t now_ms = millis();
	if (g_mode == DisplayMode::LogStreaming)
	{
		g_log_streaming.tick(now_ms);
		if (g_log_streaming.needs_render())
		{
			g_log_streaming.render();
		}
		return;
	}

	if (g_system_view.needs_render())
	{
		g_system_view.render();
	}
}

void EpaperRender::switch_mode_(DisplayMode next_mode)
{
	g_mode = next_mode;
	if (g_mode == DisplayMode::LogStreaming)
	{
		g_log_streaming.force_refresh();
		return;
	}

	g_system_view.mark_dirty();
}