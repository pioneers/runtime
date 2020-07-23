#/usr/bin/env bash

# docker pull arm32v7/debian:buster
# docker pull ${DOCKER_USERNAME}/c-runtime:arm32
DOCKER_BUILDKIT=1 docker build --build-arg BUILDKIT_INLINE_CACHE=1 --cache-from avsurfer123/c-runtime:arm32 -t avsurfer123/c-runtime:arm32 -f docker/Dockerfile .
docker login -u ${DOCKER_USERNAME} -p ${DOCKER_PASSWORD}
docker push ${DOCKER_USERNAME}/c-runtime:arm32
