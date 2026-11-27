/* Unlocked landscapes and browser-local progress; no network or server. */
(function (S) {
    'use strict';
    const STORAGE_KEY = 'sentinel.known-levels.v2';
    const LEGACY_STORAGE_KEY = 'sentinel.known-levels.v1';
    const validNumber = number => Number.isInteger(number) && number >= 0 && number <= 9999;
    class KnownLevels {
        constructor(storage) {
            this.unlocked = new Set([0]);
            this.results = new Map();
            try {
                this.storage = storage === undefined ? window.localStorage : storage;
                const raw = this.storage?.getItem(STORAGE_KEY);
                if (raw != null) {
                    const saved = JSON.parse(raw);
                    if (saved.version !== 2 || !Array.isArray(saved.unlocked) || !Array.isArray(saved.results)) return;
                    for (const number of saved.unlocked) if (validNumber(number)) this.unlocked.add(number);
                    for (const entry of saved.results) {
                        if (Array.isArray(entry) && this.unlocked.has(entry[0]) && (entry[1] === 'won' || entry[1] === 'lost'))
                            this.results.set(entry[0],entry[1]);
                    }
                } else {
                    const legacy = this.storage?.getItem(LEGACY_STORAGE_KEY);
                    if (legacy != null) this.migrateLegacy(JSON.parse(legacy));
                }
            } catch (_) { /* Storage is optional for locally opened pages. */ }
        }
        migrateLegacy(entries) {
            if (Array.isArray(entries)) for (const entry of entries) {
                if (!Array.isArray(entry)) continue;
                const [number,code,result] = entry;
                if (!validNumber(number) || typeof code !== 'string' || !/^[0-9]{8}$/.test(code) || S.World.secret(number) !== code) continue;
                this.unlocked.add(number);
                if (result === 'won' || result === 'lost') this.results.set(number,result);
            }
            this.save();
        }
        unlock(number) {
            if (!validNumber(number) || this.unlocked.has(number)) return false;
            this.unlocked.add(number);
            this.save();
            return true;
        }
        reset() {
            this.unlocked = new Set([0]);
            this.results.clear();
            this.save();
        }
        recordResult(number, result) {
            if (!this.unlocked.has(number) || this.results.get(number) === result) return false;
            this.results.set(number,result);
            this.save();
            return true;
        }
        save() {
            const saved = {version: 2, unlocked: [...this.unlocked], results: [...this.results]};
            try { this.storage?.setItem(STORAGE_KEY,JSON.stringify(saved)); }
            catch (_) { /* Keep session progress when browser storage is unavailable. */ }
        }
        numbers() { return [...this.unlocked].sort((a,b)=>a-b); }
    }
    S.KnownLevels = KnownLevels;
}(window.Sentinel));
