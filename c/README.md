# C reconstruction

The game builds as an unpacked DOS executable. No reference executable, assembly source or packer is needed. Images and models in `assets/` are compiled into C tables during the build.

On x86-64 Ubuntu, install the host dependencies, install the pinned IA-16 compiler locally, then build:

```sh
sudo apt install make python3 python3-pil curl dpkg libmpc3 libmpfr6 libgmp10 libstdc++6
make toolchain
make
```

`make toolchain` downloads checksum-verified packages and extracts them into `tools/vendor/ia16/`; it does not change system compiler settings. Subsequent builds work offline. To use another compatible IA-16 installation, set `TOOLCHAIN` to a launcher accepting `gcc`, `ld` and `objcopy` as its first argument.

The output is `build/Sentinel.exe`. With DOSBox-X installed, use `make run`. Alternatively, run the executable in DOS with the three `.bin` files from `build/` alongside it. `make clean` removes build outputs but keeps the local compiler.
