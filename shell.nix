{ pkgs ? import <nixpkgs> { } }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    clang # Clang compiler
    cmake # CMake for configuring the project
    git # Git for version control
    glew # GLEW for handling OpenGL extensions
    glfw # GLFW3 for creating windows and OpenGL contexts
    gnumake # Make for building the project
    libxkbcommon # Keymap handling library (shared between X11 and Wayland)
    pkg-config # pkg-config to manage library paths
    wayland # Wayland development libraries
    wayland-protocols # Wayland protocols for Wayland support
  ];
}
