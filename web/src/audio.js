/* Original pitch divisors, note stream and voice envelopes; Web Audio replaces PIT I/O. */
(function (S) {
    'use strict';
    const DIVISORS = [0x3550,0x348c,0x33cb,0x330d,0x3252,0x3199,0x30e3,0x3030,
        0x2f7f,0x2ed1,0x2e25,0x2d7b,0x2cd5,0x2c30,0x2b8e,0x2aee,
        0x2a50,0x29b5,0x291c,0x2885,0x27f0,0x275e,0x26cd,0x263f,
        0x25b3,0x2528,0x24a0,0x2419,0x2395,0x2312,0x2292,0x2213,
        0x2196,0x211a,0x20a1,0x2029,0x1fb3,0x1f3f,0x1ecc,0x1e5b,
        0x1dec,0x1d7e,0x1d12,0x1ca7,0x1c3e,0x1bd6,0x1b70,0x1b0b];
    const MELODY = [200,8,36,207,36,200,20,48,207,48,200,28,56,207,56,200,0,28,207,28,200,8,36,36,255,
        200,8,36,207,36,200,20,48,207,48,200,16,44,207,44,200,0,28,207,28,200,8,36,36,255,
        204,8,36,64,68,215,84,202,104,84,68,64,36,212,8,255,
        202,8,36,212,36,202,0,28,205,28,202,28,56,206,56,202,20,48,207,48,202,16,28,209,28,202,8,36,36,255];
    const EFFECTS = [
        [0x84,0,191,1,2,7,-2,7,0,0,255,1,0,0,0,0],
        [0x88,0,0,1,1,15,-2,7,0,16,1,1,0,0,0,0],
        [0x84,0,255,4,1,15,0,255,0,0,255,255,0,0,0,0],
        [0x80,0,34,1,15,1,-1,4,1,1,1,9,-1,1,0,8],
        [0x81,4,34,5,1,6,-1,6,6,2,1,5,-6,2,0,0],
        [0x82,0,144,1,15,1,-1,15,1,2,1,1,-1,1,0,7],
        [0x84,0,0,1,15,1,-15,1,0,0,255,1,0,0,0,0]
    ];
    // Keep the original 16-byte records as the reference, but decode them once
    // so playback operates on sound concepts rather than record offsets.
    const EFFECT_DEFINITIONS = EFFECTS.map(([
        control, level, pitch, envelopePeriod, envelopeStep, envelopeSteps,
        nextEnvelopeStep, nextEnvelopeSteps, firstPitchStep, firstPitchSteps,
        pitchPeriod, nextEnvelopePeriod, secondPitchStep, secondPitchSteps,
        thirdPitchStep, thirdPitchSteps
    ]) => ({
        control: {voice: control & 3, noise: Boolean(control & 4), octaveReduction: (control >>> 3) & 7},
        level, pitch, pitchPeriod,
        envelope: {period: envelopePeriod, step: envelopeStep, steps: envelopeSteps,
            nextStep: nextEnvelopeStep, nextSteps: nextEnvelopeSteps, nextPeriod: nextEnvelopePeriod},
        pitchPhases: [{step: firstPitchStep, steps: firstPitchSteps},
            {step: secondPitchStep, steps: secondPitchSteps}, {step: thirdPitchStep, steps: thirdPitchSteps}]
    }));
    function divisor(pitch, reduction = 0) {
        const octave = Math.floor(pitch / 48) - reduction;
        let value = DIVISORS[pitch % 48];
        if (Math.abs(octave) > 16) return 0;
        value = octave >= 0 ? value >>> octave : (value << -octave) & 65535;
        return value;
    }

    class Sound {
        constructor() {
            this.context = null; this.nodes = []; this.voices = [null, null, null, null];
            this.cursor = -1; this.countdown = 0; this.duration = 0; this.muted = false;
            this.warning = false; this.warningCountdown = 0;
            this.random = new S.World.Random(0x1234, 1, 0x5678);
        }
        enable() {
            if (!this.context) {
                const Audio = window.AudioContext || window.webkitAudioContext;
                if (!Audio) return Promise.resolve();
                this.context = new Audio();
                for (let i = 0; i < this.voices.length; ++i) {
                    const oscillator = this.context.createOscillator(), gain = this.context.createGain();
                    oscillator.type = 'square'; gain.gain.value = 0;
                    oscillator.connect(gain); gain.connect(this.context.destination); oscillator.start();
                    this.nodes.push({oscillator, gain});
                }
            }
            return this.context.resume().catch(() => {});
        }
        pause() { if (this.context) this.context.suspend().catch(() => {}); }
        silence() {
            this.voices.fill(null); this.cursor = -1; this.warning = false; this.warningCountdown = 0; this.updateOutput();
        }
        setWarning(active) {
            if (active === this.warning) return;
            this.warning = active; this.voices[1] = null;
        }
        melody(start) { this.cursor = start; this.countdown = 0; this.duration = 0; }
        effect(index, pitch) {
            const definition = EFFECT_DEFINITIONS[index];
            if (this.muted || !definition) return;
            const effect = {...definition, envelope: {...definition.envelope}};
            if (pitch !== undefined) effect.pitch = pitch;
            // Keep melody notes on their original channel, with an extra
            // effects channel so immediate player actions cannot replace them.
            const originalSlot = effect.control.voice;
            const slot = originalSlot === 0 && index !== 3 ? 3 : originalSlot;
            this.voices[slot] = {effect, envelopeCountdown: effect.envelope.period, pitchCountdown: effect.pitchPeriod,
                pitchStep: effect.pitchPhases[0].step, pitchSteps: effect.pitchPhases[0].steps, phase: 0, frequency: 100,
                ticksLeft: index === 2 ? 16 : undefined};
        }
        tick(random = this.random) {
            // Autoplay approval/resume is asynchronous. Do not consume notes
            // while the context cannot play them (including after a pause).
            if (this.context && this.context.state !== 'running') return;
            // Original dispatcher 008Bh: warning mode 4 triggers effect 4 at
            // 36h-tick intervals. Energy-drain feedback is separate (effect 5).
            if (this.warningCountdown) --this.warningCountdown;
            if (this.warning && this.cursor < 0 && !this.warningCountdown) {
                this.effect(4); this.warningCountdown = 0x36;
            }
            if (this.countdown) --this.countdown;
            if (this.cursor >= 0 && !this.countdown) {
                let guard = 0;
                while (++guard < 20) {
                    const note = MELODY[this.cursor++];
                    if (note === undefined || note === 255) { this.cursor = -1; break; }
                    if (note >= 200) { this.duration = (note - 200) * 4; continue; }
                    this.effect(3, note); this.countdown = this.duration; break;
                }
            }
            for (let slot = this.voices.length - 1; slot >= 0; --slot) {
                const voice = this.voices[slot];
                if (!voice) continue;
                if (this.muted) { this.voices[slot] = null; continue; }
                // The redraw noise owns its timeout. Replacing it with another
                // effect or note must never leave a timer that silences that sound.
                if (voice.ticksLeft !== undefined && --voice.ticksLeft <= 0) {
                    this.voices[slot] = null; continue;
                }
                const effect = voice.effect, envelope = effect.envelope;
                let d = divisor(effect.pitch, effect.control.octaveReduction);
                if (effect.control.noise) d = (((~effect.pitch) & 255) << 8) | random.next();
                if (voice.envelopeCountdown >= 2) --voice.envelopeCountdown;
                else {
                    voice.envelopeCountdown = envelope.period;
                    if (!envelope.steps) {
                        if (envelope.step === envelope.nextStep) { this.voices[slot] = null; continue; }
                        envelope.step = envelope.nextStep; envelope.steps = envelope.nextSteps;
                        envelope.period = envelope.nextPeriod; voice.envelopeCountdown = envelope.nextPeriod;
                    }
                    --envelope.steps; const level = (effect.level + envelope.step) & 255;
                    effect.level = level & 128 ? 0 : Math.min(level, 15);
                }
                if (!(voice.pitchCountdown & 128)) {
                    if (voice.pitchCountdown >= 2) --voice.pitchCountdown;
                    else {
                        voice.pitchCountdown = effect.pitchPeriod;
                        let guard = 0;
                        while (!voice.pitchSteps && ++guard < 4) {
                            voice.phase = (voice.phase + 1) % 3;
                            const phase = effect.pitchPhases[voice.phase];
                            voice.pitchStep = phase.step; voice.pitchSteps = phase.steps;
                        }
                        if (voice.pitchSteps) {
                            --voice.pitchSteps; effect.pitch = (effect.pitch + voice.pitchStep) & 255;
                            d = divisor(effect.pitch, effect.control.octaveReduction);
                        }
                    }
                }
                voice.frequency = d ? Math.min(18000, 1193182 / d) : 0;
            }
            this.updateOutput();
        }
        updateOutput() {
            if (!this.context) return;
            const now = this.context.currentTime;
            this.nodes.forEach((node, slot) => {
                const v = this.voices[slot], gain = !this.muted && v && v.frequency ? v.effect.level / 15 * 0.025 : 0;
                node.gain.gain.setTargetAtTime(gain, now, 0.003);
                if (v && v.frequency) node.oscillator.frequency.setTargetAtTime(v.frequency, now, 0.001);
            });
        }
    }
    S.Sound = Sound;
    S.SoundData = {DIVISORS, MELODY, EFFECTS, divisor};
}(window.Sentinel));
