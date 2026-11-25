/* Unlocked landscapes and browser-local progress; no network or server. */
(function (S) {
    'use strict';
    const STORAGE_KEY = 'sentinel.known-levels.v1';
    class KnownLevels {
        constructor(storage) {
            this.codes = new Map([[0,S.World.secret(0)]]);
            this.results = new Map();
            try {
                this.storage = storage === undefined ? window.localStorage : storage;
                const raw = this.storage?.getItem(STORAGE_KEY);
                const saved = raw == null ? null : JSON.parse(raw);
                if (Array.isArray(saved)) {
                    this.codes = new Map([[0,S.World.secret(0)]]);
                    for (const entry of saved) {
                        if (!Array.isArray(entry) || !this.valid(entry[0],entry[1])) continue;
                        this.codes.set(entry[0],entry[1]);
                        if (entry[2] === 'won' || entry[2] === 'lost') this.results.set(entry[0],entry[2]);
                    }
                }
            } catch (_) { /* Storage is optional for locally opened pages. */ }
        }
        valid(number, code) {
            return Number.isInteger(number) && number >= 0 && number <= 9999 &&
                typeof code === 'string' && /^[0-9]{8}$/.test(code) && S.World.secret(number) === code;
        }
        remember(number, code) {
            if (this.codes.get(number) === code || !this.valid(number,code)) return false;
            this.codes.set(number,code);
            this.save();
            return true;
        }
        reset() {
            this.codes = new Map([[0,S.World.secret(0)]]);
            this.results.clear();
            this.save();
        }
        recordResult(number, result) {
            if (!this.codes.has(number) || this.results.get(number) === result) return false;
            this.results.set(number,result);
            this.save();
            return true;
        }
        save() {
            const entries = [...this.codes].map(([number,code]) => this.results.has(number)
                ? [number,code,this.results.get(number)] : [number,code]);
            try { this.storage?.setItem(STORAGE_KEY,JSON.stringify(entries)); }
            catch (_) { /* Keep session progress when browser storage is unavailable. */ }
        }
        numbers() { return [...this.codes.keys()].sort((a,b)=>a-b); }
    }
    S.KnownLevels = KnownLevels;
}(window.Sentinel));
