{
  stdenvNoCC,
  requireFile,
  unzip,
}:
stdenvNoCC.mkDerivation (final: {
  pname = "steamworks-sdk";
  version = "165";
  src = requireFile {
    name = "steamworks_sdk_${final.version}.zip";
    sha256 = "1x62nywl4dlc1nxx9sc1c4n31d09k6n5hy4nrc7brl095pg93z4c";
    message = ''
      Download steamworks sdk via https://partner.steamgames.com/?goto=%2Fdownloads%2Fsteamworks_sdk_${final.version}.zip
    '';
  };
  nativeBuildInputs = [
    unzip
  ];
  unpackPhase = ''
    mkdir $out
    unzip $src -d $out
  '';
})
