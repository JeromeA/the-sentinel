#!/usr/bin/env python3
"""Write the DOS header and relocation table from the linker's ELF metadata.

This handles ordinary compiler-generated far calls; it does not transform or
pack code. Only the MZ relocation table is completed after the GNU link.
"""
from pathlib import Path
import struct
import subprocess
import sys


def finish(elf_path, output, toolchain):
    elf=elf_path.read_bytes()
    assert elf[:6]==b'\x7fELF\x01\x01'
    shoff=struct.unpack_from('<I',elf,32)[0]
    shsize,shcount,shnames=struct.unpack_from('<HHH',elf,46)
    sections=[struct.unpack_from('<10I',elf,shoff+i*shsize) for i in range(shcount)]
    names_section=sections[shnames]
    names=elf[names_section[4]:names_section[4]+names_section[5]]
    section_name=lambda section:names[section[0]:names.index(0,section[0])].decode()
    header_section=next(s for s in sections if section_name(s)=='.mz')
    header=bytearray(elf[header_section[4]:header_section[4]+header_section[5]])
    assert len(header)==512
    phoff=struct.unpack_from('<I',elf,28)[0]
    phsize,phcount=struct.unpack_from('<HH',elf,42)
    loads=[struct.unpack_from('<8I',elf,phoff+i*phsize) for i in range(phcount)]
    def load_address(section):
        for kind,offset,virtual,physical,filesz,memsz,flags,align in loads:
            if kind==1 and offset<=section[4] and section[4]+section[5]<=offset+filesz:
                return physical+section[4]-offset
        raise ValueError('No load address for '+section_name(section))
    count=struct.unpack_from('<H',header,6)[0]
    fixups={struct.unpack_from('<HH',header,28+4*i) for i in range(count)}
    for section in sections:
        if section[1]!=9: continue # SHT_REL
        target=sections[section[7]]
        symbols=sections[section[6]]
        for offset in range(section[4],section[4]+section[5],section[9]):
            address,info=struct.unpack_from('<II',elf,offset)
            if (info&255)==0x15 and target[2]&4: # R_386_PC16 in executable code
                symbol_offset=symbols[4]+(info>>8)*symbols[9]
                symbol_section=struct.unpack_from('<H',elf,symbol_offset+14)[0]
                if 0<symbol_section<len(sections) and sections[symbol_section][2]&4:
                    # Each linked executable section is a distinct 16-bit CS.
                    # In particular, implicit memcpy/libgcc calls must stay local.
                    if symbol_section!=section[7]:
                        raise ValueError('Near call crosses code segments: '+
                                         section_name(target)+' -> '+section_name(sections[symbol_section]))
            if (info&255) != 0x50: continue # R_386_OZSEG16
            physical=load_address(target)+address-target[3]
            fixups.add((physical&15,physical>>4))
    # Leave the fixed metadata prefix intact when far calls need more entries.
    relocation_start=28 if len(fixups)<=117 else 512
    header_size=512 if relocation_start==28 else ((512+4*len(fixups)+511)//512)*512
    header.extend(bytes(header_size-512))
    header[28:496]=bytes(468)
    struct.pack_into('<H',header,8,header_size//16)
    struct.pack_into('<H',header,24,relocation_start)
    for index,(offset,segment) in enumerate(sorted(fixups,key=lambda p:p[1]*16+p[0])):
        struct.pack_into('<HH',header,relocation_start+4*index,offset,segment)
    struct.pack_into('<H',header,6,len(fixups))
    load=output.with_suffix('.load')
    subprocess.run([str(toolchain),'objcopy','-O','binary','-R','.mz',str(elf_path),str(load)],check=True)
    image=load.read_bytes()
    struct.pack_into('<HH',header,2,(header_size+len(image))%512,(header_size+len(image)+511)//512)
    output.write_bytes(header+image)
    load.unlink()


if __name__=='__main__':
    finish(Path(sys.argv[1]),Path(sys.argv[2]),Path(sys.argv[3]))
