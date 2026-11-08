"""Copy transient Steam logs while game-ci/steam-deploy is running."""

import pathlib
import shutil
import sys
import time


source_root = pathlib.Path(sys.argv[1])
destination_root = pathlib.Path(sys.argv[2])

while True:
    candidates = list(source_root.glob("steamcmd-*/home/Steam/logs/*"))
    candidates.extend(source_root.glob("**/BuildOutput/*.log"))
    candidates.extend(source_root.glob("**/build_output/*.log"))
    for source in candidates:
        if not source.is_file():
            continue
        destination = destination_root / source.relative_to(source_root)
        destination.parent.mkdir(parents=True, exist_ok=True)
        try:
            source_stat = source.stat()
            if destination.exists():
                destination_stat = destination.stat()
                if (destination_stat.st_size == source_stat.st_size and
                        destination_stat.st_mtime_ns == source_stat.st_mtime_ns):
                    continue
            shutil.copy2(source, destination)
        except FileNotFoundError:
            # game-ci may remove its temporary directory during a scan.
            pass
    time.sleep(0.1)
