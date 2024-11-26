# **Celestium** [![CodeFactor](https://www.codefactor.io/repository/github/nikita-skakun/celestium/badge)](https://www.codefactor.io/repository/github/nikita-skakun/celestium) [![Codacy](https://app.codacy.com/project/badge/Grade/27c30d00a98d42678bd8c30600178499)](https://app.codacy.com/gh/nikita-skakun/celestium/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)

## What is this?

**Celestium** is a 2D space station simulation and management game that explores procedural generation, simulation mechanics, and AI pathfinding.
This is a passion project, created as a way to explore advanced programming concepts and to unify them in a coherent and engaging game.

## Feature Progress

| Feature Name                      | Progress             | Description                                                                                                                                 |
| :-------------------------------- | :------------------- | :------------------------------------------------------------------------------------------------------------------------------------------ |
| Component-based Tiles             | :white_check_mark:   | Each tile is made from a number of components, each changing the behavior of the tile and it's effect on the environment                    |
| Station Tile Rendering            | :white_check_mark:   | The station tile sprites are made from a number of sub-sprites, which are pieced together to dynamically respond to their neighbors         |
| Crew Pathfinding                  | :white_check_mark:   | Crew pathfinding is done with A\* algorithm, which allows for pathing over complex station designs                                          |
| Air Diffusion                     | :white_check_mark:   | Oxygen spreads from high concentration to low, simulating real-life processes. Crew and fire consume oxygen, while certain tiles produce it |
| Dynamic Doors                     | :white_check_mark:   | Doors now dynamically separate rooms, only allowing air to spread when it is open                                                           |
| Cross-Platform Builds             | :construction:       | Implement cross-platform for Linux (Done) and Windows (WIP). MacOS is currently unplanned.                                                  |
| Definitions for Core Game Aspects | :construction:       | The goal is to allow for easy modifications to core game features, (such as what components a tile is composed of)                          |
| Environmental Effects             | :construction:       | Environmental hazards and objects (like fire and foam), which can effect the crew and the station                                           |
| Power Storage and Connections     | :construction:       | A rework is necessary to make power connections be part of the physical game world                                                          |
| Player Station Design             | :construction:       | Currently the player can add, move, rotate and delete tiles. Better build tools and warnings are still necessary                            |
| Settings Menu                     | :construction:       | Some basic settings are available, but more work is necessary (key re-binding, etc)                                                         |
| Music and Sound Effects           | :construction:       | A basic implementation of sound effects is available, but more sounds and testing is still required                                         |
| Main Menu                         | :white_large_square: | A main menu to start a new game, and load existing game                                                                                     |
| Saving and Loading Game State     | :white_large_square: | Serializing the game state should be relatively straightforward, as tiles already have a tile definition file                               |
| Resources and Usage               | :white_large_square: | Require resources to build new tiles, and the building process should require crew to work on it, instead of being instant                  |
| A lot more...                     | :white_large_square: | This project is under active development! Feel free to [suggest a feature](https://github.com/nikita-skakun/celestium/issues/new).          |

## Progress Snapshots

If you are here, you probably came for the pretty pictures, so here you go!

### November 11, 2024

![progress-update-2024-11-11](https://github.com/user-attachments/assets/0a84e0ca-16ba-426c-a119-fe7602ed5e3c)

### October 14, 2024

![progress-update-2024-10-14](https://github.com/user-attachments/assets/2b5e5327-fdcc-4765-b26d-074e8e610307)

### September 29, 2024

[progress-update-2024-09-29](https://github.com/user-attachments/assets/4367d366-b55f-4bcb-a2c6-ae5160bae2b7)

## Build Instructions

Before you can start building Celestium, you will need to have Nix installed. If you don't already have it, please follow the [installation instructions](https://nixos.org/download/) for your operating system.

Once you have Nix installed, navigate to the root directory of this project and run:

```bash
nix-shell
```

This will create a new shell environment, and download all necessary dependencies. Then, create a new build directory by running:

```bash
mkdir build && cd build
```

Now, you should be in the newly created `build` directory. To configure and build Celestium, run:

```bash
cmake .. && make -j$(nproc) && ./celestium
```

If something doesn't work, feel free to [leave an issue](https://github.com/nikita-skakun/celestium/issues/new).

## License

Celestium is an source available project licensed under the [CC BY-NC 4.0 License](LICENSE). You are free to use and modify the code for personal and educational purposes. However, certain assets within the project are inspired by external resources. Please ensure you adhere to the following attribution requirements and respect the rights of the original creators:

- [raylib](https://github.com/raysan5/raylib) library, licensed under the [Zlib License](https://github.com/raysan5/raylib/blob/master/LICENSE).

- [raygui](https://github.com/raysan5/raygui) library, licensed under the [Zlib License](https://github.com/raysan5/raygui/blob/master/LICENSE).

- [Termcolor](https://github.com/ikalnytskyi/termcolor) library, licensed under the [BSD (3-clause) License](https://github.com/ikalnytskyi/termcolor/blob/master/LICENSE).

- [Magic Enum C++](https://github.com/Neargye/magic_enum) library, licensed under the [MIT License](https://github.com/Neargye/magic_enum/blob/master/LICENSE).

- Visual assets inspired by [Jonik9i's Space Station Game Asset](https://jonik9i.itch.io/free-space-station-game-asset), licensed under the [Creative Commons Zero v1.0 Universal (CC0)](https://creativecommons.org/publicdomain/zero/1.0/).

- [Jersey25](https://github.com/scfried/soft-type-jersey) font, licensed under the [SIL Open Font License, Version 1.1](assets/fonts/OFL.txt).

- [Opusfile](https://github.com/xiph/opusfile) library, licensed under the [BSD (3-clause) License](https://github.com/xiph/opusfile?tab=BSD-3-Clause-1-ov-file#readme).

- [RtAudio](https://github.com/thestk/rtaudio) library, licensed under the [MIT-like License](https://github.com/thestk/rtaudio/blob/master/LICENSE).

- [Rapid YAML](https://github.com/biojppm/rapidyaml) library, licensed under the [MIT License](https://github.com/biojppm/rapidyaml/blob/master/LICENSE.txt).
