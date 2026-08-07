{
  lib,
  gcc13Stdenv,
  symlinkJoin,
  makeWrapper,

  cmake,
  python3,

  openal,
  freetype,
  vulkan-headers,
  curl,
  zlib,
  libglvnd,
  alsa-lib,
  pipewire,
  libogg,
  libvorbis,

  libx11,
  libxcb,
  libxext,
  libxrandr,
  libXinerama,

  wayland,
  wayland-scanner,
  libffi,
  libxkbcommon,
  libdecor,

  steamworks-sdk ? null,
  warfork-assets,
}:
let
  fs = lib.fileset;
  libsForSDL = [
    libglvnd
    (libvorbis.overrideAttrs (prev: {
      nativeBuildInputs = prev.nativeBuildInputs ++ [
        cmake
      ];
      cmakeFlags = [
        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
        "-DBUILD_SHARED_LIBS=1"
      ];
      outputs = [
        "out"
        "dev"
      ];
    }))

    libx11
    libxcb
    libxext
    libxrandr
    libXinerama

    wayland
    wayland-scanner
    libffi
    libxkbcommon
    libdecor
  ];
  warfork-no-assets = gcc13Stdenv.mkDerivation {
    name = "warfork-no-assets";
    src = fs.toSource {
      root = ../.;
      fileset = fs.unions [
        ../source
        ../icons
        ../third-party
      ];
    };
    nativeBuildInputs = [
      cmake
      python3
    ];
    buildInputs = [
      (openal.override {
        stdenv = gcc13Stdenv;
      })
      freetype
      vulkan-headers
      curl
      zlib
      alsa-lib
      pipewire
      libogg
    ]
    ++ libsForSDL;
    unpackPhase = ''
      cp -r $src/{source,third-party} .
      chmod -R +w source third-party
      ln -s $src/icons .
      ${lib.optionalString (steamworks-sdk != null)
        # bash
        ''
          ln -s ${steamworks-sdk}/sdk third-party/steamworks
        ''
      }
    '';
    configurePhase = ''
      cd source
      cmake \
        -B build \
        --preset workflow-linux-release \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        -DBUILD_STEAMLIB=${if steamworks-sdk != null then "1" else "0"} \
        -DSDL_RPATH=1 \
        -DUSE_GRAPHICS_X11=1 \
        -DUSE_GRAPHICS_WAYLAND=1 \
        -DUSE_SYSTEM_ZLIB=1 \
        -DUSE_SYSTEM_OPENAL=1 \
        -DUSE_SYSTEM_CURL=1 \
        -DUSE_SYSTEM_OGG=1 \
        -DUSE_SYSTEM_FREETYPE=1 \
        -DUSE_SYSTEM_VORBIS=1
    '';
    buildPhase = ''
      JOBS="$(nproc 2>/dev/null || echo 4)"
      cmake --build build -j"$JOBS"
    '';
    installPhase = ''
      mkdir $out
      cp -r build/warfork-qfusion/* $out
      cp build/libs/libTracyClient_x86_64.so* $out/libs
    '';
    preFixup = ''
      bads=(
        libs/libref_nri_x86_64.so
        libs/libsnd_openal_x86_64.so
        libs/libref_gl_x86_64.so
        wftv_server.x86_64
        warfork.x86_64
        wf_server.x86_64
      )
      for p in "''${bads[@]}"; do
        patchelf \
          --shrink-rpath \
          --allowed-rpath-prefixes /nix/store/ \
          $out/$p
        patchelf \
          --add-rpath $out/libs \
          $out/$p
      done
    '';
    postFixup = ''
      patchelf \
        --add-rpath ${lib.makeLibraryPath libsForSDL} \
        $out/libs/libSDL2-2.0_x86_64.so
    '';
  };
  joinDerivations = [
    warfork-no-assets
    warfork-assets
  ];
in
symlinkJoin {
  name = "warfork";
  paths = joinDerivations;
  nativeBuildInputs = [
    makeWrapper
  ];
  passthru = {
    # Allow us to mkShell's inputsFrom
    inherit joinDerivations libsForSDL;
  };
  postBuild = ''
    wrapProgram $out/warfork.x86_64 \
      --chdir $out
  '';
}
