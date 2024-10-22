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
    glew # GLEW for handling OpenGL extensions
    glfw # GLFW3 for creating windows and OpenGL contexts
    gnumake # Make for building the project
    jack2 # JACK for audio handling
    libogg # Ogg for audio handling
    libopus # Opus for audio handling
    libxkbcommon # Keymap handling library (shared between X11 and Wayland)
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
