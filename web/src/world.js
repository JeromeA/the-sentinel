window.Sentinel = window.Sentinel || {};
(function (S) {
    'use strict';
    const MODEL = Object.freeze({ROBOT: 0, SENTRY: 1, TREE: 2, BOULDER: 3,
        MEANIE: 4, SENTINEL: 5, PEDESTAL: 6});
    const ENERGY = [3, 3, 1, 2, 1, 4, 0];
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
        if (a === b && b === c && c === d) return 0;
        if (a === b && b === c) return a < d ? 10 : 3;
        if (b === c && c === d) return d < a ? 11 : 2;
        if (a === b && b === d) return d < c ? 6 : 15;
        if (a === c && c === d) return d < b ? 14 : 7;
        if (a === b && c === d) return d < b ? 1 : 9;
        if (a === c && b === d) return d < c ? 5 : 13;
        return d === b || a === c ? 4 : 12;
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
        flat(x, y) { return this.inside(x, y) && this.shape(x, y) === 0; }
        filter(mode, fixed, work) {
            const vertical = !!(mode & 128);
            for (let i = 34; i >= 0; --i) {
                const c = i & 31;
                work[i] = this.height(vertical ? fixed : c, vertical ? c : fixed);
            }
            if (mode & 64) {
                for (let i = 31; i >= 0; --i) {
                    const a = work[i], b = work[i + 1], c = work[i + 2];
                    if (b < a && b < c) work[i + 1] = Math.min(a, c);
                    else if (b > a && b > c) work[i + 1] = Math.max(a, c);
                }
            } else {
                for (let i = 0; i < 32; ++i) work[i] = (work[i] + work[i + 1] + work[i + 2] + work[i + 3]) >>> 2;
            }
            for (let i = 31; i >= 0; --i) {
                const x = vertical ? fixed : i, y = vertical ? i : fixed;
                this.heights[y * 32 + x] = work[i];
            }
        }
        smooth(mode, work) {
            for (let pass = 0; pass < 2; ++pass) {
                for (let y = 31; y >= 0; --y) this.filter(mode, y, work);
                for (let x = 31; x >= 0; --x) this.filter(mode | 128, x, work);
            }
        }
        generate() {
            // The original seeded unused scratch bytes here. Their values are
            // overwritten by filtering, but the 81 PRNG advances remain essential.
            for (let i = 0; i < 81; ++i) this.random.next();
            const work = new Uint8Array(35);
            const scale = this.number === 0 ? 24 : this.random.sum() + 14;
            for (let y = 31; y >= 0; --y) for (let x = 31; x >= 0; --x) this.heights[y * 32 + x] = this.random.next();
            this.smooth(0, work);
            for (let y = 31; y >= 0; --y) for (let x = 31; x >= 0; --x) {
                let height = byte(Math.floor((this.height(x, y) - 128) * scale / 256) + 6);
                if (height & 128) height = 0;
                this.heights[y * 32 + x] = Math.min(height + 1, 11);
            }
            this.smooth(64, work);
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
            const regions = new Uint8Array(64), xs = new Uint8Array(64), ys = new Uint8Array(64);
            let threshold = 0;
            for (let region = 0; region < 64; ++region) {
                const x0 = (region & 7) * 4, y0 = (region >>> 3) * 4;
                let h = 0;
                for (let y = y0; y < Math.min(y0 + 4, 31); ++y) for (let x = x0; x < Math.min(x0 + 4, 31); ++x) {
                    const height = this.height(x, y) << 4;
                    if (!this.flat(x, y) || height < h) continue;
                    h = height; xs[region] = x; ys[region] = y;
                }
                regions[region] = h; threshold = Math.max(threshold, h);
            }
            for (let slot = 0; slot < this.sentryCount; ++slot) {
                let candidates;
                do {
                    candidates = [];
                    for (let r = 63; r >= 0; --r) if (regions[r] === threshold) candidates.push(r);
                    if (!candidates.length) threshold -= 16;
                } while (!candidates.length && threshold >= 0);
                if (!candidates.length) throw new Error('No Sentinel placement');
                const mask = (1 << Math.ceil(Math.log2(candidates.length + 1))) - 1;
                let choice;
                do { choice = this.random.next() & mask; } while (choice >= candidates.length);
                const region = candidates[choice];
                for (const d of [-9, -8, -7, -1, 0, 1, 7, 8, 9]) if (region + d >= 0 && region + d < 64) regions[region + d] = 0;
                if (!slot) {
                    this.sentinelSquare = {x: xs[region], y: ys[region]};
                    const pedestal = this.allocate(MODEL.PEDESTAL);
                    this.attach(pedestal, xs[region], ys[region]); pedestal.heading = 0;
                }
                const sentry = this.allocate(slot ? MODEL.SENTRY : MODEL.SENTINEL, slot);
                this.attach(sentry, xs[region], ys[region]);
                const random = this.random.next();
                sentry.scanDelay = ((random >>> 1) & 63) | 5;
                sentry.turnStep = random & 1 ? -20 : 20;
                sentry.turnDelay = 0;
            }
            this.heightLimit = threshold >>> 4;
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
            if ([6, 7, 14, 15].includes(this.shape(ix, iy))) return u + v < 1 ? a + (b - a) * u + (c - a) * v : d + (c - d) * (1 - u) + (b - d) * (1 - v);
            return u > v ? a + (b - a) * u + (d - b) * v : a + (d - c) * u + (c - a) * v;
        }
    }
    const cache = new Map();
    function secret(number) {
        if (!cache.has(number)) cache.set(number, new Landscape(number).code);
        return cache.get(number);
    }
    S.World = {MODEL, ENERGY, Random, Landscape, classify, secret, packBCD};
}(window.Sentinel));
