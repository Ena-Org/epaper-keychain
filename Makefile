SHELL := /bin/bash
.DEFAULT_GOAL := help

help::
	@echo "fedora:  检查并修复 Fedora Linux 的系统依赖缺失"
	@echo "python:  管理 Python 虚拟环境"
	@echo "opencode: 管理项目本地 OpenCode 安装"

include make/fedora.mk
include make/python.mk
include make/opencode.mk