{
  lib,
  gcc13Stdenv,

  cmake,
  zip,
  strip-nondeterminism,
}:
let
  fs = lib.fileset;
in
gcc13Stdenv.mkDerivation {
  name = "warfork-assets";
  src = fs.toSource {
    root = ../.;
    fileset = fs.unions [
      ../assets
      ../source/package_assets.cmake
    ];
  };
  nativeBuildInputs = [
    cmake
    zip
    strip-nondeterminism
  ];
  dontUnpack = true;
  dontConfigure = true;
  dontInstall = true;
  dontFixup = true;
  buildPhase = ''
    cmake \
      -DCMAKE_SYSTEM_NAME=Linux \
      -DBIN_DIR="$out" \
      -DASSET_ROOT="$src/assets" \
      -P "$src/source/package_assets.cmake"
  '';
}
