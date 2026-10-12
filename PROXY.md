# Proxy WebSocket ↔ UDP

## Rôle

Le navigateur ne peut pas ouvrir de sockets UDP. Chaque client WASM ouvre un **WebSocket binaire** vers Node.js. Le proxy crée un socket UDP local par client et ne l’envoie **qu’** vers un serveur Warfork de la liste blanche.

```
Chrome / Firefox
  Warfork WASM  --WebSocket-->  Node.js (HTTP + WS + UDP)  --UDP-->  wf_server
```

## Configuration (`.env`)

| Variable | Défaut | Rôle |
| --- | --- | --- |
| `WEB_HOST` | `0.0.0.0` | Adresse d’écoute HTTP/WS |
| `WEB_PORT` | `8081` | Port HTTP (le WS est sur le même port) |
| `WS_PATH` | `/ws` | Chemin WebSocket |
| `WARFORK_HOST` | `127.0.0.1` | Cible UDP par défaut |
| `WARFORK_PORT` | `44400` | Port UDP Warfork |
| `ALLOWED_SERVERS` | `127.0.0.1:44400` | Liste `host:port` autorisés |
| `DEBUG` | `false` | Tailles de paquets et erreurs détaillées |

Un client **ne peut pas** faire envoyer le proxy vers une IP arbitraire.

## Framing

Chaque datagramme Warfork est encapsulé :

```
[4 octets big-endian = longueur][payload]
```

Un message WebSocket peut contenir un ou plusieurs cadres. Le proxy reconstitue les frontières de paquets avant `sendto`.

## Logs

```
[WS] Client connected
[WS] Client disconnected
[UDP] Connected to Warfork server
[UDP] Packet received
[UDP] Packet sent
[PROXY] Client mapping created
[PROXY] Client mapping removed
```

Avec `DEBUG=true`, la taille de chaque paquet est affichée, pas le contenu.

## Plusieurs joueurs

Chaque WebSocket a son propre socket UDP (port éphémère). Le dédié Warfork voit des clients distincts. Deux navigateurs = deux mappings.

## Déconnexion

Fermeture WS → fermeture UDP → mapping supprimé. Relancer le dédié : les paquets suivants échouent jusqu’à ce qu’il réécoute ; le WS reste ouvert.

## Message de contrôle (optionnel)

Texte JSON, uniquement si la cible est dans `ALLOWED_SERVERS` :

```json
{"cmd":"connect","host":"127.0.0.1","port":44400}
```
