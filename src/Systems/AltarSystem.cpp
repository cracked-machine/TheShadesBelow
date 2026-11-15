#include <Audio/SoundBank.hpp>
#include <Components/AbsoluteAlpha.hpp>
#include <Components/Altar/Activated.hpp>
#include <Components/Altar/AltarActivationTimer.hpp>
#include <Components/Altar/ArtefactCount.hpp>
#include <Components/Altar/MultiBlock.hpp>
#include <Components/Altar/Offering.hpp>
#include <Components/Altar/Segment.hpp>
#include <Components/AnimData.hpp>
#include <Components/Armable.hpp>
#include <Components/DestroyedObstacle.hpp>
#include <Components/Inventory/FlashUIAnima.hpp>
#include <Components/Inventory/FlashUIExtraLife.hpp>
#include <Components/Inventory/FlashUIHealth.hpp>
#include <Components/Inventory/FlashUIInventory.hpp>
#include <Components/Inventory/FlashUIWealth.hpp>
#include <Components/Inventory/PlayerInventorySlot.hpp>
#include <Components/LootContainer.hpp>
#include <Components/Npc/NoPathFinding.hpp>
#include <Components/Npc/Npc.hpp>
#include <Components/Npc/SummonOffering.hpp>
#include <Components/Particle/SpriteOwner.hpp>
#include <Components/Persistent/AltarActivationAnimaCost.hpp>
#include <Components/Persistent/AltarOfferingCooldown.hpp>
#include <Components/Persistent/AltarSummonOfferingCooldown.hpp>
#include <Components/Player/Character.hpp>
#include <Components/Player/ExtraLife.hpp>
#include <Components/Player/KeysCount.hpp>
#include <Components/Player/ToxicImmunity.hpp>
#include <Components/Player/Wealth.hpp>
#include <Components/Random.hpp>
#include <Components/RectBounds.hpp>
#include <Components/Stats/ActionReward.hpp>
#include <Components/Stats/OfferingAction.hpp>
#include <Components/Stats/PlayerStats.hpp>
#include <Components/Stats/SideEffects.hpp>
#include <Components/Toxicity/Bradycadia.hpp>
#include <Components/Toxicity/Halucinogen.hpp>
#include <Components/Toxicity/Hypoxia.hpp>
#include <Components/Toxicity/Phototoxia.hpp>
#include <Components/Toxicity/Tachycardia.hpp>
#include <Components/Toxicity/Venom.hpp>
#include <Components/Toxicity/Vertigo.hpp>
#include <Components/UUID.hpp>
#include <Components/ZOrderValue.hpp>
#include <Events/AddInventoryEvent.hpp>
#include <Events/AltarActivatedEvent.hpp>
#include <Events/AltarActivationAnimaEvent.hpp>
#include <Events/AltarApproachedEvent.hpp>
#include <Events/ArtefactRefusedEvent.hpp>
#include <Events/CreateItemEvent.hpp>
#include <Events/DestroyNpcEvent.hpp>
#include <Events/GraveTrailCompleteEvent.hpp>
#include <Events/InsufficientAnimaEvent.hpp>
#include <Events/ItemPickedUpEvent.hpp>
#include <Events/PlayerActionEvent.hpp>
#include <Factory/AltarFactory.hpp>
#include <Factory/ParticleFactory.hpp>
#include <Factory/PlayerFactory.hpp>
#include <SFML/System/Time.hpp>
#include <Systems/AltarSystem.hpp>
#include <Systems/ParticleSystem.hpp>
#include <Systems/PersistSystemImpl.hpp>
#include <Systems/Render/RenderSystem.hpp>
#include <Systems/Stores/ItemStore.hpp>
#include <Systems/Stores/SpriteStore.hpp>
#include <Systems/Stores/StoreKey.hpp>
#include <Utils/Npc.hpp>
#include <Utils/Optimizations.hpp>
#include <Utils/Player.hpp>
#include <Utils/Utils.hpp>

#include <stdexcept>
#include <vector>

namespace Game::Sys
{

AltarSystem::AltarSystem( entt::registry &reg, sf::RenderWindow &window, Audio::SoundBank &sound_bank )
    : BaseSystem( reg, window, sound_bank )
{
  std::ignore = get_systems_event_queue().sink<Events::PlayerActionEvent>().connect<&AltarSystem::on_player_action>( this );
  std::ignore = get_systems_event_queue().sink<Events::GraveTrailCompleteEvent>().connect<&AltarSystem::on_grave_trail_complete>( this );
}

void AltarSystem::on_grave_trail_complete( [[maybe_unused]] const Events::GraveTrailCompleteEvent &event )
{
  // reset the activated altar to unlit, so it takes its four artefacts again for another trail
  for ( auto [altar_entt, artefact_cmp, altar_uuid] : reg().view<Cmp::Altar::Activated, Cmp::Altar::ArtefactCount, Cmp::UUID>().each() )
  {
    artefact_cmp.reset();
    // put out its corner lights and centre column; an extra life's particles on the same altar are left alone.
    // Collected first, so the particle view is not modified while it is iterated
    std::vector<entt::entity> particle_entts;
    for ( auto [particle_entt, particle_cmp, particle_uuid] : reg().view<Cmp::Particle::SpriteOwner, Cmp::UUID>().each() )
    {
      if ( particle_uuid != altar_uuid ) continue;
      const auto tag = particle_cmp.sprite->get_tag();
      if ( tag != Factory::Altar::kCandleFlameParticleTag and tag != Factory::Altar::kActivatedParticleTag ) continue;
      particle_entts.push_back( particle_entt );
    }
    reg().destroy( particle_entts.begin(), particle_entts.end() );
    if ( auto *anim_cmp = reg().try_get<Cmp::AnimData>( altar_entt ) )
    {
      anim_cmp->m_sprite_type = Factory::Altar::get_altar_sprite( anim_cmp->m_sprite_type, false );
    }
    SPDLOG_INFO( "Grave trail complete, altar {} reset", static_cast<uint32_t>( altar_entt ) );
  }
  reg().clear<Cmp::Altar::Activated>();
}

void AltarSystem::on_player_action( Events::PlayerActionEvent ev )
{
  if ( ev.action != Events::PlayerActionEvent::GameActions::ACTIVATE ) return;

  for ( auto [altar_entity, altar_cmp] : reg().view<Cmp::Altar::MultiBlock>().each() )
  {
    if ( not Utils::Player::is_player_near( reg(), altar_cmp ) ) continue;

    SPDLOG_DEBUG( "Player collided with Altar at ({}, {})", altar_cmp.position.x, altar_cmp.position.y );
    // whatever the player holds, an altar with every artefact offered can be activated, unless the press was spent
    // on an offering
    if ( not check_player_altar_offering( altar_entity, altar_cmp ) ) try_activate_altar( altar_entity, altar_cmp );
  }
}

void AltarSystem::update( sf::Time dt )
{
  if ( auto *altar_timer = reg().try_get<Cmp::Altar::ActivationTimer>( Utils::Player::get_entity( reg() ) ) ) *altar_timer += dt;
  check_player_approached_altar();
  // tidy up any dead altar offering animations
  auto altar_offering_view = reg().view<Cmp::Altar::Offering, Cmp::AnimData>();
  for ( auto [altar_offering_entt, altar_offering_cmp, altar_offering_anim_cmp] : altar_offering_view.each() )
  {
    if ( not altar_offering_anim_cmp.m_enabled )
    {
      if ( reg().valid( altar_offering_entt ) ) { reg().destroy( altar_offering_entt ); }
    }
  }
}

void AltarSystem::check_player_approached_altar()
{
  // only a graveyard altar still waiting for artefacts asks for them
  std::optional<sf::FloatRect> near_altar;
  for ( auto [altar_entt, altar_cmp, artefact_cmp] : reg().view<Cmp::Altar::MultiBlock, Cmp::Altar::ArtefactCount>().each() )
  {
    if ( altar_cmp.get_kind() != Cmp::Altar::Kind::GRAVEYARD or artefact_cmp.all_lit() ) continue;
    if ( Utils::Player::is_player_near( reg(), altar_cmp ) )
    {
      near_altar = sf::FloatRect( altar_cmp.position, altar_cmp.size );
      break;
    }
  }
  // once per approach, not every frame the player stays near
  if ( near_altar and not m_player_near_altar ) get_systems_event_queue().trigger( Events::AltarApproachedEvent{ *near_altar } );
  m_player_near_altar = near_altar.has_value();
}

bool AltarSystem::check_player_altar_offering( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp )
{
  auto *altar_timer = reg().try_get<Cmp::Altar::ActivationTimer>( Utils::Player::get_entity( reg() ) );
  if ( altar_timer )
  {
    // summoning offerings (e.g. Azazel's jewelry) come in batches, so they can be offered in quicker succession
    const auto inventory_entt = Utils::Player::get_inventory_entt( reg() );
    const bool is_summon_offering = reg().valid( inventory_entt ) and reg().all_of<Cmp::Npc::SummonOffering>( inventory_entt );
    const sf::Time timeout = is_summon_offering
                                 ? sf::seconds( Sys::PersistSystem::get<Cmp::Persist::AltarSummonOfferingCooldown>( reg() ).get_value() )
                                 : sf::seconds( Sys::PersistSystem::get<Cmp::Persist::AltarOfferingCooldown>( reg() ).get_value() );
    if ( *altar_timer < timeout ) return false;
  }

  auto *altar_uuid_cmp = reg().try_get<Cmp::UUID>( altar_entity );
  if ( not altar_uuid_cmp ) throw std::runtime_error( "Altar does not have UUID" );

  auto [_, inventory_type, _] = Utils::Player::get_inventory( reg() );
  if ( inventory_type == "" ) return false;
  const auto &item = Sys::ItemStore::instance().get( inventory_type );
  if ( not item.is_offerable() ) return false;

  auto is_carrying_tool = [&]()
  {
    return item.item_type.contains( "item.axe" ) or item.item_type.contains( "item.pickaxe" ) or item.item_type.contains( "item.shovel" ) or
           item.item_type.contains( "item.dowsingrod" );
  };

  // only spent tools can be offered
  if ( is_carrying_tool() and Utils::Player::get_inventory_wear_level( reg() ) > 0 ) { return false; }

  // the altar refuses an offering whose NPC it has already summoned up to max_instances, otherwise the item is consumed for nothing
  if ( const auto spawn_npc = item.get_offering_npc_spawn_effect();
       spawn_npc and spawn_npc->max_instances > 0 and Utils::Npc::count_spawned_by( reg(), *altar_uuid_cmp ) >= spawn_npc->max_instances )
  {
    return false;
  }

  // an offering whose rewards are all for another kind of altar is refused (e.g. nightshade at the crypt altar)
  const auto reward = item.get_offering_reward( altar_cmp.get_kind() );
  if ( not reward and not item.get_rewards<Cmp::OfferingAction>().empty() ) return false;

  // the altar may refuse the offering (e.g. a relic once the exitkey has dropped), so only consume the item once it has accepted
  if ( not grant_offering_reward( altar_entity, altar_cmp, *altar_uuid_cmp, reward ) ) return false;

  SPDLOG_INFO( "Player made an offering: {}", item.item_type );
  const auto offering_action = Utils::Player::apply_action_from_item_store<Cmp::OfferingAction>( reg(), item.item_type );
  play_action_sfx( offering_action );
  trigger_side_effects( offering_action, Cmp::Position( altar_cmp.position, altar_cmp.size ), altar_entity );
  Factory::Player::destroy_inventory( reg(), item.item_type );
  // the exit key has left the inventory, so the crypt key can take its place
  if ( reward and reward->altar == Cmp::ActionReward::AltarEffect::CRYPT_KEY ) give_reward_key( "item.cryptkey" );
  // the offering has left the inventory, so an inventory reward item can take its place
  if ( reward and not reward->item.empty() )
  {
    // items dropped by a summoning offering are bound to the summoned NPC, e.g. Azazel stays until its jewelry is offered
    std::function<void( entt::entity )> on_created;
    if ( offering_action.side_effects().spawn_npc )
    {
      on_created = [this, summoner = *altar_uuid_cmp]( entt::entity item_entt )
      { reg().emplace_or_replace<Cmp::Npc::SummonOffering>( item_entt, summoner ); };
    }
    offering_for_item( altar_cmp, *reward, on_created );
  }
  return true;
}

bool AltarSystem::grant_offering_reward( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid,
                                         const std::optional<Cmp::ActionReward> &reward )
{
  if ( not reward )
  {
    play_offering_anim( altar_cmp, OfferingAnimType::DEFAULT );
    return true;
  }
  if ( not reward->item.empty() )
  {
    play_offering_anim( altar_cmp, OfferingAnimType::DEFAULT );
    return true;
  }
  if ( reward->wealth > 0 )
  {
    offering_for_wealth( altar_cmp, reward->wealth );
    return true;
  }

  using AltarEffect = Cmp::ActionReward::AltarEffect;
  switch ( reward->altar )
  {
    case AltarEffect::ARTEFACT:
      if ( not reward->artefact_group ) return false;
      return offering_artefact( altar_entity, altar_cmp, altar_uuid, *reward->artefact_group );
    case AltarEffect::BANISH:
      offering_for_banish( altar_cmp );
      return true;
    case AltarEffect::EXTRA_LIFE:
      return offering_for_extra_life( altar_entity, altar_cmp, altar_uuid );
    case AltarEffect::CRYPT_KEY:
      return offering_cryptkey( altar_cmp );
    case AltarEffect::TOXIC_IMMUNITY:
      offering_for_toxic_immunity( altar_cmp, reward->tick );
      return true;
    case AltarEffect::NONE:
      play_offering_anim( altar_cmp, OfferingAnimType::DEFAULT );
      return true;
  }
  return false;
}

void AltarSystem::offering_for_item( const Cmp::Altar::MultiBlock &altar_cmp, const Cmp::ActionReward &reward,
                                     const std::function<void( entt::entity )> &on_created )
{
  if ( reward.target == Cmp::ActionReward::Target::DEFAULT )
  {
    get_systems_event_queue().trigger( Events::CreateItemEvent( Utils::Player::get_position( reg() ),
                                                                Sys::ItemStore::instance().pick_item( reward.item ), "drop_loot", 0.f, false, 0,
                                                                entt::null, on_created ) );
    return;
  }
  grant_item_reward( reward, Cmp::Position( altar_cmp.position, altar_cmp.size ), entt::null, on_created );
}

void AltarSystem::offering_for_wealth( const Cmp::Altar::MultiBlock &altar_cmp, int wealth )
{
  play_offering_anim( altar_cmp, OfferingAnimType::DEFAULT );
  Utils::Player::get_wealth( reg() ).wealth += wealth;
  m_sound_bank.get_effect( "get_loot" ).play();
  // signal UI to flash
  reg().emplace_or_replace<Cmp::FlashUIWealth>( reg().create() );
}

void AltarSystem::offering_for_toxic_immunity( const Cmp::Altar::MultiBlock &altar_cmp, int tick )
{
  play_offering_anim( altar_cmp, OfferingAnimType::DEFAULT );
  // offering again restarts the timer. PlayerSystem keeps the PlayerStats flag in sync, but set it now so
  // immunity applies from this frame
  reg().emplace_or_replace<Cmp::Player::ToxicImmunity>( Utils::Player::get_entity( reg() ), sf::seconds( static_cast<float>( tick ) ) );
  Utils::Player::get_stats( reg() ).set_toxic_immune( true );
}

void AltarSystem::offering_for_banish( const Cmp::Altar::MultiBlock &altar_cmp )
{
  // player kills all spawned ghosts in the game
  play_offering_anim( altar_cmp, OfferingAnimType::DEFAULT );
  for ( auto [npc_entt, npc_cmp, npc_pos_cmp, npc_anim_cmp] : reg().view<Cmp::Npc::NPC, Cmp::Position, Cmp::AnimData>().each() )
  {
    if ( not npc_anim_cmp.m_sprite_type.contains( "ghost" ) ) continue;
    get_systems_event_queue().trigger( Events::DestroyNpcEvent( npc_entt, Sys::SpriteKey( "sprite.death.anim.banish" ) ) );
  }
  m_sound_bank.get_effect( "witches_jar_offering" ).play();
}

bool AltarSystem::offering_for_extra_life( entt::entity altar_entity, const Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid )
{
  // refuse the offering while an extra life is already active, so it isn't wasted
  if ( Utils::Player::player_has_extra_life( reg() ) ) return false;

  // only the crypt altar grants an extra life; a graveyard altar's active sprite shows its grave trail instead
  if ( altar_cmp.get_kind() != Cmp::Altar::Kind::CRYPT ) return false;
  if ( not Utils::Player::get_projected_position( reg() ).findIntersection( altar_cmp ) ) return false;

  // the player respawns at the crypt start position
  Cmp::Player::ExtraLife extra_life;
  extra_life.altar_uuid = altar_uuid;

  // the altar stays lit while the extra life is held, see Factory::Player::remove_player_extra_life()
  activate_altar( altar_entity, altar_cmp, altar_uuid, Factory::Altar::kExtraLifeParticleTag );
  // both go on the player so they transfer between scenes
  reg().emplace_or_replace<Cmp::FlashUIExtraLife>( Utils::Player::get_entity( reg() ) );
  reg().emplace_or_replace<Cmp::Player::ExtraLife>( Utils::Player::get_entity( reg() ), extra_life );
  m_sound_bank.get_effect( "witches_jar_offering" ).play();
  return true;
}

bool AltarSystem::offering_artefact( entt::entity altar_entity, Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid,
                                     Cmp::Altar::ArtefactGroup group )
{
  auto *artefact_cmp = reg().try_get<Cmp::Altar::ArtefactCount>( altar_entity );
  if ( not artefact_cmp ) return false;

  auto *anim_cmp = reg().try_get<Cmp::AnimData>( altar_entity );
  if ( not anim_cmp ) return false;
  // the corner light offsets are defined on the inactive sprite
  const Sys::SpriteKey inactive_sprite_type = Factory::Altar::get_altar_sprite( anim_cmp->m_sprite_type, false );
  if ( inactive_sprite_type == anim_cmp->m_sprite_type and not inactive_sprite_type.str().ends_with( ".inactive" ) )
  {
    SPDLOG_WARN( "Altar sprite {} has no active/inactive variant", inactive_sprite_type.str() );
    return false;
  }

  auto [_, item_type, _] = Utils::Player::get_inventory( reg() );
  if ( item_type.empty() ) return false;
  const Sys::ItemKey artefact_key( item_type );

  // once an altar is activated its grave trail must be completed first, so no altar takes artefacts until then
  if ( not reg().view<Cmp::Altar::Activated>().empty() )
  {
    SPDLOG_INFO( "An altar is already activated, refusing {} until its grave trail is complete", item_type );
    get_systems_event_queue().trigger( Events::ArtefactRefusedEvent( artefact_key, sf::FloatRect( altar_cmp.position, altar_cmp.size ) ) );
    return false;
  }
  // only one artefact of each group
  if ( artefact_cmp->activated( group ) ) return false;

  artefact_cmp->activate( group );
  SPDLOG_INFO( "Offered {} for {} ({} of {})", item_type, Cmp::Altar::to_string( group ), artefact_cmp->count(), artefact_cmp->size() );

  // the group picks its corner light, whatever order the artefacts are offered in
  const std::size_t flame_idx = Cmp::Altar::ArtefactCount::index( group );
  const auto &flame_offsets = Sys::SpriteStore::instance().get( inactive_sprite_type ).particle_offsets();
  if ( flame_idx < flame_offsets.size() )
  {
    sf::Vector2f flame_ps_pos = altar_cmp.position + flame_offsets[flame_idx];
    Factory::Particle::add_flame( m_reg, Factory::Altar::kCandleFlameParticleTag, altar_uuid, flame_ps_pos, 5000, Cmp::Particle::kAltarScalePreset );
  }
  else { SPDLOG_WARN( "{} has no particle_offsets entry for artefact index {}", inactive_sprite_type.str(), flame_idx ); }
  play_offering_anim( altar_cmp, OfferingAnimType::DEFAULT );
  return true;
}

void AltarSystem::try_activate_altar( entt::entity altar_entity, const Cmp::Altar::MultiBlock &altar_cmp )
{
  if ( altar_cmp.get_kind() != Cmp::Altar::Kind::GRAVEYARD ) return;
  const auto *artefact_cmp = reg().try_get<Cmp::Altar::ArtefactCount>( altar_entity );
  if ( not artefact_cmp or not artefact_cmp->all_lit() ) return;
  // only one grave trail at a time
  if ( not reg().view<Cmp::Altar::Activated>().empty() ) return;
  // the same cooldown as an offering, so the press that offers the last artefact doesn't also activate the altar
  if ( const auto *altar_timer = reg().try_get<Cmp::Altar::ActivationTimer>( Utils::Player::get_entity( reg() ) );
       altar_timer and *altar_timer < sf::seconds( Sys::PersistSystem::get<Cmp::Persist::AltarOfferingCooldown>( reg() ).get_value() ) )
  {
    return;
  }
  auto *altar_uuid_cmp = reg().try_get<Cmp::UUID>( altar_entity );
  if ( not altar_uuid_cmp ) throw std::runtime_error( "Altar does not have UUID" );

  auto &stats = Utils::Player::get_stats( reg() );
  const int anima_cost = Sys::PersistSystem::get<Cmp::Persist::AltarActivationAnimaCost>( reg() ).get_value();
  if ( stats.anima() < anima_cost )
  {
    SPDLOG_INFO( "Not enough anima to activate the altar (need {}, have {})", anima_cost, stats.anima() );
    // signal UI to flash the anima meter and explain why
    reg().emplace_or_replace<Cmp::FlashUIAnima>( reg().create() );
    get_systems_event_queue().trigger( Events::AltarActivationAnimaEvent{ sf::FloatRect( altar_cmp.position, altar_cmp.size ) } );
    // so a held ACTIVATE doesn't repeat the refusal every frame
    reg().emplace_or_replace<Cmp::Altar::ActivationTimer>( Utils::Player::get_entity( reg() ) );
    return;
  }
  stats.apply( { {}, {}, {}, {}, {}, Cmp::Stats::Anima{ -anima_cost }, {} } );

  // light the centre column
  activate_altar( altar_entity, altar_cmp, *altar_uuid_cmp, Factory::Altar::kActivatedParticleTag );
  reg().emplace_or_replace<Cmp::Altar::Activated>( altar_entity );
  play_offering_anim( altar_cmp, OfferingAnimType::DEFAULT );
  SPDLOG_INFO( "Altar {} activated for {} anima", static_cast<uint32_t>( altar_entity ), anima_cost );
  // GraveSystem picks the trail start graves that TrailGuideSystem then guides the player to
  get_systems_event_queue().trigger( Events::AltarActivatedEvent{ Cmp::Position( altar_cmp.position, altar_cmp.size ) } );
}

bool AltarSystem::can_afford_offering( const Sys::ItemKey &item_key )
{
  // the item's (negative) offering anima is its cost, paid by check_player_altar_offering() once the altar accepts it
  const int anima_cost = -Utils::Player::get_action_from_item_store<Cmp::OfferingAction>( item_key ).anima();
  if ( Utils::Player::get_stats( reg() ).anima() >= anima_cost ) return true;

  SPDLOG_INFO( "Not enough anima to offer {} (need {})", item_key.str(), anima_cost );
  // signal UI to flash the anima meter and explain why
  reg().emplace_or_replace<Cmp::FlashUIAnima>( reg().create() );
  get_systems_event_queue().trigger( Events::InsufficientAnimaEvent( item_key ) );
  return false;
}

bool AltarSystem::offering_cryptkey( const Cmp::Altar::MultiBlock &altar_cmp )
{
  if ( not can_afford_offering( "item.exitkey" ) ) return false;
  play_offering_anim( altar_cmp, OfferingAnimType::KEY );
  m_sound_bank.get_effect( "crypt_altar_activate" ).play();
  return true;
}

void AltarSystem::activate_altar( entt::entity altar_entity, const Cmp::Altar::MultiBlock &altar_cmp, Cmp::UUID &altar_uuid,
                                  const char *particle_tag )
{
  auto *anim_cmp = reg().try_get<Cmp::AnimData>( altar_entity );
  if ( not anim_cmp ) return;
  const Sys::SpriteKey active_sprite_type = Factory::Altar::get_altar_sprite( anim_cmp->m_sprite_type, true );
  if ( anim_cmp->m_sprite_type == active_sprite_type ) return;

  anim_cmp->m_sprite_type = active_sprite_type;
  const sf::Vector2f particle_offset = ( altar_cmp.get_kind() == Cmp::Altar::Kind::CRYPT ) ? sf::Vector2f{ 8.f, 24.f } : sf::Vector2f{ 40.f, 21.f };
  Factory::Particle::add_crypt_altar_ps( m_reg, particle_tag, 0.5, 25.f, altar_uuid, altar_cmp.position + particle_offset, 5000 );
  m_sound_bank.get_effect( "crypt_altar_activate" ).play();
}

void AltarSystem::play_offering_anim( const Cmp::Altar::MultiBlock &altar_cmp, OfferingAnimType offering_anim_type )
{
  Sys::SpriteKey sprite_type = ( offering_anim_type == OfferingAnimType::KEY ) ? "sprite.graveyard.altar.key.anim"
                                                                               : "sprite.graveyard.altar.relic.anim";

  float altar_offering_anim_height = Sys::SpriteStore::instance().get( sprite_type ).sprite_size().y;
  // get the center (topleft coord), then adjust to center the altar_offering_anim, then adjust for altar_offering_anim height
  Cmp::Position new_pos( altar_cmp.getCenter() - sf::Vector2{ 7.f, 28.f } - sf::Vector2{ 0.f, altar_offering_anim_height }, Constants::kGridSizePxF );
  Factory::Altar::create_altar_offering_anim( reg(), new_pos, sprite_type );

  reg().emplace_or_replace<Cmp::Altar::ActivationTimer>( Utils::Player::get_entity( reg() ) );
}

void AltarSystem::give_reward_key( const Sys::ItemKey &item_key )
{
  get_systems_event_queue().trigger( Events::AddInventoryEvent( item_key ) );
  m_sound_bank.get_effect( "equip_inventory" ).play();
  // signal UI to flash
  reg().emplace_or_replace<Cmp::FlashUIInventory>( reg().create() );
  // enqueued rather than triggered, as the caller may still be iterating registry views
  get_systems_event_queue().enqueue( Events::ItemPickedUpEvent( item_key ) );
  SPDLOG_INFO( "Altar granted {}", item_key.str() );
}

} // namespace Game::Sys
