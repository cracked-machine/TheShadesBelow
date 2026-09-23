
[WORK IN PROGRESS]

You play a victorian grave digger who has become lost in a limbo dimension... a city of the dead. You must dig your way through obstacles to find the exit. The exit must be unlocked using items found in the surrounding graves. To survive in this world you can find plants that be used or consumed. Some may help you, others may not. You can sell your ill-gotten wares to surgeons of dubious ethics once you exit the level. 

Here is a (very) short [demo](https://youtu.be/ckoGOSE4Rc4).

# Game Engine

This uses a custom game engine using OpenGL/SFML3. The engine is managed using an [Entity Component System](https://en.wikipedia.org/wiki/Entity_component_system) architecture via the [Entt](https://github.com/skypjack/entt) library. 

# Build System

The project is built using GCC and CMake. 

The build environment runs within a [docker container](https://github.com/cracked-machine/TheShadesBelow/tree/main/.devcontainer/Dockerfile). All third party libraries are built from source. 

This project is setup to build on a linux system and cross-compile for a windows target via [MinGW](https://packages.debian.org/search?keywords=g%2B%2B-mingw-w64-x86-64&searchon=names&suite=all&section=all).  

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

This project should be built on a Linux PC. The cross-compiled output binary for Windows can be found in `build-x86_64-w64-mingw32/bin` and the native output binary for Linux is `build-x86_64-linux-gnu/bin`.

The project is configured to use VSCode remote extensions. This means you can run the build on a Linux PC from a Windows PC over SSH. You can build it natively from Linux if you wish but obviously if you want to test the cross-compiled windows binary you will need a Windows PC as well. Also note that this project is intended to be used from a Windows PC so of you run it directly in VSCode on Linux the task.json are not tested. 

## Samba

This lets you create a share on the Linux build host and run the executable from Windows explorer. Your `/etc/samba/smb.conf` should look like this:

```
[projects]
        path = /home/chris/projects
        browseable = yes
        read only = no
        writeable = yes
        # Disable oplocks entirely for better executable performance
        oplocks = no
        level2 oplocks = no
        kernel oplocks = no
        strict locking = no
        posix locking = no
        map archive = no
        map hidden = no
        map system = no
        # Add these for better performance
        socket options = TCP_NODELAY IPTOS_LOWDELAY SO_RCVBUF=131072 SO_SNDBUF=131072
        min receivefile size = 16384
```

Note we are disabling oplocks to prevent errors - `the parameter is incorrect` - when you rebuild the executable.  (see https://www.samba.org/samba/docs/current/man-html/smb.conf.5.html)


If you still see this error, the last resort is to ssh into the build host and restart the samba server after each build(!)

```
sudo systemctl restart smb
```

## Running the build in a debugger

In theory you can get windows to create a crash dump, but trying to use this windows-only crash dump with MinGW gcc symbols is a non-starter. Much simpler is to run the executable in GDB directly and observe the debugger when it crashes. Obviously this isn't much help after the fact.  

1. install mingw and gdb on windows via msys - the version must be equal or higher than the mingw supplied by debian.
1. Download the bin folder to your Windows PC (or use the Samba mapping)

        - `cd /c/path/to/bin`
        - `cd X:\\path\\to\\build\\bin\\`

1. Run gdb with the exe - 

        - `gdb X:\\cpp\\games\\temp\\build\\bin\\ProceduralMaze.exe -ex run`
        - `/c/path/to/bin/ProceduralMaze.exe -ex run`

1. Or run with a breakpoint - 

        - `gdb /x/cpp/games/top-down-maze-procedural/build/bin/ProceduralMaze.exe -ex "break Engine.cpp:30" -ex "run" -ex "next" -ex "next"`
        - `gdb /c/path/to/bin/ProceduralMaze.exe -ex "break Engine.cpp:30" -ex "run" -ex "next" -ex "next"`

## Installing and launching the game from remote build server

After building the exe on a remote Linux build server, it is recommended to copy (install) the files onto the local windows workstation before launching the game. For performance reasons, it is not recommended to launch the game over a network/samba share. Using SCP can speed up this copy/install process:

```
del /q "C:\Users\chris\Desktop\TheShadesBelow\bin"
mkdir "C:\Users\chris\Desktop\TheShadesBelow\bin"
scp -r 192.168.1.106:/home/chris/projects/cpp/games/TheShadesBelow\build-x86_64-w64-mingw32\bin C:\Users\chris\Desktop\TheShadesBelow

start /wait /b /d "C:\Users\chris\Desktop\TheShadesBelow\bin\" TheShadesBelow.exe
```


# Design 

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

## Sprite Factory

Sprite sheets are loaded using the [SpriteFactory](src/Factory/SpriteFactory.cpp) from [spritesheets.json](res/json/spritesheets.json). This supports both single block (16x16) and multi-block sprites (some multiple of 16x16). Each entry in the JSON file must have a unique name. This is used by the SpriteFactory to create the sprite and serves as a key to retrieve the sprite later on. 

| property | description |
|:-|:-|
|display_name|Cosmetic name used in the game UI|
|texture_path|The texture file that contains the sprite artwork. e.g. png|
|sprite_indices|The 16x16 grid index location for each sprite that should be loaded from the texture file.|
|zorder|Each sprite index can have an independent zorder value. If you set the value to zero then the y-axis position is used.|
|grid_size|Multi-block setting. The width and height of the sprite in 16x16 grid size.|
|sprites_per_frame|For single-block sprites this should be set to 1. For multi-block sprites this should be set to the product of the `grid_size` width and height|
|sprites_per_sequence|Animation setting. To enable animation this should match the length of the `sprite_indices` field. To disable animation set it to 1.|

## Particle System

The particle system uses the standard [emit/simulate/render](https://en.wikipedia.org/wiki/Particle_system) design. The top level [ParticleSystem](src/Systems/ParticleSystem.hpp) takes any class derived from [ParticleSpriteBase](src/Components/Particle/ParticleSpriteBase.hpp). This uses a strategy pattern to allow custom emission/simulation/rendering implementations via a common interface. Example implementations for flames, smoke, etc can be found in the [src/Components/Particle](src/Components/Particle) directory.

## Pathfinding

Pathfinding for NPCs uses the [A* search](https://en.wikipedia.org/wiki/A*_search_algorithm) algorithm. This is an efficient algorithm that combines BFS with a heuristic to avoid redundant search paths. The algorithm implementation can be found in [src/PathFinding/AStar.hpp](src/PathFinding/AStar.hpp). 

To avoid querying all positions in the ECS registry, the game level is tracked using a [spatial hash grid](src/PathFinding/SpatialHashGrid.hpp). This stores the valid positions for path finding, but is used elsewhere for O(1) lookup. The hash grid works by using the x/y coordinate as a key to store/lookup a bucket of entities at that position.  Finding the nearest neighbour to any game position then becomes a trivial operation. 

## Scene Manager

See [src/SceneControl](src/SceneControl)

The [Scene Manager](src/SceneControl/SceneManager.hpp) allows the transition from one game scene to another. For example, TitleScene > GraveyardScene > CryptScene > GameOverScene > TitleScene.

Scenes are managed using a [stack](src/SceneControl/SceneStack.hpp). The scene at the top of the stack is the active scene. When a scene is pushed onto the stack the SceneManager will run its `on_init` member function to initialise the scene. Depending on the type of scene it may also call `on_enter`. The engine will call `on_update` for the active scene every frame. When the player exits the scene (or dies) the scene is popped off the stack and the SceneManager can optionally call the `on_exit` function. 

In order to provide some state persistence it is necessary to clone some ECS registry components when transitioning between scenes. This is done using the [RegistryTransfer](src/SceneControl/RegistryTransfer.cpp). This is done in the `RegistryTransfer::init_missing_cmp_storages` method. 

## Player stat system

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
