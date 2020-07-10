FROM ubuntu:18.04

RUN apt-get update && \
  apt-get install -y git
WORKDIR /app
RUN git clone https://github.com/fcitx/fcitx
WORKDIR /app/fcitx
RUN apt-get update && \
  apt-get install -y \
  extra-cmake-modules libxkbcommon-dev libenchant-dev libxml2-dev \
  iso-codes libxkbfile-dev libjson-c-dev qt4-default libcairo2-dev \
  cmake curl

WORKDIR /app/fcitx/build
RUN cmake ..
RUN make
RUN make install
