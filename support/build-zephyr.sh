#!/bin/bash

echo "Building Zephyr..."

. .venv/bin/activate && west init -l zephyr && west update && west zephyr-export