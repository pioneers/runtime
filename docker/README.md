# Docker

Docker is a system that allows you to consistently reconstruct the same development environment every time. This is especially important to use since not everyone has a Raspberry Pi to test on but Docker allows you to test your code in the same environment. To learn more about Docker and why we use it, read our [wiki page](https://github.com/pioneers/c-runtime/wiki/Docker).

Our latest Docker images will be stored on Docker Hub here https://hub.docker.com/repository/docker/pierobotics/runtime.

## Installation

To install Docker, go here https://docs.docker.com/get-docker/. Note that to install Docker for Windows 10 Home, you first need to install WSL 2 here https://docs.microsoft.com/en-us/windows/wsl/install-win10.

# Usage with Docker Compose (Recommended)

## Installing

If you are on Windows/Mac, you will be using Docker Desktop which will already have `docker-compose` installed. If you are on Linux, you can install it with

    pip3 install docker-compose

## Running

If you are just starting out and want to get into development quickly, go to the `runtime` folder and do

    docker-compose run --rm runtime bash

which will begin a bash shell in the Docker container. From there you can call `runtime run` and the other `runtime` commands mentioned in the root README. Also, in another terminal you can do

    docker-compose exec runtime bash

to run/test other things within the container. By default, the `runtime/` git folder and `/root/runtime` will be linked so changes in one place will also change in the other place.

## Stopping

To stop the container, just exit all your shells or do 

    docker-compose down

# Usage with Docker CLI (Intermediate)

## Pulling

To get the image with the code from the `master` branch, do 

    docker pull pierobotics/runtime:latest
    
## Building

To build your own image instead of using the one on Docker Hub, so that it uses your up to date code instead of code on `master`, do
    
    docker build -t pierobotics/runtime:latest -f docker/Dockerfile ./

## Running

To run the Docker container, do

    docker run -it --rm pierobotics/runtime:latest <CMD>

This CMD can be `./runtime run` to start Runtime, or `bash` to open a shell, or any other command you want to run inside the container. You can also run other things in the container by in another terminal doing `docker exec -it $(docker ps -q) bash` and running whatever you want inside the shell. The `runtime` Git repo will be located at `/root/runtime`.

### Devices

If you want to have the container access any Arduino devices, add the flag `--device /dev/ttyACM0` or whichever path to you device you want to `docker run`. You can do this multiple times for multiple devices.

### Mounting

If you want to mount the `runtime` folder to the Docker container, add the flag `-v $PWD:/root/runtime` to `docker run`. This will allow changes you make inside the Docker container to change the files within your `runtime` directory on your computer, and vice versa as well. This is especially useful if you are using the Docker container to do development.

## Stopping

To stop the container, either exit from the `./runtime run` shell with Ctrl+C or do `docker stop -t 5 $(docker ps -q)` in a separate shell.

# Advanced Topics

## Multi-Architecture Images

The Raspberry Pi 4 uses the arm32/v7 architecture while most consumer computers nowadays use x64/amd64 architecture. To have Docker images that work for both development on our computers and production on the Pis, we make it so that the same tag points to both architectures. When you do `docker pull` or `docker build`, Docker will automatically get the image that has your architecture. This is ideal in most cases.

If you're on a x64 machine and would like to run the arm32 images instead though (other direction isn't possible), you first need to have QEMU, a binary emulator, installed. If you are on Windows/Mac and so have Docker Desktop, QEMU is already installed for you. If you are on Linux, you can install QEMU on Linux with `docker run --rm --privileged multiarch/qemu-user-static --reset -p yes`. Now you can build the `arm32` image with

    DOCKER_CLI_EXPERIMENTAL=enabled docker build --platform linux/arm/v7 -t pierobotics/runtime:latest -f docker/Dockerfile ./

## Base Image (only for PMs)

We use a base image to make sure that the building of the actual image is fast. Building it for multi-architectures requires some experimental Docker features to enable `buildx`, which can be done by setting an environment variable. If for some reason the base image needs to be updated and pushed, do

    cd docker/base
    DOCKER_CLI_EXPERIMENTAL=enabled docker buildx create --use      # Only needed the very first time on a computer
    DOCKER_CLI_EXPERIMENTAL=enabled docker buildx build --platform linux/amd64,linux/arm/v7 --build-arg BUILDKIT_INLINE_CACHE=1 -t pierobotics/runtime:base --push ./

If you haven't run this already on your machine and so don't have local caches, it will take several hours. Importantly, this can only be run on x64 machines that have QEMU, a binary emulator, installed. If you're on Windows/Mac, Docker Desktop already has QEMU preinstalled. If you are using Linux, you will need to install QEMU before building by doing `docker run --rm --privileged multiarch/qemu-user-static --reset -p yes`.
