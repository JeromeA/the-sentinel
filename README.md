# The Sentinel

Three independent implementations:

- `asm/`: annotated NASM reconstruction, including EXEPACK compression to rebuild the original DOS executable.
- `c/`: C reconstruction compiled into an unpacked DOS executable with the Linux-hosted IA-16 toolchain.
- `web/`: the standalone Three.js remaster. Open `web/index.html` in a desktop browser; no build or server is needed.

Each folder has its own build/run instructions. DOS runtime resources are included with both DOS versions. Build outputs and downloaded compiler tools are ignored by Git.
