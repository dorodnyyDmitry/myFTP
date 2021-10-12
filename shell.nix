with import <nixpkgs> {};
gcc10Stdenv.mkDerivation {
  name = "env";
  nativeBuildInputs = [ cmake ninja ];
  buildInputs = [
    boost
  ];
}



