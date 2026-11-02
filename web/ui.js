(function () {
  'use strict';
  const $ = (s) => document.querySelector(s);
  const D = SAUNA_DATA, K = D.knobs, SEC = D.sections, PRE = D.presets;
  const NK = K.length, NS = SEC.length;
  const clamp = (x, a, b) => (x < a ? a : x > b ? b : x);

  // ---------- model (mirrors SaunaModel.h)
  function steamStage (v) { return v < 0.12 ? 'Dry room' : v < 0.37 ? 'Warming up' : v < 0.62 ? 'Proper sauna' : v < 0.87 ? "Can't see" : 'Pure steam'; }
  const gainIdx = K.findIndex((k) => k.id === 'outGain');
  function applySteam (v, steam) {
    const e = v.slice(), lo = steam <= 0.5, t = lo ? steam / 0.5 : (steam - 0.5) / 0.5, tp = Math.pow(t, 1.3);
    K.forEach((k, i) => {
      if (k.steamTarget < 0) return;
      e[i] = lo ? (k.steamIsWet ? v[i] * t : v[i]) : v[i] + Math.max(0, k.steamTarget - v[i]) * tp;
    });
    if (!lo) e[gainIdx] = v[gainIdx] * Math.pow(10, -5 * t / 20);
    return e;
  }
  function fmt (k, v) {
    switch (k.fmt) {
      case 1: { const d = (v - 0.5) * 24; return (d >= 0 ? '+' : '') + d.toFixed(1) + ' dB'; }
      case 2: return Math.floor(v * 46 + 2.9) + ' voices';
      case 3: { const b = v * 2 - 1; return (b >= 0 ? '+' : '') + b.toFixed(2); }
      case 4: { const g = v * 1.25; if (g < 1e-4) return '-inf dB'; const d = 20 * Math.log10(g); return (d >= 0 ? '+' : '') + d.toFixed(1) + ' dB'; }
      case 5: return Math.round(v * 100) + '% tape';
      default: return Math.round(v * 100) + '%';
    }
  }
  const steamCount = K.filter((k) => k.steamTarget >= 0).length + 1;

  // ---------- state
  let S = { v: PRE[0].v.slice(), on: SEC.map(() => true), steam: PRE[0].steam, preset: 0, edited: false };
  try {
    const j = JSON.parse(localStorage.getItem('belgianSauna') || 'null');
    if (j && Array.isArray(j.v) && j.v.length === NK) S = Object.assign(S, j);
  } catch (e) { /* storage unavailable */ }
  let plunge = false, loop = true, saveT = 0;
  function save () { clearTimeout(saveT); saveT = setTimeout(() => { try { localStorage.setItem('belgianSauna', JSON.stringify(S)); } catch (e) { /* ignore */ } }, 400); }

  // ---------- wasm
  const wasmBytes = Uint8Array.from(atob(SAUNA_WASM_B64), (c) => c.charCodeAt(0));
  const modP = WebAssembly.compile(wasmBytes);
  const wasiStub = { fd_close: () => 0, fd_seek: () => 0, fd_write: () => 0 };
  class Sauna {
    constructor (mod, sr) {
      this.x = new WebAssembly.Instance(mod, { wasi_snapshot_preview1: wasiStub }).exports;
      this.x._initialize(); this.sr = sr; this.inited = false;
    }
    set (st, bypass) {
      const x = this.x;
      for (let i = 0; i < NK; i++) x.sauna_set(i, st.v[i]);
      for (let s = 0; s < NS; s++) x.sauna_set_on(s, st.on[s] ? 1 : 0);
      x.sauna_set_steam(st.steam);
      if (!this.inited) { x.sauna_init(this.sr); this.inited = true; }
      x.sauna_commit(); x.sauna_set_bypass(bypass ? 1 : 0);
    }
    process (inL, inR, outL, outR, n) {
      const x = this.x;
      for (let s = 0; s < n; s += 4096) {
        const c = Math.min(4096, n - s);
        const L = new Float32Array(x.memory.buffer, x.sauna_buf_l(), 4096), R = new Float32Array(x.memory.buffer, x.sauna_buf_r(), 4096);
        L.set(inL.subarray(s, s + c)); R.set(inR.subarray(s, s + c));
        x.sauna_process(c);
        const L2 = new Float32Array(x.memory.buffer, x.sauna_buf_l(), 4096), R2 = new Float32Array(x.memory.buffer, x.sauna_buf_r(), 4096);
        outL.set(L2.subarray(0, c), s); outR.set(R2.subarray(0, c), s);
      }
    }
    meters () { const m = new Float32Array(this.x.memory.buffer, this.x.sauna_meters(), 3); return { i: m[0], o: m[1], g: m[2] }; }
  }

  // ---------- LCD
  let infoT = 0;
  function info (msg, sticky) { $('#lcdInfo').textContent = msg; clearTimeout(infoT); if (!sticky) infoT = setTimeout(idleInfo, 4500); }
  function idleInfo () {
    $('#lcdInfo').textContent = plunge ? 'Cold plunge: the raw vocal. Bracing.' : playing ? 'Drag a knob. The pink arcs follow STEAM.' : buffer ? 'Press Play. The stones are hot.' : 'Load a voice memo or press Play to hear the demo voice.';
  }

  // ---------- build rooms
  function arc (cx, cy, r, a0, a1) {
    if (a1 - a0 < 0.6) return '';
    const p = (a) => { const t = a * Math.PI / 180; return [cx + r * Math.sin(t), cy - r * Math.cos(t)]; };
    const [x0, y0] = p(a0), [x1, y1] = p(a1);
    return `M${x0.toFixed(2)} ${y0.toFixed(2)}A${r} ${r} 0 ${a1 - a0 > 180 ? 1 : 0} 1 ${x1.toFixed(2)} ${y1.toFixed(2)}`;
  }
  const KE = [], RE = [];
  let lastTap = { t: 0, id: '' }, drag = null;
  function attachDial (el, id, get, set, reset, onInfo) {
    el.addEventListener('pointerdown', (e) => {
      if (e.button > 0) return;
      e.preventDefault(); el.focus({ preventScroll: true });
      const now = performance.now();
      if (now - lastTap.t < 320 && lastTap.id === id) { reset(); lastTap = { t: 0, id: '' }; onInfo(); return; }
      lastTap = { t: now, id };
      try { el.setPointerCapture(e.pointerId); } catch (err) { /* ignore */ }
      drag = { id, y: e.clientY, n: get() }; onInfo();
    });
    el.addEventListener('pointermove', (e) => { if (!drag || drag.id !== id) return; set(clamp(drag.n + (drag.y - e.clientY) / (e.shiftKey ? 800 : 230), 0, 1)); onInfo(); });
    const end = () => { if (drag && drag.id === id) drag = null; };
    el.addEventListener('pointerup', end); el.addEventListener('pointercancel', end);
    el.addEventListener('keydown', (e) => {
      const m = { ArrowUp: 0.01, ArrowRight: 0.01, ArrowDown: -0.01, ArrowLeft: -0.01, PageUp: 0.1, PageDown: -0.1 };
      if (e.key in m) { e.preventDefault(); set(clamp(get() + m[e.key], 0, 1)); onInfo(); }
      else if (e.key === 'Home') { e.preventDefault(); set(0); onInfo(); }
      else if (e.key === 'End') { e.preventDefault(); set(1); onInfo(); }
    });
  }
  const rooms = $('#rooms');
  SEC.forEach((s, si) => {
    const el = document.createElement('section');
    el.className = 'room'; el.dataset.w = String(s.count); el.dataset.id = s.id;
    el.style.order = String(s.id === 'outOn' ? 7 : si < 3 ? si * 2 : si * 2 + 2);
    el.innerHTML = `<div class="room-h"><button class="pwr" type="button" aria-label="${s.name} on or off"><span class="dot"></span></button><div><h2>${s.name}</h2><p>${s.what}</p></div></div>`;
    const pwr = el.querySelector('.pwr');
    pwr.addEventListener('click', () => { S.on[si] = !S.on[si]; S.edited = true; update(); info(`${s.name.toUpperCase()} ${S.on[si] ? 'ON' : 'OFF'}`); });
    const grid = document.createElement('div'); grid.className = 'knobs';
    for (let i = s.first; i < s.first + s.count; i++) {
      const k = K[i], kn = document.createElement('div'); kn.className = 'knob';
      kn.innerHTML = `<div class="dial" role="slider" tabindex="0" aria-label="${k.joke}: ${k.what}" aria-valuemin="0" aria-valuemax="100">
        <svg viewBox="0 0 60 60" aria-hidden="true"><path class="trk" d="${arc(30, 30, 25, -135, 135)}"/><path class="eff"/><path class="val"/></svg>
        <div class="cap"><span class="ptr"></span></div></div><div class="kread"></div><div class="kname">${k.joke}</div><div class="kwhat">${k.what}</div>`;
      const o = { i, dial: kn.querySelector('.dial'), val: kn.querySelector('.val'), eff: kn.querySelector('.eff'), cap: kn.querySelector('.cap'), read: kn.querySelector('.kread') };
      attachDial(o.dial, k.id, () => S.v[i], (v) => setKnob(i, v), () => setKnob(i, k.def), () => info(`${k.joke.toUpperCase()} → ${k.what} ${fmt(k, S.v[i])}`));
      KE.push(o); grid.appendChild(kn);
    }
    el.appendChild(grid); rooms.appendChild(el); RE.push({ el, pwr });
  });
  function setKnob (i, v) { if (S.v[i] === v) return; S.v[i] = v; S.edited = true; update(); }

  // STEAM dial
  const svg = $('#steamSvg'), LEDN = 31, leds = [];
  for (let i = 0; i < LEDN; i++) {
    const a = (-135 + 270 * i / (LEDN - 1)) * Math.PI / 180, c = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    c.setAttribute('cx', (80 + 71 * Math.sin(a)).toFixed(2)); c.setAttribute('cy', (80 - 71 * Math.cos(a)).toFixed(2)); c.setAttribute('r', '3.6');
    svg.appendChild(c); leds.push(c);
  }
  attachDial($('#steamDial'), 'steam', () => S.steam, (v) => { S.steam = v; S.edited = S.edited || v !== PRE[S.preset].steam; update(); }, () => { S.steam = 0.5; update(); },
    () => info(`STEAM → ${steamStage(S.steam).toUpperCase()} ${Math.round(S.steam * 100)}%`));
  function mix (a, b, t) { const pa = parseInt(a.slice(1), 16), pb = parseInt(b.slice(1), 16); const ch = (s) => Math.round(((pa >> s) & 255) * (1 - t) + ((pb >> s) & 255) * t); return `rgb(${ch(16)},${ch(8)},${ch(0)})`; }

  // presets
  const sel = $('#preset');
  PRE.forEach((p, i) => { const o = document.createElement('option'); o.value = i; o.textContent = `${String(i + 1).padStart(2, '0')} · ${p.name}`; sel.appendChild(o); });
  function loadPreset (i) {
    i = (i + PRE.length) % PRE.length;
    S = { v: PRE[i].v.slice(), on: SEC.map(() => true), steam: PRE[i].steam, preset: i, edited: false };
    update(); info(`PRESET ${String(i + 1).padStart(2, '0')} · ${PRE[i].name.toUpperCase()}`);
  }
  sel.addEventListener('change', () => loadPreset(+sel.value));
  $('#prevBtn').addEventListener('click', () => loadPreset(S.preset - 1));
  $('#nextBtn').addEventListener('click', () => loadPreset(S.preset + 1));

  // ---------- render
  let raf = 0;
  function update () { if (!raf) raf = requestAnimationFrame(() => { raf = 0; render(); sendParams(); save(); }); }
  function render () {
    const E = applySteam(S.v, S.steam);
    for (const o of KE) {
      const v = S.v[o.i], e = E[o.i];
      o.val.setAttribute('d', arc(30, 30, 25, -135, -135 + 270 * v));
      o.eff.setAttribute('d', Math.abs(e - v) > 0.004 ? arc(30, 30, 29.5, -135, -135 + 270 * e) : '');
      o.cap.style.transform = `rotate(${(-135 + 270 * v).toFixed(1)}deg)`;
      const t = fmt(K[o.i], v); o.read.textContent = t;
      o.dial.setAttribute('aria-valuenow', Math.round(v * 100)); o.dial.setAttribute('aria-valuetext', t);
    }
    RE.forEach((r, i) => { r.pwr.setAttribute('aria-pressed', S.on[i] ? 'true' : 'false'); r.el.classList.toggle('off', !S.on[i]); });
    const lit = Math.round(S.steam * (LEDN - 1));
    leds.forEach((c, i) => {
      const t = i / (LEDN - 1), on = i <= lit && S.steam > 0.001;
      c.setAttribute('fill', on ? (t < 0.5 ? mix('#ff7a45', '#ffb38a', t * 2) : mix('#ffb38a', '#fbeee0', (t - 0.5) * 2)) : '#3a2219');
      c.style.filter = on ? 'drop-shadow(0 0 3px rgba(255,179,138,.8))' : 'none';
    });
    $('#stone').style.transform = `rotate(${(-135 + 270 * S.steam).toFixed(1)}deg)`;
    $('#steamStage').textContent = steamStage(S.steam);
    $('#steamSub').textContent = `${Math.round(S.steam * 100)}% · steams ${steamCount} controls`;
    const sd = $('#steamDial'); sd.setAttribute('aria-valuenow', Math.round(S.steam * 100)); sd.setAttribute('aria-valuetext', `${steamStage(S.steam)} ${Math.round(S.steam * 100)}%`);
    sel.value = S.preset;
    const p = PRE[S.preset];
    $('#lcdPreset').textContent = `P${String(S.preset + 1).padStart(2, '0')} ${p.name.toUpperCase()}${S.edited ? ' *' : ''}`;
    $('#lcdJoke').textContent = S.edited ? `${p.joke} (Adjusted. Like the thermostat.)` : p.joke;
    $('#plungeLed').classList.toggle('on', plunge); $('#plungeBtn').setAttribute('aria-pressed', plunge ? 'true' : 'false');
    $('#loopLed').classList.toggle('on', loop); $('#loopBtn').setAttribute('aria-pressed', loop ? 'true' : 'false');
  }

  // ---------- audio
  let ctx = null, node = null, mainEng = null, mode = '', src = null, buffer = null, bufName = '', playing = false, startAt = 0, offset = 0;
  let mAcc = { i: 0, o: 0, g: 1 };
  function onMeters (m) { mAcc.i = Math.max(mAcc.i, m.i); mAcc.o = Math.max(mAcc.o, m.o); mAcc.g = Math.min(mAcc.g, m.g); }
  function sendParams () {
    if (!node) return;
    const st = { v: S.v, on: S.on, steam: S.steam };
    if (mode === 'worklet') node.port.postMessage({ type: 'params', st, bypass: plunge });
    else if (mainEng) mainEng.set(st, plunge);
  }
  const WORKLET = `
const wasiStub = { fd_close: () => 0, fd_seek: () => 0, fd_write: () => 0 };
class SaunaProc extends AudioWorkletProcessor {
  constructor (o) {
    super();
    this.x = new WebAssembly.Instance(o.processorOptions.mod, { wasi_snapshot_preview1: wasiStub }).exports;
    this.x._initialize(); this.inited = false; this.mc = 0; this.z = new Float32Array(128);
    this.port.onmessage = (e) => { const d = e.data; if (d.type === 'params') this.apply(d.st, d.bypass); };
  }
  apply (st, bypass) {
    const x = this.x;
    for (let i = 0; i < st.v.length; i++) x.sauna_set(i, st.v[i]);
    for (let s = 0; s < st.on.length; s++) x.sauna_set_on(s, st.on[s] ? 1 : 0);
    x.sauna_set_steam(st.steam);
    if (!this.inited) { x.sauna_init(sampleRate); this.inited = true; }
    x.sauna_commit(); x.sauna_set_bypass(bypass ? 1 : 0);
  }
  process (inputs, outputs) {
    const o = outputs[0]; if (!o || !o[0] || !this.inited) return true;
    const n = o[0].length, inp = inputs[0], L = (inp && inp[0]) || this.z, R = (inp && inp[1]) || L, x = this.x;
    new Float32Array(x.memory.buffer, x.sauna_buf_l(), n).set(L.subarray(0, n));
    new Float32Array(x.memory.buffer, x.sauna_buf_r(), n).set(R.subarray(0, n));
    x.sauna_process(n);
    o[0].set(new Float32Array(x.memory.buffer, x.sauna_buf_l(), n));
    if (o[1]) o[1].set(new Float32Array(x.memory.buffer, x.sauna_buf_r(), n));
    this.mc += n;
    if (this.mc >= 1024) { this.mc = 0; const m = new Float32Array(x.memory.buffer, x.sauna_meters(), 3); this.port.postMessage({ i: m[0], o: m[1], g: m[2] }); }
    return true;
  }
}
registerProcessor('belgian-sauna', SaunaProc);`;
  async function ensureAudio () {
    if (ctx) { if (ctx.state === 'suspended') await ctx.resume(); return true; }
    const AC = window.AudioContext || window.webkitAudioContext;
    if (!AC) { info('This browser has no Web Audio, so the sauna is closed here.', true); return false; }
    ctx = new AC({ latencyHint: 'playback' });
    const mod = await modP;
    try {
      const url = URL.createObjectURL(new Blob([WORKLET], { type: 'application/javascript' }));
      await ctx.audioWorklet.addModule(url);
      node = new AudioWorkletNode(ctx, 'belgian-sauna', { numberOfInputs: 1, numberOfOutputs: 1, outputChannelCount: [2], channelCount: 2, channelCountMode: 'explicit', channelInterpretation: 'speakers', processorOptions: { mod } });
      node.port.onmessage = (e) => onMeters(e.data);
      mode = 'worklet';
    } catch (e) {
      mainEng = new Sauna(mod, ctx.sampleRate);
      const sp = ctx.createScriptProcessor(2048, 2, 2);
      sp.onaudioprocess = (ev) => {
        const ib = ev.inputBuffer, ob = ev.outputBuffer;
        mainEng.process(ib.getChannelData(0), ib.getChannelData(1), ob.getChannelData(0), ob.getChannelData(1), ib.length);
        onMeters(mainEng.meters());
      };
      node = sp; mode = 'compat';
    }
    node.connect(ctx.destination);
    sendParams();
    if (ctx.state === 'suspended') await ctx.resume();
    return true;
  }
  const dur = () => (buffer ? buffer.duration : 0);
  function position () { if (!playing) return offset; const p = ctx.currentTime - startAt; return loop ? p % dur() : Math.min(p, dur()); }
  async function play () {
    if (!buffer) return;
    if (!(await ensureAudio())) return;
    const s = ctx.createBufferSource(); s.buffer = buffer; s.loop = loop;
    s.connect(node); s.start(0, offset); startAt = ctx.currentTime - offset; src = s; playing = true;
    s.onended = () => { if (src === s && !loop) { playing = false; offset = 0; src = null; renderTransport(); idleInfo(); } };
    renderTransport(); idleInfo();
  }
  function stop () {
    if (src) { offset = position(); const s = src; src = null; s.onended = null; try { s.stop(); } catch (e) { /* ignore */ } s.disconnect(); }
    playing = false; renderTransport(); idleInfo();
  }
  function renderTransport () { $('#playTxt').textContent = playing ? 'Stop' : 'Play'; $('#playIco').className = playing ? 'ico-stop' : 'ico-play'; }
  $('#playBtn').addEventListener('click', () => { if (playing) stop(); else play(); });
  $('#plungeBtn').addEventListener('click', () => { plunge = !plunge; update(); info(plunge ? 'COLD PLUNGE: THE RAW VOCAL. BRACING.' : 'BACK IN THE STEAM.'); });
  $('#loopBtn').addEventListener('click', () => { loop = !loop; if (src) { src.loop = loop; if (loop) { src.loopStart = 0; src.loopEnd = dur(); } } render(); info(loop ? 'LOOP ON.' : 'LOOP OFF.'); });

  // ---------- loading
  function fmtTime (t) { const m = Math.floor(t / 60), s = t - m * 60; return `${m}:${s < 10 ? '0' : ''}${s.toFixed(1)}`; }
  function setBuffer (b, name) {
    if (playing) stop();
    buffer = b; bufName = name; offset = 0;
    $('#lcdSrc').textContent = `${name} · ${b.numberOfChannels === 1 ? 'mono' : 'stereo'} · ${fmtTime(b.duration)}`;
    $('#lcdRate').textContent = `${(b.sampleRate / 1000).toFixed(1)} kHz`;
    drawWave();
  }
  function makeDemo () {
    // A breathy sung "aah" phrase so the sauna has something to steam.
    const sr = 48000, notes = [[0.3, 0.9, 64], [1.25, 0.5, 62], [1.8, 1.4, 59], [3.6, 0.45, 62], [4.1, 0.45, 64], [4.6, 1.8, 67], [7.0, 0.6, 59], [7.7, 2.0, 57]];
    const len = Math.round(10.5 * sr), out = new Float32Array(len);
    const F = [[800, 80, 1], [1150, 90, 0.5], [2900, 120, 0.25]];
    const bq = F.map(([f, bw, g]) => { const w = 2 * Math.PI * f / sr, q = f / bw, al = Math.sin(w) / (2 * q), a0 = 1 + al; return { b0: al / a0, b2: -al / a0, a1: -2 * Math.cos(w) / a0, a2: (1 - al) / a0, g, z1: 0, z2: 0 }; });
    let ph = 0, midi = notes[0][2], env = 0, seed = 7, tilt = 0, ni = 0, peak = 0;
    const rnd = () => { seed = (seed * 1664525 + 1013904223) >>> 0; return seed / 4294967296 * 2 - 1; };
    for (let i = 0; i < len; i++) {
      const t = i / sr;
      while (ni < notes.length - 1 && t >= notes[ni + 1][0]) ni++;
      const [st, d, m] = notes[ni], on = t >= st && t < st + d;
      midi += (m - midi) * 0.0004; env += ((on ? 1 : 0) - env) * (on ? 0.0004 : 0.00012);
      const f0 = 440 * Math.pow(2, (midi - 69 + Math.min(1, Math.max(0, (t - st - 0.3) / 0.4)) * 0.3 * Math.sin(2 * Math.PI * 5.2 * t)) / 12);
      ph += f0 / sr; if (ph >= 1) ph -= 1;
      tilt += ((2 * ph - 1) - tilt) * 0.08;
      const srcS = tilt * env + rnd() * 0.05 * env;
      let y = 0;
      for (const b of bq) { const o = b.b0 * srcS + b.z1; b.z1 = -b.a1 * o + b.z2; b.z2 = b.b2 * srcS - b.a2 * o; y += o * b.g; }
      out[i] = y; peak = Math.max(peak, Math.abs(y));
    }
    for (let i = 0; i < len; i++) out[i] *= 0.45 / (peak || 1);
    const b = new AudioBuffer({ length: len, numberOfChannels: 1, sampleRate: sr }); b.copyToChannel(out, 0);
    setBuffer(b, 'DEMO VOICE (SYNTHETIC)');
  }
  function str4 (v, p) { return String.fromCharCode(v.getUint8(p), v.getUint8(p + 1), v.getUint8(p + 2), v.getUint8(p + 3)); }
  function sniffRate (ab) {
    try {
      const v = new DataView(ab);
      if (ab.byteLength > 32 && str4(v, 0) === 'RIFF' && str4(v, 8) === 'WAVE') {
        let p = 12; while (p + 8 <= ab.byteLength) { const id = str4(v, p), sz = v.getUint32(p + 4, true); if (id === 'fmt ') return v.getUint32(p + 12, true); p += 8 + sz + (sz & 1); }
      }
    } catch (e) { /* not a wav */ }
    return 0;
  }
  async function loadFile (f) {
    info(`OPENING ${f.name.toUpperCase()}…`, true);
    try {
      const ab = await f.arrayBuffer(), rate = sniffRate(ab) || (ctx ? ctx.sampleRate : 48000);
      let b;
      try { b = await new OfflineAudioContext(2, 1, clamp(rate, 8000, 192000)).decodeAudioData(ab.slice(0)); }
      catch (e) { b = await new OfflineAudioContext(2, 1, 48000).decodeAudioData(ab.slice(0)); }
      setBuffer(b, f.name); info(`LOADED ${f.name.toUpperCase()}. PRESS PLAY.`);
    } catch (e) { info(`Couldn't open ${f.name}. Try a WAV, M4A or MP3.`); }
  }
  $('#file').addEventListener('change', (e) => { const f = e.target.files && e.target.files[0]; if (f) loadFile(f); e.target.value = ''; });
  $('#demoBtn').addEventListener('click', () => { makeDemo(); info('DEMO VOICE LOADED. IT SINGS "AAH". IT IS NO FIN.'); });

  // ---------- waveform + meters
  const wave = $('#wave'), wavebox = $('#wavebox');
  function drawWave () {
    const r = wave.getBoundingClientRect(), dpr = Math.min(2, window.devicePixelRatio || 1);
    wave.width = Math.max(1, Math.round(r.width * dpr)); wave.height = Math.max(1, Math.round(r.height * dpr));
    const g = wave.getContext('2d'), W = wave.width, H = wave.height;
    g.clearRect(0, 0, W, H); g.fillStyle = '#ffcf8a'; g.globalAlpha = 0.25; g.fillRect(0, Math.floor(H / 2), W, 1); g.globalAlpha = 0.85;
    if (!buffer) return;
    const a = buffer.getChannelData(0), step = a.length / W;
    let peak = 0; for (let i = 0; i < a.length; i += 7) peak = Math.max(peak, Math.abs(a[i]));
    const sc = (H / 2) * 0.92 / Math.max(0.05, peak);
    for (let x = 0; x < W; x++) {
      let mn = 0, mx = 0; const s0 = Math.floor(x * step), s1 = Math.min(a.length, Math.floor((x + 1) * step) + 1);
      for (let i = s0; i < s1; i += 2) { const v = a[i]; if (v < mn) mn = v; if (v > mx) mx = v; }
      g.fillRect(x, H / 2 - mx * sc, 1, Math.max(1, (mx - mn) * sc));
    }
  }
  if (window.ResizeObserver) new ResizeObserver(drawWave).observe(wavebox); else window.addEventListener('resize', drawWave);
  wavebox.addEventListener('click', (e) => {
    if (!buffer) return;
    const r = wavebox.getBoundingClientRect(), t = clamp((e.clientX - r.left) / r.width, 0, 1) * dur();
    if (playing) { stop(); offset = t; play(); } else offset = t;
  });
  let dIn = 0, dOut = 0, dGr = 0;
  const lv = (x) => clamp((20 * Math.log10(x + 1e-9) + 60) / 60, 0, 1);
  (function frame () {
    dIn = Math.max(lv(mAcc.i), dIn - 0.02); dOut = Math.max(lv(mAcc.o), dOut - 0.02);
    dGr = Math.max(clamp(-20 * Math.log10(mAcc.g || 1) / 12, 0, 1), dGr - 0.03);
    mAcc = { i: 0, o: 0, g: 1 };
    $('#mIn').style.transform = `scaleY(${dIn.toFixed(3)})`; $('#mOut').style.transform = `scaleY(${dOut.toFixed(3)})`; $('#mGr').style.transform = `scaleY(${dGr.toFixed(3)})`;
    if (buffer) { const p = position(); $('#playhead').style.transform = `translateX(${(p / dur() * wavebox.clientWidth).toFixed(1)}px)`; $('#lcdPos').textContent = `${fmtTime(p)} / ${fmtTime(dur())}`; }
    requestAnimationFrame(frame);
  })();

  // ---------- export
  let exporting = false, dlNS, lastExport = null;
  async function getDownloads () {
    if (dlNS !== undefined) return dlNS;
    try { dlNS = window.claude && window.claude.use ? await window.claude.use('downloads') : null; } catch (e) { dlNS = null; }
    return dlNS;
  }
  function encodeWav24 (L, R, sr) {
    const n = L.length, size = n * 6, buf = new ArrayBuffer(44 + size), v = new DataView(buf), u = new Uint8Array(buf);
    const ws = (o, s) => { for (let i = 0; i < 4; i++) u[o + i] = s.charCodeAt(i); };
    ws(0, 'RIFF'); v.setUint32(4, 36 + size, true); ws(8, 'WAVE'); ws(12, 'fmt '); v.setUint32(16, 16, true); v.setUint16(20, 1, true); v.setUint16(22, 2, true);
    v.setUint32(24, sr, true); v.setUint32(28, sr * 6, true); v.setUint16(32, 6, true); v.setUint16(34, 24, true); ws(36, 'data'); v.setUint32(40, size, true);
    let o = 44;
    for (let i = 0; i < n; i++) for (let c = 0; c < 2; c++) {
      let s = c ? R[i] : L[i]; s = s > 1 ? 1 : s < -1 ? -1 : s; let x = Math.round(s * 8388607); if (x < 0) x += 16777216;
      u[o++] = x & 255; u[o++] = (x >> 8) & 255; u[o++] = (x >> 16) & 255;
    }
    return u;
  }
  const CRC = (() => { const t = new Uint32Array(256); for (let n = 0; n < 256; n++) { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; t[n] = c >>> 0; } return t; })();
  function crc32 (u) { let c = 0xffffffff; for (let i = 0; i < u.length; i++) c = CRC[(c ^ u[i]) & 255] ^ (c >>> 8); return (c ^ 0xffffffff) >>> 0; }
  function makeZip (name, data) {
    const nm = new TextEncoder().encode(name), crc = crc32(data), d = new Date();
    const dt = ((d.getFullYear() - 1980) << 9) | ((d.getMonth() + 1) << 5) | d.getDate(), tm = (d.getHours() << 11) | (d.getMinutes() << 5) | (d.getSeconds() >> 1);
    const lh = new Uint8Array(30 + nm.length), l = new DataView(lh.buffer);
    l.setUint32(0, 0x04034b50, true); l.setUint16(4, 20, true); l.setUint16(10, tm, true); l.setUint16(12, dt, true); l.setUint32(14, crc, true); l.setUint32(18, data.length, true); l.setUint32(22, data.length, true); l.setUint16(26, nm.length, true); lh.set(nm, 30);
    const cd = new Uint8Array(46 + nm.length), c = new DataView(cd.buffer);
    c.setUint32(0, 0x02014b50, true); c.setUint16(4, 20, true); c.setUint16(6, 20, true); c.setUint16(12, tm, true); c.setUint16(14, dt, true); c.setUint32(16, crc, true); c.setUint32(20, data.length, true); c.setUint32(24, data.length, true); c.setUint16(28, nm.length, true); cd.set(nm, 46);
    const ed = new Uint8Array(22), e = new DataView(ed.buffer);
    e.setUint32(0, 0x06054b50, true); e.setUint16(8, 1, true); e.setUint16(10, 1, true); e.setUint32(12, cd.length, true); e.setUint32(16, lh.length + data.length, true);
    return new Blob([lh, data, cd, ed]);
  }
  const slug = (s) => s.replace(/\.[a-z0-9]+$/i, '').replace(/[^a-z0-9]+/gi, '-').replace(/^-+|-+$/g, '').slice(0, 48) || 'vocal';
  async function doExport (fxOnly) {
    if (exporting) return;
    if (!buffer) { info('Load a vocal first.'); return; }
    const dl = await getDownloads();
    if (!dl) { info('Saving needs this page open in the Claude app or claude.ai.', true); return; }
    const key = JSON.stringify([S, plunge, fxOnly, bufName, buffer.length]);
    exporting = true; $('#expBtn').disabled = $('#expFxBtn').disabled = true;
    const prog = $('#progress');
    try {
      let blob, fname;
      if (lastExport && lastExport.key === key) ({ blob, fname } = lastExport);
      else {
        const sr = buffer.sampleRate, eng = new Sauna(await modP, sr);
        const st = { v: S.v.slice(), on: S.on.slice(), steam: S.steam };
        if (fxOnly) { // only the steam: choir, echo and reverb, without the dry vocal
          const mi = K.findIndex((k) => k.id === 'outMix');
          st.v[mi] = 1;
        }
        eng.set(st, plunge);
        const len = buffer.length, E = applySteam(st.v, st.steam);
        const revSec = st.on[6] ? 2 + 10 * E[K.findIndex((k) => k.id === 'cosmosBig')] * E[K.findIndex((k) => k.id === 'cosmosReplace')] : 0;
        const total = len + Math.round(Math.min(20, Math.max(1.5, revSec + 3)) * sr);
        const inL = buffer.getChannelData(0), inR = buffer.numberOfChannels > 1 ? buffer.getChannelData(1) : inL;
        const oL = new Float32Array(total), oR = new Float32Array(total), B = 4096, bL = new Float32Array(B), bR = new Float32Array(B), tL = new Float32Array(B), tR = new Float32Array(B);
        // For "steam only", run a second pass with the time effects off and subtract it, leaving just the wet sound.
        let dry = null;
        if (fxOnly) {
          dry = new Sauna(await modP, sr);
          const d = { v: st.v.slice(), on: st.on.slice(), steam: st.steam };
          d.on[4] = d.on[5] = d.on[6] = false; dry.set(d, plunge);
        }
        const dL = new Float32Array(B), dR = new Float32Array(B);
        let last = performance.now();
        for (let s = 0; s < total; s += B) {
          const c = Math.min(B, total - s);
          for (let i = 0; i < c; i++) { const j = s + i; bL[i] = j < len ? inL[j] : 0; bR[i] = j < len ? inR[j] : 0; }
          eng.process(bL, bR, tL, tR, c);
          if (dry) { dry.process(bL, bR, dL, dR, c); for (let i = 0; i < c; i++) { tL[i] -= dL[i]; tR[i] -= dR[i]; } }
          oL.set(tL.subarray(0, c), s); oR.set(tR.subarray(0, c), s);
          if (performance.now() - last > 50) { const f = s / total; prog.style.transform = `scaleX(${f.toFixed(3)})`; info(`STEAMING… ${Math.round(f * 100)}%`, true); await new Promise((r) => setTimeout(r, 0)); last = performance.now(); }
        }
        let end = total; while (end > len && Math.abs(oL[end - 1]) < 3e-5 && Math.abs(oR[end - 1]) < 3e-5) end--;
        end = Math.min(total, end + Math.round(0.05 * sr));
        info('PACKING THE WAV…', true); await new Promise((r) => setTimeout(r, 0));
        const base = slug(bufName), pn = slug(PRE[S.preset].name), tag = fxOnly ? 'SteamOnly' : pn;
        blob = makeZip(`${base}_BelgianSauna_${tag}.wav`, encodeWav24(oL.subarray(0, end), oR.subarray(0, end), sr));
        fname = `${base}_BelgianSauna_${tag}.zip`;
        lastExport = { key, blob, fname };
      }
      prog.style.transform = 'scaleX(1)';
      info('CONFIRM THE SAVE IN THE PROMPT…', true);
      await dl.save({ filename: fname, data: blob });
      info(`SAVED ${fname.toUpperCase()}. UNZIP IT AND DROP THE WAV AT BAR 1.`);
    } catch (err) {
      const code = err && err.code;
      if (code === 'declined') info('Save cancelled. The render is kept, so trying again is instant.');
      else if (code === 'rate_limited') info('A save prompt is already open. Finish that one first.');
      else if (code === 'too_large') info('That file is too big to save here. Try a shorter section.');
      else if (code === 'unavailable' || code === 'not_granted' || code === 'capability_disabled' || code === 'capability_removed' || code === 'extension_not_enabled') info('Saving is not available in this view.');
      else info('Export failed: ' + ((err && err.message) || 'unknown error'));
    } finally {
      exporting = false; $('#expBtn').disabled = $('#expFxBtn').disabled = false;
      setTimeout(() => { prog.style.transform = 'scaleX(0)'; }, 900);
    }
  }
  $('#expBtn').addEventListener('click', () => doExport(false));
  $('#expFxBtn').addEventListener('click', () => doExport(true));

  // ---------- boot
  $('#lcdSrc').textContent = 'Heating the stones…';
  render();
  setTimeout(() => { makeDemo(); idleInfo(); }, 30);
})();
