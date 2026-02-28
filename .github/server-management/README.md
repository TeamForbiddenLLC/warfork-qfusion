# Warfork Server Management

Automated deployment of Warfork dedicated servers via GitHub Actions.

## Setup

### Server prerequisites

Each server needs to be a Linux box with SSH access. The `steam` user is assumed by default in `server-config.json`.

The `steam` user needs passwordless sudo to run the initial setup:

```shell
steam ALL=(ALL) NOPASSWD: /bin/bash /home/steam/scripts/Warfork.sh setup
```

After that, everything the deploy workflow does (install, update, start, stop) runs as `steam` without sudo.

### GitHub secrets

Go to **Settings → Secrets and variables → Actions** in the repository and add:

| Secret | What it is |
| -------- | ------------ |
| `SERVER_RCON_PASSWORD` (optional) | Remote console password (`rcon_password`) |
| `SERVER_OPERATOR_PASSWORD` (optional) | In-game operator password (`g_operator_password`) |
| `DO_SSH_PRIVATE_KEY_US` | SSH private key for the US East server |
| `DO_SSH_PRIVATE_KEY_EU` | SSH private key for the EU server |

Passwords are passed to the server via `+set` at deploy time. They are never written to disk on the runner or stored in this repo.

### First deploy

Run **Actions → Deploy Servers** with action `provision`. This installs system packages, downloads SteamCMD, installs the game, uploads configs, and starts all server processes in one go.

After the first deploy you can check that sessions are running:

```bash
tmux ls
```

Each server runs in a session named `wf-{port}`, e.g. `wf-44401`. Attach with `tmux attach -t wf-44401`.

## Directory structure

```shell
server-management/
  server-config.json    Server registry — regions, ports, gametypes, hostnames
  Warfork.sh            Uploaded to ~/scripts/ on each deploy
  configs/
    _base.cfg           Shared settings loaded by every server type
    clan-arena.cfg      Type-specific settings (execs _base.cfg then adds overrides)
    duel.cfg
    votable.cfg
    instagib.cfg
    race.cfg
```

## Config layering

When a server starts, the command line looks roughly like this:

```cfg
+exec configs/clan-arena.cfg   <- runs first, loads _base.cfg then type settings
+set net_port 44401            <- CLI overrides anything set in the cfg
+set sv_hostname "..."
+set rcon_password "..."       <- injected from GitHub secret, runs last
```

Since `+set` on the command line always wins over `set` in a cfg file, the values in `server-config.json` and the GitHub secrets are always authoritative.

| Setting | Where it lives |
| --------- | --------------- |
| `net_port`, `g_gametype`, `sv_hostname`, `sv_maxclients`, `dedicated` | `server-config.json` |
| `sv_defaultmap`, `g_votable_gametypes`, per-type vote overrides | `configs/{type}.cfg` |
| Logging, masterservers, HTTP, antilag, recording, vote defaults | `configs/_base.cfg` |
| `rcon_password`, `g_operator_password` | GitHub secrets |

## Deploying

Trigger **Actions → Deploy Servers** with:

| Input | Options | Default |
| ------- | --------- | --------- |
| `steam_branch` | `beta`, `public` | `beta` |
| `regions` | `US`, `EU`, or `all` | `all` |
| `server_types` | `clan-arena`, `duel`, etc., or `all` | `all` |
| `action` | `provision`, `update-and-restart`, `update-only`, `restart-only`, `stop`, `status` | `update-and-restart` |

| Action | What it does |
| -------- | -------------- |
| `provision` | Full first-time setup: installs packages, SteamCMD, game, then starts servers |
| `update-and-restart` | Stop → update via SteamCMD → start |
| `update-only` | Update via SteamCMD without restarting |
| `restart-only` | Restart without updating |
| `stop` | Stop all matching sessions |
| `status` | Report whether sessions are running |

## Adding a server type

Add an entry to `server_types` in `server-config.json`:

```json
"my-type": {
  "label": "My Type",
  "port": 44408,
  "cvars": {
    "sv_hostname": "[ WF Beta - ${REGION_LABEL} ] My Type",
    "g_gametype": "mygametype"
  }
}
```

Then create `configs/my-type.cfg`:

```cfg
exec configs/_base.cfg
set sv_defaultmap "wfmap1"
set g_votable_gametypes "mygametype"
```

## Adding a region

Regions are per-server entries, so subregions like `US-E`, `EU-DE`, `EU-FN`, `SG` each get their own block in `servers`:

```json
"EU-DE": {
  "host": "eu-de.warfork.com",
  "key_secret": "DO_SSH_PRIVATE_KEY_EU_DE",
  "username": "steam",
  "label": "EU Frankfurt"
}
```

Add the corresponding SSH private key as a GitHub secret. The same `configs/` directory deploys to all of them.

## Manual server commands

```bash
WF_PARAMS="..." STEAM_BRANCH=beta ~/scripts/Warfork.sh status
WF_PARAMS="..." ~/scripts/Warfork.sh stop
WF_PARAMS="..." ~/scripts/Warfork.sh restart
```

Sessions are named by port (`wf-44401`, `wf-44402`, etc.) so multiple server types can run on the same machine without conflicting.
