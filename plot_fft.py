#!/usr/bin/python3

import numpy as np
import matplotlib
import matplotlib.pyplot as plt

DPI = 100
WIDTH = 12
HEIGHT = 8
IMSHOW_INTERPOLATION = "bilinear"
MPL_BACKEND = "cairo"

CENTER_FREQ = 108e6
SAMPLE_RATE = 10.24e6
NFFT = 256
SAMPLE_FILE = "fft_test.raw"
PNG_FILE = "fft_test.png"


matplotlib.use(MPL_BACKEND)
i = np.fromfile(SAMPLE_FILE, dtype=np.float32)
sample_count = i.shape[0]
i = np.roll(i.reshape(-1, NFFT).swapaxes(0, 1), int(NFFT / 2), 0)
fc = CENTER_FREQ / 1e6
fo = SAMPLE_RATE / 1e6 / 2
extent = (0, sample_count / SAMPLE_RATE, fc - fo, fc + fo)
fig = plt.figure()
fig.set_size_inches(WIDTH, HEIGHT)
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
plt.savefig(PNG_FILE, dpi=DPI)
