#!/bin/bash
set -e
cd VkFFT && mkdir build && cd build && CMAKE_BUILD_TYPE=Release cmake .. && make -j $(nproc) && cd ../.. && \
  mkdir build && cd build && cmake ../lib && make -j $(nproc) && cd ..
