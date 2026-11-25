# Annotated assembly

Requires GNU Make, NASM and Python 3. On Ubuntu:

```sh
sudo apt install make nasm python3
make
make verify
```

The build assembles `sentinel.asm` into an expanded MZ executable and EXEPACK support, then compresses it with `exepack.py`. `build/Sentinel.exe` matches the original executable byte for byte; `make verify` checks its SHA-256.

With DOSBox-X installed, `make run` starts the game. Alternatively, run `build/Sentinel.exe` in DOS with the three `.bin` files from `build/` alongside it. `make clean` removes build outputs.
