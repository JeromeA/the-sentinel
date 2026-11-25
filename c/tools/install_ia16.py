#!/usr/bin/env python3
"""Unpack pinned Linux IA-16 packages locally; never modify apt or /usr."""
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
VENDOR = ROOT / 'tools/vendor/ia16'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    manifest = json.loads((ROOT / 'tools/ia16-packages.json').read_text())
    cache = VENDOR / 'packages'
    prefix = VENDOR / 'root'
    cache.mkdir(parents=True, exist_ok=True)
    prefix.mkdir(parents=True, exist_ok=True)
    for package in manifest['packages']:
        archive = cache / Path(package['Filename']).name
        if not archive.exists() or digest(archive) != package['SHA256']:
            temporary = archive.with_suffix('.part')
            subprocess.run(['curl', '--fail', '--location', '--silent',
                            '--show-error', '--retry', '3', '--connect-timeout', '30',
                            '--output', str(temporary),
                            manifest['base_url'] + package['Filename']], check=True)
            if digest(temporary) != package['SHA256']:
                raise RuntimeError(f'Checksum mismatch: {archive.name}')
            temporary.replace(archive)
        print(f"Extracting {package['Package']} {package['Version']}", flush=True)
        subprocess.run(['dpkg-deb', '--extract', str(archive), str(prefix)], check=True)
    (VENDOR / 'installed-packages.json').write_text(json.dumps(manifest, indent=2) + '\n')
    for tool in ('gcc', 'as', 'ld'):
        subprocess.run([str(ROOT / 'tools/ia16'), tool, '--version'], check=True)


if __name__ == '__main__':
    main()
