#!/usr/bin/env python3
"""Render the original Three-Wisdom gong and feature fanfare."""

import argparse
from array import array
import math
from pathlib import Path
import sys
import wave

RATE = 22_050
TAU = 2 * math.pi


def hz(note):
    return 440 * 2 ** ((note - 69) / 12)


def smooth(value):
    return (1 - math.cos(math.pi * min(1, max(0, value)))) * 0.5


def normalize(mix, peak, release=0.080):
    tail = round(release * RATE)
    for index in range(1, tail + 1):
        mix[-index] *= smooth((index - 1) / tail)
    gain = peak / max(abs(value) for value in mix)
    samples = [round(value * gain) for value in mix]
    assert samples[0] == samples[-1] == 0
    return samples


def render_gong():
    mix = [0.0] * RATE
    # The low modes bloom after the mallet strike. The brighter modes decay
    # quickly and give the sound enough presence on a small speaker.
    modes = (
        # Hz, gain, attack, decay, initial pitch bend, phase
        (103.2, 0.52, 0.085, 0.48, 0.008, 0.32),
        (157.7, 0.38, 0.044, 0.40, 0.013, 1.70),
        (225.3, 0.54, 0.024, 0.38, 0.014, 0.48),
        (239.9, 0.22, 0.058, 0.45, 0.009, 2.13),
        (353.6, 0.45, 0.013, 0.31, 0.021, 1.40),
        (419.5, 0.23, 0.019, 0.30, 0.014, 2.81),
        (556.2, 0.22, 0.010, 0.19, 0.019, 0.71),
        (707.9, 0.18, 0.008, 0.15, 0.018, 1.91),
        (907.0, 0.13, 0.006, 0.12, 0.016, 0.24),
        (1171.2, 0.11, 0.003, 0.086, 0.013, 2.58),
        (1534.4, 0.06, 0.002, 0.049, 0.015, 0.95),
        (2077.8, 0.045, 0.002, 0.023, 0.012, 1.83),
    )
    settle = 0.16
    for index in range(len(mix)):
        time = index / RATE
        shift = settle * (1 - math.exp(-time / settle))
        for frequency, amount, attack, decay, bend, initial_phase in modes:
            envelope = smooth(time / attack) * math.exp(-time / decay)
            phase = TAU * frequency * (time + bend * shift) + initial_phase
            mix[index] += amount * envelope * math.sin(phase)
    reflections(mix, 0.88)
    return normalize(mix, 7_336, release=0.225)


def pluck(mix, onset, note, level, decay):
    """A short struck string, with a rounded body under its pick."""
    first = round(onset * RATE)
    length = min(len(mix) - first, round(5.4 * decay * RATE))
    step = TAU * hz(note) / RATE
    partials = ((1.0, 0.67, 1.0), (2.005, 0.23, 0.58), (3.008, 0.10, 0.37),
                (4.020, 0.047, 0.22), (5.030, 0.020, 0.15))
    for index in range(length):
        time = index / RATE
        phase = step * index
        envelope = smooth(time / 0.006) * smooth((length - index - 1) / (0.035 * RATE))
        tone = sum(
            gain * math.exp(-time / (decay * sustain)) * math.sin(phase * ratio + ratio * 0.26)
            for ratio, gain, sustain in partials
        )
        mix[first + index] += level * envelope * tone


def reed(mix, onset, duration, note, level, balance=0.28):
    """A soft reed and string voice with a slight late vibrato."""
    first = round(onset * RATE)
    length = min(len(mix) - first, round(duration * RATE))
    step = TAU * hz(note) / RATE
    for index in range(length):
        time = index / RATE
        depth = 0.15 * smooth((time - 0.10) / 0.25)
        vibrato = depth * math.sin(TAU * 4.4 * time)
        phase = step * index + vibrato
        detuned = step * index * 1.0031 + 0.52 - vibrato * 0.65
        tone = (
            (0.69 - 0.12 * balance) * math.sin(phase)
            + (0.15 + 0.10 * balance) * math.sin(phase * 2 + 0.47)
            + (0.09 + 0.06 * balance) * math.sin(phase * 3 + 0.28)
            + 0.035 * math.sin(phase * 4)
            + 0.14 * math.sin(detuned)
        )
        envelope = smooth(time / 0.070) * smooth((duration - time) / 0.22)
        mix[first + index] += level * envelope * tone


def reflections(mix, amount):
    dry = mix[:]
    for seconds, gain in ((0.037, 0.082), (0.064, 0.058), (0.113, 0.037),
                          (0.158, 0.026), (0.223, 0.017)):
        delay = round(seconds * RATE)
        for index in range(delay, len(mix)):
            mixed = 0.5 * (dry[index - delay] + dry[max(0, index - delay - 5)])
            mix[index] += amount * gain * mixed


def render_fanfare():
    mix = [0.0] * 156_996
    lead_adjustment = 0.135  # Leaves a 90 ms lead on the opening pluck.
    # D, G, B minor, A suspended and D form a short open-to-closed cadence.
    accompaniment = (
        (0.334, 1.30, (50, 57, 66), 0.144),
        (1.615, 1.38, (43, 55, 62), 0.123),
        (3.025, 1.49, (47, 54, 62), 0.133),
        (4.486, 1.33, (45, 57, 62), 0.142),
        (5.755, 1.29, (50, 57, 66), 0.167),
    )
    for onset, duration, notes, level in accompaniment:
        for index, note in enumerate(notes):
            reed(mix, onset - lead_adjustment + 0.024 * index, duration, note, level / len(notes))

    melody = (
        # onset, note, pluck gain, pluck decay, reed length
        (0.225, 66, 0.64, 0.19, 0.00),
        (0.491, 69, 0.55, 0.17, 0.31),
        (0.788, 74, 0.63, 0.24, 0.44),
        (1.204, 71, 0.52, 0.24, 0.41),
        (1.618, 67, 0.50, 0.19, 0.33),
        (1.913, 69, 0.55, 0.20, 0.31),
        (2.167, 71, 0.60, 0.24, 0.36),
        (2.465, 66, 0.48, 0.23, 0.40),
        (3.018, 74, 0.68, 0.24, 0.36),
        (3.269, 73, 0.55, 0.19, 0.32),
        (3.577, 69, 0.54, 0.22, 0.38),
        (3.884, 66, 0.54, 0.22, 0.45),
        (4.472, 64, 0.55, 0.22, 0.43),
        (4.778, 69, 0.58, 0.20, 0.38),
        (5.054, 74, 0.66, 0.23, 0.39),
        (5.371, 76, 0.58, 0.27, 0.43),
        (5.754, 74, 0.75, 0.39, 0.91),
        (6.210, 69, 0.26, 0.26, 0.36),
    )
    for onset, note, level, decay, support in melody:
        start = onset - lead_adjustment
        pluck(mix, start, note, level, decay)
        if support:
            reed(mix, start + 0.024, support, note, level * 0.22, balance=0.18)
    for event in (
        (0.231, 50, 0.37, 0.26), (0.760, 57, 0.21, 0.20),
        (1.615, 43, 0.32, 0.29), (2.146, 55, 0.23, 0.23),
        (3.018, 47, 0.35, 0.29), (3.570, 59, 0.21, 0.20),
        (4.475, 45, 0.33, 0.28), (5.048, 57, 0.21, 0.24),
        (5.757, 50, 0.41, 0.46), (5.802, 57, 0.27, 0.39),
        (5.855, 66, 0.30, 0.35),
    ):
        onset, note, level, decay = event
        pluck(mix, onset - lead_adjustment, note, level, decay)

    reflections(mix, 1.11)
    return normalize(mix, 13_045)


def write_clip(directory, wav_directory, name, symbol, samples):
    with (directory / (name + '.inc')).open('w', encoding='ascii') as stream:
        stream.write('// Original procedural synthesis. Rendered at 22050 Hz, signed mono PCM.\n')
        stream.write(f'alignas(4) constexpr int16_t {symbol}[] = {{\n')
        for index in range(0, len(samples), 16):
            values = ', '.join(str(value) for value in samples[index:index + 16])
            stream.write('  ' + values + ',\n')
        stream.write('};\n')
    if wav_directory is not None:
        pcm = array('h', samples)
        if sys.byteorder != 'little':
            pcm.byteswap()
        with wave.open(str(wav_directory / (name + '.wav')), 'wb') as stream:
            stream.setparams((1, 2, RATE, len(samples), 'NONE', 'not compressed'))
            stream.writeframes(pcm.tobytes())
    peak = 20 * math.log10(max(abs(x) for x in samples) / 32_768)
    rms = 10 * math.log10(sum(x * x for x in samples) / len(samples) / 32_768 ** 2)
    print(f'{name}: {len(samples)} frames, peak {peak:.2f} dBFS, RMS {rms:.2f} dBFS')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = Path(__file__).resolve().parents[1] / 'firmware/src'
    parser.add_argument('--out', default=source, type=Path)
    parser.add_argument('--wav-dir', type=Path)
    args = parser.parse_args()
    output = args.out
    output.mkdir(parents=True, exist_ok=True)
    if args.wav_dir is not None:
        args.wav_dir.mkdir(parents=True, exist_ok=True)
    write_clip(output, args.wav_dir, 'audio_wisdom_gong_generated', 'k_wisdom_gong', render_gong())
    write_clip(output, args.wav_dir, 'audio_wisdom_three_start_generated', 'k_wisdom_three_start', render_fanfare())


if __name__ == '__main__':
    main()
