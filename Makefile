# Makefile

.PHONY: install run upload clean serial-perms

PYTHON ?= python3
VENV_DIR := .pio/venv
VENV_BIN := $(VENV_DIR)/bin
PIP := $(VENV_BIN)/pip
PLATFORMIO := $(VENV_BIN)/platformio
PORT ?= /dev/ttyACM0

$(VENV_DIR):
	@echo "Checking pip availability..."
	@$(PYTHON) -m pip --version >/dev/null 2>&1 || { \
		echo "pip is missing. Installing python3-pip..."; \
		sudo apt install -y python3-pip; \
	}
	@echo "Creating virtual environment..."
	@$(PYTHON) -m venv $(VENV_DIR) || { \
		echo "venv failed; installing python3.12-venv..."; \
		sudo apt install -y python3.12-venv; \
		$(PYTHON) -m venv $(VENV_DIR); \
	}
	@echo "Installing PlatformIO..."
	@$(PIP) install --upgrade pip
	@$(PIP) install platformio
	@echo "Environment ready at $(VENV_DIR)"

install: $(VENV_DIR) serial-perms
	@echo "Install complete; serial permissions applied to $(PORT)"

run: $(VENV_DIR)
	@echo "Running PlatformIO..."
	@$(PLATFORMIO) run

serial-perms:
	@echo "Granting temporary RW access to $(PORT) (sudo; lasts until unplug/reboot)..."
	@sudo chmod a+rw $(PORT) || { echo "Port $(PORT) not available; plug in device."; exit 1; }

upload: $(VENV_DIR) serial-perms
	@echo "Uploading via PlatformIO..."
	@$(PLATFORMIO) run --target upload

clean:
	rm -rf $(VENV_DIR)
