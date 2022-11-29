#!/bin/sh
cd VkFFT && mkdir build && cd build && cmake .. && make -j $(nproc) && cd ../.. && \
  mkdir build && cd build && cmake .. && make && make test && valgrind --leak-check=yes --error-exitcode=1 ./sample_pipeline_test && cd .. && \
  cppcheck *cpp
