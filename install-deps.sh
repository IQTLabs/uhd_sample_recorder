#!/bin/sh

sudo apt-get update && sudo apt-get install -qy build-essential cmake libuhd-dev libboost-all-dev cppcheck libarmadillo-dev unzip wget libvulkan-dev valgrind && \
  wget https://sourceforge.net/projects/sigpack/files/sigpack-1.2.7.zip -O sigpack.zip && unzip sigpack.zip && ln -s sigpack-*/sigpack . && \
  git clone https://github.com/DTolm/VkFFT && \
  cd VkFFT && git checkout 7113de9abc16f7411412e7e1137fc8d0beebd847 && patch < ../vkfft-main-patch.txt && cd .. && \
  wget https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
