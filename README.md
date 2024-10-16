# **Celestium** [![CodeFactor](https://www.codefactor.io/repository/github/nikita-skakun/celestium/badge)](https://www.codefactor.io/repository/github/nikita-skakun/celestium) [![Codacy](https://app.codacy.com/project/badge/Grade/27c30d00a98d42678bd8c30600178499)](https://app.codacy.com/gh/nikita-skakun/celestium/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)

## What is this?

**Celestium** is a 2D space station simulation and management game that explores procedural generation, simulation mechanics, and AI pathfinding.
This is a passion project, created as a way to explore advanced programming concepts and to unify them in a coherent and engaging game.

## Progress Update

Exciting new features have been added to **Celestium**! Here's a quick look at the latest developments:

- **A\* Pathfinding**: Crew members can now navigate the station using A\* pathfinding, ensuring efficient movement and intelligent routing around obstacles, bringing crew interactions to life.

- **Air Diffusion Simulation**: Oxygen management has become a key aspect of station life. Oxygen production systems can now be installed, and crew members consume oxygen as they go about their tasks. Maintaining a steady supply of breathable air is critical to keeping your crew alive and the station functional.

- **Power Storage and Connections**: Energy is now a crucial resource. Batteries can store and distribute power throughout the station, ensuring that critical life support systems stay online.

These features bring the station to life and enhance player interaction, setting the stage for deeper simulation gameplay. Stay tuned for more!

![progress-update-2024-10-14](https://github.com/user-attachments/assets/2b5e5327-fdcc-4765-b26d-074e8e610307)

[progress-update-2024-09-29](https://github.com/user-attachments/assets/4367d366-b55f-4bcb-a2c6-ae5160bae2b7)

## Build Instructions

Before you can start building Celestium, you will need to have Nix installed. If you don't already have it, please follow the [installation instructions](https://nixos.org/download/) for your operating system.

Once you have Nix installed, navigate to the root directory of this project and run:
```
nix-shell
```
This will create a new shell environment, and download all necessary dependencies. Then, create a new build directory by running:
```
mkdir build && cd build
```
Now, you should be in the newly created `build` directory. To configure and build Celestium, run:
```
cmake .. && make -j$(nproc) && ./celestium
```

## License

Celestium is an source available project licensed under the [CC BY-NC 4.0 License](LICENSE). You are free to use and modify the code for personal and educational purposes. However, certain assets within the project are inspired by external resources. Please ensure you adhere to the following attribution requirements and respect the rights of the original creators:

- [raylib](https://github.com/raysan5/raylib) library, licensed under the [Zlib License](https://github.com/raysan5/raylib/blob/master/LICENSE).

- [raygui](https://github.com/raysan5/raygui) library, licensed under the [Zlib License](https://github.com/raysan5/raygui/blob/master/LICENSE).

- [Termcolor](https://github.com/ikalnytskyi/termcolor) library, licensed under the [BSD (3-clause) License](https://github.com/ikalnytskyi/termcolor/blob/master/LICENSE).

- [Magic Enum C++](https://github.com/Neargye/magic_enum) library, licensed under the [MIT License](https://github.com/Neargye/magic_enum/blob/master/LICENSE).

- Visual assets inspired by [Jonik9i's Space Station Game Asset](https://jonik9i.itch.io/free-space-station-game-asset), licensed under the [Creative Commons Zero v1.0 Universal (CC0)](https://creativecommons.org/publicdomain/zero/1.0/).

- [Jersey25](https://github.com/scfried/soft-type-jersey) font, licensed under the [SIL Open Font License, Version 1.1](assets/fonts/OFL.txt).
