/* Browser-native readers: binary PLY, planar resources and indexed PNG atlases. */
window.Sentinel = window.Sentinel || {};
(function (S) {
    'use strict';
    const EGA = [0x000000, 0x0000aa, 0x00aa00, 0x00aaaa,
        0xaa0000, 0xaa00aa, 0xaa5500, 0xaaaaaa,
        0x555555, 0x5555ff, 0x55ff55, 0x55ffff,
        0xff5555, 0xff55ff, 0xffff55, 0xffffff];

    function bytes(base64) {
        return Uint8Array.from(atob(base64), c => c.charCodeAt(0));
    }

    function parsePLY(buffer) {
        const raw = buffer instanceof Uint8Array ? buffer : new Uint8Array(buffer);
        const marker = new TextEncoder().encode('end_header');
        let offset = -1;
        outer: for (let i = 0; i < Math.min(raw.length, 8192); ++i) {
            if (i && raw[i-1] !== 10) continue;
            for (let j = 0; j < marker.length; ++j) if (raw[i + j] !== marker[j]) continue outer;
            let end = i + marker.length;
            if (raw[end] === 13) ++end;
            if (raw[end] !== 10) continue;
            offset = end + 1;
            break;
        }
        if (offset < 0) throw new Error('PLY header is missing');
        const header = new TextDecoder().decode(raw.subarray(0, offset));
        if (!header.includes('format binary_little_endian 1.0')) throw new Error('Expected binary little-endian PLY');
        const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        const types = {
            char: ['getInt8', 1], uchar: ['getUint8', 1], short: ['getInt16', 2],
            ushort: ['getUint16', 2], int: ['getInt32', 4], uint: ['getUint32', 4],
            float: ['getFloat32', 4], double: ['getFloat64', 8]
        };
        Object.assign(types, {int8: types.char, uint8: types.uchar, int16: types.short,
            uint16: types.ushort, int32: types.int, uint32: types.uint,
            float32: types.float, float64: types.double});
        const elements = [];
        for (const line of header.split('\n')) {
            const p = line.trim().split(/\s+/);
            if (p[0] === 'element') elements.push({name: p[1], count: Number(p[2]), props: []});
            if (p[0] === 'property') elements[elements.length - 1].props.push(p.slice(1));
        }
        function scalar(type) {
            if (!types[type]) throw new Error('Unsupported PLY type: ' + type);
            const [get, size] = types[type];
            const value = view[get](offset, true);
            offset += size;
            return value;
        }
        const vertices = [], faces = [];
        for (const element of elements) {
            for (let i = 0; i < element.count; ++i) {
                const record = {};
                for (const prop of element.props) {
                    if (prop[0] === 'list') {
                        const count = scalar(prop[1]);
                        if (count < 0 || count > 4096) throw new Error('Invalid PLY list');
                        record[prop[3]] = Array.from({length: count}, () => scalar(prop[2]));
                    } else record[prop[1]] = scalar(prop[0]);
                }
                if (element.name === 'vertex') vertices[record.sentinel_vertex_id ?? i] = [record.x, record.y, record.z];
                if (element.name === 'face') faces[record.sentinel_face_id ?? i] = record.vertex_indices;
            }
        }
        for (const face of faces) for (const index of face) {
            if (!vertices[index]) throw new Error('PLY face references a missing vertex');
        }
        return {vertices, faces};
    }

    function decodeResource(raw) {
        if (raw.length !== 32000) throw new Error('A Sentinel resource must contain 32000 bytes');
        const pixels = new Uint8Array(320 * 200);
        for (let group = 0; group < 4000; ++group) {
            for (let bit = 0; bit < 16; ++bit) {
                let color = 0;
                for (let p = 0; p < 4; ++p) {
                    const plane = (raw[group * 8 + p * 2] << 8) | raw[group * 8 + p * 2 + 1];
                    color |= ((plane >>> (15 - bit)) & 1) << p;
                }
                pixels[group * 16 + bit] = color;
            }
        }
        return pixels;
    }

    function readPNG(base64) {
        return new Promise((resolve, reject) => {
            const image = new Image();
            image.onload = () => {
                try {
                    const canvas = document.createElement('canvas');
                    canvas.width = image.width; canvas.height = image.height;
                    const context = canvas.getContext('2d', {willReadFrequently: true});
                    context.drawImage(image, 0, 0);
                    resolve({width: image.width, height: image.height,
                        data: context.getImageData(0, 0, image.width, image.height).data});
                } catch (error) { reject(error); }
            };
            image.onerror = () => reject(new Error('Unable to decode embedded PNG'));
            image.src = 'data:image/png;base64,' + base64;
        });
    }

    function readSVG(base64) {
        return new Promise((resolve, reject) => {
            const image = new Image();
            image.onload = () => resolve(image);
            image.onerror = () => reject(new Error('Unable to decode title mountain SVG'));
            image.src = 'data:image/svg+xml;base64,' + base64;
        });
    }

    async function loadAssets(files = S.assetFiles) {
        const json = name => JSON.parse(new TextDecoder().decode(bytes(files[name])));
        const manifest = json('models/models.json');
        const models = manifest.models.map(name => {
            const metadata = json('models/' + name);
            const mesh = parsePLY(bytes(files['models/' + metadata.mesh]));
            if (!metadata.faces || metadata.faces.length !== mesh.faces.length) throw new Error(name + ': face metadata count differs from PLY');
            for (const face of metadata.faces) if (!face.palette_pair || face.palette_pair.some(n => !Number.isInteger(n) || n < 0 || n > 15)) throw new Error(name + ': invalid face palette');
            return {...metadata, materials: metadata.faces, ...mesh};
        });
        const [font, icons, mountains] = await Promise.all([readPNG(files['images/font.png']),
            readPNG(files['images/color.png']), readSVG(files['images/title-mountains.svg'])]);
        if (font.width !== 128 || font.height !== 32 || icons.width !== 80 || icons.height !== 8) throw new Error('Font must be 128×32 and icons 80×8');
        const fontRows = Array.from({length: 64}, (_, c) => Array.from({length: 8}, (_, y) => {
            let bits = 0;
            for (let x = 0; x < 8; ++x) {
                const at = (((c >>> 4) * 8 + y) * font.width + (c & 15) * 8 + x) * 4;
                if (font.data[at] > 127 && font.data[at + 3]) bits |= 128 >>> x;
            }
            return bits;
        }));
        const resources = {};
        for (const name of ['Sentry', 'Skybox', 'Tfinal']) resources[name] = decodeResource(bytes(files['original/' + name + '.bin']));
        return {models, font, fontRows, icons, mountains, resources, palette: EGA};
    }
    S.Assets = {bytes, parsePLY, decodeResource, readPNG, load: loadAssets, palette: EGA};
}(window.Sentinel));
