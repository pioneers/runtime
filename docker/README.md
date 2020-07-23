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

To get the image to emulate a Raspberry Pi, do `docker pull avsurfer123/c-runtime:arm32`

## Building

To build your own image instead of using the one on Docker Hub, do 
    
    DOCKER_BUILDKIT=1 docker build --pull --build-arg BUILDKIT_INLINE_CACHE=1 --cache-from avsurfer123/c-runtime:arm32 -t avsurfer123/c-runtime:arm32 -f docker/Dockerfile .

## Pushing

To push a built image to Docker Hub (only if you have push access), do `docker push avsurfer123/c-runtime:{TAG}`. You can change the tag before pushing by `docker tag avsurfer123/c-runtime:{OLD_TAG} avsurfer123/c-runtime:{NEW_TAG}`.

## Running

To run the latest Runtime inside the Docker image, do `docker run -it avsurfer123/c-runtime:arm32`. Then you can test by in another terminal doing `docker exec -it $(docker ps -q) bash` and running whatever you want inside the container.

This above method will use the code that was added when the image was built. If you want to run your own Runtime code, follow these commands:

    # Terminal 1
    docker run -it ${DOCKER_USERNAME}/c-runtime:arm32 bash
    # Terminal 2
    docker cp . $(docker ps -q):/root/runtime/
    docker exec -it $(docker ps -q) bash -c "./build.sh && ./run.sh"

If you want to have the container access any Arduino devices, add the flag `--device /dev/ttyACM0` or what ever devices you want to `docker run`.

## Stopping

To stop the container that was ran with method 1, do `docker kill -s INT $(docker ps -q)`. To stop the container that was ran with method 2, do `docker stop $(docker ps -q)`.

