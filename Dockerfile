FROM ubuntu:noble
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -qq
RUN apt-get install  -yq
RUN  apt-get install -yq \
     build-essential \
     cmake \
     gcovr \
     git \
     g++-13 \
     libgtest-dev \
     software-properties-common

RUN ln -sf /usr/bin/g++-13 /usr/bin/g++
RUN ln -sf /usr/bin/gcc-13 /usr/bin/gcc
RUN ln -sf /usr/bin/gcov-13 /usr/bin/gcov

# FloppyFloat; commented out for CI
# RUN git clone --recurse-submodules https://github.com/not-chciken/FloppyFloat.git
# WORKDIR FloppyFloat
# RUN mkdir build
# WORKDIR build
# RUN cmake ..
# RUN cmake --build . -j$(nproc)

ENTRYPOINT bash