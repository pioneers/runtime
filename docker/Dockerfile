FROM avsurfer123/protobuf-c:arm32

# Install Cython
RUN pip3 install Cython

# Install arduino-cli
ENV arduino_folder arduino_cli
RUN wget https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Linux_ARMv7.tar.gz -O ${arduino_folder}.tar.gz && \
    mkdir ${arduino_folder} && tar xzf ${arduino_folder}.tar.gz -C ${arduino_folder} && \
    rm ${arduino_folder}.tar.gz && \
    mv ${arduino_folder}/arduino-cli /usr/bin/ && \
    rm -r ${arduino_folder} && \
    arduino-cli core update-index

# Install debug packages
RUN apt-get -y install procps htop tmux nano

WORKDIR /root/runtime

# Adds all files in the repo that are not in .dockerignore
ADD ./ ./

# Build Runtime processes
RUN ./build.sh

# Set run.sh as command that runs with docker run
CMD ./run.sh
