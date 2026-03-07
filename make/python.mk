.PHONY: help python\:install python\:clean

VENV_DIR := .pio/venv
VENV_BIN := $(VENV_DIR)/bin
VENV_PY := $(VENV_BIN)/python
VENV_READY := $(VENV_DIR)/.ready

help::
	@echo "  make python:install    创建 Python 虚拟环境"
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
	@touch $@
	@echo "Environment ready at $(VENV_DIR)"

python\:install: $(VENV_READY)

python\:clean:
	rm -rf "$(VENV_DIR)"