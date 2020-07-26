# Docker

Docker is a system that allows you to consistently reconstruct the same development environment every time. This is especially important to use since not everyone has a Raspberry Pi to test on but Docker allows you to test your code in the same environment. 

## Terminology

A Docker **container** can be thought of as a lightweight VM that encapsulates your dependencies and your code. It is essentially shielded from the rest of your computer and even allows it to emulate other OSes. 

A Docker **image** is a compressed object holding all the dependencies that you specify. A Docker image runs inside a Docker container.

A **Dockerfile** is how you specify the dependencies in your image. It uses a simplified command structure (see https://docs.docker.com/engine/reference/builder/) which allows you to run any bash command. The nice thing about a Docker image is that it caches the image between every `RUN`, `ADD` or `COPY` command. This allows you to build images quickly after you've built it the first time. 

Our latest Docker images will be stored on Docker Hub here https://hub.docker.com/repository/docker/avsurfer123/c-runtime.


## Installation

To install Docker, go here https://docs.docker.com/get-docker/.

## Pulling

To get the image to emulate a Raspberry Pi, do `docker pull avsurfer123/c-runtime:latest`. This will have the code from the `master` branch.

## Building

To build your own image instead of using the one on Docker Hub, so that it uses your up to date code instead of code on `master`, do
    
    DOCKER_BUILDKIT=1 docker build --build-arg BUILDKIT_INLINE_CACHE=1 -t avsurfer123/c-runtime:latest -f docker/Dockerfile .

## Running

To run the latest Runtime inside a Docker container, do `docker run -it --rm avsurfer123/c-runtime:latest`. Then you can test by in another terminal doing `docker exec -it $(docker ps -q) bash` and running whatever you want inside the container. The `c-runtime` repo will be located at `/root/runtime`.

If you want to have the container access any Arduino devices, add the flag `--device /dev/ttyACM0` or what ever devices you want to `docker run`.

## Stopping

To stop the container, either exit from the `docker run` shell with Ctrl+C or do `docker kill -s INT $(docker ps -q)`.

## Pushing

To push a built image to Docker Hub (only if you have push access), do `docker push avsurfer123/c-runtime:{TAG}`. You can change the tag before pushing by `docker tag avsurfer123/c-runtime:{OLD_TAG} avsurfer123/c-runtime:{NEW_TAG}`.
