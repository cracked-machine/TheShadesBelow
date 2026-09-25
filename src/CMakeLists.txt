set(TARGET TheShadesBelow)

add_executable(${TARGET} 
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/Engine.cpp
    ${CMAKE_SOURCE_DIR}/src/Audio/EffectsItem.cpp
    ${CMAKE_SOURCE_DIR}/src/Audio/MusicItem.cpp
    ${CMAKE_SOURCE_DIR}/src/Audio/SoundBank.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Particle/SpriteBase.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Particle/FlameParticleSprite.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Particle/ShockWave.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Particle/SpriteTest.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Particle/Smoke.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Particle/CryptAltarParticleSprite.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Particle/RuneParticleSprite.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Particle/WormholeParticleSprite.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Particle/ObstacleDigParticleSprite.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Particle/EatingCrumbsParticleSprite.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Particle/PlantLeavesParticleSprite.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Particle/PlantTwigsParticleSprite.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Particle/SkeleBonesParticleSprite.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Particle/WatchmanGunfireParticleSprite.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Particle/PlayerHealingParticleSprite.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Particle/AshPileParticleSprite.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Crypt/RoomBase.cpp
    ${CMAKE_SOURCE_DIR}/src/Debug/AssertHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/Shaders/BaseShaderSprite.cpp
    ${CMAKE_SOURCE_DIR}/src/Shaders/TitleScreenShader.cpp
    ${CMAKE_SOURCE_DIR}/src/Shaders/DarkModeShader.cpp
    ${CMAKE_SOURCE_DIR}/src/Shaders/DrippingBloodShader.cpp
    ${CMAKE_SOURCE_DIR}/src/Shaders/FloodWaterShader.cpp
    ${CMAKE_SOURCE_DIR}/src/Shaders/NightStaticShader.cpp
    ${CMAKE_SOURCE_DIR}/src/Shaders/MistShader.cpp
    ${CMAKE_SOURCE_DIR}/src/Shaders/CircularDistortionShader.cpp
    ${CMAKE_SOURCE_DIR}/src/Shaders/RedVignetteShader.cpp
    ${CMAKE_SOURCE_DIR}/src/Shaders/TunnelVisionShader.cpp
    ${CMAKE_SOURCE_DIR}/src/Sprites/SpriteSheet.cpp
    ${CMAKE_SOURCE_DIR}/src/Sprites/VertexFloor.cpp
    ${CMAKE_SOURCE_DIR}/src/Sprites/Shockwave.cpp
    ${CMAKE_SOURCE_DIR}/src/Sprites/CircleSegment.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/ArrowSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/ProcGen/LevelGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/ProcGen/CellAutomataSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/ProcGen/DLASystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/ProcGen/PassageSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/ProcGen/PassageAlgorithms.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/Render/RenderSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/Render/RenderGameSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/Render/RenderMenuSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/Render/RenderOverlaySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/Render/UiData.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/AnimSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/BaseSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/ActionSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/InventorySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/ExitSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/FootstepSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/PersistSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/PlayerSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/Threats/BombSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/Threats/NpcSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/Threats/WatchmanSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/Threats/WispSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/Threats/HazardFieldSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/Threats/WormholeSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/Threats/LightningSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/GraveSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/AltarSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/LootSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/CryptSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/HealingSpringSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/RuinSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/ShopSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/ItemSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/GrimoireSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/Stores/BaseStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/Stores/ItemStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/Stores/NpcStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/Stores/SystemStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/ParticleSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/ShaderSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/Systems/Threats/ShockwaveSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/SceneControl/SceneInputRouter.cpp
    ${CMAKE_SOURCE_DIR}/src/SceneControl/SceneManager.cpp
    ${CMAKE_SOURCE_DIR}/src/SceneControl/Scenes/GraveyardScene.cpp
    ${CMAKE_SOURCE_DIR}/src/SceneControl/Scenes/SettingsMenuScene.cpp
    ${CMAKE_SOURCE_DIR}/src/SceneControl/Scenes/PausedMenuScene.cpp
    ${CMAKE_SOURCE_DIR}/src/SceneControl/Scenes/GameOverScene.cpp
    ${CMAKE_SOURCE_DIR}/src/SceneControl/Scenes/TitleScene.cpp
    ${CMAKE_SOURCE_DIR}/src/SceneControl/Scenes/LevelCompleteScene.cpp
    ${CMAKE_SOURCE_DIR}/src/SceneControl/Scenes/CryptScene.cpp
    ${CMAKE_SOURCE_DIR}/src/SceneControl/Scenes/HealingSpringScene.cpp
    ${CMAKE_SOURCE_DIR}/src/SceneControl/Scenes/RuinSceneLowerFloor.cpp
    ${CMAKE_SOURCE_DIR}/src/SceneControl/Scenes/RuinSceneUpperFloor.cpp
    ${CMAKE_SOURCE_DIR}/src/SceneControl/Scenes/ShopScene.cpp
    ${CMAKE_SOURCE_DIR}/src/SceneControl/SceneData.cpp
    ${CMAKE_SOURCE_DIR}/src/SceneControl/RegistryTransfer.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/ActionFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/SpriteFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/PlayerFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/NpcFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/LootFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/ObstacleFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/BombFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/CryptFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/AltarFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/WallFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/RuinFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/PlantFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/MultiblockFactoryAltarGrave.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/MultiblockFactoryCrypt.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/MultiblockFactoryRuinStairs.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/MultiblockFactoryRuinBuildings.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/MultiblockFactorySpring.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/MultiblockFactoryPlant.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/ShaderFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/ParticleFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/Factory/PathfindingFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/Utils/Utils.cpp
    ${CMAKE_SOURCE_DIR}/src/Utils/ImGuiTheme.cpp
    ${CMAKE_SOURCE_DIR}/src/Utils/Maths.cpp
    ${CMAKE_SOURCE_DIR}/src/Utils/Player.cpp
    ${CMAKE_SOURCE_DIR}/src/Utils/Npc.cpp
    ${CMAKE_SOURCE_DIR}/src/Utils/Collision.cpp
    ${CMAKE_SOURCE_DIR}/src/Utils/Crypt.cpp
    ${CMAKE_SOURCE_DIR}/src/Utils/JsonDeserializer.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Font.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Persistent/BasePersistent.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Persistent/PlayerStartPosition.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Stats/BaseAction.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Stats/PlayerStats.cpp
    ${CMAKE_SOURCE_DIR}/src/PathFinding/SpatialHashGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/PathFinding/AStar.cpp
    ${CMAKE_SOURCE_DIR}/src/Components/Toxicity/Toxidrome.cpp
)

target_precompile_headers(TheShadesBelow PRIVATE

    # STL
    <vector>
    <queue>
    <set>
    <string>
    <memory>
    <algorithm>
    <functional>
    
    # Heavy external libraries
    <entt/entity/registry.hpp>
    <entt/entt.hpp>
    <SFML/Graphics.hpp>
    <SFML/System/Vector2.hpp>
    <spdlog/spdlog.h>
)

target_link_libraries(${TARGET} PRIVATE
    SFML::Graphics
    SFML::Audio
    spdlog::spdlog
    EnTT
    $<$<PLATFORM_ID:Windows>:dbghelp>
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
    stdc++exp
    TracyClient
)
target_include_directories(${TARGET} SYSTEM PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${FETCHCONTENT_BASE_DIR}/imgui-src
    $<$<STREQUAL:${TARGET_TRIPLET},x86_64-w64-mingw32>:/usr/x86_64-w64-mingw32/include/GL/>
)

target_compile_options(${TARGET} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:
        -std=c++2c
        -Werror                         # treat warnings as errors
        -Wpedantic                      # enable all warnings demanded by ISO C++ standard
        -Wall                           # enable all warnings
        -Wextra                         # enable extra warnings
        -ftrivial-auto-var-init=zero    # initialize all automatic variables to zero
        # mingw limits object data sizes so enable large object files (fixes 'file too big' error)
        $<$<STREQUAL:${TARGET_TRIPLET},x86_64-w64-mingw32>:-Wa,-mbig-obj>
    >
)

# Already be disabled with release
# May add some performance benefit with debug but has obvious disadvantages
# target_compile_definitions(${TARGET} PUBLIC ENTT_DISABLE_ASSERT)

