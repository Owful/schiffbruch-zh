"""Render the island sound set from documented recordings and original synthesis.

Requires NumPy and ffmpeg. Raw sources are fetched by fetch_audio_sources.py.
The axe preserves the original CC BY recording; other audio uses documented CC0 sources.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import numpy as np

RATE = 44100
RNG = np.random.default_rng(721047)
MANIFEST = []


def samples(seconds):
    return np.arange(round(seconds * RATE), dtype=np.float64) / RATE


def stereo(data):
    return np.repeat(data[:, None], 2, axis=1) if data.ndim == 1 else data


def filter_audio(data, low=45, high=6500):
    data = stereo(data)
    frequencies = np.fft.rfftfreq(len(data), 1 / RATE)
    curve = 1 / np.sqrt(1 + (low / np.maximum(frequencies, .01)) ** 4)
    curve /= np.sqrt(1 + (frequencies / high) ** 8)
    return np.fft.irfft(np.fft.rfft(data, axis=0) * curve[:, None], n=len(data), axis=0)


def noise(seconds, low=70, high=5000):
    data = filter_audio(RNG.normal(size=(round(seconds * RATE), 2)), low, high)
    return data / max(np.sqrt(np.mean(data ** 2)), 1e-8)


def fade(data, attack=.012, release=.05):
    data = stereo(data).copy()
    a, b = min(len(data), round(attack * RATE)), min(len(data), round(release * RATE))
    if a:
        data[:a] *= np.sin(np.linspace(0, np.pi / 2, a))[:, None] ** 2
    if b:
        data[-b:] *= np.cos(np.linspace(0, np.pi / 2, b))[:, None] ** 2
    return data


def loop(data, overlap=.8):
    data = stereo(data)
    count = min(round(overlap * RATE), len(data) // 4)
    out = data[:-count].copy()
    weight = np.linspace(0, 1, count)[:, None]
    out[:count] = data[-count:] * (1 - weight) + data[:count] * weight
    return out


def fit(data, seconds, speed=1):
    data = stereo(data)
    indices = np.arange(round(seconds * RATE)) * speed
    return np.column_stack([np.interp(indices, np.arange(len(data)), data[:, channel], left=0, right=0) for channel in range(2)])


def place(target, data, at, gain=1, pan=0):
    data = stereo(data)
    start = round(at * RATE)
    count = min(len(data), len(target) - start)
    if count <= 0 or start < 0:
        return
    gains = np.array([np.sqrt(1 - max(0, pan)), np.sqrt(1 + min(0, pan))])
    target[start:start + count] += data[:count] * gains * gain


def bed(data, seconds):
    out = np.zeros((round(seconds * RATE), 2))
    length = len(data) / RATE
    if length >= seconds:
        return data[:len(out)].copy()
    for at in np.arange(0, seconds, max(.25, length - .6)):
        place(out, fade(data, .3, .35), at)
    return out


def resonance(seconds, frequency, decay=.15):
    time = samples(seconds)
    tone = sum(np.sin(2 * np.pi * frequency * multiple * time + phase) * np.exp(-time / (decay / multiple ** .35)) / multiple ** 1.6 for multiple, phase in ((1, 0), (2.37, .6), (4.11, 1.1)))
    return fade(tone, .001, .02)


def refined_actions(sources, read):
    """Keep the old axe cadence; use real mouth recordings for eating/drinking."""
    original = filter_audio(read(sources / "original-axe.wav"), 65, 4400)
    axe = fade(original + filter_audio(original, 65, 350) * .25, .003, .012)
    eating_path = next((sources / "eating").rglob("crunch.5.ogg"))
    bite = filter_audio(read(eating_path, .5, .25), 140, 2400)
    food = np.zeros((round(.65 * RATE), 2))
    place(food, fade(bite, .015, .08), .035)
    drink = np.zeros((round(1.12 * RATE), 2))
    for filename, at, gain in (("swallow-03.flac", .12, 1), ("swallow-01.flac", .65, .65)):
        path = next((sources / "drinks").rglob(filename))
        place(drink, fade(filter_audio(read(path), 100, 3100), .008, .04), at, gain)
    return (
        ("faellen.wav", axe, "original axe rhythm and accented wood strike, lightly cleaned", ["Schiffbruch original faellen.wav (CC BY 4.0)"], True, .15, .8),
        ("eat.wav", food, "quiet real bite and chewing", ["Tito / StarNinjas crunch.5 (CC0)"], False, .055, .5),
        ("trinken.wav", drink, "two quiet real swallows", ["qubodup swallow-03 and swallow-01 (CC0)"], False, .06, .5),
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sources", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--only-actions", action="store_true", help="Render only the three corrected action sounds")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        parser.error("ffmpeg must be on PATH")

    def read(path, seconds=None, start=0):
        command = [ffmpeg, "-v", "error", "-ss", str(start), "-i", str(path)]
        if seconds:
            command += ["-t", str(seconds)]
        raw = subprocess.check_output(command + ["-ar", str(RATE), "-ac", "2", "-f", "f32le", "-"])
        return np.frombuffer(raw, dtype="<f4").reshape(-1, 2).astype(np.float64)

    def kenney(name):
        return read(args.sources / "kenney" / "Audio" / (name + ".ogg"))

    def export(name, data, purpose, sources, looping=False, rms=.10, peak=.65):
        data = stereo(data)
        data -= np.mean(data, axis=0)
        scale = min(rms / max(np.sqrt(np.mean(data ** 2)), 1e-8), peak / max(np.max(np.abs(data)), 1e-8))
        data *= scale
        if not looping:
            data = fade(data, .003, .025)
        destination = args.output / name
        command = [ffmpeg, "-v", "error", "-y", "-f", "f32le", "-ar", str(RATE), "-ac", "2", "-i", "-"]
        command += ["-c:a", "libvorbis", "-q:a", "5"] if name.endswith(".ogg") else ["-c:a", "pcm_s16le"]
        subprocess.run(command + [str(destination)], input=data.astype("<f4").tobytes(), check=True)
        MANIFEST.append({"file": name, "purpose": purpose, "sources": sources, "loop": looping,
                         "license": "CC-BY-4.0" if name == "faellen.wav" else "CC0-1.0",
                         "seconds": round(len(data) / RATE, 4), "peak_dbfs": round(20 * np.log10(max(np.max(np.abs(data)), 1e-9)), 2),
                         "rms_dbfs": round(20 * np.log10(max(np.sqrt(np.mean(data ** 2)), 1e-9)), 2),
                         "sha256": hashlib.sha256(destination.read_bytes()).hexdigest()})
        print(f"Rendered {name}: {len(data)/RATE:.2f}s", flush=True)
        return data

    for entry in refined_actions(args.sources, read):
        export(*entry)
    if args.only_actions:
        inputs = json.loads((args.sources / "sources.json").read_text(encoding="utf-8"))
        (args.output / "audio-manifest.json").write_text(json.dumps({"sample_rate": RATE, "channels": 2, "old_audio_sampled": True, "inputs": inputs, "assets": MANIFEST}, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        return

    sea = filter_audio(read(args.sources / "sea.flac"), 65, 5800)
    river = filter_audio(read(args.sources / "river.wav", 22, 2), 90, 6200)
    forest = filter_audio(read(args.sources / "birds.wav", 26, 2), 120, 6500)
    rain = filter_audio(read(args.sources / "rain" / "2.ogg", 22), 100, 6500)
    export("brandung.ogg", loop(bed(sea, 22)), "gentle coastal surf", ["jasinski sea recording"], True, .10)
    export("fluss.ogg", loop(river), "nearby fresh stream", ["Thimras river recording"], True, .085)
    export("wald.ogg", loop(forest), "leaves and sparse birds", ["Thimras birds recording"], True, .055)
    export("rain.ogg", loop(rain), "steady rainfall", ["Ylmir rain recording"], True, .09)

    time = samples(19)
    storm = noise(19, 35, 1800) * (.05 + .035 * np.sin(2 * np.pi * time / 6) ** 2)[:, None]
    for at in (3.2, 12.6):
        thunder_time = samples(3.7)
        rumble = noise(3.7, 35, 330) * (.11 * np.sin(np.pi * np.minimum(1, thunder_time / .6)) ** 2 * np.exp(-thunder_time / 1.4))[:, None]
        place(storm, fade(rumble, .4, .8), at)
    export("sturm.ogg", loop(storm), "storm wind and distant low thunder", ["original synthesis"], True, .08)

    chop = filter_audio(kenney("chop"), 70, 5800)
    cloth = filter_audio(kenney("cloth2"), 180, 7000)
    leather = filter_audio(kenney("handleSmallLeather"), 120, 5500)
    creak = filter_audio(kenney("creak2"), 70, 4600)
    step = filter_audio(kenney("footstep04"), 80, 4600)
    hit = fit(chop, .48, 1.15) * .55 + resonance(.48, 1250, .075) * .08
    export("schlagen.wav", hit, "stone tool shaping", ["Kenney chop", "original synthesis"], rms=.11)
    export("hammer.wav", fit(chop, .44, 1.25) * .65 + resonance(.44, 420, .08) * .1, "wooden hammer", ["Kenney chop", "original synthesis"], rms=.12)
    export("schleuder.wav", fit(kenney("knifeSlice"), .42) * .65 + fit(cloth, .42, 1.4) * .2, "sling cord release", ["Kenney knifeSlice and cloth2"], rms=.085)
    export("schaufeln.wav", fit(step, .75, .72) * .6 + fit(cloth, .75, .65) * .45, "soil scrape", ["Kenney footstep04 and cloth2"], rms=.1)
    export("knistern.wav", fit(cloth, .8, .8) + fit(leather, .8) * .2, "searching and binding material", ["Kenney cloth2 and handleSmallLeather"], rms=.075)
    export("step.wav", fit(step, .32, 1.05), "quiet ground footfall", ["Kenney footstep04"], rms=.085)
    export("bandage.wav", fit(cloth, 1.8, .75) * .65 + fit(leather, 1.8, .65) * .25, "cloth wrapping a wound", ["Kenney cloth2 and handleSmallLeather"], rms=.075)

    splash = fit(filter_audio(sea, 140, 7200), 1.0, 1.05)
    export("platsch.wav", fade(splash, .035, .45), "small water splash", ["jasinski sea recording"], rms=.12)
    swim = np.zeros((round(1.2 * RATE), 2))
    place(swim, fade(fit(splash, .65), .05, .35), .02, .8, -.2)
    place(swim, fade(fit(splash, .58, 1.12), .05, .3), .57, .6, .2)
    export("schwimmen.wav", swim, "two gentle swimming strokes", ["jasinski sea recording"], rms=.09)
    fill_water = fade(fit(river, 1.5) * .8, .12, .3)
    place(fill_water, fit(leather, .35), 0, .3)
    export("fill.wav", fill_water, "filling the waterskin", ["Thimras river recording", "Kenney handleSmallLeather"], rms=.10)
    fish = np.zeros((round(.7 * RATE), 2))
    place(fish, fit(leather, .25), 0, .25)
    place(fish, fade(fit(splash, .45), .02, .22), .2, .5)
    export("angel.wav", fish, "line and small water contact", ["Kenney handleSmallLeather", "jasinski sea recording"], rms=.075)

    timber = np.zeros((round(2.5 * RATE), 2))
    place(timber, fit(creak, 1.8, .62), 0, .7)
    place(timber, fit(chop, .7, .65), .95, .9)
    place(timber, fade(noise(1.25, 120, 3300), .08, .85), 1.2, .025)
    export("baumfaellt.wav", timber, "wood bends, breaks and settles", ["Kenney creak2 and chop", "original synthesis"], rms=.1)
    crash = np.zeros((round(3.2 * RATE), 2))
    place(crash, fit(creak, 2, .55), 0, .6)
    place(crash, fit(chop, 1, .6), .45, .9)
    place(crash, fade(bed(sea, 2.5), .08, 1.1), .6, .65)
    export("crash.wav", crash, "ship timber and water impact", ["Kenney creak2 and chop", "jasinski sea recording"], rms=.13)

    fire = noise(14, 90, 4500) * .012
    for at in RNG.uniform(0, 13.5, 70):
        duration = RNG.uniform(.012, .09)
        grain = noise(duration, 300, 6000) * np.exp(-samples(duration) / (duration / 3))[:, None]
        place(fire, fade(grain, .001, .004), at, RNG.uniform(.014, .045), RNG.uniform(-.6, .6))
    export("feuer.ogg", loop(fire), "small campfire and dry pops", ["original synthesis"], True, .065)
    t = samples(7.2)
    breathing = noise(7.2, 180, 1700) * (.009 * np.maximum(0, np.sin(2 * np.pi * t / 3.6)) ** 1.7)[:, None]
    export("schnarchen.ogg", loop(breathing, .4), "quiet breathing during sleep", ["original synthesis"], True, .035)
    t = samples(1.75)
    call = np.sin(2 * np.pi * (390 * t + 22 * np.sin(2 * np.pi * .7 * t))) * np.sin(np.pi * t / 1.75) ** 2 * np.exp(-t / 1.2)
    export("wolf.wav", filter_audio(stereo(call), 180, 1300), "distant nocturnal bird", ["original synthesis"], rms=.045)

    def pluck(seconds, midi, softness=1):
        t = samples(seconds)
        frequency = 440 * 2 ** ((midi - 69) / 12)
        data = sum(np.sin(2 * np.pi * frequency * harmonic * t + .2 * np.sin(2 * np.pi * .7 * t)) * np.exp(-t / (softness * (1.5 / harmonic ** .55))) / harmonic ** 2.0 for harmonic in (1, 2, 3, 4, 5))
        return fade(data, .007, .2)

    def motif(outro=False):
        beat = 60 / 72
        bars = 12 if outro else 16
        length = bars * 4 * beat
        out = np.zeros((round(length * RATE), 2))
        chords = ((50, 57, 60, 64), (46, 53, 57, 60), (48, 55, 60, 62), (45, 52, 57, 60))
        melody = (74, 69, 72, 65, 67, 69, 64, 62, 65, 69, 72, 74, 72, 69, 67, 65)
        for bar in range(bars):
            chord = chords[(bar // 2 + (1 if outro else 0)) % len(chords)]
            for index, note in enumerate(chord):
                at = bar * 4 * beat + index * .13
                place(out, pluck(4, note, 1.5), at, .035, (index - 1.5) * .27)
            if bar % 2 == 0:
                for index in range(2):
                    at = (bar * 4 + 1 + index * 1.75) * beat
                    note = melody[(bar + index + (4 if outro else 0)) % len(melody)]
                    place(out, pluck(3.8, note, 1.2), at, .045 if index == 0 else .032, .1 if index else -.1)
        # A soft stereo room tail. Circular delays preserve a continuous musical loop.
        wet = out.copy()
        for delay, gain in ((.173, .15), (.291, .1), (.433, .065)):
            wet += np.roll(out[:, ::-1], round(delay * RATE), axis=0) * gain
        return wet
    menu = export("logo.ogg", motif(), "original quiet shore motif, 72 BPM", ["original composition and synthesis"], True, .085)
    export("abspann.ogg", motif(True), "original hopeful reprise", ["original composition and synthesis"], True, .085)
    for name, notes, purpose in (("klick.wav", (76,), "muted wooden interface tap"), ("klick2.wav", (79, 83), "soft confirmation"), ("erfindung.wav", (62, 69, 74), "tool ready, short wooden chime")):
        length = .13 if name == "klick.wav" else .25 if name == "klick2.wav" else 1.05
        out = np.zeros((round(length * RATE), 2))
        for index, note in enumerate(notes):
            place(out, pluck(length, note, .3), index * (.04 if len(notes) < 3 else .1), .08)
        export(name, out, purpose, ["original synthesis"], rms=.05 if len(notes) < 3 else .075)

    inputs = json.loads((args.sources / "sources.json").read_text(encoding="utf-8")) if (args.sources / "sources.json").exists() else []
    manifest = {"sample_rate": RATE, "channels": 2, "old_audio_sampled": True, "inputs": inputs, "assets": MANIFEST}
    (args.output / "audio-manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    # Short, quieter showcase for review; no per-gameplay mixing is baked into assets.
    preview = np.zeros((RATE * 24, 2))
    place(preview, fit(menu, 24), 0, .5)
    place(preview, fade(bed(sea, 24), .5, 1), 0, .6)
    for name, at in (("faellen.wav", 4), ("baumfaellt.wav", 5), ("fill.wav", 9), ("trinken.wav", 12), ("bandage.wav", 16), ("erfindung.wav", 20)):
        place(preview, read(args.output / name), at, .65)
    export("audio-preview.ogg", fade(preview, .7, 1.5), "24 second review montage", ["see individual files"], rms=.10)

if __name__ == "__main__":
    main()
