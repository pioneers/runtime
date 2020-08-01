# Docker

Docker is a system that allows you to consistently reconstruct the same development environment every time. This is especially important to use since not everyone has a Raspberry Pi to test on but Docker allows you to test your code in the same environment. 

## Terminology

A Docker **container** can be thought of as a lightweight VM that encapsulates your dependencies and your code. It is essentially shielded from the rest of your computer and even allows it to emulate other OSes. 

A Docker **image** is a compressed object holding all the dependencies that you specify. A Docker image runs inside a Docker container.

A **Dockerfile** is how you specify the dependencies in your image. It uses a simplified command structure (see https://docs.docker.com/engine/reference/builder/) which allows you to run any bash command. The nice thing about a Docker image is that it caches the image between every `RUN`, `ADD` or `COPY` command. This allows you to build images quickly after you've built it the first time. 

Our latest Docker images will be stored on Docker Hub here https://hub.docker.com/repository/docker/pierobotics/c-runtime.


## Installation

To install Docker, go here https://docs.docker.com/get-docker/. If you're on Windows and don't have Windows 10 Pro, it is highly recommended to install WSL 2 here https://docs.microsoft.com/en-us/windows/wsl/install-win10.

## Pulling

To get the image to emulate a Raspberry Pi, do `docker pull pierobotics/c-runtime:latest`. This will have the code from the `master` branch.

## Building

To build your own image instead of using the one on Docker Hub, so that it uses your up to date code instead of code on `master`, do
    
    docker build -t pierobotics/c-runtime:latest -f docker/Dockerfile .

If you're on a x64 machine and have the ability to emulate ARM32 either by using Docker Desktop on Windows/Mac or have installed QEMU on Linux (with `docker run --rm --privileged multiarch/qemu-user-static --reset -p yes`), you can instead build an ARM32 image with 

    DOCKER_CLI_EXPERIMENTAL=enabled docker build --platform linux/arm/v7 -t pierobotics/c-runtime:$TAG -f docker/Dockerfile .

### Base Multi-Architecture Image

We use a base image to make sure that the building of the actual image is fast. Our images are also multiarchitecture and building them requires some experimental Docker features to enable `buildx`. If for some reason the base image needs to be updated and pushed, do

    cd docker/base
    DOCKER_CLI_EXPERIMENTAL=enabled docker buildx create --use   # Only needed the very first time on a computer
    DOCKER_CLI_EXPERIMENTAL=enabled docker buildx build --platform linux/amd64,linux/arm/v7 --build-arg BUILDKIT_INLINE_CACHE=1 --cache-from pierobotics/c-runtime:base -t pierobotics/c-runtime:base --push .

If you haven't run this already on your machine and so don't have local caches, it will take several hours. Also, this can only be run on x64 machines. If you are using Linux, you will also need to install QEMU before building by doing `docker run --rm --privileged multiarch/qemu-user-static --reset -p yes`.

## Running

To run the latest Runtime inside a Docker container, do `docker run -it --rm pierobotics/c-runtime:latest`. Then you can test by in another terminal doing `docker exec -it $(docker ps -q) bash` and running whatever you want inside the container. The `c-runtime` repo will be located at `/root/runtime`.

### Devices

If you want to have the container access any Arduino devices, add the flag `--device /dev/ttyACM0` or what ever devices you want to `docker run`.

## Stopping

To stop the container, either exit from the `./run.sh` shell with Ctrl+C or do `docker stop -t 5 $(docker ps -q)`.

