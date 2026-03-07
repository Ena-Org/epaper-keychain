.PHONY: help init-all clean-all

help::
	@echo "  make init-all          初始化完整本地开发环境"
	@echo "  make clean-all         删除 Python 虚拟环境和项目本地 OpenCode"

init-all: python\:install opencode\:install
	@echo "Full local environment is ready."

clean-all: python\:clean opencode\:clean