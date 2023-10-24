#!/bin/bash
set -e
cd VkFFT && mkdir build && cd build && CMAKE_BUILD_TYPE=Release cmake -DGLSLANG_GIT_TAG=13.0.0 .. && make -j $(nproc) && cd ../.. && \
  mkdir build && cd build && cmake ../lib && make -j $(nproc) && cd ..
