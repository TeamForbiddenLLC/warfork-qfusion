{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    systems = {
      url = "github:nix-systems/default";
      flake = false;
    };
    self.submodules = true;
  };
  outputs =
    { self, nixpkgs, ... }@inputs:
    let
      inherit (nixpkgs) lib;
      eachSystem = lib.genAttrs (import inputs.systems);
      eachSystemWithPkgs = f: lib.mapAttrs f (eachSystem (system: nixpkgs.legacyPackages.${system}));
    in
    {
      devShells = eachSystemWithPkgs (
        system: pkgs:
        let
          warfork = self.packages.${system}.warfork-no-steam;
        in
        {
          default =
            pkgs.mkShell.override
              {
                stdenv = pkgs.gcc13Stdenv;
              }
              {
                inputsFrom = warfork.joinDerivations;
                NIX_SDL_RPATH = pkgs.lib.makeLibraryPath warfork.libsForSDL;
              };
        }
      );
      packages = eachSystemWithPkgs (
        system: pkgs:
        let
          inherit (pkgs) callPackage;
          myPackages = self.packages.${system};
        in
        {
          default = myPackages.warfork;

          steamworks-sdk = callPackage ./nix/steamworks-sdk.nix { };
          warfork-assets = callPackage ./nix/warfork-assets.nix { };
          warfork-no-steam = callPackage ./nix/warfork.nix {
            inherit (myPackages) warfork-assets;
          };
          warfork = callPackage ./nix/warfork.nix {
            inherit (myPackages) steamworks-sdk warfork-assets;
          };
        }
      );
      apps = eachSystemWithPkgs (
        system: pkgs:
        let
          myPackages = self.packages.${system};
        in
        {
          default = {
            type = "app";
            program = "${myPackages.warfork}/warfork.x86_64";
          };
        }
      );
    };
}
