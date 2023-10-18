#!/bin/sh

sudo apt-get update && sudo apt-get install -qy \
  build-essential \
  cmake \
  cppcheck \
  libarmadillo-dev \
  libboost-all-dev \
  libuhd-dev \
  libvulkan-dev \
  unzip \
  valgrind \
  wget \
  && \
  wget https://sourceforge.net/projects/sigpack/files/sigpack-1.2.7.zip -O sigpack.zip && unzip sigpack.zip && ln -s sigpack-*/sigpack . && \
  wget https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp && \
  git clone https://github.com/DTolm/VkFFT -b v1.3.1 && \
  sed -i -E 's/GIT_TAG\s+"origin.main"/GIT_TAG "13.0.0"/g' VkFFT/CMakeLists.txt
