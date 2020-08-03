# Docker

Docker is a system that allows you to consistently reconstruct the same development environment every time. This is especially important to use since not everyone has a Raspberry Pi to test on but Docker allows you to test your code in the same environment. 

## Terminology

A Docker **container** can be thought of as a lightweight VM that encapsulates your dependencies and your code. It is essentially shielded from the rest of your computer and even allows it to emulate other OSes. 

A Docker **image** is a compressed object holding all the dependencies that you specify. A Docker image runs inside a Docker container.

A **Dockerfile** is how you specify the dependencies in your image. It uses a simplified command structure (see https://docs.docker.com/engine/reference/builder/) which allows you to run any bash command. The nice thing about a Docker image is that it caches the image between every `RUN`, `ADD` or `COPY` command. This allows you to build images quickly after you've built it the first time. 

Our latest Docker images will be stored on Docker Hub here https://hub.docker.com/repository/docker/pierobotics/c-runtime.


## Installation

To install Docker, go here https://docs.docker.com/get-docker/. Note that to install Docker for Windows 10 Home, you first need to install WSL 2 here https://docs.microsoft.com/en-us/windows/wsl/install-win10.


# Basic Usage with Docker Compose


## Running

If you are just starting out and want to get into development quickly, go to the `c-runtime` folder and do

    docker-compose up

which will begin a Runtime instance in a Docker container. Then in another terminal you can do

    docker-compose exec runtime bash

to run/test other things within the container.

## Building

To build Runtime with the code on your machine, do

    docker-compose up --build

## Stopping

To stop the container, do 

    docker-compose down


# Advanced Usage with Docker CLI

## Pulling

To get the image with the code from the `master` branch, do 

    docker pull pierobotics/c-runtime:latest
    
## Building

To build your own image instead of using the one on Docker Hub, so that it uses your up to date code instead of code on `master`, do
    
    docker build -t pierobotics/c-runtime:latest -f docker/Dockerfile .

### Multi-Architecture Images

The Raspberry Pi 4 uses the arm32/v7 architecture while most consumer computers nowadays use x64/amd64 architecture. To have Docker images that work for both development on our computers and production on the Pis, we make it so that the same tag points to both architectures. When you do `docker pull` or `docker build`, Docker will automatically get the image that has your architecture. This is ideal in most cases.

If you're on a x64 machine and would like to run the arm32 images instead though (other direction isn't possible), you first need to have QEMU, a binary emulator, installed. If you are on Windows/Mac and so have Docker Desktop, QEMU is already installed for you. If you are on Linux, you can install QEMU on Linux with `docker run --rm --privileged multiarch/qemu-user-static --reset -p yes`. Now you can build the `arm32` image with

    DOCKER_CLI_EXPERIMENTAL=enabled docker build --platform linux/arm/v7 -t pierobotics/c-runtime:latest -f docker/Dockerfile .

### Base Image

We use a base image to make sure that the building of the actual image is fast. Building it for multi-architectures requires some experimental Docker features to enable `buildx`, which can be done by setting an environment variable. If for some reason the base image needs to be updated and pushed, do

    cd docker/base
    DOCKER_CLI_EXPERIMENTAL=enabled docker buildx create --use   # Only needed the very first time on a computer
    DOCKER_CLI_EXPERIMENTAL=enabled docker buildx build --platform linux/amd64,linux/arm/v7 --build-arg BUILDKIT_INLINE_CACHE=1 --cache-from pierobotics/c-runtime:base -t pierobotics/c-runtime:base --push .

If you haven't run this already on your machine and so don't have local caches, it will take several hours. Importantly, this can only be run on x64 machines that have QEMU, a binary emulator, installed. If you're on Windows/Mac, Docker Desktop already has QEMU preinstalled. If you are using Linux, you will need to install QEMU before building by doing `docker run --rm --privileged multiarch/qemu-user-static --reset -p yes`.

## Running

To run the Docker container, do

    docker run -it --rm pierobotics/c-runtime:latest <CMD>

This CMD can be `./run.sh` to start Runtime, or `bash` to open a shell, or any other command you want to run inside the container. You can also run other things in the container by in another terminal doing `docker exec -it $(docker ps -q) bash` and running whatever you want inside the shell. The `c-runtime` Git repo will be located at `/root/runtime`.

### Devices

If you want to have the container access any Arduino devices, add the flag `--device /dev/ttyACM0` or whichever path to you device you want to `docker run`. You can do this multiple times for multiple devices.

### Mounting

If you want to mount the `c-runtime` folder to the Docker container, add the flag `-v $PWD:/root/runtime` to `docker run`. This will allow changes you make inside the Docker container to change the files within your `c-runtime` directory on your computer, and vice versa as well. This is especially useful if you are using the Docker container to do development.

## Stopping

To stop the container, either exit from the `./run.sh` shell with Ctrl+C or do `docker stop -t 5 $(docker ps -q)`.

