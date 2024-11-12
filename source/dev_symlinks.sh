#!/bin/bash

if [ ! -d "assets" ]; then
  cd ..
fi

TGT=$(realpath source/build/basewf)
SRC=$(realpath assets)

PKS="data0_000_21 data0_000_21pure data0_21 data0_21pure data1_21pure"
CFG="$SRC/config"

for PK in $PKS; do
  for dir in "$SRC/$PK/"*; do
    if [ -d "$dir" ]; then
      name=$(basename "$dir")
      target="$TGT/$name"

      while read -r file; do
        relative=$(realpath --relative-to="$dir" "$file")
        if [ -L "$target/$relative" ]; then
          echo "removing old link $target/$relative"
          rm "$target/$relative"
        fi

        echo "linking $target/$relative -> $dir/$relative"
        mkdir -p "$target/$(dirname "$relative")"
        ln -s "$dir/$relative" "$target/$relative"
      done < <(find "$dir" -type f)
    fi
  done
done

if [ -L "$TGT/config" ]; then
  echo "removing old link $TGT/config"
  sudo rm "$TGT/config"
fi
echo "linking $CFG to $TGT/config"
sudo ln -s "$CFG" "$TGT/config"
