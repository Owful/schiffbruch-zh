"""Create reviewed source and Windows archives without copying player saves."""

import argparse
import hashlib
from pathlib import Path
import subprocess
import zipfile


ROOT = Path(__file__).resolve().parents[1]
VERSION = "v0.1.0-preview.10"
NAME = f"Stillshore-{VERSION}"
RUNTIME_FOLDER = "余岸-Stillshore"
DOCUMENTS = (
    "README.md",
    "README.upstream.md",
    "CHANGELOG.md",
    "THIRD_PARTY.md",
    "AUDIO_CREDITS.md",
    "audio-manifest.json",
    "audio-validation.json",
    "audio-action-validation.json",
    "IMPROVEMENTS.zh-CN.md",
    "验证记录.md",
    "行动与制作对照.md",
    "LICENSE",
)
EXCLUDED_NAMES = {"save.dat", "save.dat.tmp", "legacy-save.dat"}


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def add_file(archive, path, name):
    if path.name.lower() in EXCLUDED_NAMES or path.is_symlink():
        raise ValueError(f"Unsafe release entry: {path}")
    if not path.is_file():
        raise FileNotFoundError(path)
    archive.write(path, name)


def checked_archive(path, entries):
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=7) as archive:
        for source, destination in sorted(entries, key=lambda item: item[1]):
            add_file(archive, source, destination)
    with zipfile.ZipFile(path) as archive:
        if archive.testzip() is not None:
            raise ValueError(f"Corrupt ZIP: {path}")
        names = archive.namelist()
        if len(names) != len(set(names)):
            raise ValueError(f"Duplicate ZIP member: {path}")
        if any(Path(name).name.lower() in EXCLUDED_NAMES for name in names):
            raise ValueError(f"Player save in ZIP: {path}")
        if any(any(part == ".git" or part.startswith("build-") or part.startswith("qa-") for part in Path(name).parts[:-1]) for name in names):
            raise ValueError(f"Development files in ZIP: {path}")
    return len(names)


def tracked_sources():
    result = subprocess.run(
        ["git", "ls-files", "-z"], cwd=ROOT, capture_output=True, check=True
    )
    return [ROOT / Path(name.decode("utf-8")) for name in result.stdout.split(b"\0") if name]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime", type=Path, required=True, help="Playable installation directory")
    parser.add_argument("--output", type=Path, default=ROOT / "dist")
    parser.add_argument("--verify-build", type=Path, help="Release executable to compare byte for byte")
    parser.add_argument("--executable", type=Path, help="Validated executable to package without changing the player's installation")
    args = parser.parse_args()
    runtime = args.runtime.resolve()
    output = args.output.resolve()
    exe = (args.executable or runtime / "bin" / "Schiffbruch.exe").resolve()
    if not exe.is_file():
        parser.error(f"Missing release executable: {exe}")
    build = (args.verify_build or ROOT / "build-release" / "Schiffbruch.exe").resolve()
    if build.exists() and exe.read_bytes() != build.read_bytes():
        parser.error("Runtime executable differs from validated Release build")
    cache = ROOT / "build-release" / "CMakeCache.txt"
    if cache.exists() and "SCHIFFBRUCH_QA_PREVIEW:BOOL=OFF" not in cache.read_text(encoding="utf-8"):
        parser.error("Release build has QA preview enabled")

    output.mkdir(parents=True, exist_ok=True)
    binaries = [path for path in (runtime / "bin").iterdir() if path.is_file() and path.suffix.lower() in {".exe", ".dll"}]
    if len([path for path in binaries if path.suffix.lower() == ".dll"]) < 4:
        parser.error("Runtime dependencies appear incomplete")
    runtime_entries = [(exe if path.name == "Schiffbruch.exe" else path, f"{RUNTIME_FOLDER}/bin/{path.name}") for path in binaries]
    for folder in ("images", "sounds"):
        runtime_entries.extend((path, f"{RUNTIME_FOLDER}/bin/{folder}/{path.name}") for path in (ROOT / "bin" / folder).iterdir() if path.is_file())
    runtime_entries.extend((ROOT / name, f"{RUNTIME_FOLDER}/{name}") for name in DOCUMENTS)
    runtime_entries.extend((path, f"{RUNTIME_FOLDER}/{path.relative_to(ROOT).as_posix()}") for path in (ROOT / "docs").rglob("*") if path.is_file())
    runtime_entries.extend((path, f"{RUNTIME_FOLDER}/licenses/{path.name}") for path in (ROOT / "licenses").iterdir() if path.is_file())
    runtime_entries.append((ROOT / "README.md", f"{RUNTIME_FOLDER}/bin/README.md"))
    runtime_entries.append((ROOT / "Start-Game.vbs", f"{RUNTIME_FOLDER}/开始游戏.vbs"))

    source_entries = [(path, f"Stillshore/{path.relative_to(ROOT).as_posix()}") for path in tracked_sources()]
    results = []
    for filename, entries in (
        (f"{NAME}-Windows-x64.zip", runtime_entries),
        (f"{NAME}-source.zip", source_entries),
    ):
        destination = output / filename
        count = checked_archive(destination, entries)
        results.append((destination, count))
    openal_source = output / "OpenAL-Soft-1.18.1-source.zip"
    if not openal_source.is_file():
        parser.error(f"Missing official OpenAL Soft source archive: {openal_source}")
    with zipfile.ZipFile(openal_source) as archive:
        if archive.testzip() is not None or not any(name.endswith("/COPYING") for name in archive.namelist()):
            parser.error("OpenAL Soft source archive lacks a valid COPYING file")
        results.append((openal_source, len(archive.namelist())))
    checksums = output / "SHA256SUMS.txt"
    checksums.write_text("".join(f"{sha256(path)}  {path.name}\n" for path, _ in results), encoding="ascii")
    for path, count in results:
        print(f"{path.name}: {count} files, {path.stat().st_size} bytes, SHA256 {sha256(path)}")
    print(f"Checksums: {checksums}")


if __name__ == "__main__":
    main()
