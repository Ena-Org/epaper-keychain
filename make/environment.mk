.PHONY: help init-all clean-all

help::
	@echo "  make init-all          初始化完整本地开发环境"
	@echo "  make clean-all         清理 PlatformIO 构建产物并删除本地开发环境"

init-all: deps\:check platformio\:export python\:install platformio\:install opencode\:install
	@echo "Full local environment is ready."

clean-all: platformio\:clean python\:clean opencode\:clean