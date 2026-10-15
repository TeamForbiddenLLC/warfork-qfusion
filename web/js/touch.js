(function () {
  const KEY_GAME = 0;
  const STORAGE_KEY = 'warfork-touch-layout-v4';
  const overlay = document.getElementById('touch-overlay');
  if (!overlay) {
    return;
  }

  const stickKnob = overlay.querySelector('.touch-stick-knob');
  const lookPad = overlay.querySelector('.touch-look');
  const lookKnob = overlay.querySelector('.touch-look-knob');
  const movable = overlay.querySelectorAll('[data-id]');
  const hold = Object.create(null);
  const pointers = new Map();
  let enabled = false;
  let inGame = false;
  let lookOn = true;
  let lookSens = 2.5;
  let editing = false;
  let layoutHoldOff = 0;
  let lastDx = 0;
  let lastDy = 0;

  const defaults = {
    stick: { x: 0.04, y: 0.68 },
    look: { x: 0.58, y: 0.62 },
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
    if (lookKnob) {
      lookKnob.style.transform = 'translate(-50%, -50%)';
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
      if (id === 'look') {
        return;
      }
      const rect = el.getBoundingClientRect();
      const vw = Math.max(1, window.innerWidth);
      const vh = Math.max(1, window.innerHeight);
      layout[id] = {
        x: rect.left / vw,
        y: rect.top / vh,
      };
      if (id === 'fire') {
        layout[id].w = rect.width / vw;
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
    const id = el.getAttribute('data-id');
    if (id === 'look') {
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
    layoutHoldOff = 0;
    releaseAll();
    pointers.clear();
    sendCmd('in_touchlayout 0');
    refreshVisible();
  }

  function exitEdit() {
    editing = false;
    layoutHoldOff = 30;
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

  function handleEditAction(action) {
    if (action === 'open') {
      enterEdit();
    } else if (action === 'done') {
      exitEdit();
    } else if (action === 'reset') {
      resetLayout();
    }
  }

  overlay.addEventListener('click', (event) => {
    const editBtn = event.target.closest('[data-edit]');
    if (!editBtn) {
      return;
    }
    event.preventDefault();
    event.stopPropagation();
    handleEditAction(editBtn.getAttribute('data-edit'));
  }, { capture: true });

  function controlFromPoint(x, y) {
    const el = document.elementFromPoint(x, y);
    if (!el) {
      return lookOn && !editing ? lookPad : null;
    }
    const editBtn = el.closest('[data-edit]');
    if (editBtn) {
      return editBtn;
    }
    const idel = el.closest('[data-id]');
    if (idel && idel.getAttribute('data-id') !== 'look') {
      return idel;
    }
    return lookOn && !editing ? lookPad : null;
  }

  function beginPointer(id, clientX, clientY, targetEl) {
    if (pointers.has(id)) {
      return;
    }
    const editBtn = targetEl && targetEl.closest ? targetEl.closest('[data-edit]') : null;
    if (editBtn) {
      handleEditAction(editBtn.getAttribute('data-edit'));
      return;
    }
    let target = targetEl && targetEl.closest ? targetEl.closest('[data-id]') : null;
    if (target && target.getAttribute('data-id') === 'look') {
      target = lookPad;
    }
    if (!target || target.getAttribute('data-id') === 'look') {
      const hit = controlFromPoint(clientX, clientY);
      if (hit && hit.getAttribute('data-id') !== 'look') {
        target = hit;
      } else if (lookOn && !editing) {
        target = lookPad;
      }
    }
    if (!target || !target.getAttribute) {
      return;
    }
    if (target.getAttribute('data-edit')) {
      handleEditAction(target.getAttribute('data-edit'));
      return;
    }
    const role = target.getAttribute('data-role');
    const cmd = target.getAttribute('data-cmd');
    if (editing && role === 'look') {
      return;
    }
    const rect = target.getBoundingClientRect();
    pointers.set(id, {
      role,
      cmd,
      el: target,
      editing,
      cx: rect.left + rect.width / 2,
      cy: rect.top + rect.height / 2,
      radius: Math.min(rect.width, rect.height) / 2,
      lastX: clientX,
      lastY: clientY,
      offX: clientX - rect.left,
      offY: clientY - rect.top,
    });
    if (role !== 'look') {
      target.classList.add('active');
    }
    if (editing) {
      return;
    }
    if (role === 'stick') {
      updateStick(rect.left + rect.width / 2, rect.top + rect.height / 2, clientX, clientY, Math.min(rect.width, rect.height) / 2);
    } else if (role === 'look' && lookOn) {
      lastDx = 0;
      lastDy = 0;
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
  }

  function movePointer(id, clientX, clientY) {
    const info = pointers.get(id);
    if (!info) {
      return;
    }
    if (info.editing) {
      if (info.role === 'look') {
        return;
      }
      const vw = Math.max(1, window.innerWidth);
      const vh = Math.max(1, window.innerHeight);
      const w = info.el.offsetWidth;
      const h = info.el.offsetHeight;
      let left = clientX - info.offX;
      let top = clientY - info.offY;
      left = Math.max(0, Math.min(vw - w, left));
      top = Math.max(0, Math.min(vh - h, top));
      info.el.style.left = ((left / vw) * 100) + '%';
      info.el.style.top = ((top / vh) * 100) + '%';
      info.el.style.right = 'auto';
      info.el.style.bottom = 'auto';
      return;
    }
    if (info.role === 'stick') {
      updateStick(info.cx, info.cy, clientX, clientY, info.radius);
    } else if (info.role === 'look' && lookOn) {
      lastDx += clientX - info.lastX;
      lastDy += clientY - info.lastY;
      info.lastX = clientX;
      info.lastY = clientY;
    }
  }

  function endPointerById(id) {
    const info = pointers.get(id);
    if (!info) {
      return;
    }
    pointers.delete(id);
    info.el.classList.remove('active');
    if (info.editing) {
      saveLayout();
      return;
    }
    if (info.role === 'stick') {
      applyMove(0, 0);
      stickKnob.style.transform = 'translate(-50%, -50%)';
    } else if (info.role === 'look') {
      lastDx = 0;
      lastDy = 0;
    } else if (info.role === 'button' && info.cmd && info.cmd.charAt(0) === '+') {
      setHold(info.cmd.slice(1), false);
    }
  }

  overlay.addEventListener('pointerdown', (event) => {
    if (event.pointerType === 'touch') {
      return;
    }
    event.preventDefault();
    event.stopPropagation();
    beginPointer(event.pointerId, event.clientX, event.clientY, event.target);
    try {
      overlay.setPointerCapture(event.pointerId);
    } catch (err) {
      /* ignore */
    }
  }, { capture: true });

  overlay.addEventListener('pointermove', (event) => {
    if (event.pointerType === 'touch') {
      return;
    }
    if (!pointers.has(event.pointerId)) {
      return;
    }
    event.preventDefault();
    movePointer(event.pointerId, event.clientX, event.clientY);
  }, { capture: true });

  function endMousePointer(event) {
    if (event.pointerType === 'touch') {
      return;
    }
    endPointerById(event.pointerId);
  }
  overlay.addEventListener('pointerup', endMousePointer, { capture: true });
  overlay.addEventListener('pointercancel', endMousePointer, { capture: true });

  overlay.addEventListener('touchstart', (event) => {
    event.preventDefault();
    event.stopPropagation();
    for (let i = 0; i < event.changedTouches.length; i++) {
      const t = event.changedTouches[i];
      beginPointer('t' + t.identifier, t.clientX, t.clientY, event.target);
    }
  }, { capture: true, passive: false });

  function onTouchMove(event) {
    let used = false;
    for (let i = 0; i < event.changedTouches.length; i++) {
      const t = event.changedTouches[i];
      const id = 't' + t.identifier;
      if (pointers.has(id)) {
        used = true;
        movePointer(id, t.clientX, t.clientY);
      }
    }
    if (used) {
      event.preventDefault();
    }
  }
  window.addEventListener('touchmove', onTouchMove, { capture: true, passive: false });

  function onTouchEnd(event) {
    for (let i = 0; i < event.changedTouches.length; i++) {
      endPointerById('t' + event.changedTouches[i].identifier);
    }
  }
  window.addEventListener('touchend', onTouchEnd, { capture: true, passive: false });
  window.addEventListener('touchcancel', onTouchEnd, { capture: true, passive: false });

  let lookFracX = 0;
  let lookFracY = 0;

  function pumpLook() {
    if (!editing && lookOn) {
      lookFracX += lastDx * lookSens;
      lookFracY += lastDy * lookSens;
      lastDx = 0;
      lastDy = 0;
      const ix = lookFracX | 0;
      const iy = lookFracY | 0;
      lookFracX -= ix;
      lookFracY -= iy;
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
      overlay.classList.toggle('in-game', enabled && inGame);
      lookPad.hidden = false;
      lookPad.style.display = lookOn && !editing ? '' : 'none';
      if (layoutHoldOff > 0) {
        layoutHoldOff -= 1;
        if (layoutFlag) {
          sendCmd('in_touchlayout 0');
        }
      } else if (layoutFlag && !editing) {
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
