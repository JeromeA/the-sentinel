#!/usr/bin/env python3
"""Deterministic EXEPACK encoder for Sentinel's 18-byte-header DOS variant.

Input is an ordinary, fully expanded MZ executable. Support is assembled from
sentinel.asm with -DEXEPACK_SUPPORT=1: a 512-byte MZ header template followed
by the original decompressor and its error message. It contains no game image
or precomputed compression records. This is not a general EXEPACK replacement.
"""
import argparse
import itertools
from pathlib import Path
import struct

ERROR_MESSAGE = b'Packed file is corrupt'
HEADER_BYTES = 512


def mz_header(data):
    if len(data) < 28:
        raise ValueError('Truncated DOS header')
    header = list(struct.unpack_from('<14H', data))
    if header[0] != 0x5a4d or header[2] == 0 or header[1] > 511:
        raise ValueError('Invalid DOS MZ header')
    return header


def compress(image):
    """Encode maximal byte runs, preserving the observed Microsoft policy.

    Record payload precedes its count and opcode because decoding runs
    backwards. The lowest-address record has the terminal bit set. Interior
    runs need eight bytes to offset both fill and additional literal framing;
    a trailing run can use four. Counts are limited to 16 bits.
    """
    if not image:
        raise ValueError('Cannot pack an empty load image')
    records = []

    def record(payload, count, opcode):
        while count:
            n = min(count, 65535)
            records.append((payload[:n] if opcode == 0xb2 else payload, n, opcode))
            if opcode == 0xb2:
                payload = payload[n:]
            count -= n

    position = literal_start = 0
    for value, group in itertools.groupby(image):
        count = sum(1 for _ in group)
        end = position + count
        if count >= 8 or (end == len(image) and count >= 4):
            if position > literal_start:
                record(image[literal_start:position], position-literal_start, 0xb2)
            record(bytes([value]), count, 0xb0)
            literal_start = end
        position = end
    if literal_start < len(image):
        record(image[literal_start:], len(image)-literal_start, 0xb2)
    stream = b''.join(payload + struct.pack('<HB', count, opcode | (i == 0))
                      for i, (payload, count, opcode) in enumerate(records))
    return stream + b'\xff' * (-len(stream) % 16)


def packed_relocations(executable, header, image_size):
    begin, count = header[12], header[3]
    if begin < 28 or begin + count*4 > header[4]*16:
        raise ValueError('Relocation table lies outside DOS header')
    groups = [[] for _ in range(16)]
    for i in range(count):
        offset, segment = struct.unpack_from('<HH', executable, begin+i*4)
        address = segment*16 + offset
        if address+2 > image_size or address >= 0x100000:
            raise ValueError('Relocation lies outside the load image')
        groups[address >> 16].append(address & 0xffff)
    result = bytearray()
    for group in groups:
        if len(group) > 65535:
            raise ValueError('Too many relocations in a 64 KiB block')
        result.extend(struct.pack('<H', len(group)))
        for offset in group:
            result.extend(struct.pack('<H', offset))
    return bytes(result)


def pack(executable, support):
    original = mz_header(executable)
    template = mz_header(support)
    if template[4]*16 != HEADER_BYTES or len(support) <= HEADER_BYTES:
        raise ValueError('Unsupported EXEPACK support header')
    stub = support[HEADER_BYTES:]
    if not stub.endswith(ERROR_MESSAGE) or len(stub) != 283:
        raise ValueError('Expected the original 283-byte EXEPACK stub and message')
    size = (original[2]-1)*512 + (original[1] or 512)
    if size != len(executable) or original[4]*16 > size or original[13] != 0:
        raise ValueError('Truncated executable, overlays, or invalid header size')
    image = executable[original[4]*16:]
    if not image:
        raise ValueError('Empty DOS load image')
    relocations = packed_relocations(executable, original, len(image))
    image += b'\0' * (-len(image) % 16)
    stream = compress(image)
    block_size = 18 + len(stub) + len(relocations)
    image_paragraphs = len(image)//16
    stream_paragraphs = len(stream)//16
    block_paragraphs = (block_size+15)//16
    # The stub copies itself above the expanded image before unpacking. Its
    # temporary stack needs eight paragraphs; preserve the application's own
    # allocation request as well, adjusting for the smaller on-disk image.
    packed_ss = image_paragraphs + block_paragraphs
    required_paragraphs = max(image_paragraphs + original[5], packed_ss + 8)
    minimum_allocation = max(0, required_paragraphs-stream_paragraphs-block_paragraphs)
    final_size = HEADER_BYTES + len(stream) + block_size
    packed = [0x5a4d, final_size % 512, (final_size+511)//512, 0, HEADER_BYTES//16,
              minimum_allocation, original[6], packed_ss, 0x80, 0, 18,
              stream_paragraphs, template[12], 0]
    ep = [original[10], original[11], 0, block_size, original[8], original[7],
          image_paragraphs, 1, 0x4252]
    if any(value > 65535 for value in packed + ep):
        raise ValueError('Executable exceeds this EXEPACK variant\'s 16-bit limits')
    if len(stream) > len(image):
        raise ValueError('Image does not compress: this stub needs a nonexpanding stream')
    if original[6] != 0xffff and original[6] < minimum_allocation:
        raise ValueError('DOS maximum allocation is smaller than required packed allocation')
    header = bytearray(support[:HEADER_BYTES])
    struct.pack_into('<14H', header, 0, *packed)
    return bytes(header) + stream + struct.pack('<9H', *ep) + stub + relocations


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path, help='expanded MZ executable')
    parser.add_argument('output', type=Path, help='packed MZ executable')
    parser.add_argument('--support', type=Path, required=True)
    args = parser.parse_args()
    try:
        data = args.input.read_bytes()
        result = pack(data, args.support.read_bytes())
    except (ValueError, OSError) as error:
        parser.exit(1, f'EXEPACK: {error}\n')
    args.output.write_bytes(result)
    print(f'EXEPACK: {len(data):,} -> {len(result):,} bytes ({args.output})')


if __name__ == '__main__':
    main()
