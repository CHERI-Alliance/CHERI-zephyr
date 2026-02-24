#!/bin/sh
set -e

export DOCKER_BUILDKIT=1

docker build -f ./cheri-zephyr-toolbuilder.Dockerfile -t cheri-toolbuilder .

container_id=$(docker create cheri-toolbuilder:latest)
docker cp $container_id:/home/user/output/. .
docker rm $container_id
