/* Game state and actions have no dependency on DOM, audio or Three.js. */
(function (S) {
    'use strict';
    const {MODEL: M, ENERGY, Landscape} = S.World;
    const TAU = Math.PI * 2;
    const VISIBILITY = Object.freeze({NONE: 0, PARTIAL: 64, FULL: 128});
    const angleDifference = (a, b) => Math.atan2(Math.sin(a - b), Math.cos(a - b));
    const modelTop = [0.12, 0.25, 0.05, -0.375, 0.1, 0.25, 0.125];
    const TICK_HZ = 1193182 / 21845;

    class Game {
        constructor(number = 0) {
            this.landscape = new Landscape(number);
            this.number = number;
            this.playerId = this.landscape.playerId;
            this.energy = 10;
            this.status = 'preview';
            this.warning = 0;
            this.ticks = 0;
            this.countdownPhase = 0;
            this.events = [];
            this.revision = 0;
        }
        get player() { return this.landscape.objects[this.playerId]; }
        emit(type, detail = {}) { this.events.push({type, ...detail}); }
        takeEvents() { return this.events.splice(0); }
        start() { if (this.status === 'preview') this.status = 'playing'; }
        changeEnergy(amount) { this.energy = (this.energy + amount) & 63; this.emit('energy', {energy: this.energy}); }
        fail(message) { this.emit('error', {message}); return false; }
        changed(type, object) { ++this.revision; this.emit(type, {id: object.id, model: object.model}); }

        /* Enemy sight samples the target's upper and lower elevations.
         * Player actions use the cursor ray's surface contact instead. */
        clearLine(from, to, ignored = []) {
            const world = this.landscape;
            const dx = to.x - from.x, dy = to.y - from.y, dz = to.z - from.z;
            const distance = Math.hypot(dx, dy);
            const steps = Math.max(2, Math.ceil(distance * 32));
            for (let i = 1; i < steps; ++i) {
                const t = i / steps, x = from.x + dx * t, y = from.y + dy * t, z = from.z + dz * t;
                if (world.surface(x, y) > z + 0.015) return false;
                for (let object = world.head(Math.floor(x), Math.floor(y)); object; object = world.objects[object.support]) {
                    if (ignored.includes(object.id)) continue;
                    /* Objects are centered in their square; do not fill the whole
                     * square with an invisible wall around a slender tree. */
                    if (Math.abs(x - object.x - 0.5) > 0.28 || Math.abs(y - object.y - 0.5) > 0.28) continue;
                    if (object.z + modelTop[object.model] > z && object.z - 0.875 < z) return false;
                }
            }
            return true;
        }
        sight(observer, target, width = 20) {
            if (!observer || !target || observer.id === target.id) return VISIBILITY.NONE;
            const dx = target.x - observer.x, dy = target.y - observer.y;
            const bearing = Math.atan2(dx, dy);
            if (Math.abs(angleDifference(bearing, observer.heading / 256 * TAU)) > width / 512 * TAU) return VISIBILITY.NONE;
            const from = {x: observer.x + 0.5, y: observer.y + 0.5, z: observer.z};
            const upper = {x: target.x + 0.5, y: target.y + 0.5, z: target.z};
            const lower = {...upper, z: target.z - 224 / 256};
            const ignore = [observer.id, target.id];
            if (target.support >= 0) ignore.push(target.support);
            const top = this.clearLine(from, upper, ignore);
            const bottom = this.clearLine(from, lower, ignore);
            return bottom ? VISIBILITY.FULL : top ? VISIBILITY.PARTIAL : VISIBILITY.NONE;
        }
        supportAt(x, y) {
            for (let object = this.landscape.head(x,y); object; object = this.landscape.objects[object.support]) {
                if (object.model === M.BOULDER) return {kind: 'boulder', object};
                if (object.model === M.PEDESTAL) return {kind: 'pedestal', object};
            }
            return {kind: 'ground', object: null};
        }
        canUseContact(contact) {
            if (!contact || !this.player) return false;
            const {x,y,point,kind,supportId,downward} = contact, world = this.landscape;
            if (!world.inside(x,y) || !world.flat(x,y) || (x === this.player.x && y === this.player.y)) return false;
            if (!point || ![point.x,point.y,point.z].every(Number.isFinite) ||
                point.x < x || point.x > x + 1 || point.y < y || point.y > y + 1) return false;
            const support = this.supportAt(x,y);
            if (kind !== support.kind || supportId !== (support.object ? support.object.id : null)) return false;
            // Occlusion is resolved by the cursor ray's first surface hit.
            // Never replace that contact with a square-center visibility test.
            if (kind === 'ground') return downward && Math.abs(point.z - world.height(x,y)) < 0.00001;
            const object = support.object;
            if (kind === 'boulder') return Math.hypot(point.x - x - 0.5,point.y - y - 0.5) <= 7 / 16 + 0.015 &&
                point.z >= object.z - 0.875 - 0.015 && point.z <= object.z - 0.375 + 0.015;
            // Original pedestal contact is its top 1/16-height band, not its sides.
            const top = object.z + 0.125;
            return downward && Math.max(Math.abs(point.x - x - 0.5),Math.abs(point.y - y - 0.5)) < 100 / 256 &&
                point.z >= top - 1 / 16 && point.z <= top + 0.00001;
        }
        action(action, contact) {
            if (this.status !== 'playing') return false;
            if (action === 'hyperspace') return this.hyperspace();
            const world = this.landscape;
            if (!contact || !world.inside(contact.x, contact.y)) return this.fail('AIM AT A SUPPORTING SURFACE');
            const head = world.head(contact.x, contact.y);
            if (head && head.id === this.playerId) return this.fail('THIS IS YOUR ROBOT');
            if (!this.canUseContact(contact)) return this.fail('AIM AT THE SQUARE OR ITS TOP SUPPORT');
            if (action === 'transfer') {
                if (!head || head.model !== M.ROBOT) return this.fail('AIM AT ANOTHER ROBOT');
                this.playerId = head.id;
                this.changed('transfer', head);
                if (!world.objects[0] && head.x === world.sentinelSquare.x && head.y === world.sentinelSquare.y)
                    this.emit('message', {message: 'PRESS H TO HYPERSPACE TO THE NEXT LANDSCAPE'});
                return true;
            }
            if (action === 'absorb') {
                if (!head || head.model === M.PEDESTAL) return this.fail('NOTHING HERE CAN BE ABSORBED');
                if (!world.objects[0]) {
                    const onPedestal = this.player.x === world.sentinelSquare.x && this.player.y === world.sentinelSquare.y;
                    return this.fail(onPedestal ? 'PRESS H TO HYPERSPACE TO THE NEXT LANDSCAPE'
                        : 'THE SENTINEL IS GONE - REACH ITS PEDESTAL');
                }
                const energy = ENERGY[head.model];
                if (!world.detach(head)) return this.fail('REMOVE THE TOP OBJECT FIRST');
                for (const owner of world.objects) if (owner && owner.meanie === head.id) owner.meanie = -1;
                this.changeEnergy(energy); this.changed('absorb', head);
                if (head.model === M.SENTINEL) this.emit('message', {message: 'CREATE A ROBOT ON THE PEDESTAL. TRANSFER, THEN HYPERSPACE.'});
                return true;
            }
            const model = {tree: M.TREE, boulder: M.BOULDER, robot: M.ROBOT}[action];
            if (model === undefined) return this.fail('UNKNOWN ACTION');
            if (head && ![M.BOULDER, M.PEDESTAL].includes(head.model)) return this.fail('THAT SQUARE IS OCCUPIED');
            if (this.energy < ENERGY[model]) return this.fail('NOT ENOUGH ENERGY');
            const object = world.allocate(model);
            if (!object) return this.fail('NO FREE OBJECT SLOTS');
            if (!world.attach(object, contact.x, contact.y)) { world.objects[object.id] = null; return this.fail('CANNOT CREATE HERE'); }
            if (model === M.ROBOT) object.heading = this.player.heading ^ 128;
            this.changeEnergy(-ENERGY[model]); this.changed('create', object);
            return true;
        }
        hyperspace(cause = null) {
            const world = this.landscape, previous = this.player;
            const robot = world.allocate(M.ROBOT);
            if (!robot) return this.fail('NO FREE ROBOT SLOT');
            if (!world.placeFlat(robot, Math.floor(previous.z) + 1)) { world.objects[robot.id] = null; return this.fail('NO HYPERSPACE DESTINATION'); }
            if (this.energy < 3) {
                world.detach(robot); this.lose('HYPERSPACE NEEDS THREE ENERGY',cause); return false;
            }
            this.changeEnergy(-3);
            this.playerId = robot.id;
            this.changed('hyperspace', robot);
            if (previous.x === world.sentinelSquare.x && previous.y === world.sentinelSquare.y && !world.objects[0]) {
                this.status = 'won';
                this.nextNumber = (this.number + this.energy) % 10000;
                this.nextCode = S.World.secret(this.nextNumber);
                this.emit('win', {number: this.nextNumber, code: this.nextCode});
            }
            return true;
        }
        lose(reason, attacker = null) {
            this.status = 'lost'; this.warning = 1;
            this.lossModel = attacker ? attacker.model : M.ROBOT;
            this.lossReason = reason;
            this.emit('loss', {message: reason,model: this.lossModel});
        }
        absorbUnit(owner, victim) {
            if (victim.id === this.playerId) {
                if (!this.energy) this.lose('YOUR ROBOT HAS BEEN ABSORBED',owner);
                else { this.changeEnergy(-1); this.emit('drain'); }
            } else if (victim.model === M.TREE || victim.model === M.MEANIE) {
                this.landscape.detach(victim); this.changed('enemyAbsorb', victim);
            } else {
                victim.model = victim.model === M.ROBOT ? M.BOULDER : M.TREE;
                this.changed('enemyAbsorb', victim);
            }
            ++owner.replant; owner.scanDelay = 30;
        }
        makeMeanie(owner, target) {
            const world = this.landscape;
            const trees = world.objects.filter(o => o && o.model === M.TREE && Math.abs(o.x - target.x) < 10 && Math.abs(o.y - target.y) < 10);
            for (const tree of trees.sort((a, b) => b.id - a.id)) {
                if (this.sight(target, tree, 40) === VISIBILITY.NONE) continue;
                tree.model = M.MEANIE; tree.meanieTarget = target.id;
                owner.meanie = tree.id; owner.target = target.id;
                owner.scanDelay = 50; owner.absorbDelay = 1; owner.sight = VISIBILITY.PARTIAL;
                this.changed('meanie', tree); return true;
            }
            return false;
        }
        chooseSentryTarget(owner) {
            const world = this.landscape;
            let partialPlayer = null;
            // Fully visible robots have priority, highest slot first. Only the
            // player's robot is worth retaining when merely partially visible.
            for (let slot = 63; slot >= 0; --slot) {
                const object = world.objects[slot];
                if (!object || object.model !== M.ROBOT) continue;
                const visibility = this.sight(owner,object);
                if (visibility === VISIBILITY.FULL && world.head(object.x,object.y) === object)
                    return {object, visibility, drainImmediately: false};
                if (object.id === this.playerId && visibility !== VISIBILITY.NONE) partialPlayer = object;
            }
            if (partialPlayer) return {object: partialPlayer, visibility: VISIBILITY.PARTIAL, drainImmediately: false};
            // Other energy sources drain immediately, but only when exposed.
            // Bare trees are already the minimum-energy form; leave them alone
            // instead of continually absorbing and replanting the same forest.
            for (let slot = 63; slot >= 0; --slot) {
                const object = world.objects[slot];
                if (!object || ![M.TREE,M.BOULDER].includes(object.model) ||
                    (object.support < 0 && object.model !== M.BOULDER) || world.head(object.x,object.y) !== object) continue;
                const visibility = this.sight(owner,object);
                if (visibility === VISIBILITY.FULL) return {object, visibility, drainImmediately: true};
            }
            return {object: null, visibility: VISIBILITY.NONE, drainImmediately: false};
        }
        tick() {
            if (this.status !== 'playing') return;
            ++this.ticks;
            const old = this.countdownPhase;
            this.countdownPhase = (old + 161) & 255;
            const world = this.landscape;
            if (old + 161 <= 255) {
                for (const o of world.objects) if (o && [M.SENTRY, M.SENTINEL].includes(o.model)) {
                    for (const counter of ['scanDelay', 'turnDelay', 'absorbDelay']) if (o[counter] >= 2) --o[counter];
                }
            }
            this.warning = 0;
            for (const owner of world.objects.slice(0, 8)) {
                if (!owner || ![M.SENTRY, M.SENTINEL].includes(owner.model)) continue;
                if (owner.replant) {
                    const tree = world.allocate(M.TREE);
                    if (tree) {
                        if (world.placeFlat(tree, world.heightLimit)) { --owner.replant; this.changed('replant', tree); continue; }
                        else world.objects[tree.id] = null;
                    }
                }
                const meanie = world.objects[owner.meanie];
                if (meanie) {
                    if (owner.scanDelay > 1) continue;
                    const target = world.objects[meanie.meanieTarget];
                    const difference = target ? angleDifference(Math.atan2(target.x - meanie.x, target.y - meanie.y), meanie.heading / 256 * TAU) : 0;
                    if (target && Math.abs(difference) > 20 / 512 * TAU) {
                        meanie.heading = (meanie.heading + Math.sign(difference) * 8) & 255;
                        owner.scanDelay = 10; this.emit('meanieTurn');
                    } else if (!target || target.id !== this.playerId || this.sight(meanie, target, 20) === VISIBILITY.NONE) {
                        meanie.model = M.TREE; owner.meanie = -1; owner.absorbDelay = 0;
                        this.changed('meanieEnd', meanie);
                    } else {
                        this.emit('forcedHyperspace'); this.hyperspace(meanie);
                        if (this.status !== 'playing') break;
                    }
                    continue;
                }
                if (owner.scanDelay > 1) continue;
                let victim = owner.absorbDelay ? world.objects[owner.target] : null;
                let visibility = victim ? this.sight(owner, victim) : VISIBILITY.NONE, drainImmediately = false;
                if (visibility === VISIBILITY.NONE) {
                    owner.absorbDelay = 0;
                    const target = this.chooseSentryTarget(owner);
                    victim = target.object; visibility = target.visibility; drainImmediately = target.drainImmediately;
                }
                if (victim && visibility !== VISIBILITY.NONE) {
                    owner.target = victim.id; owner.sight = visibility;
                    if (drainImmediately) { this.absorbUnit(owner,victim); owner.absorbDelay = 0; }
                    else if (!owner.absorbDelay) owner.absorbDelay = 120;
                    else if (owner.absorbDelay === 1) {
                        if (visibility === VISIBILITY.FULL) {
                            this.absorbUnit(owner, victim);
                            // A retained player target remains acquired after its
                            // first drain; the 30-count scan cooldown paces later units.
                            owner.absorbDelay = victim.id === this.playerId ? 1 : 0;
                        } else {
                            if (!this.makeMeanie(owner, victim)) owner.absorbDelay = 0;
                        }
                    }
                    if (victim.id === this.playerId) this.warning = Math.max(this.warning, visibility === VISIBILITY.FULL ? 1 : 0.5);
                } else {
                    owner.target = -1; owner.sight = VISIBILITY.NONE;
                    if (owner.turnDelay <= 1) {
                        owner.heading = (owner.heading + owner.turnStep) & 255;
                        owner.turnDelay = 200;
                        this.emit('sentryTurn');
                    }
                }
                if (this.status !== 'playing') break;
            }
            if (this.status === 'playing') {
                this.warning = 0;
                for (const owner of world.objects.slice(0,8)) {
                    if (owner && [M.SENTRY,M.SENTINEL].includes(owner.model) && owner.target === this.playerId && owner.absorbDelay) {
                        this.warning = Math.max(this.warning, owner.sight === VISIBILITY.FULL ? 1 : 0.5);
                    }
                }
            }
        }
        snapshot() {
            return {number: this.number, energy: this.energy, playerId: this.playerId,
                status: this.status, ticks: this.ticks, warning: this.warning,
                random: {...this.landscape.random},
                objects: this.landscape.objects.map(o => o ? {...o} : null)};
        }
    }
    S.Game = Game;
    S.TICK_HZ = TICK_HZ;
}(window.Sentinel));
