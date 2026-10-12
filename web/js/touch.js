(function () {
  const KEY_GAME = 0;
  const STORAGE_KEY = 'warfork-touch-layout-v1';
  const overlay = document.getElementById('touch-overlay');
  if (!overlay) {
    return;
  }

  const stickKnob = overlay.querySelector('.touch-stick-knob');
  const lookPad = overlay.querySelector('.touch-look');
  const movable = overlay.querySelectorAll('[data-id]');
  const hold = Object.create(null);
  const pointers = new Map();
  let enabled = false;
  let inGame = false;
  let lookOn = true;
  let lookSens = 2.5;
  let editing = false;
  let lastDx = 0;
  let lastDy = 0;

  const defaults = {
    stick: { x: 0.04, y: 0.68 },
    look: { x: 0.48, y: 0.12, w: 0.52, h: 0.52 },
    jump: { x: 0.70, y: 0.62 },
    special: { x: 0.82, y: 0.62 },
    prev: { x: 0.94, y: 0.62 },
    fire: { x: 0.70, y: 0.74, w: 0.22 },
    next: { x: 0.94, y: 0.74 },
    duck: { x: 0.70, y: 0.86 },
    menu: { x: 0.82, y: 0.86 },
  };

  function ccall(name, ret, types, args) {
    const mod = window.Module;
    if (!mod || typeof mod.ccall !== 'function') {
      return;
    }
    try {
      return mod.ccall(name, ret, types, args);
    } catch (err) {
      /* engine not ready */
    }
  }

  function sendCmd(cmd) {
    if (!cmd) {
      return;
    }
    ccall('WASM_TouchCmd', null, ['string'], [cmd]);
  }

  function setHold(cmd, down) {
    const on = !!down;
    if (!!hold[cmd] === on) {
      return;
    }
    hold[cmd] = on;
    sendCmd((on ? '+' : '-') + cmd);
  }

  function releaseAll() {
    Object.keys(hold).forEach((cmd) => {
      if (hold[cmd]) {
        hold[cmd] = false;
        sendCmd('-' + cmd);
      }
    });
    lastDx = 0;
    lastDy = 0;
    if (stickKnob) {
      stickKnob.style.transform = 'translate(-50%, -50%)';
    }
  }

  function applyMove(nx, ny) {
    const dead = 0.28;
    setHold('forward', ny < -dead);
    setHold('back', ny > dead);
    setHold('moveleft', nx < -dead);
    setHold('moveright', nx > dead);
  }

  function loadLayout() {
    try {
      const raw = localStorage.getItem(STORAGE_KEY);
      if (!raw) {
        return { ...defaults };
      }
      return { ...defaults, ...JSON.parse(raw) };
    } catch (err) {
      return { ...defaults };
    }
  }

  function saveLayout() {
    const layout = {};
    movable.forEach((el) => {
      const id = el.getAttribute('data-id');
      const rect = el.getBoundingClientRect();
      const vw = Math.max(1, window.innerWidth);
      const vh = Math.max(1, window.innerHeight);
      layout[id] = {
        x: rect.left / vw,
        y: rect.top / vh,
      };
      if (id === 'look' || id === 'fire') {
        layout[id].w = rect.width / vw;
        if (id === 'look') {
          layout[id].h = rect.height / vh;
        }
      }
    });
    try {
      localStorage.setItem(STORAGE_KEY, JSON.stringify(layout));
    } catch (err) {
      /* ignore quota */
    }
  }

  function place(el, pos) {
    if (!pos) {
      return;
    }
    el.style.left = (pos.x * 100) + '%';
    el.style.top = (pos.y * 100) + '%';
    el.style.right = 'auto';
    el.style.bottom = 'auto';
    if (pos.w) {
      el.style.width = (pos.w * 100) + '%';
    }
    if (pos.h) {
      el.style.height = (pos.h * 100) + '%';
    }
  }

  function applyLayout(layout) {
    movable.forEach((el) => {
      place(el, layout[el.getAttribute('data-id')]);
    });
  }

  function setVisible(show) {
    overlay.hidden = !show;
    overlay.style.display = show ? 'block' : 'none';
    document.body.classList.toggle('touch-on', show);
    overlay.classList.toggle('editing', editing);
    if (!show) {
      pointers.clear();
      releaseAll();
    }
  }

  function refreshVisible() {
    setVisible(editing || (enabled && inGame));
  }

  function enterEdit() {
    editing = true;
    releaseAll();
    pointers.clear();
    sendCmd('in_touchlayout 1');
    refreshVisible();
  }

  function exitEdit() {
    editing = false;
    saveLayout();
    sendCmd('in_touchlayout 0');
    refreshVisible();
  }

  function resetLayout() {
    try {
      localStorage.removeItem(STORAGE_KEY);
    } catch (err) {
      /* ignore */
    }
    applyLayout(defaults);
  }

  function updateStick(cx, cy, px, py, radius) {
    let dx = px - cx;
    let dy = py - cy;
    const len = Math.hypot(dx, dy) || 1;
    const max = radius * 0.55;
    if (len > max) {
      dx = (dx / len) * max;
      dy = (dy / len) * max;
    }
    stickKnob.style.transform = `translate(calc(-50% + ${dx}px), calc(-50% + ${dy}px))`;
    applyMove(dx / max, dy / max);
  }

  overlay.addEventListener('pointerdown', (event) => {
    const editBtn = event.target.closest('[data-edit]');
    if (editBtn) {
      event.preventDefault();
      event.stopPropagation();
      const action = editBtn.getAttribute('data-edit');
      if (action === 'open') {
        enterEdit();
      } else if (action === 'done') {
        exitEdit();
      } else if (action === 'reset') {
        resetLayout();
      }
      return;
    }

    const target = event.target.closest('[data-id]');
    if (!target) {
      return;
    }
    event.preventDefault();
    event.stopPropagation();
    overlay.setPointerCapture(event.pointerId);
    const role = target.getAttribute('data-role');
    const cmd = target.getAttribute('data-cmd');
    const rect = target.getBoundingClientRect();
    pointers.set(event.pointerId, {
      role,
      cmd,
      el: target,
      editing,
      cx: rect.left + rect.width / 2,
      cy: rect.top + rect.height / 2,
      radius: Math.min(rect.width, rect.height) / 2,
      lastX: event.clientX,
      lastY: event.clientY,
      offX: event.clientX - rect.left,
      offY: event.clientY - rect.top,
    });
    target.classList.add('active');
    if (editing) {
      return;
    }
    if (role === 'stick') {
      updateStick(rect.left + rect.width / 2, rect.top + rect.height / 2, event.clientX, event.clientY, Math.min(rect.width, rect.height) / 2);
    } else if (role === 'button' && cmd) {
      if (cmd === 'escape') {
        ccall('WASM_TouchKey', null, ['number', 'number'], [27, 1]);
        ccall('WASM_TouchKey', null, ['number', 'number'], [27, 0]);
      } else if (cmd.charAt(0) === '+') {
        setHold(cmd.slice(1), true);
      } else {
        sendCmd(cmd);
      }
    }
  }, { capture: true });

  overlay.addEventListener('pointermove', (event) => {
    const info = pointers.get(event.pointerId);
    if (!info) {
      return;
    }
    event.preventDefault();
    event.stopPropagation();
    if (info.editing) {
      const vw = Math.max(1, window.innerWidth);
      const vh = Math.max(1, window.innerHeight);
      const w = info.el.offsetWidth;
      const h = info.el.offsetHeight;
      let left = event.clientX - info.offX;
      let top = event.clientY - info.offY;
      left = Math.max(0, Math.min(vw - w, left));
      top = Math.max(0, Math.min(vh - h, top));
      info.el.style.left = ((left / vw) * 100) + '%';
      info.el.style.top = ((top / vh) * 100) + '%';
      info.el.style.right = 'auto';
      info.el.style.bottom = 'auto';
      return;
    }
    if (info.role === 'stick') {
      updateStick(info.cx, info.cy, event.clientX, event.clientY, info.radius);
    } else if (info.role === 'look' && lookOn) {
      lastDx += event.clientX - info.lastX;
      lastDy += event.clientY - info.lastY;
      info.lastX = event.clientX;
      info.lastY = event.clientY;
    }
  }, { capture: true });

  function endPointer(event) {
    const info = pointers.get(event.pointerId);
    if (!info) {
      return;
    }
    event.preventDefault();
    event.stopPropagation();
    pointers.delete(event.pointerId);
    info.el.classList.remove('active');
    if (info.editing) {
      saveLayout();
      return;
    }
    if (info.role === 'stick') {
      applyMove(0, 0);
      stickKnob.style.transform = 'translate(-50%, -50%)';
    } else if (info.role === 'button' && info.cmd && info.cmd.charAt(0) === '+') {
      setHold(info.cmd.slice(1), false);
    }
  }

  overlay.addEventListener('pointerup', endPointer, { capture: true });
  overlay.addEventListener('pointercancel', endPointer, { capture: true });

  function pumpLook() {
    if (!editing && (lastDx || lastDy)) {
      const sx = lastDx * lookSens;
      const sy = lastDy * lookSens;
      lastDx = 0;
      lastDy = 0;
      const ix = sx | 0;
      const iy = sy | 0;
      if (ix || iy) {
        ccall('WASM_TouchMove', null, ['number', 'number'], [ix, iy]);
      }
    }
    requestAnimationFrame(pumpLook);
  }
  requestAnimationFrame(pumpLook);

  window.WarforkTouch = {
    sync(keyDest, on, scale, opacity, look, lookSens100, layoutFlag) {
      enabled = !!on;
      inGame = keyDest === KEY_GAME;
      lookOn = !!look;
      lookSens = Math.max(0.2, (Number(lookSens100) || 250) / 100);
      const sc = Math.max(60, Math.min(160, Number(scale) || 100)) / 100;
      const op = Math.max(15, Math.min(90, Number(opacity) || 55)) / 100;
      overlay.style.setProperty('--touch-scale', String(sc));
      overlay.style.setProperty('--touch-opacity', String(op));
      lookPad.hidden = !lookOn && !editing;
      if (layoutFlag && !editing) {
        enterEdit();
      }
      refreshVisible();
    },
    isActive() {
      return editing || (enabled && inGame);
    },
  };

  applyLayout(loadLayout());
  overlay.addEventListener('contextmenu', (event) => event.preventDefault());
  window.addEventListener('resize', () => applyLayout(loadLayout()));
})();
