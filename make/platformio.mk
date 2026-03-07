.PHONY: help platformio\:export platformio\:install platformio\:build platformio\:clean

VENV_DIR ?= .pio/venv
VENV_BIN ?= $(VENV_DIR)/bin
VENV_PY ?= $(VENV_BIN)/python
PIP ?= $(VENV_PY) -m pip
PLATFORMIO_BIN ?= $(VENV_BIN)/platformio
PLATFORMIO_SETTING_PROJECTS_DIR ?= /home/ena/workspace/epaper-keychain

help::
	@echo "  make platformio:export  导出 PlatformIO 项目目录环境变量"
	@echo "  make platformio:install 在虚拟环境中安装 PlatformIO"
	@echo "  make platformio:build   使用 PlatformIO 构建项目"
	@echo "  make platformio:clean   清理 PlatformIO 构建产物"

platformio\:export:
	@echo "export PLATFORMIO_SETTING_PROJECTS_DIR=$(PLATFORMIO_SETTING_PROJECTS_DIR)"

platformio\:install: platformio\:export python\:install
	@echo "Installing PlatformIO into $(VENV_DIR)..."
	@$(PIP) install --upgrade pip
	@$(PIP) install platformio

platformio\:build: platformio\:export platformio\:install
	@echo "Running PlatformIO build..."
	@PLATFORMIO_SETTING_PROJECTS_DIR="$(PLATFORMIO_SETTING_PROJECTS_DIR)" "$(PLATFORMIO_BIN)" run

platformio\:clean: platformio\:export
	@if [ -x "$(PLATFORMIO_BIN)" ]; then \
		echo "Cleaning PlatformIO build artifacts..."; \
		PLATFORMIO_SETTING_PROJECTS_DIR="$(PLATFORMIO_SETTING_PROJECTS_DIR)" "$(PLATFORMIO_BIN)" run --target clean; \
	else \
		echo "PlatformIO not found at $(PLATFORMIO_BIN), skipping build clean."; \
	fi