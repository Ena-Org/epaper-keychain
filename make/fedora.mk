.PHONY: fedora\:help fedora\:dep-check fedora\:dep-install

FEDORA_DEPS := xdg-user-dirs

fedora\:help::
	@echo "  make deps:check        检查系统依赖是否已安装"
	@echo "  make deps:install      安装系统依赖 (需要 sudo)"

fedora\:dep-check:
	@missing=""; \
	for cmd in xdg-user-dir; do \
		if ! command -v "$$cmd" >/dev/null 2>&1; then \
			missing="$$missing $$cmd"; \
		fi; \
	done; \
	if [ -n "$$missing" ]; then \
		echo "缺少以下命令:$$missing"; \
		echo "请执行 make deps:install 安装"; \
		exit 1; \
	else \
		echo "系统依赖检查通过"; \
	fi

fedora\:dep-install:
	@if ! command -v dnf >/dev/null 2>&1; then \
		echo "Error: dnf 未找到，此目标仅适用于 Fedora Linux"; \
		exit 1; \
	fi
	@echo "正在安装系统依赖: $(FEDORA_DEPS)..."
	sudo dnf install -y $(FEDORA_DEPS)
	@echo "系统依赖安装完成"
