
[WORK IN PROGRESS]

You play a victorian grave digger who has become lost in a limbo dimension... a city of the dead. To unlock the exit you must use your skills to explore the graveyard and uncover secrets that may help... or hinder... your escape. The mysterious Dr Knox is on hand to guide you on your journey back to the overworld. 

# Game Engine

This uses a custom game engine using OpenGL/SFML3. The engine is managed using an [Entity Component System](https://en.wikipedia.org/wiki/Entity_component_system) architecture via the [Entt](https://github.com/skypjack/entt) library. 

# Build System

Both Linux and Windows builds are supported. 

The project is built using GCC/[MinGW](https://packages.debian.org/search?keywords=g%2B%2B-mingw-w64-x86-64&searchon=names&suite=all&section=all) 14.2 and CMake. All builds should be run on Linux. Use the mingw32 target to cross-compile for Windows.

The project is configured to use VSCode remote extension to run the build on a Linux PC remotely over SSH from a Windows PC. If you want to run the build on a local linux machine you can run the scripts/build.sh directly. See the .vscode/tasks.json for examples. The build environment runs within a [docker container](https://github.com/cracked-machine/TheShadesBelow/tree/main/.devcontainer/debian-trixie/Dockerfile) using Debian. All third party libraries are built from source. 

### Devcontainers

There are two devcontainer configurations. VSCode asks which one to use on "Reopen in Container", and you can change at any time with the "Dev Containers: Switch Container" command. Both share the same workspace; the AlmaLinux 8 image sets `BUILD_DIR_SUFFIX=-el8` so its build directories never collide with the Debian ones.

| | Debian trixie (default) | AlmaLinux 8 |
|---|---|---|
| Devcontainer | [.devcontainer/debian-trixie/](.devcontainer/debian-trixie/) | [.devcontainer/almalinux8/](.devcontainer/almalinux8/) |
| Purpose | Day-to-day development, Windows cross-build | Portable Linux release build, troubleshooting old-glibc issues |
| Base image | `debian:trixie` | `almalinux:8` |
| glibc (minimum required on the target for Linux builds) | 2.41 | 2.28 (RHEL8+, Ubuntu 20.04+, Debian 10+, Steam Deck) |
| GCC | 14 (distro) | 14 (`gcc-toolset-14`) |
| Targets | `x86_64-linux-gnu`, `x86_64-w64-mingw32` | `x86_64-linux-gnu` |
| Build dir | `build-<target>-<CONFIG>` | `build-<target>-el8-<CONFIG>` |
| Linux runtime linking (Release) | libstdc++/libgcc static, glibc dynamic | libstdc++/libgcc static, glibc dynamic |
| Windows runtime linking | Release: static. Debug: ships libstdc++/libgcc/winpthread DLLs ([GCC PR 110572](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=110572)) | n/a |
| CI builds | `x86_64-w64-mingw32`, `x86_64-linux-gnu` | `x86_64-linux-gnu` |
| CI artifacts (Actions run page, kept 7 days) | `build-x86_64-w64-mingw32-<type>`, `build-x86_64-linux-gnu-trixie-<type>` | `build-x86_64-linux-gnu-<type>` |
| Published in GitHub Releases | `x86_64-w64-mingw32` | `x86_64-linux-gnu` |

glibc cannot be statically linked (SFML needs the system's OpenGL, X11, udev and audio libraries), so Linux portability comes from building against an old glibc. To check the minimum glibc a Linux binary needs:

```
objdump -T build-x86_64-linux-gnu-el8-Release/bin/TheShadesBelow | grep -o 'GLIBC_[0-9.]*' | sort -uV | tail -1
```

You can determine the C-Runtime (CRT) by running objdump on the binary output:

```
x86_64-w64-mingw32-objdump -p build-x86_64-w64-mingw32/bin/ProceduralMaze.exe | grep -iE 'msvcrt|ucrt|vcruntime|api-ms-win-crt'
```

The OpenGL loader used entirely depends on the runtime OS. This is printed in the log file when the game is executed. For example on Windows 11:

```
Engine.cpp:57 - OpenGL: 4.6.0 Compatibility Profile Context 26.1.1.251223
Engine.cpp:58 - Renderer: AMD Radeon RX 7900 GRE
Engine.cpp:59 - Vendor: ATI Technologies Inc.
```

## Installing and launching the game from remote build server

The cross-compiled output binary for Windows can be found in `build-x86_64-w64-mingw32-<CONFIG>/bin` and the native output binary for Linux is `build-x86_64-linux-gnu-<CONFIG>/bin`. Where `<CONFIG>` is either Debug or Release. 

Note that the build script also copies game assets into the bin directory. If you change assets or configuration (JSON) files you will need to run the build script again to re-sync the bin directory; This doesn't rebuild the game. To do a clean rebuild just delete the build dir and run the build script again. 

For the Windows build it is recommended to copy (install) the files onto the local windows workstation before launching the game. For performance reasons, it is not recommended to launch the game over a network/samba share. Using SCP can speed up this copy/install process:

```
del /q "C:\Users\chris\Desktop\TheShadesBelow\bin"
mkdir "C:\Users\chris\Desktop\TheShadesBelow\bin"
scp -r 192.168.1.106:/home/chris/projects/cpp/games/TheShadesBelow\build-x86_64-w64-mingw32\bin C:\Users\chris\Desktop\TheShadesBelow

start /wait /b /d "C:\Users\chris\Desktop\TheShadesBelow\bin\" TheShadesBelow.exe
```

## Troubleshooting

The information below is for the debug build only. 

Any exceptions are caught by the game engine and displayed before terminating. This will most likely be due to a bad configuration file or missing game assets. If there is an unexpected error it will display the stack trace. Always check the log file for more details. This can be found in the same directory as the executable. 

You can also run the build in GDB:

1. install mingw and gdb on windows via msys - the version must be equal or higher than the mingw version supplied by debian. At the time of writing this is GCC 14.2 with Debian Trixie.
1. Download the bin folder to your Windows PC (or use the Samba mapping)

        - `cd /c/path/to/bin`
        - `cd X:\\path\\to\\build\\bin\\`

1. Run gdb with the exe - 

        - `gdb X:\\cpp\\games\\temp\\build\\bin\\ProceduralMaze.exe -ex run`
        - `/c/path/to/bin/ProceduralMaze.exe -ex run`

1. Or run with a breakpoint - 

        - `gdb /x/cpp/games/top-down-maze-procedural/build/bin/ProceduralMaze.exe -ex "break Engine.cpp:30" -ex "run" -ex "next" -ex "next"`
        - `gdb /c/path/to/bin/ProceduralMaze.exe -ex "break Engine.cpp:30" -ex "run" -ex "next" -ex "next"`

# Design 

## Entity Component System (ECS)

The ganme is structured using [components](src/Components) that are owned by entities that are processed using [systems](src/Systems).  Systems query the ECS register to get views of entities with specific components. 

## Scene Manager

See [src/SceneControl](src/SceneControl)

The [Scene Manager](src/SceneControl/SceneManager.hpp) allows the transition from one game scene to another. For example, TitleScene > GraveyardScene > CryptScene > GameOverScene > TitleScene.

Scenes are managed using a [stack](src/SceneControl/SceneStack.hpp). The scene at the top of the stack is the active scene. When a scene is pushed onto the stack the SceneManager will run its `on_init` member function to initialise the scene. Depending on the type of scene it may also call `on_enter`. The engine will call `on_update` for the active scene every frame. When the player exits the scene (or dies) the scene is popped off the stack and the SceneManager can optionally call the `on_exit` function. 

In order to provide some state persistence it is necessary to clone some ECS registry components when transitioning between scenes. This is done using the [RegistryTransfer](src/SceneControl/RegistryTransfer.cpp). This is done in the `RegistryTransfer::init_missing_cmp_storages` method. 

## Message Architecture

1. System Store

Non-system classes can use the system store to call public methods of any System class. To use the SystemStore you need to pass it into your class and store it as a reference. This is how Scene (src/SceneControl/Scenes) classes call System class functions.

1. System to System

Systems can register and trigger/enqueue events between each other using the `m_systems_event_queue`. This event queue is defined in BaseSystem which is inherited by all System classes. 

1. Scene User Input events

User input needs to processed within the context of the current scene. For example, only user input relevant to the CryptScene should only be actioned when the CryptScene is the current scene.  Therefore, scene classes need to call a unique input handler at the end of the `update()` function. This is enqueued automatically by `src/SceneControl/Scene.hpp`, so concrete Scene classes (src/SceneControl/Scenes) do not have to remember. Concrete scene classes are defined as instantiations of their input handler type. For example:

```
class CryptScene : public Scene<Events::ProcessCryptSceneInputEvent>
```

This tells the base template class `Scene` to call the ProcessCryptSceneInputEvent handler registered in the `src/SceneControl/SceneInputRouter.cpp` constructor.

## Sprite/Item/Npc Stores

Sprite sheets are loaded using the [SpriteStore](src/Systems/Stores/SpriteStore.cpp) from [spritesheets.json](res/json/spritesheets.json). This supports both single block (16x16) and multi-block sprites (some multiple of 16x16). Each entry in the JSON file must have a unique name. This is used by the SpriteStore to create the sprite and serves as a key to retrieve the sprite later on. 

| property | description |
|:-|:-|
|display_name|Cosmetic name used in the game UI|
|texture_path|The texture file that contains the sprite artwork. e.g. png|
|sprite_indices|The 16x16 grid index location for each sprite that should be loaded from the texture file.|
|zorder|Each sprite index can have an independent zorder value. If you set the value to zero then the y-axis position is used.|
|grid_size|Multi-block setting. The width and height of the sprite in 16x16 grid size.|
|collision_mask|Optional. Which cells of the sprite collide, one boolean per `grid_size` cell in row-major order. For NPCs these are the cells it stands on, used for collision and pathfinding clearance. A single entry (e.g. `[true]`) applies to every cell. If omitted, every cell collides. All `false` means the sprite does not collide.|
|indices_per_sequence|Animation setting. To enable animation this should match the length of the `sprite_indices` field. To disable animation set it to 1.|

Items carried by the player and NPCs are also loaded from JSON configurations files. These reference sprites from the spritesheet.json file. 

## Particle System

The particle system uses the standard [emit/simulate/render](https://en.wikipedia.org/wiki/Particle_system) design. The top level [ParticleSystem](src/Systems/ParticleSystem.hpp) takes any class derived from [ParticleSpriteBase](src/Components/Particle/ParticleSpriteBase.hpp). This uses a strategy pattern to allow custom emission/simulation/rendering implementations via a common interface. Example implementations for flames, smoke, etc can be found in the [src/Components/Particle](src/Components/Particle) directory.

## Pathfinding

Pathfinding for NPCs uses the [A* search](https://en.wikipedia.org/wiki/A*_search_algorithm) algorithm. This is an efficient algorithm that combines BFS with a heuristic to avoid redundant search paths. The algorithm implementation can be found in [src/PathFinding/AStar.hpp](src/PathFinding/AStar.hpp). 

To avoid querying all positions in the ECS registry, the game level is tracked using a [spatial hash grid](src/PathFinding/SpatialHashGrid.hpp). This stores the valid positions for path finding, but is used elsewhere for O(1) lookup. The hash grid works by using the x/y coordinate as a key to store/lookup a bucket of entities at that position.  Finding the nearest neighbour to any game position then becomes a trivial operation. 

## Player stat and inventory system

The player has a single inventory slot.

The player has a number of [stats](src/Components/Stats/PlayerStats.hpp) that can be positively or negatively affected during gameplay. These are

- Health
- Fear
- Despair
- Infamy
- Toxicity

Every NPC and Item in the game has a set of action modifiers that contain the values to modify each player stat. For example, when the player picks up a certain item the CarryAction action modifies the player stats. Each action object can be found in the [src/Components/Stats](src/Components/Stats) directory. The NPC and Item stats are loaded into a store from [res/json/npc.json](res/json/npc.json) and [res/json/items.json](res/json/items.json). The stores can be found in [src/Systems/Stores](src/Systems/Stores)

## Procedural Generation and Tiled application.

The boundary of each level and static items are defined using the [Tiled](https://www.mapeditor.org/) application. This allows quick modification in a JSON format, which is then loaded into the game using [src/Utils/JsonDeserializer.hpp](src/Utils/JsonDeserializer.hpp) 
The Tiled input files can be found in [res/scenes](res/scenes). 

The randomly placed items within the game - obstacles, plants, etc... - are procedurally generated. Therefore each play has a unique layout. The following algorithms are employed:
- Cell Automata (GameOfLife) is used for the Graveyard scenes. The player is required to clear a path through the generated level.
 - Diffusion-limited aggregation is used for the Ruin scenes. The player must push/pull blocks to complete a puzzle.
 - Drunken walk is used for the Crypt Scenes. This creates meandering passageways between the rooms. 

Implemtations can be found in [src/Systems/ProcGen](src/Systems/ProcGen)

----

Copyright (C) 2026 Chris Sutton. All Rights Reserved.

This source code is published publicly solely for portfolio demonstration, educational, and peer-review purposes. 

Permission is hereby granted to view, fork, and download this repository for personal evaluation and review only. 

Strictly Prohibited Actions:
1. Compiling, modifying, or distributing this source code, binaries, or assets for commercial purposes.
2. Distributing compiled executables or modified versions of this project to the public.
3. Hosting or distributing the project or its assets on external storefronts, platforms, or websites.

This license will be updated or revoked when the project transitions to a commercial product.
