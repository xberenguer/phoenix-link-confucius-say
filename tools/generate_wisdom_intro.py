#!/usr/bin/env python3
"""Render the original two-part Three-Wisdom feature cue."""

import argparse
from array import array
import math
from pathlib import Path
import sys
import wave

RATE = 22_050
TAU = 2 * math.pi
FULL_SCALE = 32_768


def hz(note):
    """Convert a MIDI note number to frequency."""
    return 440 * 2 ** ((note - 69) / 12)


def ease(value):
    """A raised cosine with zero slope at either end."""
    return (1 - math.cos(math.pi * min(1, max(0, value)))) * 0.5


def bell(mix, onset, note, amplitude, decay):
    """A struck bell with a bright attack and a warmer decaying body."""
    start = round(onset * RATE)
    length = min(len(mix) - start, round(5.5 * decay * RATE))
    phase_step = TAU * hz(note) / RATE
    partials = (
        (1.000, 0.74, 1.00, 0.00),
        (1.997, 0.23, 0.60, 0.18),
        (2.731, 0.13, 0.32, 0.44),
        (4.086, 0.048, 0.17, 0.78),
    )
    for index in range(length):
        time = index / RATE
        phase = index * phase_step
        strike = ease(time / 0.005)
        end = ease((length - index - 1) / (0.027 * RATE))
        tone = sum(
            gain * math.exp(-time / (decay * sustain)) * math.sin(phase * ratio + offset)
            for ratio, gain, sustain, offset in partials
        )
        mix[start + index] += amplitude * strike * end * tone


def glow(mix, onset, duration, notes, amplitude):
    """A quiet, slowly animated harmonic layer under the bells."""
    start = round(onset * RATE)
    length = min(len(mix) - start, round(duration * RATE))
    phases = [TAU * hz(note) / RATE for note in notes]
    detunes = [1.0021, 0.9981, 1.0014, 0.9976]
    for index in range(length):
        time = index / RATE
        attack = ease(time / 0.22)
        release = ease((duration - time) / 0.45)
        drift = 0.10 * math.sin(TAU * 3.2 * time)
        tone = 0.0
        for voice, (step, detune) in enumerate(zip(phases, detunes)):
            phase = step * index + 0.42 * voice
            tone += (
                0.60 * math.sin(phase)
                + 0.28 * math.sin(phase * detune + drift)
                + 0.07 * math.sin(2.0 * phase + 0.4)
                + 0.04 * math.sin(3.0 * phase - drift)
            )
        mix[start + index] += amplitude * attack * release * tone / len(phases)


def add_reflections(mix, amount):
    """Small synthetic reflections around the tones, without a room recording."""
    dry = mix[:]
    for seconds, gain in ((0.033, 0.071), (0.061, 0.054), (0.097, 0.041),
                          (0.141, 0.031), (0.187, 0.021), (0.233, 0.014)):
        delay = round(seconds * RATE)
        for index in range(delay, len(mix)):
            mix[index] += amount * gain * 0.5 * (dry[index - delay] + dry[max(0, index - delay - 4)])


def wisdom_entry():
    mix = [0.0] * 156_996
    # The first call hangs briefly on an open voicing. The response begins
    # after that tail subsides and closes with a fuller, lower chord.
    glow(mix, 0.035, 2.31, (52, 59, 66, 68), 0.24)
    glow(mix, 2.860, 1.82, (49, 56, 59, 64), 0.29)
    glow(mix, 4.300, 1.78, (47, 54, 61, 64), 0.29)
    glow(mix, 5.755, 1.33, (52, 59, 68, 73), 0.32)
    lead = (
        (0.075, 76, 0.66, 0.22),
        (0.235, 83, 0.64, 0.20),
        (0.435, 78, 0.56, 0.18),
        (0.690, 80, 0.70, 0.25),
        (0.995, 85, 0.63, 0.28),
        (1.365, 83, 0.49, 0.24),
        (1.615, 80, 0.48, 0.21),
        (1.910, 88, 0.63, 0.26),
        (2.210, 83, 0.23, 0.115),
        (2.870, 78, 0.77, 0.23),
        (3.015, 83, 0.72, 0.23),
        (3.220, 85, 0.74, 0.27),
        (3.535, 88, 0.71, 0.33),
        (3.995, 90, 0.67, 0.29),
        (4.310, 85, 0.55, 0.24),
        (4.640, 83, 0.61, 0.23),
        (4.930, 85, 0.72, 0.26),
        (5.320, 90, 0.66, 0.27),
        (5.755, 88, 0.78, 0.40),
        (5.795, 85, 0.58, 0.39),
        (5.860, 83, 0.54, 0.39),
        (6.260, 76, 0.25, 0.31),
    )
    support = (
        (0.080, 64, 0.33, 0.25),
        (0.690, 71, 0.26, 0.29),
        (1.900, 64, 0.25, 0.27),
        (2.870, 61, 0.38, 0.34),
        (3.535, 68, 0.28, 0.35),
        (4.290, 59, 0.39, 0.36),
        (4.920, 66, 0.26, 0.34),
        (5.755, 64, 0.40, 0.42),
    )
    for event in (*lead, *support):
        bell(mix, *event)
    add_reflections(mix, 1.16)
    return normalize(mix, 13_045)


def normalize(mix, target_peak):
    tail = round(0.085 * RATE)
    for index in range(1, tail + 1):
        mix[-index] *= ease((index - 1) / tail)
    peak = max(abs(value) for value in mix)
    return [round(value * target_peak / peak) for value in mix]


def write_clip(include, wav_path, samples):
    assert samples[0] == 0 and samples[-1] == 0
    with include.open('w', encoding='ascii') as stream:
        stream.write('// Original procedural synthesis. Rendered at 22050 Hz, signed mono PCM.\n')
        stream.write('alignas(4) constexpr int16_t k_wisdom_three_start[] = {\n')
        for index in range(0, len(samples), 16):
            stream.write('  ' + ', '.join(str(n) for n in samples[index:index + 16]) + ',\n')
        stream.write('};\n')
    if wav_path is not None:
        pcm = array('h', samples)
        if sys.byteorder != 'little':
            pcm.byteswap()
        with wave.open(str(wav_path), 'wb') as stream:
            stream.setparams((1, 2, RATE, len(samples), 'NONE', 'not compressed'))
            stream.writeframes(pcm.tobytes())
    peak_db = 20 * math.log10(max(abs(x) for x in samples) / FULL_SCALE)
    rms_db = 10 * math.log10(sum(x * x for x in samples) / len(samples) / FULL_SCALE ** 2)
    print(f'{len(samples)} samples, {len(samples) / RATE:.3f} s, peak {peak_db:.2f} dBFS, RMS {rms_db:.2f} dBFS')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    default_include = (Path(__file__).resolve().parents[1] / 'firmware/src'
                       / 'audio_wisdom_three_start_generated.inc')
    parser.add_argument('--include', default=default_include, type=Path)
    parser.add_argument('--wav', type=Path)
    args = parser.parse_args()
    write_clip(args.include, args.wav, wisdom_entry())


if __name__ == '__main__':
    main()
