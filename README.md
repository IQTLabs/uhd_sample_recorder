# uhd_sample_recorder

Record and (optionally) run FFT on I/Q samples from an Ettus SDR. Samples can be compressed as written; FFT may be run on GPU (using Vulkan, including RPi4).

## example usage

See https://files.ettus.com/manual/page_transport.html for notes on tuning USRP transport ```--args```.

```
$ ./uhd_sample_recorder --args num_recv_frames=960,recv_frame_size=16360,type=b200 --file test.zst --duration 2 --rate 2.048e6 --freq 101e6 --nfft 2048
```

FFT points can be output along with samples. This example records 1s of samples at 10.24e6sps at 108MHz, writing compressed samples to test.ci16.zst and uncompressed FFT points to fft_test.raw.  Then, the points are plotted to an image output by plot_fft.py.

```
$ uhd_sample_recorder --args type=b200,num_recv_frames=960,recv_frame_size=16360 --duration 1 --rate 10240000 --freq 108e6 --gain 10 --type short --file test.ci16.zst --fft_file fft_test.raw --nfft 256
$ pip install numpy matplotlib
$ ./plot_fft.py
```

## Vulkan FFT support

Requires a Vulkan compatible GPU.

### Running on a Raspberry Pi4

This works on both Raspberry Pi OS (Debian) and Ubuntu.

1. Add `dtoverlay=vc4-kms-v3d-pi4` to `/boot/firmware/config.txt` (`/boot/config.txt` on Raspberry Pi OS).
2. Make sure your user has access to the ```/dev/dri/renderD128``` device (e.g. ```sudo usermod -a -G render ubuntu```).
3. If running Ubuntu 22.04 or later, proceed to the [Build section](#build). If running Raspberry Pi OS, you will first need to update your Vulkan/MESA drivers - use https://github.com/jmcerrejon/PiKISS to Configure and install Vulkan using the main branch first then proceed to the [Build section](#build).

## build

```
./bin/install-deps.sh
./bin/build.sh
./bin/test.sh
```
