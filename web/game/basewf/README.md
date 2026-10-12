# basewf

This folder must stay **empty in git**. Do not commit Warfork game files.

Copy the `basewf` directory from your Steam Warfork install **into this folder** (the files go here, not a nested `basewf/basewf`).

Typical Steam path:

```
steamapps/common/Warfork/basewf/
```

After copying you should have packs such as:

```
web/game/basewf/
  README.md
  modules_21.pk3
  data0_21.pk3
  data0_21pure.pk3
  data0_000_21.pk3
  data0_000_21pure.pk3
  data1_21pure.pk3
  map_*.pk3
  ...
```

You can also pack this repo’s `assets/`. Official Steam files are preferred for multiplayer.

Restart the Node web server after adding files, then hard-refresh the browser (Ctrl+F5).
