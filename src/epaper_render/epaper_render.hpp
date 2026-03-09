#pragma once

class EpaperRender
{
public:
	enum class DisplayMode : unsigned char
	{
		LogStreaming,
		SystemView,
	};

public:
	static void init();

	static void loop();

private:
	static void switch_mode_(DisplayMode next_mode);
};