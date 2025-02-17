{ lib, stdenv, fetchurl, fetchpatch, linuxHeaders, readline, openssl, flex, libkrb5, pam }:

# TODO: These tools are supposed to work under NetBSD and FreeBSD as
# well, so I guess it's not appropriate to place this expression in
# "os-specific/linux/ipsec-tools". Since I cannot verify that the
# expression actually builds on those platforms, I'll leave it here for
# the time being.

stdenv.mkDerivation rec {
  pname = "ipsec-tools";
  version = "0.8.2";

  src = fetchurl {
    url = "mirror://sourceforge/ipsec-tools/ipsec-tools-${version}.tar.bz2";
    sha256 = "0b9gfbz78k2nj0k7jdlm5kajig628ja9qm0z5yksiwz22s3v7dlf";
  };

  buildInputs = [ readline openssl flex libkrb5 pam ];

  patches = [
    ./dont-create-localstatedir-during-install.patch
    ./CVE-2015-4047.patch
    ./CVE-2016-10396.patch
  ];

  # fix build with newer gcc versions
  preConfigure = ''substituteInPlace configure --replace "-Werror" "" '';

  configureFlags = [
    "--sysconfdir=/etc --localstatedir=/var"
    "--with-kernel-headers=${linuxHeaders}/include"
    "--disable-security-context"
    "--enable-adminport"
    "--enable-dpd"
    "--enable-frag"
    "--enable-gssapi"
    "--enable-hybrid"
    "--enable-natt"
    "--enable-shared"
    "--enable-stats"
  ];

  meta = with lib; {
    homepage = "http://ipsec-tools.sourceforge.net/";
    description = "Port of KAME's IPsec utilities to the Linux-2.6 IPsec implementation";
    license = licenses.bsd3;
    platforms = platforms.linux;
  };
}
