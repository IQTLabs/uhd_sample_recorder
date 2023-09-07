#!/bin/bash
set -e
cd SPIRV-Tools && mkdir build && cd build && cmake .. && make -j $(nproc) && sudo make install && cd ../..
cd VkFFT && mkdir build && cd build && cmake -DALLOW_EXTERNAL_SPIRV_TOOLS=ON .. && make -j $(nproc) && cd ../.. && \
  mkdir build && cd build && cmake ../lib && make -j $(nproc) && cd ..
