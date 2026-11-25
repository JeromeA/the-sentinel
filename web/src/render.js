/* Three.js owns every game pixel: world geometry, title artwork and an orthographic HUD. */
(function (S) {
    'use strict';
    const T = window.THREE;
    const TAU = Math.PI * 2;
    const SENTRY_TURN_MS = 256;
    // c/palette.c: VGA DAC entries are six-bit channels, not EGA color numbers.
    // Phase 0 has landscape variants; phase 1 is the fixed title/code palette.
    const VGA_SCENES = [
        [[0,0,0],[39,39,63],[63,63,63],[0,63,0],[0,31,0],[23,55,23],[63,0,0],[0,15,39],
            [0,63,0],[63,63,0],[39,0,39],[23,47,47],[39,39,63],[31,15,0],[63,63,63],[0,0,23]],
        [[0,0,0],[0,39,23],[23,63,63],[31,31,55],[39,15,15],[15,15,39],[55,31,31],[0,0,31],
            [15,15,39],[39,39,15],[0,0,23],[0,0,15],[15,15,55],[0,0,15],[47,47,47],[55,55,23]],
        Array.from({length:16},()=>[0,0,63])
    ];
    const VARIANTS = [
        [[0,39,39],[63,63,63],[0,63,0]], [[39,0,39],[39,63,63],[63,63,39]],
        [[0,39,39],[63,47,47],[39,63,63]], [[55,23,0],[63,63,63],[63,63,39]],
        [[23,23,63],[39,63,63],[63,63,63]], [[63,0,0],[63,63,39],[63,47,47]],
        [[39,0,39],[63,39,63],[63,63,63]], [[23,23,63],[63,0,0],[63,63,0]]
    ];
    const dacColor = channels => channels.reduce((rgb,c)=>(rgb<<8)|((c<<2)|(c>>>4)),0);
    function scenePalette(phase, variant = 0) {
        const palette = VGA_SCENES[phase].map(dacColor);
        if (phase === 0) [palette[1],palette[2],palette[3]] = VARIANTS[variant & 7].map(dacColor);
        return palette;
    }

    class WarningPattern {
        constructor() { this.state = 1; this.sight = 0; this.pixels = new Uint8Array(64 * 4); }
        advance(sight) {
            if (!sight && !this.sight) return;
            this.sight = sight; this.pixels.fill(0);
            // c/selection_effects.c: four 16-pixel blocks, two eight-pixel
            // patterns per row. Each pattern lights a pair of VGA color-9 pixels.
            for (let block = 0; block < 4; ++block) {
                this.state = Math.imul(this.state,5) >>> 0;
                let noise = this.state >>> 16;
                for (let row = 0; row < 4; ++row) {
                    if (row) noise >>>= 2;
                    for (let half = 0; half < 2; ++half) {
                        if (half) noise >>>= 2;
                        if (!sight || (sight === 0.5 && block >= 2)) continue;
                        const at = row * 64 + block * 16 + half * 8 + (noise & 3) * 2;
                        this.pixels[at] = this.pixels[at + 1] = 9;
                    }
                }
            }
        }
    }

    function texture(image, font = false) {
        const data = new Uint8Array(image.data);
        if (font) for (let i = 0; i < data.length; i += 4) {
            data[i + 3] = data[i] > 127 ? 255 : 0;
            data[i] = data[i + 1] = data[i + 2] = 255;
        }
        const map = new T.DataTexture(data, image.width, image.height, T.RGBAFormat);
        map.magFilter = map.minFilter = T.NearestFilter;
        map.colorSpace = T.SRGBColorSpace; map.needsUpdate = true;
        return map;
    }

    function iconImage(assets) {
        const image = assets.icons, data = new Uint8Array(image.data);
        const palette = scenePalette(0);
        // The PNG stores editable EGA swatches; its indices select VGA HUD colors.
        for (let i = 0; i < data.length; i += 4) {
            const swatch = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
            const index = assets.palette.indexOf(swatch);
            if (index < 0) throw new Error('Unknown icon palette swatch');
            const color = palette[index];
            data[i] = color >>> 16; data[i + 1] = (color >>> 8) & 255; data[i + 2] = color & 255;
        }
        return {data, width: image.width, height: image.height};
    }

    class Overlay {
        constructor(assets) {
            this.scene = new T.Scene(); this.camera = new T.OrthographicCamera(0, 640, 0, 400, -10, 10);
            this.font = texture(assets.font, true); this.icons = texture(iconImage(assets));
            this.pool = []; this.cursor = 0;
        }
        begin() { this.cursor = 0; }
        quad(x, y, w, h, color = 0xffffff, map = null, rect = null, opacity = 1) {
            let mesh = this.pool[this.cursor];
            if (!mesh) {
                const material = new T.MeshBasicMaterial({transparent: true, depthTest: false,
                    depthWrite: false, toneMapped: false, side: T.DoubleSide});
                mesh = new T.Mesh(new T.PlaneGeometry(1, 1), material);
                mesh.frustumCulled = false; this.pool.push(mesh); this.scene.add(mesh);
            }
            const material = mesh.material;
            if (mesh.geometry.drawRange.count === 3) {
                const positions = mesh.geometry.attributes.position;
                positions.setXYZ(0,-0.5,0.5,0); positions.setXYZ(1,0.5,0.5,0);
                positions.setXYZ(2,-0.5,-0.5,0); positions.setXYZ(3,0.5,-0.5,0);
                positions.needsUpdate = true; mesh.geometry.setDrawRange(0,6);
            }
            material.color.setHex(color); material.opacity = opacity;
            if (Boolean(material.map) !== Boolean(map)) material.needsUpdate = true;
            material.map = map;
            mesh.visible = true; mesh.renderOrder = this.cursor++;
            mesh.position.set(x + w / 2, y + h / 2, 0); mesh.scale.set(w, -h, 1);
            const uv = mesh.geometry.attributes.uv;
            const r = rect || [0, 0, 1, 1];
            uv.setXY(0, r[0], r[1]); uv.setXY(1, r[0] + r[2], r[1]);
            uv.setXY(2, r[0], r[1] + r[3]); uv.setXY(3, r[0] + r[2], r[1] + r[3]); uv.needsUpdate = true;
            return mesh;
        }
        triangle(x0,y0,x1,y1,x2,y2,color) {
            const mesh = this.quad(0,0,1,1,color), geometry = mesh.geometry;
            mesh.position.set(0,0,0); mesh.scale.set(1,1,1);
            const positions = geometry.attributes.position;
            // The plane's first indexed triangle uses vertices 0, 2, 1.
            positions.setXYZ(0,x0,y0,0); positions.setXYZ(2,x1,y1,0); positions.setXYZ(1,x2,y2,0);
            positions.needsUpdate = true; geometry.setDrawRange(0,3);
        }
        crosshair(x,y,color) {
            for (const [dx,dy] of [[-1,0],[1,0],[0,-1],[0,1]]) {
                const bx = x + dx * 8, by = y + dy * 8;
                this.triangle(x+dx*2,y+dy*2,bx-dy*2,by+dx*2,bx+dy*2,by-dx*2,color);
            }
        }
        text(text, x, y, color = 0xffffff, size = 12, align = 'left') {
            text = String(text).toUpperCase();
            if (align === 'center') x -= text.length * size / 2;
            if (align === 'right') x -= text.length * size;
            for (let i = 0; i < text.length; ++i) {
                let c = text.charCodeAt(i) - 32;
                if (c < 0 || c >= 64) c = 31;
                this.quad(x + i * size, y, size, size, color, this.font,
                    [(c & 15) / 16, (c >>> 4) / 4, 1 / 16, 1 / 4]);
            }
        }
        tile(tile, x, y, size = 16) { this.quad(x, y, size, size, 0xffffff, this.icons, [tile / 10, 0, 0.1, 1]); }
        panel(x, y, w, h) {
            this.quad(x, y, w, h, 0x000013, null, null, 0.94);
            this.quad(x, y, w, 2, 0x22aaaa); this.quad(x, y + h - 2, w, 2, 0x22aaaa);
            this.quad(x, y, 2, h, 0x22aaaa); this.quad(x + w - 2, y, 2, h, 0x22aaaa);
        }
        end(renderer) {
            for (let i = this.cursor; i < this.pool.length; ++i) this.pool[i].visible = false;
            renderer.clearDepth(); renderer.render(this.scene, this.camera);
        }
    }

    class View {
        constructor(canvas, assets) {
            this.canvas = canvas; this.assets = assets;
            const context = canvas.getContext('webgl2', {antialias: false, alpha: false});
            if (!context) throw new Error('This game needs WebGL 2. Enable hardware acceleration in your browser.');
            this.renderer = new T.WebGLRenderer({canvas, context, antialias: false});
            this.renderer.outputColorSpace = T.SRGBColorSpace; this.renderer.autoClear = false;
            this.scene = new T.Scene(); this.scene.background = new T.Color(0x000055);
            this.camera = new T.PerspectiveCamera(62, 1.6, 0.015, 150);
            this.camera.rotation.order = 'YXZ';
            this.overlay = new Overlay(assets);
            this.warningPattern = new WarningPattern(); this.warningTick = -1; this.warningSight = 0;
            this.warningMap = texture({data: new Uint8Array(64 * 4 * 4), width: 64, height: 4});
            this.group = new T.Group(); this.scene.add(this.group);
            this.actors = new T.Group(); this.group.add(this.actors);
            this.lossFigure = new T.Group(); this.scene.add(this.lossFigure);
            this.ray = new T.Raycaster(); this.ray.far = 100;
            this.materials = new Map(); this.geometries = new Map(); this.objectMeshes = new Map();
            this.resourceMaps = new Map(); this.yaw = 0; this.pitch = -0.12;
            this.palettePhase = 1; this.paletteVariant = 0;
            this.pixelScatterState = 0x75843239;
            this.buildTitleFigure();
            this.game = null; this.mode = 'welcome';
            this.animationTime = 0; this.animationRunning = false; this.lastAnimationFrame = 0;
            this.resize(); window.addEventListener('resize', () => this.resize());
        }
        resize() {
            const viewport = document.getElementById('viewport');
            const scale = Math.min((viewport ? viewport.clientWidth : innerWidth) / 640,
                (viewport ? viewport.clientHeight : innerHeight) / 400);
            const width = Math.max(1, Math.floor(640 * scale)), height = Math.max(1, Math.floor(400 * scale));
            this.renderer.setPixelRatio(Math.min(devicePixelRatio || 1, 2));
            this.renderer.setSize(width, height, false);
            this.canvas.style.width = width + 'px'; this.canvas.style.height = height + 'px';
            this.camera.aspect = width / height; this.camera.updateProjectionMatrix();
            for (const [key,map] of this.resourceMaps) if (key.startsWith('title-gradient:')) {
                map.dispose(); this.resourceMaps.delete(key);
            }
        }
        palette() {
            return scenePalette(this.palettePhase, this.paletteVariant);
        }
        selectPalette(mode) {
            const phase = ['playing','preview','overview','lost'].includes(mode) ? 0 : 1;
            const variant = phase === 0 && this.game ? Math.max(0,this.game.landscape.sentryCount-1) & 7 : 0;
            if (phase === this.palettePhase && variant === this.paletteVariant) return;
            this.palettePhase = phase; this.paletteVariant = variant;
            const palette = this.palette();
            for (const material of this.materials.values()) {
                const [a,b] = material.userData.palettePair;
                material.uniforms.first.value.setHex(palette[a]);
                material.uniforms.second.value.setHex(palette[b]);
                material.uniforms.distanceShading.value = phase === 0 ? 1 : 0;
            }
        }
        backdrop(name, rows = 200) {
            const key = `${name}:${rows}:${this.palettePhase}:${this.paletteVariant}`;
            if (!this.resourceMaps.has(key)) {
                const source = this.assets.resources[name], palette = this.palette();
                const rgba = new Uint8Array(320 * rows * 4);
                for (let y = 0; y < rows; ++y) for (let x = 0; x < 320; ++x) {
                    const color = palette[source[y * 320 + x]], at = ((rows - y - 1) * 320 + x) * 4;
                    rgba[at] = color >>> 16; rgba[at + 1] = (color >>> 8) & 255; rgba[at + 2] = color & 255; rgba[at + 3] = 255;
                }
                this.resourceMaps.set(key, texture({data: rgba, width: 320, height: rows}));
            }
            return this.resourceMaps.get(key);
        }
        skyGradient() {
            const key = `sky-gradient:${this.palettePhase}:${this.paletteVariant}`;
            if (!this.resourceMaps.has(key)) {
                const palette = this.palette(), top = palette[0], horizon = palette[7];
                const data = new Uint8Array(200 * 4);
                for (let y = 0; y < 200; ++y) {
                    const amount = y / 199, at = (199 - y) * 4;
                    for (let channel = 0; channel < 3; ++channel) {
                        const shift = 16 - channel * 8;
                        data[at + channel] = Math.round(((top >>> shift) & 255) * (1 - amount) +
                            ((horizon >>> shift) & 255) * amount);
                    }
                    data[at + 3] = 255;
                }
                const map = texture({data, width: 1, height: 200});
                map.magFilter = map.minFilter = T.LinearFilter;
                this.resourceMaps.set(key,map);
            }
            return this.resourceMaps.get(key);
        }
        titleBackdrop() {
            const key = `title-gradient:${this.palettePhase}:${this.paletteVariant}`;
            if (!this.resourceMaps.has(key)) {
                const source = this.assets.resources.Tfinal, sky = this.assets.resources.Skybox;
                const foreground = new Uint8Array(source.length);
                // Both original resources share the same dithered sky. Their
                // differences identify the title artwork above the input strip.
                for (let i = 0; i < source.length; ++i)
                    foreground[i] = i < 320 * 157 ? Number(source[i] !== sky[i]) : Number(source[i] !== 0 && source[i] !== 7);
                const mask = foreground.slice();
                // Close isolated matching pixels inside the artwork so the new
                // sky cannot show through black shadows in the lettering.
                for (let y = 1; y < 199; ++y) for (let x = 1; x < 319; ++x) {
                    const at = y * 320 + x;
                    if (foreground[at]) continue;
                    let neighbors = 0;
                    for (let dy = -1; dy <= 1; ++dy) for (let dx = -1; dx <= 1; ++dx)
                        neighbors += foreground[at + dy * 320 + dx];
                    if (neighbors >= 5) mask[at] = 1;
                }
                const image = this.backdrop('Tfinal').image;
                const artwork = document.createElement('canvas'); artwork.width = 320; artwork.height = 200;
                const art = artwork.getContext('2d'), foregroundImage = art.createImageData(320,200);
                for (let y = 0; y < 200; ++y) for (let x = 0; x < 320; ++x) {
                    const pixel = y * 320 + x, at = ((199 - y) * 320 + x) * 4;
                    // Only the lettering remains raster artwork. The Sentinel,
                    // pedestal and mountain ridge are rendered separately.
                    if (y < 150 && x < 260 && mask[pixel])
                        foregroundImage.data.set(image.data.subarray(at,at + 4),pixel * 4);
                }
                art.putImageData(foregroundImage,0,0);
                const canvas = document.createElement('canvas');
                canvas.width = this.canvas.width; canvas.height = this.canvas.height;
                const context = canvas.getContext('2d'), palette = this.palette();
                const gradient = context.createLinearGradient(0,0,0,canvas.height);
                gradient.addColorStop(0,'#' + palette[0].toString(16).padStart(6,'0'));
                gradient.addColorStop(1,'#' + palette[7].toString(16).padStart(6,'0'));
                context.fillStyle = gradient; context.fillRect(0,0,canvas.width,canvas.height);
                // Rasterize the editable SVG at display resolution, beneath the
                // crisp original lettering rather than at DOS resolution.
                context.drawImage(this.assets.mountains,0,0,canvas.width,canvas.height);
                context.imageSmoothingEnabled = false;
                context.drawImage(artwork,0,0,canvas.width,canvas.height);
                const rgba = context.getImageData(0,0,canvas.width,canvas.height).data;
                const data = new Uint8Array(rgba.length), stride = canvas.width * 4;
                for (let y = 0; y < canvas.height; ++y)
                    data.set(rgba.subarray(y * stride,(y + 1) * stride),(canvas.height - 1 - y) * stride);
                this.resourceMaps.set(key,texture({data, width: canvas.width, height: canvas.height}));
            }
            return this.resourceMaps.get(key);
        }
        previewBackground() {
            if (!this.starfield) {
                const data = new Uint8Array(320 * 200 * 4);
                for (let i = 3; i < data.length; i += 4) data[i] = 255;
                // Original style 3: three 50-pixel scatter passes on black.
                // c/pixel_scatter.c advances the original separate 32-bit state
                // by multiplying by five, then uses its high word as VGA offset.
                const white = scenePalette(0)[14];
                for (let star = 0; star < 150; ++star) {
                    this.pixelScatterState = Math.imul(this.pixelScatterState, 5) >>> 0;
                    const offset = this.pixelScatterState >>> 16;
                    if (offset >= 64000) continue; // VGA memory past the visible image.
                    const at = (199 - Math.floor(offset / 320)) * 320 * 4 + (offset % 320) * 4;
                    data[at] = white >>> 16; data[at + 1] = (white >>> 8) & 255; data[at + 2] = white & 255;
                }
                this.starfield = texture({data, width: 320, height: 200});
            }
            return this.starfield;
        }
        material(a, b, terrain = false, midpoint = false) {
            const key = `${a}:${b}:${terrain}:${midpoint}`;
            if (!this.materials.has(key)) {
                const palette = this.palette();
                this.materials.set(key, new T.ShaderMaterial({
                    uniforms: {first: {value: new T.Color(palette[a & 15])}, second: {value: new T.Color(palette[b & 15])},
                        distanceShading: {value: this.palettePhase === 0 ? 1 : 0}},
                    vertexShader: `${terrain ? 'attribute vec3 contour; varying vec3 boundary;' : ''}
                        varying float range; void main(){ ${terrain ? 'boundary=contour;' : ''}
                        vec4 p=modelViewMatrix*vec4(position,1.0); range=length(p.xyz); gl_Position=projectionMatrix*p; }`,
                    fragmentShader: `${terrain ? 'varying vec3 boundary;' : ''}
                        uniform vec3 first; uniform vec3 second; uniform float distanceShading; varying float range;
                        void main(){float d=${midpoint ? '0.5' : 'mod(floor(gl_FragCoord.x)+floor(gl_FragCoord.y),2.0)'};
                        ${terrain ? 'vec3 pixels=boundary/max(fwidth(boundary),vec3(0.000001)); d=1.0-clamp(0.425+0.5-min(pixels.x,min(pixels.y,pixels.z)),0.0,1.0);' : ''}
                        vec3 c=mix(first,second,d); c*=1.0-smoothstep(14.0,65.0,range)*0.65*distanceShading;
                        gl_FragColor=vec4(c,1.0);
                        #include <colorspace_fragment>
                        }`, side: T.DoubleSide, toneMapped: false
                }));
                this.materials.get(key).userData.palettePair = [a & 15,b & 15];
                this.materials.get(key).userData.midpoint = midpoint;
            }
            return this.materials.get(key);
        }
        modelGeometry(id) {
            if (this.geometries.has(id)) return this.geometries.get(id);
            const model = this.assets.models[id], positions = [], buckets = new Map();
            model.faces.forEach((face, faceIndex) => {
                const pair = model.materials[faceIndex].palette_pair;
                const key = pair[0] * 16 + pair[1];
                if (!buckets.has(key)) buckets.set(key, []);
                const bucket = buckets.get(key);
                for (let i = 1; i + 1 < face.length; ++i) for (const v of [face[0], face[i], face[i + 1]]) {
                    const [x, y, z] = model.vertices[v]; bucket.push(x, z, -y);
                }
            });
            const geometry = new T.BufferGeometry();
            geometry.userData.palettePairs = [];
            for (const [key, bucket] of buckets) {
                geometry.addGroup(positions.length / 3, bucket.length / 3, geometry.userData.palettePairs.length);
                geometry.userData.palettePairs.push([key >>> 4, key & 15]);
                positions.push(...bucket);
            }
            geometry.setAttribute('position', new T.Float32BufferAttribute(positions, 3));
            geometry.computeBoundingSphere(); this.geometries.set(id, geometry); return geometry;
        }
        modelMesh(id) {
            const geometry = this.modelGeometry(id);
            const materials = geometry.userData.palettePairs.map(pair => this.material(pair[0], pair[1], false, true));
            return new T.Mesh(geometry, materials);
        }
        buildTitleFigure() {
            this.titleFigure = new T.Group(); this.scene.add(this.titleFigure);
            // Off-axis framing keeps the original eye-level pedestal horizon
            // and right-hand composition without distorting the model's pose.
            const distance = 5.73, pixelsPerUnit = 91.8;
            this.titleCamera = new T.PerspectiveCamera(
                2 * Math.atan(100 / (pixelsPerUnit * distance)) * 180 / Math.PI,1.6,0.015,150);
            this.titleCamera.projectionMatrix.elements[8] = (160 - 288) / 160;
            this.titleCamera.projectionMatrix.elements[9] = (156.5 - 100) / 100;
            this.titleCamera.projectionMatrixInverse.copy(this.titleCamera.projectionMatrix).invert();
            this.titleFigure.position.z = -distance;
            const palette = scenePalette(1), colors = new Map();
            const paint = index => {
                if (!colors.has(index)) colors.set(index,new T.MeshBasicMaterial({
                    color: palette[index], side: T.DoubleSide, toneMapped: false}));
                return colors.get(index);
            };
            const figure = (id, height, yaw) => {
                const model = this.assets.models[id], positions = [], geometry = new T.BufferGeometry(), materials = [];
                model.faces.forEach((face,index) => {
                    let color = model.materials[index].palette_pair[1];
                    // The illustration lights the yellow face and pink right
                    // shoulder, leaving the opposite shoulder in deep red.
                    if (id === 5) {
                        if (color === 9) color = 15;
                        if (index === 15) color = 4;
                    } else {
                        if (index === 2) color = 3;
                        if (index === 6) color = 2;
                        if (index === 7) color = 8;
                    }
                    const start = positions.length / 3;
                    for (let i = 1; i + 1 < face.length; ++i) for (const vertex of [face[0],face[i],face[i + 1]]) {
                        const [x,y,z] = model.vertices[vertex]; positions.push(x,z,-y);
                    }
                    geometry.addGroup(start,positions.length / 3 - start,materials.length);
                    materials.push(paint(color));
                });
                geometry.setAttribute('position',new T.Float32BufferAttribute(positions,3));
                const mesh = new T.Mesh(geometry,materials);
                mesh.position.y = height; mesh.rotation.y = yaw;
                this.titleFigure.add(mesh);
                return mesh;
            };
            this.titleSentinel = figure(5,0.875,2.806);
            this.titleFinalYaw = this.titleSentinel.rotation.y;
            figure(6,-0.125,-0.35);
        }
        startTitleTurn(now = performance.now()) {
            this.titleTurnStarted = now;
            this.updateTitleTurn(now);
        }
        updateTitleTurn(now) {
            if (this.titleTurnStarted === undefined) return;
            const elapsed = Math.max(0,now - this.titleTurnStarted - 200);
            const angle = 40 / 256 * TAU; // Two gameplay turns, approaching from the opposite side.
            let offset = 0;
            if (elapsed < 400) {
                offset = -angle * (1 - elapsed / 400);
            } else if (elapsed < 700) {
                const settling = (elapsed - 400) / 300;
                // Brake sharply into a small damped overshoot, then reverse
                // the correction to rest at exactly the original pose.
                offset = 0.5 * angle * 300 / (400 * TAU) * Math.exp(-2 * settling) *
                    Math.sin(TAU * settling) * (1 - settling) ** 2;
            }
            this.titleSentinel.rotation.y = this.titleFinalYaw + offset;
        }
        warningTexture(sight, tick) {
            if (tick !== this.warningTick || sight !== this.warningSight) {
                const steps = Math.max(1, Math.min(60,tick - this.warningTick));
                for (let i = 0; i < steps; ++i) this.warningPattern.advance(sight);
                this.warningTick = tick; this.warningSight = sight;
                const data = this.warningMap.image.data, palette = this.palette();
                for (let i = 0; i < this.warningPattern.pixels.length; ++i) {
                    const color = palette[this.warningPattern.pixels[i]], at = i * 4;
                    data[at] = color >>> 16; data[at + 1] = (color >>> 8) & 255;
                    data[at + 2] = color & 255; data[at + 3] = 255;
                }
                this.warningMap.needsUpdate = true;
            }
            return this.warningMap;
        }
        setGame(game) {
            this.animationTime = 0; this.animationRunning = false;
            this.warningPattern.advance(0); this.warningTick = -1; this.warningSight = 0;
            if (this.starfield) { this.starfield.dispose(); this.starfield = null; }
            if (this.terrain) { this.group.remove(this.terrain); this.terrain.geometry.dispose(); }
            this.actors.clear(); this.objectMeshes.clear(); this.game = game;
            const world = game.landscape, positions = [], contours = [], faceCells = [], buckets = new Map();
            const pairs = [0x33,0x10,0x10,0x12,0x12,0x12,0x10,0x12,0x11,0x10,0x12,0x10,0x10,0x12,0x12,0x10,
                0x11,0x11,0x12,0x10,0x10,0x11,0x12,0x10,0x11,0x11,0x10,0x12,0x11,0x10,0x12,0x12];
            const geometry = new T.BufferGeometry(), materials = [];
            for (let y = 0; y < 31; ++y) for (let x = 0; x < 31; ++x) {
                const points = [[x,world.height(x,y),-y], [x+1,world.height(x+1,y),-y],
                    [x,world.height(x,y+1),-y-1], [x+1,world.height(x+1,y+1),-y-1]];
                const shape = world.shape(x, y), diagonal = [6,7,14,15].includes(shape);
                const triangles = diagonal ? [[0,2,1],[1,2,3]] : [[0,3,1],[0,2,3]];
                for (let t = 0; t < 2; ++t) {
                    // Shapes 1/5/9/13 are single sloping quads in the original:
                    // our two WebGL triangles must share their polygon color.
                    // Only split cells use the second color bank at +16.
                    const bank = (shape & 3) === 1 ? 0 : t * 16;
                    let pair = pairs[shape ? shape + bank : ((x ^ y) & 1 ? 8 : 0)];
                    // Shape 12's second bank contains the flat-floor selector;
                    // both of this irregular cell's triangles are sloping.
                    if (shape === 12) pair = pairs[12];
                    if (!buckets.has(pair)) buckets.set(pair, {positions: [], contours: [], cells: []});
                    const bucket = buckets.get(pair);
                    const quad = shape === 0 || (shape & 3) === 1;
                    const hiddenEdge = diagonal ? (t === 0 ? 0 : 2) : (t === 0 ? 2 : 1);
                    for (let vertex = 0; vertex < 3; ++vertex) {
                        bucket.positions.push(...points[triangles[t][vertex]]);
                        for (let edge = 0; edge < 3; ++edge)
                            bucket.contours.push((quad && edge === hiddenEdge) || edge === vertex ? 1 : 0);
                    }
                    bucket.cells.push({x, y});
                }
            }
            for (const [pair, bucket] of buckets) {
                geometry.addGroup(positions.length / 3, bucket.positions.length / 3, materials.length);
                // VGA's high nibble colors the contour; its low nibble fills
                // the polygon. The shader measures contour width in screen pixels.
                materials.push(this.material(pair >>> 4, pair & 15, true));
                positions.push(...bucket.positions); contours.push(...bucket.contours); faceCells.push(...bucket.cells);
            }
            geometry.setAttribute('position', new T.Float32BufferAttribute(positions, 3));
            geometry.setAttribute('contour', new T.Float32BufferAttribute(contours, 3));
            geometry.computeBoundingSphere(); this.terrain = new T.Mesh(geometry, materials);
            this.terrain.userData.cells = faceCells; this.group.add(this.terrain);
            this.syncObjects();
            this.alignWithPlayer();
            this.syncRevision = -1;
        }
        alignWithPlayer() {
            this.yaw = this.game.player.heading / 256 * TAU;
            this.pitch = -0.12;
        }
        syncObjects() {
            if (!this.game) return;
            const world = this.game.landscape;
            for (const [id, mesh] of this.objectMeshes) if (!world.objects[id] || world.objects[id].model !== mesh.userData.model) {
                this.actors.remove(mesh); this.objectMeshes.delete(id);
            }
            for (const o of world.objects) {
                if (!o) continue;
                const yaw = -o.heading / 256 * TAU;
                let mesh = this.objectMeshes.get(o.id);
                if (!mesh) {
                    mesh = this.modelMesh(o.model); mesh.userData = {id: o.id, model: o.model, heading: o.heading};
                    mesh.rotation.y = yaw;
                    this.objectMeshes.set(o.id, mesh); this.actors.add(mesh);
                }
                mesh.position.set(o.x + 0.5, o.z, -o.y - 0.5);
                if (o.heading !== mesh.userData.heading && [S.World.MODEL.SENTRY,S.World.MODEL.SENTINEL].includes(o.model)) {
                    const from = mesh.rotation.y;
                    mesh.userData.turn = {from, delta: Math.atan2(Math.sin(yaw-from),Math.cos(yaw-from)), started: this.animationTime};
                }
                mesh.userData.heading = o.heading;
                const turn = mesh.userData.turn;
                if (turn) {
                    const progress = Math.min(1,(this.animationTime-turn.started)/SENTRY_TURN_MS);
                    mesh.rotation.y = turn.from + turn.delta * progress;
                    if (progress === 1) delete mesh.userData.turn;
                } else mesh.rotation.y = yaw;
                mesh.visible = this.mode !== 'playing' || o.id !== this.game.playerId;
            }
            this.syncRevision = this.game.revision;
        }
        look(dx, dy) {
            this.yaw += dx * 0.0025;
            this.pitch = Math.max(-Math.PI * 0.495, Math.min(Math.PI * 0.495, this.pitch - dy * 0.0025));
            if (this.game && this.game.player) this.game.player.heading = Math.round(this.yaw / TAU * 256) & 255;
        }
        viewpoint(mode, now) {
            this.mode = mode;
            this.selectPalette(mode);
            this.group.visible = ['playing','preview','overview'].includes(mode);
            this.lossFigure.visible = mode === 'lost';
            this.titleFigure.visible = mode === 'welcome';
            if (mode === 'lost') {
                const model = this.game.lossModel ?? S.World.MODEL.SENTINEL;
                if (this.lossFigure.userData.model !== model) {
                    this.lossFigure.clear();
                    const figure = this.modelMesh(model); figure.rotation.y = Math.PI;
                    this.lossFigure.add(figure); this.lossFigure.userData.model = model;
                }
                this.camera.position.set(0,-0.05,2.15); this.camera.lookAt(0,-0.32,0);
            } else if (this.group.visible && this.game) {
                this.syncObjects();
                if (mode === 'playing') {
                    const p = this.game.player;
                    this.camera.position.set(p.x + 0.5, p.z, -p.y - 0.5);
                    this.camera.rotation.set(this.pitch, -this.yaw, 0, 'YXZ');
                } else {
                    const angle = mode === 'overview' ? 0.4 : now * 0.00007;
                    this.camera.position.set(15.5 + Math.sin(angle) * 27, 29, -15.5 + Math.cos(angle) * 27);
                    this.camera.lookAt(15.5, 4, -15.5);
                }
            } else {
                this.camera.position.set(0, 18, 23); this.camera.lookAt(0, 0, 0);
            }
            this.scene.background = ['preview','lost'].includes(mode) ? this.previewBackground()
                : mode === 'welcome' ? this.titleBackdrop()
                : this.skyGradient();
        }
        pick() {
            if (!this.game || !this.terrain) return null;
            if (this.syncRevision !== this.game.revision) this.syncObjects();
            this.scene.updateMatrixWorld(true); this.camera.updateMatrixWorld(true);
            this.ray.setFromCamera(new T.Vector2(0, 0), this.camera);
            // Ordinary models are not selectable surfaces. Follow the actual
            // crosshair ray through them to terrain, boulders or pedestal.
            const meshes = [this.terrain, ...Array.from(this.objectMeshes.values()).filter(mesh =>
                mesh.visible && [S.World.MODEL.BOULDER,S.World.MODEL.PEDESTAL].includes(mesh.userData.model))];
            for (const hit of this.ray.intersectObjects(meshes,false)) {
                const object = hit.object === this.terrain ? null : this.game.landscape.objects[hit.object.userData.id];
                if (hit.object !== this.terrain && !object) continue;
                const {x,y} = object || this.terrain.userData.cells[hit.faceIndex];
                if (x === this.game.player.x && y === this.game.player.y) continue;
                const head = this.game.landscape.head(x,y);
                return {x,y,id: head ? head.id : -1,distance: hit.distance,
                    kind: !object ? 'ground' : object.model === S.World.MODEL.BOULDER ? 'boulder' : 'pedestal',
                    supportId: object ? object.id : null, downward: this.ray.ray.direction.y < 0,
                    point: {x: hit.point.x,y: -hit.point.z,z: hit.point.y}};
            }
            return null;
        }
        draw(app, now) {
            const mode = app.screen;
            const running = mode === 'playing' && !app.paused && !document.hidden;
            if (running && this.animationRunning) this.animationTime += Math.max(0,now-this.lastAnimationFrame);
            this.animationRunning = running; this.lastAnimationFrame = now;
            this.viewpoint(mode, now);
            if (this.titleFigure.visible) this.updateTitleTurn(now);
            this.renderer.clear(); this.renderer.render(this.scene, this.titleFigure.visible ? this.titleCamera : this.camera);
            const h = this.overlay; h.begin();
            if (mode === 'welcome') {
                h.text('THE SENTINEL REMASTER',320,352,this.palette()[2],14,'center');
                h.text('CLICK OR PRESS ENTER TO BEGIN',320,379,0xffffff,10,'center');
            } else if (mode === 'preview') {
                h.panel(0,0,640,39); h.text('LANDSCAPE '+String(this.game.number).padStart(4,'0'),16,13,0xffffff,12);
                h.text(this.game.landscape.sentryCount === 1 ? 'ONE SENTINEL' : 'SENTINEL + '+(this.game.landscape.sentryCount-1)+' SENTRIES',624,13,0x55ffff,10,'right');
                h.panel(36,326,568,55); h.text('CLICK TO ENTER YOUR ROBOT',320,337,0xffff55,13,'center');
                h.text('MOUSE LOOK - LEFT ABSORB - RIGHT TRANSFER - T/B/R CREATE',320,361,0xffffff,8,'center');
            } else if (['playing','overview'].includes(mode)) {
                h.quad(0,0,640,32,0x000000);
                let energy = this.game.energy, x = 8;
                while (energy >= 15) { h.tile(6,x,8); x += 24; energy -= 15; }
                while (energy >= 3) { h.tile(1,x,8); x += 24; energy -= 3; }
                if (energy) { h.tile(energy*2,x,8); h.tile(energy*2+1,x+16,8); }
                h.text(String(this.game.energy),294,10,0xffff55,12,'right');
                h.text('SEEN',418,12,0xaaaaaa,9); h.quad(464,8,166,16,0x444444); h.quad(466,10,162,12,0x111111);
                h.quad(466,10,162,12,0xffffff,this.warningTexture(this.game.warning,this.game.ticks));
                h.quad(0,375,640,25,0x000000); h.text('T TREE  B BOULDER  R ROBOT  H HYPERSPACE',12,382,0xaaaaaa,9);
                h.text('TAB MAP / ESC PAUSE',628,382,0x55aaaa,8,'right');
                if (mode === 'playing') {
                    app.target = this.pick();
                    const target = app.target, object = target && this.game.landscape.head(target.x,target.y);
                    const valid = this.game.canUseContact(target);
                    const color = valid ? 0x55ffff : 0xffffff;
                    h.crosshair(320,200,color);
                    if (target) {
                        const names = ['ROBOT','SENTRY','TREE','BOULDER','MEANIE','SENTINEL','PEDESTAL'];
                        const name = object ? names[object.model] : this.game.landscape.flat(target.x,target.y) ? 'EMPTY SQUARE' : 'SLOPE';
                        h.text(name,320,345,valid?0x55ffff:0xaaaaaa,10,'center');
                    }
                }
                if (mode === 'overview') { h.panel(160,335,320,30); h.text('TAB TO RETURN',320,344,0xffff55,11,'center'); }
            } else if (mode === 'lost') {
                const elapsed = Math.max(0,now - (app.lossStarted ?? now));
                const progress = Math.max(0,Math.min(1,(elapsed - 1000) / 2000));
                const fade = progress * progress * (3 - 2 * progress);
                if (fade > 0) h.quad(0,0,640,400,0x000000,null,null,fade);
                h.text(this.game.lossReason || 'YOUR ROBOT HAS BEEN ABSORBED',320,330,0xdddddd,12,'center');
                h.text('CLICK OR ENTER TO RETRY - ESC FOR TITLE',320,375,0xffffff,10,'center');
            }
            if (app.message && now < app.messageUntil) {
                h.quad(15,45,610,30,0x000000,null,null,0.8); h.text(app.message,320,54,0xffff55,Math.min(11,590/app.message.length),'center');
            }
            if (app.paused) {
                h.quad(0,0,640,400,0x000000,null,null,0.65); h.panel(65,75,510,262);
                h.text('PAUSED',320,96,0x55ffff,23,'center');
                ['MOUSE: LOOK AROUND','LEFT CLICK: ABSORB','RIGHT CLICK: TRANSFER','T / B / R: TREE / BOULDER / ROBOT','H: HYPERSPACE   TAB: OVERVIEW',
                    'M: SOUND '+(app.sound.muted?'OFF':'ON')].forEach((s,i)=>h.text(s,320,143+i*22,0xffffff,10,'center'));
                h.text('CLICK: RESUME   ENTER: RESTART   ESC: TITLE',320,312,0xffff55,10,'center');
            }
            h.end(this.renderer);
        }
    }
    S.View = View;
    S.Palettes = {scene: scenePalette, dacColor};
    S.WarningPattern = WarningPattern;
}(window.Sentinel));
