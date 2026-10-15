const statusEl = { textContent: '' };
const errorsEl = document.getElementById('errors');
const loader = document.getElementById('loader');
const loaderText = document.getElementById('loaderText');
const barFill = document.getElementById('loading-bar-fill');
const canvas = document.getElementById('canvas');
let engineReady = false;

function setProgress(pct) {
  const v = Math.max(0, Math.min(100, Number(pct) || 0));
  if (barFill) {
    barFill.style.width = v + '%';
  }
}

function defaultWsUrl() {
  return `${location.protocol === 'https:' ? 'wss' : 'ws'}://${location.host}/ws`;
}

function setStatus(text) {
  const line = String(text || '');
  statusEl.textContent = line;
  if (loaderText && loader && !loader.hidden) {
    loaderText.textContent = line;
  }
}

function showError(err) {
  errorsEl.hidden = false;
  errorsEl.textContent = String(err && err.stack ? err.stack : err);
  setStatus('Erreur');
  if (loader) {
    loader.hidden = true;
  }
  document.body.classList.add('has-error');
}

window.addEventListener('warfork-error', (event) => {
  showError(event.detail);
});

function unlockAudio() {
  try {
    const AudioCtx = window.AudioContext || window.webkitAudioContext;
    if (!AudioCtx) {
      return;
    }
    const ctx = new AudioCtx();
    ctx.resume();
  } catch (err) {
    console.warn('Audio unlock failed', err);
  }
}

function isPhone() {
  const ua = navigator.userAgent || '';
  const iPadOS = navigator.platform === 'MacIntel' && navigator.maxTouchPoints > 1;
  const coarse = window.matchMedia && matchMedia('(pointer: coarse)').matches;
  const shortSide = Math.min(screen.width || 0, screen.height || 0)
    || Math.min(window.innerWidth, window.innerHeight);
  return /Android|iPhone|iPad|iPod/i.test(ua) || iPadOS || (coarse && shortSide < 900);
}

function isPortrait() {
  if (window.matchMedia) {
    return matchMedia('(orientation: portrait)').matches;
  }
  return window.innerHeight > window.innerWidth;
}

function updateRotateHint() {
  const el = document.getElementById('rotate-phone');
  if (!el) {
    return;
  }
  const show = isPhone() && isPortrait();
  el.hidden = !show;
  document.body.classList.toggle('portrait-phone', show);
}

async function enterImmersive() {
  const root = document.documentElement;
  try {
    if (!document.fullscreenElement && !document.webkitFullscreenElement) {
      if (root.requestFullscreen) {
        await root.requestFullscreen({ navigationUI: 'hide' });
      } else if (root.webkitRequestFullscreen) {
        root.webkitRequestFullscreen();
      }
    }
  } catch (err) {
    /* Safari iOS refuse souvent hors geste utilisateur */
  }
  try {
    const ori = screen.orientation || screen.mozOrientation;
    if (ori && typeof ori.lock === 'function') {
      await ori.lock('landscape');
    }
  } catch (err) {
    /* iOS n’autorise le lock qu’en PWA / fullscreen */
  }
  sizeCanvas();
}

function sizeCanvas() {
  const w = Math.max(64, Math.round(window.innerWidth || document.documentElement.clientWidth || 64));
  const h = Math.max(64, Math.round(window.innerHeight || document.documentElement.clientHeight || 64));
  const landscape = w >= h;
  const wasLandscape = canvas.width >= canvas.height;
  if (canvas.width < 64 || canvas.height < 64 || landscape !== wasLandscape) {
    canvas.width = w;
    canvas.height = h;
  } else {
    if (w > canvas.width) {
      canvas.width = w;
    }
    if (h > canvas.height) {
      canvas.height = h;
    }
  }
  canvas.style.width = '100%';
  canvas.style.height = '100%';
  canvas.style.left = '0';
  canvas.style.top = '0';
  canvas.style.right = '0';
  canvas.style.bottom = '0';
  canvas.style.display = 'block';
  updateRotateHint();
}

function bindMenuMouseMap() {
  const map = (event) => {
    if (document.pointerLockElement) {
      return;
    }
    const r = canvas.getBoundingClientRect();
    const rw = r.width || 1;
    const rh = r.height || 1;
    window.__wfMenuMouseX = ((event.clientX - r.left) / rw) * (canvas.width || rw);
    window.__wfMenuMouseY = ((event.clientY - r.top) / rh) * (canvas.height || rh);
  };
  window.addEventListener('pointermove', map, { passive: true });
  window.addEventListener('mousemove', map, { passive: true });
}

function requestRawPointerLock() {
  if (document.pointerLockElement === canvas) {
    return;
  }
  const opts = { unadjustedMovement: true };
  const req = canvas.requestPointerLock;
  if (!req) {
    return;
  }
  try {
    const ret = canvas.requestPointerLock(opts);
    if (ret && typeof ret.catch === 'function') {
      ret.catch(() => canvas.requestPointerLock());
    }
  } catch (err) {
    try {
      canvas.requestPointerLock();
    } catch (err2) {
      /* ignore */
    }
  }
}

function yieldPaint() {
  return new Promise((resolve) => {
    requestAnimationFrame(() => {
      setTimeout(resolve, 0);
    });
  });
}

function ensureDir(FS, dirPath) {
  const parts = dirPath.split('/').filter(Boolean);
  let current = '';
  for (const part of parts) {
    current += '/' + part;
    try {
      FS.mkdir(current);
    } catch (err) {
      /* already exists */
    }
  }
}

function formatMiB(bytes) {
  return (bytes / (1024 * 1024)).toFixed(1);
}

async function mountOneFile(FS, filePath, onBytes) {
  const res = await fetch('/game/' + filePath, { cache: 'no-store' });
  if (!res.ok) {
    throw new Error('HTTP ' + res.status);
  }
  const slash = filePath.lastIndexOf('/');
  if (slash > 0) {
    ensureDir(FS, '/' + filePath.slice(0, slash));
  }
  const stream = FS.open('/' + filePath, 'w+');
  let offset = 0;
  try {
    if (res.body && typeof res.body.getReader === 'function') {
      const reader = res.body.getReader();
      while (true) {
        const { done, value } = await reader.read();
        if (done) {
          break;
        }
        if (!value || !value.length) {
          continue;
        }
        FS.write(stream, value, 0, value.length, offset);
        offset += value.length;
        onBytes(value.length);
      }
    } else {
      const data = new Uint8Array(await res.arrayBuffer());
      FS.write(stream, data, 0, data.length, 0);
      offset = data.length;
      onBytes(data.length);
    }
  } finally {
    FS.close(stream);
  }
  return offset;
}

async function mountGameFiles(FS) {
  const response = await fetch('/game/manifest.json', { cache: 'no-store' });
  if (!response.ok) {
    console.warn('Pas de manifeste /game/manifest.json');
    return { mounted: 0, pk3: 0 };
  }
  const raw = await response.json();
  if (!Array.isArray(raw) || raw.length === 0) {
    return { mounted: 0, pk3: 0 };
  }
  const files = raw.map((entry) => {
    if (typeof entry === 'string') {
      return { path: entry, size: 0 };
    }
    return { path: entry.path, size: Number(entry.size) || 0 };
  }).filter((entry) => entry.path);
  files.sort((a, b) => a.size - b.size);
  const totalBytes = files.reduce((sum, entry) => sum + (entry.size || 0), 0);
  let loadedBytes = 0;
  let mounted = 0;
  let pk3 = 0;
  for (let i = 0; i < files.length; i++) {
    const entry = files[i];
    const name = entry.path.split('/').pop();
    setStatus(`Montage ${i + 1}/${files.length} : ${name} (${formatMiB(entry.size)} Mo)`);
    if (totalBytes > 0) {
      setProgress(Math.round((loadedBytes / totalBytes) * 100));
    } else {
      setProgress(Math.round(((i + 1) / files.length) * 100));
    }
    try {
      const wrote = await mountOneFile(FS, entry.path, (n) => {
        loadedBytes += n;
        if (totalBytes > 0) {
          setProgress(Math.min(100, Math.round((loadedBytes / totalBytes) * 100)));
        }
      });
      if (!entry.size) {
        loadedBytes += wrote;
      }
      mounted += 1;
      if (/\.pk3$/i.test(entry.path)) {
        pk3 += 1;
      }
    } catch (err) {
      console.warn('Montage échoué', entry.path, err);
    }
    await yieldPaint();
  }
  return { mounted, pk3 };
}

function parseMasterAddresses(text) {
  const re = /\b(\d{1,3}(?:\.\d{1,3}){3}):(\d{1,5})\b/g;
  const servers = [];
  const seen = new Set();
  let match;
  while ((match = re.exec(String(text || '')))) {
    const parts = match[1].split('.').map(Number);
    const port = Number(match[2]);
    if (parts.some((n) => n > 255) || port < 1 || port > 65535) {
      continue;
    }
    const addr = `${match[1]}:${port}`;
    if (seen.has(addr)) {
      continue;
    }
    seen.add(addr);
    servers.push(addr);
  }
  return servers;
}

async function fetchDpmasterServers() {
  const urls = [
    '/master-servers',
    '/http-proxy/dpmaster.deathmask.net/?game=warfork&showonlylist=1',
  ];
  for (const url of urls) {
    try {
      const res = await fetch(url, { cache: 'no-store' });
      if (!res.ok) {
        continue;
      }
      const text = await res.text();
      let servers = [];
      try {
        const json = JSON.parse(text);
        if (json && Array.isArray(json.servers)) {
          servers = json.servers;
        }
      } catch {
        servers = parseMasterAddresses(text);
      }
      if (servers.length) {
        console.log('[master] ' + servers.length + ' servers from ' + url);
        return servers;
      }
    } catch (err) {
      console.warn('[master] ' + url, err);
    }
  }
  return [];
}

function injectMasterServers(servers) {
  const mod = window.Module;
  if (!mod || typeof mod.ccall !== 'function' || !servers.length) {
    return 0;
  }
  let n = 0;
  for (const addr of servers) {
    try {
      mod.ccall('WASM_AddMasterServer', null, ['string'], [String(addr)]);
      n += 1;
    } catch (err) {
      console.warn('[master] inject failed', addr, err);
      break;
    }
  }
  return n;
}

function waitForRuntime(timeoutMs) {
  return new Promise((resolve, reject) => {
    if (window.Module && window.Module.calledRun) {
      resolve();
      return;
    }
    const timer = setTimeout(() => {
      reject(new Error('Timeout: le WASM n’a pas démarré (warfork.wasm / warfork.data). Rechargez avec Ctrl+F5.'));
    }, timeoutMs);
    const prev = window.Module.onRuntimeInitialized;
    window.Module.onRuntimeInitialized = () => {
      clearTimeout(timer);
      if (typeof prev === 'function') {
        prev();
      }
      resolve();
    };
  });
}

async function startGame() {
  document.body.classList.remove('has-error');
  loader.hidden = false;
  setStatus('Chargement WASM…');
  sizeCanvas();
  bindMenuMouseMap();

  let runtimeReady;
  window.Module = {
    canvas,
    websocketUrl: defaultWsUrl(),
    refreshMasterList: async () => {
      const servers = await fetchDpmasterServers();
      const n = injectMasterServers(servers);
      console.log('[master] injected ' + n);
    },
    elementPointerLock: false,
    locateFile: (path) => '/wasm/' + String(path).split('/').pop() + '?v=' + Date.now(),
    arguments: [
      '+set', 'developer', '0',
      '+set', 'r_multithreading', '0',
      '+set', 'vid_fullscreen', '0',
      '+set', 'vid_width', String(canvas.width),
      '+set', 'vid_height', String(canvas.height),
      '+set', 'm_filter', '0',
      '+set', 'm_raw', '1',
      '+set', 'sv_pure', '0',
    ],
    print: (text) => {
      const line = String(text ?? '').trim();
      if (!line) {
        return;
      }
      console.log(line);
      if (/Warfork Initialized/i.test(line)) {
        engineReady = true;
        if (loader) {
          loader.hidden = true;
        }
        document.body.classList.add('playing');
        canvas.focus();
      }
      if (!/FS_FOpenFile:|Registering GLSL|GL_|Missing image|WARNING: shader/i.test(line)) {
        setStatus(line.slice(0, 180));
      }
    },
    printErr: (text) => {
      const line = String(text ?? '').trim();
      if (!line) {
        return;
      }
      console.error(line);
      if (/FS_FOpenFile: can't find|Couldn't execute:|Steam initialization failed/i.test(line)) {
        setStatus(line.slice(0, 180));
        return;
      }
      showError(line);
    },
    onAbort: (reason) => {
      showError(reason);
    },
    setStatus: (text) => {
      if (text) {
        setStatus(text);
      }
    },
    monitorRunDependencies: (left) => {
      if (!window.Module.totalDependencies) {
        window.Module.totalDependencies = left;
      }
      const total = window.Module.totalDependencies || 1;
      const done = Math.max(0, total - left);
      setProgress(Math.round((done / total) * 100));
      setStatus(`Dépendances WASM ${done}/${total}`);
    },
  };
  runtimeReady = waitForRuntime(60000);

  try {
    await loadEngineScript('/wasm/warfork.js?v=' + Date.now());
    setStatus('Initialisation runtime…');
    await runtimeReady;
    setStatus('WASM loaded');
    const FS = window.FS || window.Module.FS;
    if (FS) {
      setStatus('Montage basewf (cfg, GLSL, pk3)…');
      await yieldPaint();
      const n = await mountGameFiles(FS);
      setStatus(n.mounted
        ? `${n.mounted} fichiers montés (${n.pk3} pk3)`
        : 'Aucun fichier de jeu');
      await yieldPaint();
    }
    setStatus('Chargement du jeu…');
    setProgress(100);
    await yieldPaint();
    if (typeof Module.callMain === 'function') {
      Module.callMain(Module.arguments || []);
    } else {
      throw new Error('Module.callMain indisponible');
    }
  } catch (err) {
    showError(err);
  }
}

function loadEngineScript(src) {
  return new Promise((resolve, reject) => {
    const script = document.createElement('script');
    script.src = src;
    script.onload = resolve;
    script.onerror = () => reject(new Error('Impossible de charger ' + src + '. Compilez d’abord avec make wasm.'));
    document.body.appendChild(script);
  });
}

canvas.addEventListener('click', () => {
  if (!isPhone() && window.__wfWantPointerLock && !(window.WarforkTouch && window.WarforkTouch.isActive())) {
    requestRawPointerLock();
  } else if (!isPhone() && document.pointerLockElement) {
    document.exitPointerLock();
  }
  unlockAudio();
  canvas.focus();
});

document.getElementById('btn-fullscreen')?.addEventListener('click', (event) => {
  event.preventDefault();
  unlockAudio();
  enterImmersive();
});

document.addEventListener('pointerdown', () => {
  if (isPhone()) {
    enterImmersive();
  }
}, { capture: true, once: true });

document.addEventListener('pointerlockchange', () => {
  if (document.pointerLockElement === canvas) {
    unlockAudio();
  }
});

window.addEventListener('resize', () => {
  sizeCanvas();
});
window.addEventListener('orientationchange', () => {
  setTimeout(sizeCanvas, 250);
});
if (window.visualViewport) {
  window.visualViewport.addEventListener('resize', () => {
    sizeCanvas();
  });
}

startGame();
