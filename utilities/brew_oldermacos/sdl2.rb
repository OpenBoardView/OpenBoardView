# Based on upstream sdl2 furmulae from https://github.com/Homebrew/homebrew-core/raw/refs/tags/26-tahoe/Formula/s/sdl2.rb
# With addition of MACOSX_DEPLOYMENT_TARGET (same value in CMakeLists) to tell linker not produce DyldChainedFixups=0x80000034 unsupported by MacOS<10.15
# All unused features cleaned up for simplicity

class Sdl2 < Formula
  desc "Low-level access to audio, keyboard, mouse, joystick, and graphics"
  homepage "https://www.libsdl.org/"
  url "https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10.tar.gz"
  sha256 "5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165"
  license "Zlib"


  def install
    # We have to do this because most build scripts assume that all SDL modules
    # are installed to the same prefix. Consequently SDL stuff cannot be
    # keg-only but I doubt that will be needed.
    inreplace "sdl2.pc.in", "@prefix@", HOMEBREW_PREFIX

    system "./autogen.sh" if build.head?

    args = %W[--prefix=#{prefix} --enable-hidapi]
    if OS.mac?
        args += %w[--without-x]
    end
    # pass -march=x86-64 to make the binary compatible with any x86_64 MAC hardware, including the first Merom-based
    ENV["HOMEBREW_ARCHFLAGS"]="-march=x86-64"
    ENV["MACOSX_DEPLOYMENT_TARGET"] = "10.9"
    system "./configure", *args
    system "make", "install"
  end

  test do
    system bin/"sdl2-config", "--version"
  end
end
