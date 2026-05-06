#!/usr/bin/env python3
"""Plot before/after FFT magnitude spectra for two mono WAV files.

Usage:
    python scripts/plot_fft.py audio/input.wav audio/output.wav results/fft_before_after.png
"""

from __future__ import annotations

import argparse
from pathlib import Path
import sys

try:
    import matplotlib

    matplotlib.use("Agg")

    import matplotlib.pyplot as plt
    import numpy as np
    from scipy.io import wavfile
except ImportError as exc:
    print(
        f"Error: missing Python visualization dependency: {exc.name}. "
        "Install dependencies with: python -m pip install -r requirements.txt",
        file=sys.stderr,
    )
    raise SystemExit(1)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Plot before/after FFT magnitude spectra for two mono WAV files."
    )
    parser.add_argument("input_wav", help="Original input WAV path")
    parser.add_argument("output_wav", help="Filtered output WAV path")
    parser.add_argument("image_path", help="Destination PNG path")
    return parser.parse_args()


def load_mono_wav(path: Path) -> tuple[int, np.ndarray]:
    if not path.exists():
        raise ValueError(f"File does not exist: {path}")

    sample_rate, samples = wavfile.read(path)
    if sample_rate <= 0:
        raise ValueError(f"Invalid sample rate in {path}: {sample_rate}")

    if samples.ndim != 1:
        raise ValueError(f"Only mono WAV files are supported: {path}")

    if np.issubdtype(samples.dtype, np.integer):
        info = np.iinfo(samples.dtype)
        if info.min == 0:
            midpoint = (float(info.max) + 1.0) / 2.0
            normalized = (samples.astype(np.float64) - midpoint) / midpoint
        else:
            scale = max(abs(info.min), abs(info.max))
            normalized = samples.astype(np.float64) / float(scale)
    elif np.issubdtype(samples.dtype, np.floating):
        normalized = samples.astype(np.float64)
    else:
        raise ValueError(f"Unsupported WAV sample type in {path}: {samples.dtype}")

    if normalized.size == 0:
        raise ValueError(f"WAV file has no samples: {path}")

    return int(sample_rate), normalized


def magnitude_spectrum_db(samples: np.ndarray, sample_rate: int) -> tuple[np.ndarray, np.ndarray]:
    window = np.hanning(samples.size)
    windowed = samples * window
    spectrum = np.fft.rfft(windowed)
    frequencies = np.fft.rfftfreq(samples.size, d=1.0 / sample_rate)
    magnitude_db = 20.0 * np.log10(np.abs(spectrum) + 1.0e-12)
    return frequencies, magnitude_db


def main() -> int:
    args = parse_args()
    input_path = Path(args.input_wav)
    output_path = Path(args.output_wav)
    image_path = Path(args.image_path)

    try:
        input_rate, input_samples = load_mono_wav(input_path)
        output_rate, output_samples = load_mono_wav(output_path)

        if input_rate != output_rate:
            raise ValueError(
                f"Sample rates do not match: {input_path} is {input_rate} Hz, "
                f"{output_path} is {output_rate} Hz"
            )

        if input_samples.size != output_samples.size:
            raise ValueError(
                f"Sample counts do not match: {input_path} has {input_samples.size}, "
                f"{output_path} has {output_samples.size}"
            )

        input_freq, input_mag = magnitude_spectrum_db(input_samples, input_rate)
        output_freq, output_mag = magnitude_spectrum_db(output_samples, output_rate)

        image_path.parent.mkdir(parents=True, exist_ok=True)

        plt.figure(figsize=(11, 6))
        plt.plot(input_freq, input_mag, label="Before", linewidth=1.0, alpha=0.85)
        plt.plot(output_freq, output_mag, label="After", linewidth=1.0, alpha=0.85)
        plt.title(f"Before/After FFT Magnitude Spectrum ({input_rate} Hz sample rate)")
        plt.xlabel("Frequency (Hz)")
        plt.ylabel("Relative Magnitude (dB)")
        plt.grid(True, alpha=0.3)
        plt.legend()
        plt.tight_layout()
        plt.savefig(image_path, dpi=150)
        plt.close()

        print(f"Saved FFT plot: {image_path}")
        return 0
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
