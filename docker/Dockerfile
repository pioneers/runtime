FROM pierobotics/runtime:base

WORKDIR /root/runtime

# Adds all files in the repo that are not in .dockerignore
ADD ./ ./

# Add the runtime command to our PATH
RUN echo 'export PATH=$PATH:~/runtime' >> ~/.bashrc

# Build Runtime processes
RUN ./runtime build
RUN cd tests && make cli && cd .. # runtime build does not build the cli's; we want to build this in case something fails here

# By default, run Runtime when the Docker container starts
CMD ./runtime run
