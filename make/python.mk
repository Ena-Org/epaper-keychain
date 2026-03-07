.PHONY: help python\:install python\:run python\:clean

VENV_DIR := .pio/venv
VENV_BIN := $(VENV_DIR)/bin
VENV_PY := $(VENV_BIN)/python
PIP := $(VENV_PY) -m pip
PLATFORMIO := $(VENV_BIN)/platformio
VENV_READY := $(VENV_DIR)/.ready

help::
	@echo "  make python:install    创建 Python 虚拟环境并安装 PlatformIO"
	@echo "  make python:run        使用 PlatformIO 构建项目"
	@echo "  make python:clean      删除 Python 虚拟环境"

$(VENV_READY):
	@command -v $(PYTHON) >/dev/null 2>&1 || { \
		echo "Error: $(PYTHON) not found. Install Python 3 first."; \
		exit 1; \
	}
	@$(PYTHON) -m venv --help >/dev/null 2>&1 || { \
		echo "Error: Python venv module is unavailable."; \
		exit 1; \
	}
	@echo "Creating virtual environment at $(VENV_DIR)..."
	@$(PYTHON) -m venv $(VENV_DIR)
	@echo "Installing PlatformIO..."
	@$(PIP) install --upgrade pip
	@$(PIP) install platformio
	@touch $@
	@echo "Environment ready at $(VENV_DIR)"

python\:install: $(VENV_READY)

python\:run: $(VENV_READY)
	@echo "Running PlatformIO..."
	@$(PLATFORMIO) run

python\:clean:
	rm -rf "$(VENV_DIR)"