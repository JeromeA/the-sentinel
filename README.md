# The Sentinel

**[Play The Sentinel Remaster](https://jeromea.github.io/the-sentinel/web/)** in your browser.

Explore the landscapes, absorb energy, create robots and outwit the Sentinel.
No installation is needed—just a desktop browser, mouse and keyboard.

Three independent implementations:

- `asm/`: annotated NASM reconstruction, including EXEPACK compression to rebuild the original DOS executable.
- `c/`: C reconstruction compiled into an unpacked DOS executable with the Linux-hosted IA-16 toolchain.
- `web/`: the standalone Three.js remaster. Open `web/index.html` in a desktop browser; no build or server is needed.

Each folder has its own build/run instructions. DOS runtime resources are included with both DOS versions. Build outputs and downloaded compiler tools are ignored by Git.
