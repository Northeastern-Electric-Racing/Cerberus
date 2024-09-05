{
  pkgs ? import <nixpkgs> { },
}:
pkgs.mkShell {
  # nativeBuildInputs is usually what you want -- tools you need to run
  nativeBuildInputs = with pkgs.buildPackages; [
    probe-rs-tools
    linuxKernel.packages.linux_6_6.usbip
    python3
    llvmPackages_18.clang-tools
  ];
  shellHook = ''
    source ../ner-venv/bin/activate
  '';
}
