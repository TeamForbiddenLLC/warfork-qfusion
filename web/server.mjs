import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import dgram from 'node:dgram';
import dns from 'node:dns';
import net from 'node:net';
import { WebSocketServer } from 'ws';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(__dirname, '..');

function loadEnv(file) {
  const envPath = path.join(root, file);
  if (!fs.existsSync(envPath)) {
    return;
  }
  for (const line of fs.readFileSync(envPath, 'utf8').split(/\r?\n/)) {
    const trimmed = line.trim();
    if (!trimmed || trimmed.startsWith('#')) {
      continue;
    }
    const eq = trimmed.indexOf('=');
    if (eq <= 0) {
      continue;
    }
    const key = trimmed.slice(0, eq).trim();
    const value = trimmed.slice(eq + 1).trim();
    if (process.env[key] === undefined) {
      process.env[key] = value;
    }
  }
}

loadEnv('.env');

const WEB_HOST = process.env.WEB_HOST || '0.0.0.0';
const WEB_PORT = Number(process.env.WEB_PORT || 8081);
const WARFORK_HOST = process.env.WARFORK_HOST || '127.0.0.1';
const WARFORK_PORT = Number(process.env.WARFORK_PORT || 44400);
const WS_PATH = process.env.WS_PATH || '/ws';
const DEBUG = String(process.env.DEBUG || '').toLowerCase() === 'true';
const ALLOWED_SERVERS = (process.env.ALLOWED_SERVERS || `${WARFORK_HOST}:${WARFORK_PORT}`)
  .split(',')
  .map((entry) => entry.trim())
  .filter(Boolean);

const MIME = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'application/javascript; charset=utf-8',
  '.mjs': 'application/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.wasm': 'application/wasm',
  '.data': 'application/octet-stream',
  '.json': 'application/json; charset=utf-8',
  '.png': 'image/png',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon',
  '.map': 'application/json; charset=utf-8',
  '.cfg': 'text/plain; charset=utf-8',
  '.glsl': 'text/plain; charset=utf-8',
  '.txt': 'text/plain; charset=utf-8',
  '.shader': 'text/plain; charset=utf-8',
  '.pk3': 'application/octet-stream',
  '.pk2': 'application/octet-stream',
};

function log(...args) {
  console.log(...args);
}

function debug(...args) {
  if (DEBUG) {
    console.log(...args);
  }
}

function isAllowedTarget(host, port) {
  const key = `${host}:${port}`;
  return ALLOWED_SERVERS.includes(key);
}

function isInside(parent, candidate) {
  const rel = path.relative(parent, candidate);
  return rel && !rel.startsWith('..') && !path.isAbsolute(rel);
}

function isFile(filePath) {
  try {
    return fs.existsSync(filePath) && fs.statSync(filePath).isFile();
  } catch {
    return false;
  }
}

function walkFiles(dir, prefix, out) {
  if (!fs.existsSync(dir)) {
    return;
  }
  for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
    if (entry.name === '.' || entry.name === '..') {
      continue;
    }
    const rel = prefix ? `${prefix}/${entry.name}` : entry.name;
    const full = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      walkFiles(full, rel, out);
    } else if (entry.isFile()) {
      const posix = rel.replace(/\\/g, '/');
      out.set(posix, { path: posix, size: fs.statSync(full).size });
    }
  }
}

function buildGameManifest() {
  const files = new Map();
  const webGame = path.join(root, 'web', 'game');
  walkFiles(webGame, '', files);
  files.delete('manifest.json');
  return [...files.values()].sort((a, b) => a.path.localeCompare(b.path));
}

function resolveGameFile(rel) {
  const safe = path.posix.normalize(String(rel || '').replace(/\\/g, '/')).replace(/^\/+/, '');
  if (!safe || safe === '.' || safe.includes('..')) {
    return null;
  }
  const webGame = path.join(root, 'web', 'game', ...safe.split('/'));
  if (isInside(path.join(root, 'web', 'game'), webGame) && isFile(webGame)) {
    return webGame;
  }
  return null;
}

function corsHeaders() {
  return {
    'Cache-Control': 'no-cache',
    'Cross-Origin-Opener-Policy': 'same-origin',
    'Cross-Origin-Embedder-Policy': 'require-corp',
    'Cross-Origin-Resource-Policy': 'same-origin',
  };
}

function sendFile(res, filePath) {
  let stat;
  try {
    stat = fs.statSync(filePath);
  } catch {
    res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
    res.end('Not found');
    return;
  }
  const ext = path.extname(filePath).toLowerCase();
  res.writeHead(200, {
    'Content-Type': MIME[ext] || 'application/octet-stream',
    'Content-Length': stat.size,
    ...corsHeaders(),
  });
  fs.createReadStream(filePath).pipe(res);
}

const httpServer = http.createServer((req, res) => {
  const url = new URL(req.url || '/', `http://${req.headers.host || 'localhost'}`);
  let pathname = decodeURIComponent(url.pathname);
  if (pathname === '/') {
    pathname = '/index.html';
  }
  if (pathname === '/favicon.ico') {
    res.writeHead(204);
    res.end();
    return;
  }

  if (pathname === '/dns-lookup') {
    lookupDns(req, res, url.searchParams.get('host') || '');
    return;
  }

  if (pathname.startsWith('/http-proxy/')) {
    proxyHttp(req, res, pathname, url.search);
    return;
  }

  if (pathname === '/game/manifest.json') {
    const manifest = buildGameManifest();
    const body = Buffer.from(JSON.stringify(manifest));
    res.writeHead(200, {
      'Content-Type': 'application/json; charset=utf-8',
      'Content-Length': body.length,
      ...corsHeaders(),
    });
    res.end(body);
    return;
  }

  if (pathname.startsWith('/game/')) {
    const gameFile = resolveGameFile(pathname.slice('/game/'.length));
    if (!gameFile) {
      res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
      res.end('Not found');
      return;
    }
    sendFile(res, gameFile);
    return;
  }

  const webRoot = path.join(root, 'web');
  const relative = pathname.replace(/^\/+/, '');
  const candidates = [
    path.join(webRoot, relative),
    path.join(webRoot, 'wasm', relative),
  ];

  const match = candidates.find((candidate) => fs.existsSync(candidate) && fs.statSync(candidate).isFile());
  if (!match) {
    res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
    res.end('Not found');
    return;
  }
  sendFile(res, match);
});

const wss = new WebSocketServer({ server: httpServer, path: WS_PATH });
let nextClientId = 1;
const clients = new Map();


function parseFrames(buffer) {
  const packets = [];
  let offset = 0;
  while (buffer.length - offset >= 4) {
    const size = buffer.readUInt32BE(offset);
    if (size < 6 || size > 65536) {
      throw new Error(`invalid framed length ${size}`);
    }
    if (buffer.length - offset < 4 + size) {
      break;
    }
    packets.push(buffer.subarray(offset + 4, offset + 4 + size));
    offset += 4 + size;
  }
  return { packets, rest: buffer.subarray(offset) };
}

function frameUdpPacket(msg, rinfo) {
  const header = Buffer.alloc(10);
  header.writeUInt32BE(msg.length + 6, 0);
  const ip = String(rinfo.address || '').split('.');
  header[4] = Number(ip[0]) || 0;
  header[5] = Number(ip[1]) || 0;
  header[6] = Number(ip[2]) || 0;
  header[7] = Number(ip[3]) || 0;
  header.writeUInt16BE(rinfo.port || 0, 8);
  return Buffer.concat([header, msg]);
}

function parseUdpFrame(packet) {
  if (!packet || packet.length < 6) {
    return null;
  }
  const ip = `${packet[0]}.${packet[1]}.${packet[2]}.${packet[3]}`;
  const port = packet.readUInt16BE(4);
  return { ip, port, payload: packet.subarray(6) };
}

function lookupDns(req, res, host) {
  const name = String(host || '').trim().toLowerCase();
  if (!/^[a-z0-9][a-z0-9.-]{0,253}$/.test(name) || name.includes('..')) {
    res.writeHead(400, { 'Content-Type': 'text/plain; charset=utf-8', ...corsHeaders() });
    res.end('Bad host');
    return;
  }
  dns.lookup(name, { family: 4 }, (err, address) => {
    if (err || !address) {
      log(`[DNS] ${name} failed: ${err ? err.message : 'no address'}`);
      res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8', ...corsHeaders() });
      res.end('not found');
      return;
    }
    log(`[DNS] ${name} -> ${address}`);
    res.writeHead(200, { 'Content-Type': 'text/plain; charset=utf-8', ...corsHeaders() });
    res.end(address);
  });
}

function proxyHttp(req, res, pathname, search) {
  const rest = pathname.slice('/http-proxy/'.length);
  if (!rest) {
    res.writeHead(400, { 'Content-Type': 'text/plain; charset=utf-8', ...corsHeaders() });
    res.end('Bad proxy path');
    return;
  }
  const slash = rest.indexOf('/');
  const hostport = slash < 0 ? rest : rest.slice(0, slash);
  const pathPart = (slash < 0 ? '/' : rest.slice(slash)) + (search || '');
  const colon = hostport.lastIndexOf(':');
  const hostname = colon >= 0 ? hostport.slice(0, colon) : hostport;
  const port = colon >= 0 ? Number(hostport.slice(colon + 1)) || 80 : 80;
  const reqPath = pathPart.startsWith('/') ? pathPart : `/${pathPart}`;
  const target = `http://${hostport}${reqPath}`;
  log(`[HTTP-PROXY] ${target}`);

  let chunks = [];
  let settled = false;
  const finish = (status, body, contentType) => {
    if (settled || res.writableEnded) {
      return;
    }
    settled = true;
    log(`[HTTP-PROXY] ${target} -> ${status} ${body.length} bytes`);
    if (!res.headersSent) {
      res.writeHead(status, {
        'Content-Type': contentType || 'application/octet-stream',
        'Content-Length': body.length,
        ...corsHeaders(),
      });
    }
    res.end(body);
  };

  const socket = net.connect({ host: hostname, port, family: 4 }, () => {
    socket.write(
      `GET ${reqPath} HTTP/1.0\r\n` +
        `Host: ${hostport}\r\n` +
        `User-Agent: Warsow\r\n` +
        `Accept: */*\r\n` +
        `Connection: close\r\n\r\n`
    );
  });
  socket.setTimeout(12000);
  socket.on('data', (chunk) => {
    chunks.push(chunk);
  });
  socket.on('end', () => {
    const buf = Buffer.concat(chunks);
    const split = buf.indexOf('\r\n\r\n');
    if (split >= 0 && buf.subarray(0, 5).toString('ascii').startsWith('HTTP/')) {
      const head = buf.subarray(0, split).toString('latin1');
      const body = buf.subarray(split + 4);
      const statusLine = head.split('\r\n')[0] || '';
      const status = Number((statusLine.split(' ')[1]) || 200) || 200;
      const typeMatch = head.match(/content-type:\s*([^\r\n]+)/i);
      finish(status, body, typeMatch ? typeMatch[1].trim() : 'application/octet-stream');
      return;
    }
    // Some masters speak raw binary on TCP and hang up; forward the payload anyway.
    finish(buf.length ? 200 : 502, buf);
  });
  socket.on('timeout', () => {
    socket.destroy();
    finish(504, Buffer.from('Upstream timeout'), 'text/plain; charset=utf-8');
  });
  socket.on('error', (err) => {
    log(`[HTTP-PROXY] error ${err.message}`);
    finish(502, Buffer.from(String(err.message)), 'text/plain; charset=utf-8');
  });
}

wss.on('connection', (ws, req) => {
  const id = nextClientId++;
  const udp = dgram.createSocket('udp4');
  const mapping = {
    id,
    ws,
    udp,
    targetHost: WARFORK_HOST,
    targetPort: WARFORK_PORT,
    rx: Buffer.alloc(0),
    remote: req.socket.remoteAddress,
  };
  clients.set(id, mapping);
  log(`[WS] Client connected id=${id} from=${mapping.remote}`);
  log(`[PROXY] Client mapping created id=${id} -> ${mapping.targetHost}:${mapping.targetPort}`);

  udp.on('message', (msg, rinfo) => {
    if (ws.readyState !== ws.OPEN) {
      return;
    }
    if (rinfo.port === 27951 || rinfo.port === 27950 || rinfo.port === 27777 || rinfo.port === 42863) {
      log(`[UDP] Master reply id=${id} from ${rinfo.address}:${rinfo.port} size=${msg.length}`);
    } else {
      debug(`[UDP] Packet received id=${id} ${rinfo.address}:${rinfo.port} size=${msg.length}`);
    }
    ws.send(frameUdpPacket(msg, rinfo), { binary: true });
  });

  udp.on('error', (err) => {
    log(`[UDP] error id=${id}: ${err.message}`);
  });

  udp.bind(0, () => {
    const addr = udp.address();
    log(`[UDP] Connected to Warfork server via local ${addr.address}:${addr.port} -> ${mapping.targetHost}:${mapping.targetPort}`);
  });

  ws.on('message', (data, isBinary) => {
    if (!isBinary) {
      try {
        const msg = JSON.parse(String(data));
        if (msg && msg.cmd === 'connect' && msg.host && msg.port) {
          const port = Number(msg.port);
          if (!isAllowedTarget(msg.host, port)) {
            log(`[PROXY] Rejected connect id=${id} ${msg.host}:${port}`);
            ws.send(JSON.stringify({ error: 'target not allowed' }));
            return;
          }
          mapping.targetHost = msg.host;
          mapping.targetPort = port;
          log(`[PROXY] Client mapping updated id=${id} -> ${mapping.targetHost}:${mapping.targetPort}`);
        }
      } catch (err) {
        debug(`[WS] ignored text from id=${id}: ${err.message}`);
      }
      return;
    }

    const chunk = Buffer.isBuffer(data) ? data : Buffer.from(data);
    mapping.rx = Buffer.concat([mapping.rx, chunk]);
    try {
      const { packets, rest } = parseFrames(mapping.rx);
      mapping.rx = rest;
      for (const packet of packets) {
        const framed = parseUdpFrame(packet);
        const host = framed ? framed.ip : mapping.targetHost;
        const port = framed ? framed.port : mapping.targetPort;
        const payload = framed ? framed.payload : packet;
        if (port === 27951 || port === 27950) {
          log(`[UDP] Master query id=${id} size=${payload.length} -> ${host}:${port}`);
        } else {
          debug(`[UDP] Packet sent id=${id} size=${payload.length} -> ${host}:${port}`);
        }
        udp.send(payload, port, host);
      }
    } catch (err) {
      log(`[WS] framing error id=${id}: ${err.message}`);
      mapping.rx = Buffer.alloc(0);
    }
  });

  const cleanup = () => {
    if (!clients.has(id)) {
      return;
    }
    clients.delete(id);
    try {
      udp.close();
    } catch {
      /* already closed */
    }
    log(`[WS] Client disconnected id=${id}`);
    log(`[PROXY] Client mapping removed id=${id}`);
  };

  ws.on('close', cleanup);
  ws.on('error', cleanup);
});

httpServer.listen(WEB_PORT, WEB_HOST, () => {
  const displayHost = WEB_HOST === '0.0.0.0' ? 'localhost' : WEB_HOST;
  log(`Web server listening on http://${displayHost}:${WEB_PORT}`);
  log(`WebSocket proxy listening on ws://${displayHost}:${WEB_PORT}${WS_PATH}`);
  log(`Warfork UDP server: ${WARFORK_HOST}:${WARFORK_PORT}`);
  log(`Allowed UDP targets: ${ALLOWED_SERVERS.join(', ')}`);
});
