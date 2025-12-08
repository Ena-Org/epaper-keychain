# Makefile

.PHONY: install run

install:
	@echo "Creating virtual environment and installing PlatformIO..."
	python3 -m venv .pio/venv
	@.pio/venv/bin/pip install platformio

run:
	@echo "Running PlatformIO..."
	@.pio/venv/bin/platformio run
