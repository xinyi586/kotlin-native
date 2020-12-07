#!/bin/bash
set -eou pipefail

CONTAINER_NAME=kotlin-toolchain-builder
IMAGE_NAME=kotlin-toolchain-builder
TARGET=$1
VERSION=$2

docker ps -a | grep $CONTAINER_NAME > /dev/null \
  && docker stop $CONTAINER_NAME &>- \
  && docker rm $CONTAINER_NAME &>-

echo "Running build script in container..."
docker run -it -v "$PWD"/artifacts:/artifacts --env TARGET="$TARGET" --env VERSION="$VERSION" --name=$CONTAINER_NAME $IMAGE_NAME
echo "Done."