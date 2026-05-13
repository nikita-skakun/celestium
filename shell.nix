{ pkgs ? import <nixpkgs> {
    config = {
      allowUnfree = true;
    };
  }
}:

pkgs.mkShell {
  buildInputs = with pkgs; [
    alsa-lib # ALSA for audio handling
    clang # Clang compiler
    cmake # CMake for configuring the project
    git # Git for version control
    glfw # GLFW3 for creating windows and OpenGL contexts
    gnumake # Make for building the project
    jack2 # JACK for audio handling
    libGL # OpenGL library
    libogg # Ogg for audio handling
    libopus # Opus for audio handling
    libx11 # X11 library
    libxcursor # XCursor library
    libxext # X11 Extensions library
    libxfixes # X11 Fixes library
    libxi # Xi library
    libxinerama # Xinerama library
    libxkbcommon # Keymap handling library (shared between X11 and Wayland)
    libxrandr # XRandR library
    opusfile.dev # Opus for audio handling
    pkg-config # pkg-config to manage library paths
    pulseaudio # PulseAudio for audio handling
    rtaudio_6 # RtAudio for audio handling
    wayland # Wayland development libraries
    wayland-protocols # Wayland protocols for Wayland support
  ];

  shellHook = ''
    export CC=${pkgs.clang}/bin/clang
    export CXX=${pkgs.clang}/bin/clang++
    echo "Using Clang: $CC and $CXX"
  '';
}
