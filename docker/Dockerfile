FROM pierobotics/runtime:base

WORKDIR /root/runtime

# Adds all files in the repo that are not in .dockerignore
ADD ./ ./

# Add the runtime command to our PATH
RUN echo 'export PATH=$PATH:~/runtime' >> ~/.bashrc

# Build Runtime processes
RUN ./runtime build

# By default, run Runtime when the Docker container starts
CMD ./runtime run
