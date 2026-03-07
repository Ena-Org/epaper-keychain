SHELL := /bin/bash
.DEFAULT_GOAL := help

PYTHON ?= python3
CURL ?= curl

include make/deps.mk
include make/python.mk
include make/platformio.mk
include make/opencode.mk
include make/environment.mk