cmake_minimum_required(VERSION 3.16)

project(uhd_sample_recorder)
set(CMAKE_BUILD_TYPE Release)
set(SRC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
include(FindPkgConfig)
include(CTest)

find_package(Vulkan REQUIRED)
find_package(Armadillo REQUIRED)
find_package(UHD 3.15.0 REQUIRED)
find_package(
  Boost ${Boost_Version}
  COMPONENTS filesystem iostreams thread unit_test_framework program_options
  REQUIRED)

add_library(vkfft vkfft.cpp)
target_link_libraries(
  vkfft
  ${SRC_ROOT}/VkFFT/build/glslang-main/glslang/libglslang.a
  ${SRC_ROOT}/VkFFT/build/glslang-main/glslang/libMachineIndependent.a
  ${SRC_ROOT}/VkFFT/build/glslang-main/glslang/OSDependent/Unix/libOSDependent.a
  ${SRC_ROOT}/VkFFT/build/glslang-main/OGLCompilersDLL/libOGLCompiler.a
  ${SRC_ROOT}/VkFFT/build/glslang-main/glslang/libGenericCodeGen.a
  ${SRC_ROOT}/VkFFT/build/glslang-main/SPIRV/libSPIRV.a
  ${SRC_ROOT}/VkFFT/build/CMakeFiles/VkFFT_TestSuite.dir/benchmark_scripts/vkFFT_scripts/src/utils_VkFFT.cpp.o
)
target_include_directories(
  vkfft
  PUBLIC ${SRC_ROOT}/VkFFT/glslang-main/glslang/Include
         ${SRC_ROOT}/VkFFT/glslang-main/glslang/Public
         ${SRC_ROOT}/VkFFT/vkFFT
         ${SRC_ROOT}/VkFFT/benchmark_scripts/vkFFT_scripts/include
         ${SRC_ROOT}/sigpack
         ${SRC_ROOT})

add_library(sample_writer sample_writer.cpp)

add_library(sample_pipeline sample_pipeline.cpp)
target_link_libraries(sample_pipeline vkfft ${ARMADILLO_LIBRARIES}
                      ${Boost_LIBRARIES} ${Vulkan_LIBRARIES})

add_executable(sample_pipeline_test sample_pipeline_test.cpp)
target_link_libraries(sample_pipeline_test sample_pipeline sample_writer
                      ${ARMADILLO_LIBRARIES} ${Boost_LIBRARIES})

add_test(NAME sample_pipeline_test COMMAND sample_pipeline_test)

add_executable(uhd_sample_recorder uhd_sample_recorder.cpp)
target_link_libraries(uhd_sample_recorder sample_pipeline sample_writer
                      ${Boost_LIBRARIES} ${UHD_LIBRARIES})
