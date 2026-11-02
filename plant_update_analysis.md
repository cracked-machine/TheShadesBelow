# Plant Update (#348): design analysis (rev 7)

## Scope
Based on `res/json/items.json` and the code at `da10d15c` (Fixed longbow damage), re-read from disk. It assumes **every tool is available**, because tool-gating is a deliberate trade-off. It reports only gaps, balance problems and conflicts with folklore or medicine.

## Rules (checked in code)
| Rule | Where |
|---|---|
| Stats and toxidromes are applied once a second. **At 100, every toxidrome drains 5 HP/s**, except Phototoxia, which instead sets the player on fire (5 HP/s) **only while lit** (candles or lava; the torch doesn't count). FEAR or DESPAIR at 100 drains 1 HP/s. | [PlayerSystem.cpp:765-815](src/Systems/PlayerSystem.cpp#L765-L815) |
| Plant **proximity** and **burn** stat effects only apply while the player's hitbox touches the plant. A plant burns for 9s. | `is_player_near`, [PlantSystem.cpp](src/Systems/PlantSystem.cpp) `update_burning_plants` |
| **Healing spring (every level):** within 500px, every second gives +5 HP, -5 FEAR, -5 DESPAIR, -5 INFAMY and -5 to every toxidrome. | [PlayerSystem.cpp:829](src/Systems/PlayerSystem.cpp#L829) |
| **Wear is per action:** a `wear` block on `spawn_action` wears the item in the world, on `carry_action` in the inventory, and on `proximity_action` each time it's applied. With `active_only`, the proximity action only wears ticks that changed a stat. `expiryseconds: 0` means it wears through use only. Plants lose 10% per forage or Hogweed kill, and keep their wear when dug up and replanted. | `WorldItem::get_wear`, [PlayerSystem.cpp](src/Systems/PlayerSystem.cpp) `apply_plant_proximity_modifiers` |
| **Gloves** (Nettle and Hawthorn): the plant's `forage_action` stats are skipped, and the dropped item is marked **safe**, so it has no carry penalty. 3% glove wear per forage. | [PlantSystem.cpp:275](src/Systems/PlantSystem.cpp#L275) |
| One inventory slot. LUCK starts at 50 and only Rowan changes it. Lightning comes only from Hawthorn. | — |

## Resolved since rev 6
- **St John's Wort** only wears on ticks where it actually heals (`proximity_action.wear` 60s, `active_only`). At full health it doesn't wither. About +60 HP of healing per unforaged plant, minus 2 heals per flower foraged. *(was B1)*
- **Burning Hogweed** now adds Phototoxia (+5/3s while touching it), so burning clears the hazard at a cost. *(was G5)*
- **Red Campion** no longer has a leftover burn tick.
- **Longbow** now has a wear level: 1% per shot (`BowReleaseWear`), plus `damage_to_weapon.bow` when an arrow hits a plant.

---

## 1. Gaps

### G1. A worn-out Longbow keeps firing (bug, outside plant data)
When the bow reaches 0 wear, [InventorySystem.cpp:116-135](src/Systems/InventorySystem.cpp#L116-L135) treats it like a dead tool: it plays `tool_break` and **leaves it in the inventory** (unlike gloves, which fall apart). [ArrowSystem.cpp](src/Systems/ArrowSystem.cpp) `on_player_action_event` only checks that the inventory item is `item.longbow`, not its wear level. So a broken bow fires forever. The comment at [ArrowSystem.cpp:165](src/Systems/ArrowSystem.cpp#L165) ("a worn out bow is dropped") also doesn't match what happens. Fix: return early when `Utils::Player::get_inventory_wear_level( reg() ) <= 0`, which is the same guard PlantSystem uses for tools.

### G2. Vertigo and Hypoxia have only generic cures (low priority)
Their only sources are Rowan berries (Vertigo) and Elder berries (Hypoxia), at +5 each. The cures are the spring, BurdockRoot (-25 to everything), HawthornFlower (-1 to everything) and ElderWine (60s prevention). It takes about 20 berries to reach 100, so this is fine as is.

### G3. Phototoxia depends on the level's lighting
In an unlit area Phototoxia does nothing. So the costs of BloodOil (+20) and Hogweed (+5/3s while touching, carrying or burning it) only matter near candles or lava. **BloodOil (repel every NPC for 60s) is close to free in a dark level.** Accept this as counterplay, or give BloodOil a cost that doesn't depend on light (e.g. +DESPAIR).

### G4. Nightshade flowers never rot
Every other foraged edible rots (berries and StJW flower in 30s, leaves and Hawthorn flowers in 60s, seeds in 180s). The Nightshade flower has `expiryseconds: 0` in both the world and the inventory. Players can forage and drop a pile of them and sacrifice them one at a time later, each one a full Azazel and jewelry run. That's probably harmless (each run still costs INFAMY), but it's the one exception in the set. If it isn't deliberate, give it a 60s wear.

### G5. Actions that do nothing (accepted)
- Burning StJW, Burdock or Nettle only removes the plant.
- Chopping Hogweed, Foxglove or Nightshade gives no reward, because the dug-up plant is the useful item (trap / Digitalis / ExtraLife).

### G6. Single-plant stat levers (for level design, not blockers)
- **LUCK:** only Rowan, but it drives every `player_luck` roll:
  - Elder chop
  - Hawthorn lightning
  - Hogweed kill
  - Mimic teleport
- **FEAR sinks:** HawthornTea, HawthornFlower, the spring.
- **INFAMY sinks:** YewBerry, StJW plant sacrifice, the spring. There's a spring on every level, so nothing can get stuck.

---

## 2. Balance

### In range ✔
| Item | Current |
|---|---|
| Nettle | Bare-handed: -5 to forage, -1/3s while carried, +5 when eaten, so about break-even if eaten quickly. Gloves: +5 net. |
| Hawthorn | Bare-handed: -5 to forage (plus a luck roll for lightning), -1 HP and +1 DESPAIR every 3s while the flower is carried, which becomes HawthornTea (-20 FEAR, -20 heart toxidromes). Gloves: free apart from wear. |
| St John's Wort | +3 HP/3s while touching it, only wearing while it heals (about +60 HP per plant). Flower -5 DESPAIR. The plant sacrifice gives -10 INFAMY. |
| Berries, FoxgloveFlower | +3 HP for +5 toxicity |
| ElderWine | 60s immunity to every toxidrome, from one berry |
| Elder proximity | +2 FEAR/3s while touching it |
| Heart toxidromes | Bradycardia: HawthornTea, HenbaneSeed. Tachycardia: HawthornTea, Digitalis. |
| Hallucinogen | Now lethal at 100. HogweedSeed (-25) is a real cure. |
| Hogweed | Trap: luck roll per kill, 10% wear per kill. +5 Phototoxia/3s while touching, carrying or burning it. |
| Burning plants | Effects only while touching, for 9s. The worst is Nightshade at +10 Tachycardia/s. |
| Nightshade | Each flower is an Azazel encounter plus 9 jewelry (+18 wealth / +36 INFAMY). The plant gives an extra life. |
| Longbow (Yew) | About 100 shots, fewer if arrows hit plants |
| Rowan, Elder, Burdock, Red Campion, Digitalis, Antivenom | Their trade-offs are intact |

### Still worth a look
| # | Item | Issue | Suggestion |
|---|---|---|---|
| B1 | **Hogweed next to Red Campion** | The kill exception only spares the Hogweed's own spawns, so a replanted Hogweed burns Red Campion's adders. That removes the Red Campion risk for up to 3 of the Hogweed's 10 actions. | Decide whether this is an intended combo. The price seems fair. |
| B2 | **BloodOil** | See G3: 60s of total NPC repel, with a light-dependent cost | Optional extra cost |

---

## 3. Folklore and medicine
No conflicts remain in the core design. Optional polish:
| Item | Note |
|---|---|
| HogweedSeed cures Hallucinogen | No traditional basis. The **StJW flower** (*fuga daemonum*, "drives away demons") would fit better. |
| BurdockRoot cure on Sacrifice | It was traditionally eaten or drunk, so Consume fits better. |

Consistent:
- Heart axis: Yew/Foxglove ↔ Henbane, Nightshade/Henbane smoke ↔ Digitalis, Hawthorn tea for both
- Elder lore
- Hawthorn death smell and lightning
- Yew longbow and graveyard offering
- Foxglove fairies
- Belladonna
- St John's Wort oil and despair
- Nettle fibre gloves and food
- Red Campion adders and antivenom
- Burryman
- Hogweed sap and smoke burns

---

## Priority
1. **G1:** stop a worn-out Longbow firing.
2. Optional: G4 (Nightshade flower rot), G3/B2 (BloodOil cost in the dark), B1 (Hogweed vs Red Campion), §3 polish.
