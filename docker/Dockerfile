FROM pierobotics/c-runtime:base

WORKDIR /root/runtime

# Adds all files in the repo that are not in .dockerignore
ADD ./ ./

# Build Runtime processes
RUN ./runtime build

# By default, run Runtime when the Docker container starts
CMD ./runtime run
