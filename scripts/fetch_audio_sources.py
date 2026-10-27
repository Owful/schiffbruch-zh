"""Fetch the documented CC0 recordings used by the audio redesign."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import urllib.request
import zipfile

SOURCES = {
    "kenney-rpg.zip": "https://kenney.nl/media/pages/assets/rpg-audio/8e99002d76-1677590336/kenney_rpg-audio.zip",
    "sea.flac": "https://opengameart.org/sites/default/files/wave_01_cc0-18363__jasinski__alkaibeach.flac",
    "birds.wav": "https://opengameart.org/sites/default/files/park_ambience_birds.wav",
    "river.wav": "https://opengameart.org/sites/default/files/park_ambience_river.wav",
    "rain.zip": "https://opengameart.org/sites/default/files/Rain%20OGG.zip",
}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    args.directory.mkdir(parents=True, exist_ok=True)
    def fetch(item):
        name, url = item
        destination = args.directory / name
        if not destination.exists():
            temporary = destination.with_suffix(destination.suffix + ".part")
            with urllib.request.urlopen(urllib.request.Request(url, headers={"User-Agent": "Schiffbruch-ZH-audio"}), timeout=90) as response, temporary.open("wb") as out:
                while block := response.read(1024 * 1024):
                    out.write(block)
            temporary.replace(destination)
        digest = hashlib.sha256(destination.read_bytes()).hexdigest()
        print(f"{name}: {destination.stat().st_size} bytes, {digest}", flush=True)
        return {"file": name, "url": url, "sha256": digest}
    with ThreadPoolExecutor(max_workers=3) as pool:
        records = list(pool.map(fetch, SOURCES.items()))
    (args.directory / "sources.json").write_text(json.dumps(records, indent=2), encoding="utf-8")
    for archive, folder in (("kenney-rpg.zip", "kenney"), ("rain.zip", "rain")):
        destination = (args.directory / folder).resolve()
        with zipfile.ZipFile(args.directory / archive) as package:
            for name in package.namelist():
                if not (destination / name).resolve().is_relative_to(destination):
                    raise ValueError("Archive member leaves the source directory")
            package.extractall(destination)

if __name__ == "__main__":
    main()
