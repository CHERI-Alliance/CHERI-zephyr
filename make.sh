#!/bin/bash
set -euo pipefail

IMAGE_NAME="coupling-test"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

if ! docker image inspect "$IMAGE_NAME" &>/dev/null; then
    echo "Building Docker image '$IMAGE_NAME'..."
    docker build -t "$IMAGE_NAME" - < "$SCRIPT_DIR/Dockerfile"
fi

DOCKER_TTY_FLAG=""
if [ -t 0 ]; then
    DOCKER_TTY_FLAG="-it"
fi

DOCKER_USER_FLAG="--user $(id -u):$(id -g)"

exec docker run --rm $DOCKER_TTY_FLAG \
    -v "$SCRIPT_DIR:/work" \
    -e ZEPHYR_SDK=/zephyr-sdk-1.0.1 \
    -e HOME=/work \
    $DOCKER_USER_FLAG \
    "$IMAGE_NAME" \
    make "$@"
