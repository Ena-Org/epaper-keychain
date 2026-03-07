SHELL := /bin/bash
.DEFAULT_GOAL := help

PYTHON ?= python3
CURL ?= curl

include make/python.mk
include make/opencode.mk
include make/environment.mk