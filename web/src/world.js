window.Sentinel = window.Sentinel || {};
(function (S) {
    'use strict';
    const MODEL = Object.freeze({ROBOT: 0, SENTRY: 1, TREE: 2, BOULDER: 3,
        MEANIE: 4, SENTINEL: 5, PEDESTAL: 6});
    const MODEL_INFO = Object.freeze({
        [MODEL.ROBOT]: {energy: 3, sightTop: 0.12, label: 'ROBOT'},
        [MODEL.SENTRY]: {energy: 3, sightTop: 0.25, label: 'SENTRY'},
        [MODEL.TREE]: {energy: 1, sightTop: 0.05, label: 'TREE'},
        [MODEL.BOULDER]: {energy: 2, sightTop: -0.375, label: 'BOULDER'},
        [MODEL.MEANIE]: {energy: 1, sightTop: 0.1, label: 'MEANIE'},
        [MODEL.SENTINEL]: {energy: 4, sightTop: 0.25, label: 'SENTINEL'},
        [MODEL.PEDESTAL]: {energy: 0, sightTop: 0.125, label: 'PEDESTAL'}
    });
    // Cell corners are A=(x,y), B=(x+1,y), C=(x,y+1), D=(x+1,y+1).
    const TERRAIN_SHAPE = Object.freeze({
        FLAT: 0, AB_ABOVE_CD: 1, A_BELOW_OTHERS: 2, D_BELOW_OTHERS: 3,
        EQUAL_DIAGONAL: 4, AC_ABOVE_BD: 5, C_ABOVE_OTHERS: 6, B_BELOW_OTHERS: 7,
        RESERVED_8: 8, CD_ABOVE_AB: 9, D_ABOVE_OTHERS: 10, A_ABOVE_OTHERS: 11,
        UNEQUAL_DIAGONALS: 12, BD_ABOVE_AC: 13, B_ABOVE_OTHERS: 14, C_BELOW_OTHERS: 15
    });
    const splitAD = {diagonal: 'AD', triangles: [[0,3,1],[0,2,3]], interiorEdges: [2,1], singleQuad: false};
    const quadAD = {...splitAD, singleQuad: true};
    const splitBC = {diagonal: 'BC', triangles: [[0,2,1],[1,2,3]], interiorEdges: [0,2], singleQuad: false};
    const TERRAIN_TOPOLOGY = Object.freeze({
        [TERRAIN_SHAPE.FLAT]: quadAD,
        [TERRAIN_SHAPE.AB_ABOVE_CD]: quadAD,
        [TERRAIN_SHAPE.A_BELOW_OTHERS]: splitAD,
        [TERRAIN_SHAPE.D_BELOW_OTHERS]: splitAD,
        [TERRAIN_SHAPE.EQUAL_DIAGONAL]: splitAD,
        [TERRAIN_SHAPE.AC_ABOVE_BD]: quadAD,
        [TERRAIN_SHAPE.C_ABOVE_OTHERS]: splitBC,
        [TERRAIN_SHAPE.B_BELOW_OTHERS]: splitBC,
        [TERRAIN_SHAPE.RESERVED_8]: splitAD,
        [TERRAIN_SHAPE.CD_ABOVE_AB]: quadAD,
        [TERRAIN_SHAPE.D_ABOVE_OTHERS]: splitAD,
        [TERRAIN_SHAPE.A_ABOVE_OTHERS]: splitAD,
        [TERRAIN_SHAPE.UNEQUAL_DIAGONALS]: splitAD,
        [TERRAIN_SHAPE.BD_ABOVE_AC]: quadAD,
        [TERRAIN_SHAPE.B_ABOVE_OTHERS]: splitBC,
        [TERRAIN_SHAPE.C_BELOW_OTHERS]: splitBC
    });
    const byte = n => n & 255;
    const packBCD = n => parseInt(String(n).padStart(4, '0'), 16);

    class Random {
        constructor(low = 0, middle = 1, high = 0) { this.low = low; this.middle = middle; this.high = high; }
        next() {
            let {low, middle, high} = this;
            for (let bit = 0; bit < 8; ++bit) {
                const feedback = ((middle >>> 3) ^ (high >>> 8)) & 1;
                high = ((high << 1) | (middle >>> 7)) & 65535;
                middle = ((middle << 1) | (low >>> 15)) & 255;
                low = ((low << 1) | feedback) & 65535;
            }
            Object.assign(this, {low, middle, high});
            return high >>> 8;
        }
        pair() {
            let value;
            for (let i = 0; i < 4; ++i) value = this.next();
            let low = value & 15;
            if (low >= 10) low -= 6;
            let high = this.next() & 240;
            if (high >= 160) high -= 96;
            return high | low;
        }
        sum() { const n = this.next(); return (n & 7) + ((n >>> 3) & 15); }
        coordinate() { let n; do { n = this.next() & 31; } while (n === 31); return n; }
        sentries(highPair) {
            const base = (highPair >>> 4) + 2;
            let count;
            do {
                const sample = this.next(); let bits = sample & 127, magnitude = 0;
                if (!bits) magnitude = 7;
                else while (!(bits & 64)) { ++magnitude; bits <<= 1; }
                count = byte(base + (sample & 128 ? -1 - magnitude : magnitude));
            } while (count >= 8);
            return count + 1;
        }
        clone() { return new Random(this.low, this.middle, this.high); }
    }

    function classify(a, b, c, d) {
        if (a === b && b === c && c === d) return TERRAIN_SHAPE.FLAT;
        if (a === b && b === c) return a < d ? TERRAIN_SHAPE.D_ABOVE_OTHERS : TERRAIN_SHAPE.D_BELOW_OTHERS;
        if (b === c && c === d) return d < a ? TERRAIN_SHAPE.A_ABOVE_OTHERS : TERRAIN_SHAPE.A_BELOW_OTHERS;
        if (a === b && b === d) return d < c ? TERRAIN_SHAPE.C_ABOVE_OTHERS : TERRAIN_SHAPE.C_BELOW_OTHERS;
        if (a === c && c === d) return d < b ? TERRAIN_SHAPE.B_ABOVE_OTHERS : TERRAIN_SHAPE.B_BELOW_OTHERS;
        if (a === b && c === d) return d < b ? TERRAIN_SHAPE.AB_ABOVE_CD : TERRAIN_SHAPE.CD_ABOVE_AB;
        if (a === c && b === d) return d < c ? TERRAIN_SHAPE.AC_ABOVE_BD : TERRAIN_SHAPE.BD_ABOVE_AC;
        return d === b || a === c ? TERRAIN_SHAPE.EQUAL_DIAGONAL : TERRAIN_SHAPE.UNEQUAL_DIAGONALS;
    }

    function averageFourHeights(line) {
        for (let i = 0; i < 32; ++i) line[i] = (line[i] + line[i + 1] + line[i + 2] + line[i + 3]) >>> 2;
    }

    function clampHeightExtrema(line) {
        // Reverse traversal intentionally feeds each changed sample into the
        // next comparison, preserving the original terrain-generation order.
        for (let i = 31; i >= 0; --i) {
            const before = line[i], height = line[i + 1], after = line[i + 2];
            if (height < before && height < after) line[i + 1] = Math.min(before,after);
            else if (height > before && height > after) line[i + 1] = Math.max(before,after);
        }
    }

    class Landscape {
        constructor(number) {
            if (!Number.isInteger(number) || number < 0 || number > 9999) throw new Error('Landscape must be 0000–9999');
            this.number = number;
            this.random = new Random(packBCD(number));
            this.heights = new Uint8Array(1024);
            this.shapes = new Uint8Array(31 * 31);
            this.objects = Array(64).fill(null);
            this.heads = new Int16Array(31 * 31).fill(-1);
            this.sentryCount = 1;
            this.generate();
            this.placePrincipals();
            this.populate();
            /* Completion population consumes 38 pairs; the next four are the
             * eight digits compared by ordinary generation at reference 4..1. */
            const codeRandom = this.random.clone();
            for (let i = 0; i < 38; ++i) codeRandom.pair();
            this.code = Array.from({length: 4}, () => codeRandom.pair().toString(16).padStart(2, '0')).join('');
            for (let i = 0; i < 43; ++i) this.random.pair();
        }
        height(x, y) { return this.heights[(y & 31) * 32 + (x & 31)]; }
        shape(x, y) { return this.shapes[y * 31 + x]; }
        inside(x, y) { return x >= 0 && y >= 0 && x < 31 && y < 31; }
        head(x, y) { return this.inside(x, y) ? this.objects[this.heads[y * 31 + x]] || null : null; }
        flat(x, y) { return this.inside(x, y) && this.shape(x, y) === TERRAIN_SHAPE.FLAT; }
        filterLine(start, stride, transform, work) {
            for (let i = 34; i >= 0; --i) {
                work[i] = this.heights[start + (i & 31) * stride];
            }
            transform(work);
            for (let i = 31; i >= 0; --i) {
                this.heights[start + i * stride] = work[i];
            }
        }
        filterRowsAndColumns(transform, work) {
            for (let pass = 0; pass < 2; ++pass) {
                for (let row = 31; row >= 0; --row) this.filterLine(row * 32,1,transform,work);
                for (let column = 31; column >= 0; --column) this.filterLine(column,32,transform,work);
            }
        }
        generate() {
            // The original seeded unused scratch bytes here. Their values are
            // overwritten by filtering, but the 81 PRNG advances remain essential.
            for (let i = 0; i < 81; ++i) this.random.next();
            const work = new Uint8Array(35);
            const scale = this.number === 0 ? 24 : this.random.sum() + 14;
            for (let y = 31; y >= 0; --y) for (let x = 31; x >= 0; --x) this.heights[y * 32 + x] = this.random.next();
            this.filterRowsAndColumns(averageFourHeights, work);
            for (let y = 31; y >= 0; --y) for (let x = 31; x >= 0; --x) {
                let height = byte(Math.floor((this.height(x, y) - 128) * scale / 256) + 6);
                if (height & 128) height = 0;
                this.heights[y * 32 + x] = Math.min(height + 1, 11);
            }
            this.filterRowsAndColumns(clampHeightExtrema, work);
            for (let y = 30; y >= 0; --y) for (let x = 30; x >= 0; --x) {
                this.shapes[y * 31 + x] = classify(this.height(x, y), this.height(x + 1, y), this.height(x, y + 1), this.height(x + 1, y + 1));
            }
        }
        allocate(model, id) {
            if (id === undefined) { id = 63; while (id >= 0 && this.objects[id]) --id; }
            if (id < 0 || id > 63 || this.objects[id]) return null;
            const object = {id, model, x: 0, y: 0, z: 0, support: -1, heading: 0,
                scanDelay: 0, turnDelay: 0, absorbDelay: 0, target: -1, sight: 0, replant: 0, meanie: -1};
            this.objects[id] = object;
            return object;
        }
        attach(object, x, y) {
            const support = this.head(x, y);
            if (!this.flat(x, y) || (support && ![MODEL.BOULDER, MODEL.PEDESTAL].includes(support.model))) return false;
            object.x = x; object.y = y; object.support = support ? support.id : -1;
            object.z = support ? support.z + (support.model === MODEL.BOULDER ? 0.5 : 1) : this.height(x, y) + 224 / 256;
            object.heading = byte((this.random.next() & 248) + 96);
            this.heads[y * 31 + x] = object.id;
            return true;
        }
        detach(object) {
            if (this.heads[object.y * 31 + object.x] !== object.id) return false;
            this.heads[object.y * 31 + object.x] = object.support;
            this.objects[object.id] = null;
            return true;
        }
        placeFlat(object, limit) {
            let retry = 0;
            for (let attempts = 0; attempts < 40000; ++attempts) {
                retry = byte(retry - 1);
                if (!retry && ++limit >= 12) return false;
                const x = this.random.coordinate(), y = this.random.coordinate();
                if (this.flat(x, y) && !this.head(x, y) && this.height(x, y) < limit) return this.attach(object, x, y);
            }
            throw new Error('Landscape placement did not terminate');
        }
        placePrincipals() {
            const difficulty = this.number >= 100 ? 8 : Math.min(8, Math.floor(this.number / 10) + 1);
            this.sentryCount = this.number ? Math.min(this.random.sentries(packBCD(this.number) >>> 8), difficulty) : 1;
            const regions = [];
            let threshold = 0;
            for (let region = 0; region < 64; ++region) {
                const x0 = (region & 7) * 4, y0 = (region >>> 3) * 4;
                const best = {x: 0, y: 0, height: 0};
                for (let y = y0; y < Math.min(y0 + 4, 31); ++y) for (let x = x0; x < Math.min(x0 + 4, 31); ++x) {
                    const height = this.height(x, y);
                    if (!this.flat(x, y) || height < best.height) continue;
                    best.x = x; best.y = y; best.height = height;
                }
                regions.push(best); threshold = Math.max(threshold, best.height);
            }
            for (let slot = 0; slot < this.sentryCount; ++slot) {
                let candidates;
                do {
                    candidates = [];
                    for (let r = 63; r >= 0; --r) if (regions[r].height === threshold) candidates.push(r);
                    if (!candidates.length) --threshold;
                } while (!candidates.length && threshold >= 0);
                if (!candidates.length) throw new Error('No Sentinel placement');
                const mask = (1 << Math.ceil(Math.log2(candidates.length + 1))) - 1;
                let choice;
                do { choice = this.random.next() & mask; } while (choice >= candidates.length);
                const region = candidates[choice], site = regions[region];
                // Preserve the original linear neighbor offsets, including
                // their overlap across the left/right edges of a region row.
                for (const d of [-9, -8, -7, -1, 0, 1, 7, 8, 9]) if (region + d >= 0 && region + d < 64) regions[region + d].height = 0;
                if (!slot) {
                    this.sentinelSquare = {x: site.x, y: site.y};
                    const pedestal = this.allocate(MODEL.PEDESTAL);
                    this.attach(pedestal, site.x, site.y); pedestal.heading = 0;
                }
                const sentry = this.allocate(slot ? MODEL.SENTRY : MODEL.SENTINEL, slot);
                this.attach(sentry, site.x, site.y);
                const random = this.random.next();
                sentry.scanDelay = ((random >>> 1) & 63) | 5;
                sentry.turnStep = random & 1 ? -20 : 20;
                sentry.turnDelay = 0;
            }
            this.heightLimit = threshold;
        }
        populate() {
            const robot = this.allocate(MODEL.ROBOT);
            this.playerId = robot.id;
            if (!this.number) this.attach(robot, 8, 17);
            else if (!this.placeFlat(robot, Math.min(this.heightLimit, 6))) throw new Error('No robot placement');
            const trees = Math.min(this.random.sum() + 10, 48 - this.sentryCount * 3);
            for (let i = 0; i < trees; ++i) {
                const tree = this.allocate(MODEL.TREE);
                if (!tree) break;
                if (!this.placeFlat(tree, this.heightLimit)) { this.objects[tree.id] = null; break; }
            }
        }
        /* The triangle split follows terrain_triangle_orientations. */
        surface(x, y) {
            const ix = Math.floor(x), iy = Math.floor(y);
            if (!this.inside(ix, iy)) return -Infinity;
            const u = x - ix, v = y - iy;
            const a = this.height(ix, iy), b = this.height(ix + 1, iy), c = this.height(ix, iy + 1), d = this.height(ix + 1, iy + 1);
            if (TERRAIN_TOPOLOGY[this.shape(ix, iy)].diagonal === 'BC') return u + v < 1 ? a + (b - a) * u + (c - a) * v : d + (c - d) * (1 - u) + (b - d) * (1 - v);
            return u > v ? a + (b - a) * u + (d - b) * v : a + (d - c) * u + (c - a) * v;
        }
    }
    const cache = new Map();
    function secret(number) {
        if (!cache.has(number)) cache.set(number, new Landscape(number).code);
        return cache.get(number);
    }
    S.World = {MODEL, MODEL_INFO, TERRAIN_SHAPE, TERRAIN_TOPOLOGY, Random, Landscape, classify, secret, packBCD};
}(window.Sentinel));
