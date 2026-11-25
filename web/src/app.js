(function (S) {
    'use strict';
    class Application {
        constructor(canvas, assets) {
            this.canvas = canvas; this.assets = assets; this.view = new S.View(canvas, assets);
            this.sound = new S.Sound(); this.screen = 'welcome'; this.game = null;
            this.paused = false; this.target = null;
            this.message = ''; this.messageUntil = 0; this.lastTime = performance.now(); this.accumulator = 0;
            this.knownLevels = new S.KnownLevels();
            this.selectedLevel = this.knownLevels.numbers().at(-1);
            this.levelList = document.getElementById('known-level-list');
            this.renderKnownLevels();
            document.getElementById('reset-levels')?.addEventListener('click', () => {
                this.knownLevels.reset(); this.renderKnownLevels();
            });
            this.levelList?.addEventListener('click', event => {
                const button = event.target.closest('button[data-level]');
                if (!button || !this.canChooseKnownLevel()) return;
                const number = Number(button.dataset.level);
                if (!this.knownLevels.codes.has(number)) return;
                this.sound.enable(); this.beginLandscape(number); this.canvas.focus();
            });
            canvas.addEventListener('pointerdown', e => this.pointer(e));
            canvas.addEventListener('contextmenu', e => e.preventDefault());
            document.addEventListener('keydown', e => {
                if (!e.target.closest?.('#known-levels') ||
                    (this.canChooseKnownLevel() && ['ArrowUp','ArrowDown'].includes(e.key))) this.key(e);
            });
            document.addEventListener('mousemove', e => {
                if (document.pointerLockElement === canvas && this.screen === 'playing' && !this.paused) this.view.look(e.movementX, e.movementY);
            });
            document.addEventListener('pointerlockchange', () => {
                if (this.screen !== 'playing') return;
                if (document.pointerLockElement === canvas) {
                    this.paused = false; this.resetClock(); this.sound.enable();
                } else this.pause();
            });
            document.addEventListener('pointerlockerror', () => {
                this.pause(); this.notify('CLICK THE GAME TO CAPTURE THE MOUSE');
            });
            window.addEventListener('blur', () => this.pause());
            document.addEventListener('visibilitychange', () => { if (document.hidden) this.pause(); });
            // Publish the first frame before hiding the loading message. This
            // also makes a newly opened file useful before its first RAF callback.
            this.view.startTitleTurn(this.lastTime);
            this.view.draw(this, this.lastTime);
            this.canvas.dataset.screen = this.screen; this.canvas.dataset.ready = 'true';
            this.frame = this.frame.bind(this); requestAnimationFrame(this.frame);
        }
        resetClock() { this.lastTime = performance.now(); this.accumulator = 0; }
        canChooseKnownLevel() { return this.screen === 'preview'; }
        renderKnownLevels() {
            if (!this.levelList) return;
            this.levelList.replaceChildren(...this.knownLevels.numbers().map(number => {
                const button = document.createElement('button');
                button.type = 'button'; button.dataset.level = number;
                const label = String(number).padStart(4,'0'), result = this.knownLevels.results.get(number);
                const numberText = document.createElement('span'), marker = document.createElement('span');
                numberText.textContent = label;
                marker.className = 'level-result'; marker.setAttribute('aria-hidden','true');
                marker.textContent = result === 'won' ? '✓' : result === 'lost' ? '✗' : '';
                if (result) button.dataset.result = result;
                button.append(numberText,marker);
                button.setAttribute('aria-label','Landscape '+label+
                    (result === 'won' ? ', completed' : result === 'lost' ? ', last attempt failed' : ''));
                return button;
            }));
            this.levelListState = null; this.syncKnownLevels();
        }
        syncKnownLevels() {
            if (!this.levelList) return;
            const active = this.knownLevels.codes.has(this.selectedLevel) ? this.selectedLevel : null;
            const enabled = this.canChooseKnownLevel(), state = `${enabled}:${active}`;
            if (state === this.levelListState) return;
            this.levelListState = state;
            let selected;
            for (const button of this.levelList.children) {
                button.disabled = !enabled;
                if (Number(button.dataset.level) === active) {
                    button.setAttribute('aria-current','true'); selected = button;
                }
                else button.removeAttribute('aria-current');
            }
            if (selected) {
                const list = this.levelList.getBoundingClientRect(), item = selected.getBoundingClientRect();
                if (item.top < list.top) this.levelList.scrollTop -= list.top - item.top;
                else if (item.bottom > list.bottom) this.levelList.scrollTop += item.bottom - list.bottom;
            }
        }
        rememberLevel(number, code) {
            if (this.knownLevels?.remember(number,code)) this.renderKnownLevels();
        }
        recordLevelResult(result) {
            if (this.knownLevels?.recordResult(this.game.number,result)) this.renderKnownLevels();
        }
        notify(message, milliseconds = 2800) { this.message = message; this.messageUntil = performance.now() + milliseconds; }
        unlock() { if (document.pointerLockElement === this.canvas) document.exitPointerLock(); }
        capture() {
            this.paused = true;
            try {
                const result = this.canvas.requestPointerLock();
                if (result && result.catch) result.catch(() => { this.pause(); this.notify('CLICK TO RESUME MOUSE LOOK'); });
            } catch (_) { this.pause(); this.notify('MOUSE CAPTURE IS NOT AVAILABLE IN THIS BROWSER'); }
        }
        pause() {
            if (!['playing','overview'].includes(this.screen)) return;
            if (!this.paused) this.pauseStarted = performance.now();
            this.paused = true; this.resetClock(); this.sound.pause(); this.unlock();
        }
        title() {
            this.screen = 'welcome'; this.paused = false; this.unlock(); this.sound.silence(); this.introMusic = false;
            this.message = ''; this.resetClock();
            this.view.startTitleTurn(this.lastTime);
        }
        beginLandscape(number) {
            this.selectedLevel = number;
            this.game = new S.Game(number);
            this.rememberLevel(number,this.game.landscape.code);
            this.view.setGame(this.game); this.screen = 'preview'; this.paused = false;
            this.unlock(); if (!this.introMusic) this.sound.silence(); this.message = ''; this.resetClock();
            this.syncKnownLevels();
        }
        enterGame() {
            this.game.start(); this.screen = 'playing'; this.paused = true;
            this.sound.enable(); this.capture();
        }
        primary() {
            if (this.screen === 'welcome') {
                this.sound.enable(); this.sound.melody(0); this.introMusic = true;
                this.beginLandscape(this.knownLevels.codes.has(this.selectedLevel)
                    ? this.selectedLevel : this.knownLevels.numbers().at(-1));
            } else if (this.screen === 'preview') this.enterGame();
            else if (this.screen === 'lost') this.beginLandscape(this.game.number);
        }
        pointer(event) {
            event.preventDefault(); this.canvas.focus();
            if (this.screen === 'playing') {
                if (this.paused || document.pointerLockElement !== this.canvas) { this.sound.enable(); this.capture(); return; }
                if (event.button === 0) this.act('absorb');
                else if (event.button === 2) this.act('transfer');
            } else if (this.screen === 'overview' && this.paused) {
                this.screen = 'playing'; this.sound.enable(); this.capture();
            } else if (event.button === 0) this.primary();
        }
        key(event) {
            if (event.ctrlKey || event.metaKey || event.altKey) return;
            const key = event.key, lower = key.toLowerCase();
            const selecting = this.canChooseKnownLevel() && (key === 'ArrowUp' || key === 'ArrowDown');
            if (event.repeat && !selecting) return;
            if (['Tab',' ','ArrowUp','ArrowDown','ArrowLeft','ArrowRight','Escape','Enter'].includes(key)) event.preventDefault();
            if (selecting) {
                const levels = this.knownLevels.numbers(), index = levels.indexOf(this.selectedLevel);
                const next = Math.max(0,Math.min(levels.length-1,index+(key === 'ArrowUp' ? -1 : 1)));
                if (levels[next] !== this.selectedLevel) {
                    this.sound.enable(); this.beginLandscape(levels[next]); this.canvas.focus();
                }
                return;
            }
            if (lower === 'm') {
                this.sound.muted = !this.sound.muted; this.sound.updateOutput(); return;
            }
            if (this.screen === 'playing' || this.screen === 'overview') {
                if (this.paused) {
                    if (key === 'Escape' && performance.now() - (this.pauseStarted || 0) > 250) this.title();
                    else if (key === 'Enter') this.beginLandscape(this.game.number);
                    return;
                }
                if (key === 'Escape') { this.pause(); return; }
                if (key === 'Tab') {
                    if (this.screen === 'playing') { this.screen = 'overview'; this.unlock(); this.sound.pause(); this.resetClock(); }
                    else { this.screen = 'playing'; this.sound.enable(); this.capture(); }
                    return;
                }
                if (this.screen !== 'playing' || document.pointerLockElement !== this.canvas) return;
                const action = {t:'tree',b:'boulder',r:'robot',h:'hyperspace'}[lower];
                if (action) this.act(action);
                return;
            }
            if (key === 'Escape') { this.title(); return; }
            if (key === 'Enter' || key === ' ') this.primary();
        }
        act(action) {
            const before = this.game.playerId;
            this.target = this.view.pick(); this.game.action(action, this.target);
            if (this.game.playerId !== before) this.view.alignWithPlayer();
            this.processEvents();
        }
        processEvents() {
            for (const event of this.game.takeEvents()) {
                if (event.type === 'error') { this.notify(event.message); this.sound.effect(5,0xaa); }
                if (event.type === 'message') this.notify(event.message, 6500);
                if (event.type === 'absorb' || event.type === 'create') {
                    this.sound.effect(2);
                }
                if (event.type === 'transfer' || event.type === 'hyperspace') this.sound.melody(25);
                if (event.type === 'drain') this.sound.effect(5);
                if (event.type === 'meanie') { this.notify('A TREE HAS BECOME A MEANIE'); this.sound.effect(4); }
                if (event.type === 'meanieTurn') this.sound.effect(1);
                if (event.type === 'sentryTurn') this.sound.effect(0);
                if (event.type === 'forcedHyperspace') this.notify('THE MEANIE FORCES YOU INTO HYPERSPACE');
                if (event.type === 'loss') {
                    this.recordLevelResult('lost');
                    this.screen = 'lost'; this.paused = false; this.unlock(); this.introMusic = false;
                    this.lossStarted = performance.now();
                    this.sound.silence(); this.sound.melody(50); this.message = '';
                }
                if (event.type === 'win') {
                    this.recordLevelResult('won');
                    this.rememberLevel(event.number,event.code);
                    this.introMusic = false;
                    this.beginLandscape(event.number);
                    this.sound.melody(66);
                }
            }
        }
        frame(now) {
            try {
                if (!this.paused && !document.hidden && this.screen !== 'overview') {
                    this.accumulator += Math.min(0.25, (now - this.lastTime) / 1000);
                    const step = 1 / S.TICK_HZ;
                    while (this.accumulator >= step) {
                        if (this.screen === 'playing') {
                            const before = this.game.playerId;
                            this.game.tick(); this.processEvents();
                            if (this.screen !== 'playing') break;
                            if (before !== this.game.playerId) this.view.alignWithPlayer();
                        }
                        this.sound.setWarning(this.screen === 'playing' && this.game.warning > 0);
                        this.sound.tick(this.game ? this.game.landscape.random : undefined);
                        this.accumulator -= step;
                    }
                }
                this.lastTime = now;
                if (this.introMusic && this.sound.cursor < 0 && this.sound.voices.every(voice => !voice)) this.introMusic = false;
                this.view.draw(this, now);
                this.syncKnownLevels();
                this.canvas.dataset.screen = this.screen;
                this.canvas.dataset.ready = 'true';
                if (this.game) this.canvas.dataset.energy = this.game.energy;
            } catch (error) { this.crash(error); return; }
            requestAnimationFrame(this.frame);
        }
        crash(error) {
            console.error(error); this.sound.silence(); this.unlock();
            const status = document.getElementById('status');
            status.hidden = false; status.textContent = 'Unable to run Sentinel: ' + error.message;
            document.body.dataset.error = error.message;
        }
    }
    S.Application = Application;
    window.addEventListener('DOMContentLoaded', async () => {
        try {
            const assets = await S.Assets.load();
            S.app = new Application(document.getElementById('game'), assets);
            document.getElementById('status').hidden = true;
        } catch (error) {
            document.getElementById('status').textContent = 'Unable to start: ' + error.message;
            document.body.dataset.error = error.message; console.error(error);
        }
    });
}(window.Sentinel));
