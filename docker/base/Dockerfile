FROM arm32v7/python:3.7-buster

RUN apt-get -q update && \
    apt-get -y install python3-dev wget tar pkg-config && \
    apt-get clean

WORKDIR /tmp
SHELL ["/bin/bash", "-c"]

# Install protobuf
ENV protobuf_folder protobuf
RUN wget https://github.com/protocolbuffers/protobuf/releases/download/v3.12.3/protobuf-cpp-3.12.3.tar.gz -O ${protobuf_folder}.tar.gz && \
    mkdir ${protobuf_folder} && tar xzf ${protobuf_folder}.tar.gz -C ${protobuf_folder} --strip-components 1 && \
    rm ${protobuf_folder}.tar.gz && \
    pushd ${protobuf_folder} && \
    ./configure && make && make install && ldconfig && \
    popd && rm -r ${protobuf_folder}

# Install protobuf-c
ENV protobuf_c_folder protobuf-c
RUN wget https://github.com/protobuf-c/protobuf-c/releases/download/v1.3.3/protobuf-c-1.3.3.tar.gz -O ${protobuf_c_folder}.tar.gz && \
    mkdir ${protobuf_c_folder} && tar xzf ${protobuf_c_folder}.tar.gz -C ${protobuf_c_folder} --strip-components 1 && \
    rm ${protobuf_c_folder}.tar.gz && \
    pushd ${protobuf_c_folder} && \
    ./configure && make && make install && ldconfig && \
    popd && rm -r ${protobuf_c_folder}

WORKDIR /root

CMD ["bash"]
