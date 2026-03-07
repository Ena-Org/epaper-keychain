.PHONY: help opencode\:install opencode\:run opencode\:clean

OPENCODE_DIR := .tools/opencode
OPENCODE_BIN_DIR := $(OPENCODE_DIR)/bin
OPENCODE_BIN := $(OPENCODE_BIN_DIR)/opencode

help::
	@echo "  make opencode:install  安装项目本地 OpenCode"
	@echo "  make opencode:run      启动项目本地 OpenCode"
	@echo "  make opencode:clean    删除项目本地 OpenCode"

$(OPENCODE_BIN):
	@command -v $(CURL) >/dev/null 2>&1 || { \
		echo "Error: curl not found. Install curl first."; \
		exit 1; \
	}
	@mkdir -p "$(OPENCODE_BIN_DIR)"
	@echo "Installing OpenCode into $(OPENCODE_BIN_DIR)..."
	@OPENCODE_INSTALL_DIR="$(abspath $(OPENCODE_BIN_DIR))" $(CURL) -fsSL https://opencode.ai/install | bash
	@if [ -x "$(OPENCODE_BIN)" ]; then \
		echo "OpenCode installed at $(OPENCODE_BIN)"; \
	elif [ -x "$$HOME/.opencode/bin/opencode" ]; then \
		echo "Warning: installer ignored OPENCODE_INSTALL_DIR"; \
		echo "Creating project-local symlink fallback..."; \
		ln -sf "$$HOME/.opencode/bin/opencode" "$(OPENCODE_BIN)"; \
		echo "Symlink created at $(OPENCODE_BIN)"; \
	else \
		echo "Error: OpenCode installation failed; binary not found."; \
		exit 1; \
	fi

opencode\:install: $(OPENCODE_BIN)

opencode\:run: $(OPENCODE_BIN)
	@"$(OPENCODE_BIN)"

opencode\:clean:
	rm -rf "$(OPENCODE_DIR)"