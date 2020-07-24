FROM avsurfer123/c-runtime:base

WORKDIR /root/runtime

# Adds all files in the repo that are not in .dockerignore
ADD ./ ./

# Build Runtime processes
RUN ./build.sh

# Set run.sh as command that runs with docker run
CMD ./run.sh
