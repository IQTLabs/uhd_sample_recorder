#!/bin/sh
cd VkFFT && mkdir build && cd build && cmake .. && make -j $(nproc) && cd ../.. && \
  mkdir build && cd build && cmake ../lib && make -j $(nproc) && cd ..
