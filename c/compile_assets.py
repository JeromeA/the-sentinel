#!/usr/bin/env python3
"""Compile editable PNG/PLY assets to the compact tables used by the DOS game.

The converter reads only assets and metadata, never C initializers, a reference
executable or NASM. Geometry stays in PLY; reference polar encodings disambiguate
unchanged vertices (notably zero-radius vertices and signed zero heights).
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct


EGA_PALETTE = [(0,0,0),(0,0,170),(0,170,0),(0,170,170),
               (170,0,0),(170,0,170),(170,85,0),(170,170,170),
               (85,85,85),(85,85,255),(85,255,85),(85,255,255),
               (255,85,85),(255,85,255),(255,255,85),(255,255,255)]
PLY_TYPES = {'char':'b','uchar':'B','short':'h','ushort':'H','int':'i',
             'uint':'I','float':'f','double':'d',
             'int8':'b','uint8':'B','int16':'h','uint16':'H',
             'int32':'i','uint32':'I','float32':'f','float64':'d'}
VISIBILITY = {'single_pass':0,'heading_ordered':2,'height_ordered':3}
GEOMETRY_TOLERANCE = 1e-6


def load_json(path):
    return json.loads(Path(path).read_text())


def read_pixels(path, width, height, palette):
    from PIL import Image
    with Image.open(path) as image:
        if image.format != 'PNG' or image.size != (width,height):
            raise ValueError(f'{path}: expected a {width}x{height} PNG')
        raw=image.convert('RGBA').tobytes()
        rgba=zip(raw[0::4],raw[1::4],raw[2::4],raw[3::4])
    colors={tuple(color):index for index,color in enumerate(palette)}
    if len(colors)!=len(palette):raise ValueError('Logical palette colors must be distinct')
    pixels=[]
    for offset,(*rgb,alpha) in enumerate(rgba):
        if alpha==0:
            pixels.append(0)
        elif alpha==255 and tuple(rgb) in colors:
            pixels.append(colors[tuple(rgb)])
        else:
            raise ValueError(f'{path}: pixel ({offset%width},{offset//width}) is outside the logical palette; no antialiasing or partial alpha is allowed')
    return pixels


def write_indexed_png(path, width, height, pixels, palette):
    from PIL import Image
    image=Image.new('P',(width,height))
    image.putpalette([component for color in palette for component in color]+[0]*(768-3*len(palette)))
    image.putdata(pixels)
    image.save(path,format='PNG',optimize=False)


def pack_icon_sheet(pixels, layout, count):
    """Tiles run left to right; rows run top to bottom in every encoding."""
    data=bytearray();width=count*8
    for tile in range(count):
        for y in range(8):
            row=pixels[y*width+tile*8:y*width+tile*8+8]
            if layout=='cga':
                if any(p>3 for p in row):raise ValueError('CGA pixels must be in 0..3')
                for x in (0,4):data.append(sum(row[x+i]<<(6-2*i) for i in range(4)))
            elif layout=='packed':
                for x in (0,2,4,6):data.append(row[x]<<4|row[x+1])
            elif layout=='ega':
                for plane in range(4):data.append(sum(((row[x]>>plane)&1)<<(7-x) for x in range(8)))
            else:raise ValueError('Unknown icon layout: '+layout)
    return bytes(data)


def pack_font(pixels, columns, count):
    width=columns*8;data=bytearray()
    for glyph in range(count):
        for y in range(8):
            offset=((glyph//columns)*8+y)*width+(glyph%columns)*8
            data.append(sum(pixels[offset+x]<<(7-x) for x in range(8)))
    return bytes(data)


def read_ply(path):
    """Read standard little-endian PLY, including editor-added scalar fields.

    Stable IDs, when present, restore original order after an editor reorders
    elements. With ordinary PLY files lacking IDs, file order is authoritative.
    """
    with Path(path).open('rb') as stream:
        if stream.readline().strip()!=b'ply':raise ValueError(f'{path}: expected PLY magic')
        if stream.readline().strip()!=b'format binary_little_endian 1.0':
            raise ValueError(f'{path}: export binary little-endian PLY 1.0')
        elements=[]
        for _ in range(256):
            line=stream.readline(4096).decode('ascii').strip();parts=line.split()
            if line=='end_header':break
            if not parts:raise ValueError(f'{path}: incomplete PLY header')
            if parts[0] in ('comment','obj_info'):continue
            if parts[0]=='element' and len(parts)==3:
                count=int(parts[2])
                if not 0<=count<=4096:raise ValueError(f'{path}: unsupported element count')
                elements.append([parts[1],count,[]])
            elif parts[0]=='property' and elements:
                if len(parts)==3 and parts[1] in PLY_TYPES:
                    elements[-1][2].append((parts[2],parts[1],None))
                elif len(parts)==5 and parts[1]=='list' and parts[2] in PLY_TYPES and parts[3] in PLY_TYPES:
                    if PLY_TYPES[parts[2]] not in 'bBhHiI':raise ValueError('PLY list count must be integral')
                    elements[-1][2].append((parts[4],parts[3],parts[2]))
                else:raise ValueError(f'{path}: unsupported PLY property: {line}')
            else:raise ValueError(f'{path}: unsupported PLY header: {line}')
        else:raise ValueError(f'{path}: PLY header too long')
        def scalar(kind):
            fmt='<'+PLY_TYPES[kind];raw=stream.read(struct.calcsize(fmt))
            if len(raw)!=struct.calcsize(fmt):raise ValueError(f'{path}: truncated PLY payload')
            return struct.unpack(fmt,raw)[0]
        records={}
        for name,count,properties in elements:
            if name in records:raise ValueError(f'{path}: duplicate element {name}')
            rows=[]
            for _ in range(count):
                row={}
                for field,kind,list_kind in properties:
                    if list_kind:
                        length=scalar(list_kind)
                        if not 0<=length<=256:raise ValueError(f'{path}: invalid PLY list length')
                        row[field]=[scalar(kind) for _ in range(length)]
                    else:row[field]=scalar(kind)
                rows.append(row)
            records[name]=rows
        if stream.read(1):raise ValueError(f'{path}: trailing PLY payload')
    vertices=records.get('vertex',[]);faces=records.get('face',[])
    if not vertices or not faces:raise ValueError(f'{path}: mesh must have vertices and faces')
    def ids(rows,key):
        present=[key in row for row in rows]
        if any(present) and not all(present):raise ValueError(f'{path}: incomplete {key} attributes')
        values=[row[key] if all(present) else i for i,row in enumerate(rows)]
        if any(type(v)!=int for v in values) or sorted(values)!=list(range(len(rows))):
            raise ValueError(f'{path}: {key} must be a permutation of consecutive IDs')
        return values
    vertex_ids=ids(vertices,'sentinel_vertex_id');face_ids=ids(faces,'sentinel_face_id')
    positions=[None]*len(vertices);polygons=[None]*len(faces)
    for row,identity in zip(vertices,vertex_ids):
        try:position=tuple(float(row[axis]) for axis in ('x','y','z'))
        except (KeyError,TypeError):raise ValueError(f'{path}: missing scalar x/y/z vertex coordinates')
        if not all(math.isfinite(v) for v in position):raise ValueError(f'{path}: non-finite coordinate')
        positions[identity]=position
    for row,identity in zip(faces,face_ids):
        indices=row.get('vertex_indices',row.get('vertex_index'))
        if not isinstance(indices,list) or len(indices) not in (3,4):raise ValueError(f'{path}: Sentinel supports triangles and quads; disable triangulation on export')
        if any(type(v)!=int or not 0<=v<len(vertices) for v in indices):raise ValueError(f'{path}: face vertex outside mesh')
        polygons[identity]=[vertex_ids[v] for v in indices]
    return positions,polygons


def write_ply(path, vertices, faces):
    header=('ply\nformat binary_little_endian 1.0\n'
            'comment Sentinel coordinates: X right, Y forward, Z up; 1 unit = 256 fixed-point units\n'
            f'element vertex {len(vertices)}\nproperty float x\nproperty float y\nproperty float z\nproperty uint sentinel_vertex_id\n'
            f'element face {len(faces)}\nproperty list uchar uint vertex_indices\nproperty uint sentinel_face_id\nend_header\n')
    with Path(path).open('wb') as stream:
        stream.write(header.encode('ascii'))
        for identity,position in enumerate(vertices):stream.write(struct.pack('<3fI',*position,identity))
        for identity,face in enumerate(faces):
            stream.write(struct.pack('<B',len(face)))
            stream.write(struct.pack('<'+'I'*len(face),*face))
            stream.write(struct.pack('<I',identity))


def polar_position(encoded):
    azimuth,height,radius=encoded
    angle=azimuth*math.tau/256
    return (radius*math.sin(angle)/256,radius*math.cos(angle)/256,
            (height&127)*(-2 if height&128 else 2)/256)


def encode_position(position, reference):
    if len(reference)!=3 or any(type(v)!=int or not 0<=v<=255 for v in reference):
        raise ValueError('Reference polar encoding must contain three bytes')
    if max(abs(a-b) for a,b in zip(position,polar_position(reference)))<=GEOMETRY_TOLERANCE:
        return list(reference)
    x,y,z=position
    if not all(math.isfinite(v) for v in position):raise ValueError('Non-finite model coordinate')
    radius=math.floor(math.hypot(x,y)*256+0.5)
    height=math.floor(abs(z)*128+0.5)
    if radius>255 or height>127:raise ValueError('Edited vertex exceeds radius 255 or height +/-254 fixed-point units')
    azimuth=math.floor(math.atan2(x,y)*256/math.tau+0.5)%256 if radius else reference[0]
    encoded_height=height|(128 if z<0 else 0)
    return [azimuth,encoded_height,radius]


def c_bytes(name, data):
    rows=['    '+', '.join(f'0x{v:02x}' for v in data[i:i+12])+',' for i in range(0,len(data),12)]
    return f'uint8_t {name}[{len(data)}] = {{\n'+'\n'.join(rows)+'\n};\n'


def compile_icons(root):
    meta=load_json(root/'images/icons.json');count=len(meta['tiles']);arrays={}
    if count!=10 or len(set(tile['name'] for tile in meta['tiles']))!=count:raise ValueError('Expected ten uniquely named HUD tiles')
    for bank in meta['banks']:
        layout=bank['layout'];palette=bank['palette']
        if len(palette)!=(4 if layout=='cga' else 16):raise ValueError('Invalid icon palette size')
        pixels=read_pixels(root/'images'/bank['file'],count*8,8,palette)
        arrays[bank['symbol']]=pack_icon_sheet(pixels,layout,count)
    order=['status_tiles_ega','status_tiles_packed','status_tiles_cga']
    if set(arrays)!=set(order):raise ValueError('Expected EGA, packed and CGA icon banks')
    return ''.join(c_bytes(name,arrays[name]) for name in order),arrays


def compile_font(root):
    meta=load_json(root/'images/font.json')
    if meta['first_character']!=32 or meta['glyph_count']!=64 or meta['columns']!=16:
        raise ValueError('Sentinel font requires 64 8x8 glyphs starting at character 32, in 16 columns')
    pixels=read_pixels(root/'images'/meta['file'],128,32,[(0,0,0),(255,255,255)])
    data=pack_font(pixels,16,64)
    return c_bytes('game_font_8x8',data),{'game_font_8x8':data}


def compile_models(root, preview=None):
    directory=root/'models';layout=load_json(directory/'models.json')
    reserved=(directory/'reserved.bin').read_bytes();arrays={};loops={};pointers=[]
    vertex_ranges=[0];face_ranges=[0];azimuths=[];heights=[];radii=[];visibility=[];colors=[];flags=[]
    changed=0
    if len(layout['models'])!=10:raise ValueError('Expected ten model IDs')
    for identity,filename in enumerate(layout['models']):
        meta=load_json(directory/filename)
        if meta['id']!=identity:raise ValueError('Model order must match GameObjectModel IDs')
        positions,faces=read_ply(directory/meta['mesh'])
        reference=meta['reference_polar']
        if len(positions)!=len(reference) or len(faces)!=len(meta['faces']):
            raise ValueError(f'{filename}: retain vertex/face counts and stable IDs; topology expansion requires a format migration')
        if len(positions)>32:raise ValueError(f'{filename}: projection cache supports at most 32 vertices')
        quantized=[]
        for vertex,(position,original) in enumerate(zip(positions,reference)):
            try:encoded=encode_position(position,original)
            except ValueError as error:raise ValueError(f'{filename}: vertex {vertex}: {error}') from error
            changed+=encoded!=original
            azimuths.append(encoded[0]);heights.append(encoded[1]);radii.append(encoded[2])
            quantized.append(polar_position(encoded))
        vertex_ranges.append(len(azimuths))
        if meta['visibility'] not in VISIBILITY:raise ValueError(f'{filename}: unknown visibility ordering')
        visibility.append(VISIBILITY[meta['visibility']])
        for face,appearance in zip(faces,meta['faces']):
            pair=appearance['palette_pair']
            if len(pair)!=2 or any(type(c)!=int or not 0<=c<16 for c in pair):raise ValueError(f'{filename}: palette slots must be in 0..15')
            if type(appearance['back_pass'])!=bool:raise ValueError('back_pass must be true or false')
            symbol=appearance['loop']
            if not symbol.isidentifier() or not symbol.isascii():raise ValueError('Invalid face-loop symbol')
            loop=bytes([0x40+v for v in face]+[0x40+face[0]])
            if symbol in loops and loops[symbol]!=loop:
                raise ValueError(f'{filename}: shared face loop {symbol} has inconsistent topology; preserve shared text-block face indices')
            loops[symbol]=loop;pointers.append(symbol)
            colors.append(pair[0]<<4|pair[1]);flags.append(len(face)|(128 if appearance['back_pass'] else 0))
        face_ranges.append(len(pointers))
        if preview:
            preview.mkdir(parents=True,exist_ok=True)
            write_ply(preview/meta['mesh'],quantized,faces)
    if len(azimuths)!=160 or len(pointers)!=160:raise ValueError('Current tables require 160 total vertices and faces')
    arrays.update(model_vertex_ranges=bytes(vertex_ranges),model_face_ranges=bytes(face_ranges),
                  model_vertex_azimuth=bytes(azimuths),model_vertex_height=bytes(heights),model_vertex_radius=bytes(radii),
                  model_face_color_pairs=bytes(colors),model_face_flags_and_counts=bytes(flags))
    for record in layout['reserved']:
        offset,size=record['offset'],record['size'];chunk=reserved[offset:offset+size]
        if len(chunk)!=size:raise ValueError('Truncated model reserved data')
        if record['symbol']=='model_visibility_tail':arrays['model_visibility_flags']=bytes(visibility)+chunk
        else:arrays[record['symbol']]=chunk
    arrays.update(loops)
    order=layout['storage_order'];names=set(order)
    if names!=set(arrays)|{'model_face_index_pointers'} or len(order)!=len(names):raise ValueError('Model storage manifest is incomplete or contains duplicate names')
    declarations=[];definitions=[]
    byte_views={'model_vertex_height':'MeshHeight',
                'model_face_color_pairs':'MaterialColors',
                'model_face_flags_and_counts':'MeshFaceHeader'}
    for name in order:
        if name=='model_face_index_pointers':
            declarations.append('extern struct UnalignedWord model_face_index_pointers[160];')
            entries=[f'{{(uint16_t)({symbol}+0)}}' for symbol in pointers]
            definitions.append('struct UnalignedWord model_face_index_pointers[160] = {\n'+
                               '\n'.join('    '+', '.join(entries[i:i+12])+',' for i in range(0,160,12))+'\n};\n')
        elif name in byte_views:
            kind=byte_views[name]
            declarations.append(f'extern union {kind} {name}[{len(arrays[name])}];')
            entries=[f'{{.raw = 0x{value:02x}}}' for value in arrays[name]]
            definitions.append(f'union {kind} {name}[{len(arrays[name])}] = {{\n'+
                               '\n'.join('    '+', '.join(entries[i:i+8])+',' for i in range(0,len(entries),8))+'\n};\n')
        else:
            declarations.append(f'extern uint8_t {name}[{len(arrays[name])}];')
            definitions.append(c_bytes(name,arrays[name]))
    return '\n'.join(declarations)+'\n\n'+'\n'.join(definitions),arrays,pointers,changed


def compile_all(root, output):
    root=Path(root);output=Path(output);output.mkdir(parents=True,exist_ok=True)
    icons,icon_arrays=compile_icons(root);font,font_arrays=compile_font(root)
    models,model_arrays,pointers,changed=compile_models(root,output/'quantized-models')
    for name,content in [('icon_assets.h',icons),('font_assets.h',font),('model_assets.h',models)]:
        (output/name).write_text('/* Generated by compile_assets.py; edit assets, not this file. */\n'+content)
    arrays={**icon_arrays,**font_arrays,**model_arrays}
    report={'arrays':{name:{'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()} for name,data in arrays.items()},
            'model_face_pointers':pointers,'quantized_edited_vertices':changed}
    (output/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    return report


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--assets',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    try:report=compile_all(args.assets,args.out)
    except ImportError:parser.exit(1,'PNG assets require Pillow (Ubuntu package: python3-pil).\n')
    except (ValueError,KeyError,OSError) as error:parser.exit(1,f'Asset error: {error}\n')
    print(f"Compiled 3 icon banks, 64 glyphs and 10 models; {report['quantized_edited_vertices']} edited vertices quantized")


if __name__=='__main__':main()
