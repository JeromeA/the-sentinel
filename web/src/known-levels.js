/* Recorded entry codes and browser-local progress; no network or server. */
(function (S) {
    'use strict';
    // Verified against the original generator: original/codes.txt, plus the
    // level-13 code recorded during the earlier original-game playthrough.
    const RECORDED = [
        [13,'71664005'],[14,'86076867'],[15,'19876496'],[17,'38796274'],
        [32,'87717819'],[38,'65506867'],[42,'67887349'],[59,'48439574'],
        [65,'94634608'],[70,'62793765'],[88,'69572284'],[106,'57649461'],
        [122,'64869119'],[136,'96456855'],[144,'64558555'],[168,'64998798'],
        [193,'47714471'],[214,'69480644'],[237,'72814583'],[271,'93908966'],
        [298,'53476943'],[335,'61440960'],[375,'42545683'],[399,'93288616'],[425,'94598959']
    ];
    const STORAGE_KEY = 'sentinel.known-levels.v1';
    class KnownLevels {
        constructor(storage) {
            this.codes = new Map([[0,S.World.secret(0)],...RECORDED]);
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
