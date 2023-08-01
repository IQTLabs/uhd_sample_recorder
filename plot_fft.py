#!/usr/bin/python3

import numpy as np
import matplotlib
import matplotlib.pyplot as plt
import argparse

IMSHOW_INTERPOLATION = "bilinear"
MPL_BACKEND = "cairo"


def main():
    parser = argparse.ArgumentParser(description="Generate FFT plot from raw file")
    parser.add_argument("filename", type=str, help="Path to the input raw file")
    parser.add_argument(
        "--center_freq",
        type=float,
        default=108e6,
        help="Center frequency in Hz (e.g., 108e6 for 108 MHz), default is 108e6",
    )
    parser.add_argument(
        "--sample_rate",
        type=float,
        default=10.24e6,
        help="Sample rate in Hz (e.g., 10.24e6 for 10.24 MHz), default is 10.24e6",
    )
    parser.add_argument(
        "--nfft",
        type=int,
        default=256,
        help="Number of data points used in each block for the FFT, default is 256",
    )
    parser.add_argument(
        "--dpi",
        type=int,
        default=100,
        help="DPI (dots per inch) for the output PNG, default is 100",
    )
    parser.add_argument(
        "--width",
        type=float,
        default=12,
        help="Width of the output PNG in inches, default is 12",
    )
    parser.add_argument(
        "--height",
        type=float,
        default=8,
        help="Height of the output PNG in inches, default is 8",
    )
    args = parser.parse_args()

    matplotlib.use(MPL_BACKEND)
    i = np.fromfile(args.filename, dtype=np.float32)
    sample_count = i.shape[0]
    i = np.roll(i.reshape(-1, args.nfft).swapaxes(0, 1), int(args.nfft / 2), 0)
    fc = args.center_freq / 1e6
    fo = args.sample_rate / 1e6 / 2
    extent = (0, sample_count / args.sample_rate, fc - fo, fc + fo)
    
    png_file = f"{args.filename}_{int(args.center_freq)}_{int(args.sample_rate)}.png"
    
    fig = plt.figure()
    fig.set_size_inches(args.width, args.height)
    axes = fig.add_subplot(111)
    axes.set_xlabel("time (s)")
    axes.set_ylabel("freq (MHz)")
    axes.axis("auto")
    axes.minorticks_on()
    im = axes.imshow(
        i,
        cmap="turbo",
        origin="lower",
        extent=extent,
        aspect="auto",
        interpolation=IMSHOW_INTERPOLATION,
    )
    plt.sci(im)
    plt.savefig(png_file, dpi=args.dpi)
    plt.show()


if __name__ == "__main__":
    main()
