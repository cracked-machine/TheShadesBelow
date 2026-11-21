# Plant Stat Rebalance

## Design intent

The environment raises the player's stats over time:

| Stat | Sources |
|---|---|
| Fear | Nearby NPCs |
| Despair | Talismans |
| Infamy | Exhumed loot, cadavers |
| Luck | Ticks down continuously |

The plant ecosystem is the player's counter to this pressure. Plants and plant-derived items (forage, destroy and offering products) **only benefit the player**, with two exceptions:

- **Toxidromes** stay. They reflect real plant toxicology, e.g. taxine in yew and atropine in nightshade.
- **Physical stings and thorns** stay (nettle, hawthorn, burr). They drive the gloves mechanic through `WorldItem::stings()`.

Stat polarity: higher health and luck are good; higher fear, despair and infamy are bad.

## Specialty stat per plant

Each plant counters one pressure, chosen from its folklore or medicinal history.

| Plant | Specialty | Basis |
|---|---|---|
| Rowan | Luck + | "Rowan tree and red thread" witch charm |
| Henbane | Luck + | Germanic divination and rain rites, Delphic oracle fumes |
| Red campion | Luck + | "Robin's flower" (Robin Goodfellow) |
| Elder | Fear − | Hung over doors to bar witches |
| Nettle | Fear − | Carried with yarrow "to drive away fear" |
| Nightshade | Fear − | Belladonna as an anodyne and sedative |
| Hawthorn | Despair − | Heart tonic; May blossom as a symbol of hope |
| St John's wort | Despair − | Historic remedy for melancholy; *fuga daemonum* |
| Yew | Infamy − | Churchyard guardian of the dead |
| Burdock | Health + | Blood purifier |
| Foxglove | Health + | Withering's digitalis heart remedy (1785) |
| Hogweed | Health + | *Heracleum*, Hercules' strength herb |

## Magnitudes

| Source | Specialty change | Notes |
|---|---|---|
| Plant `offering_action` | ±20 | One-off |
| Plant `forage_action` | ±5 | A lesser dose; repeats until the plant withers (about 10 forages) |
| Rowan, elder and hawthorn `proximity_action` | ±1 every 3s | |
| Edible product `consume_action` | ±10 | Berries, flowers, leaf, seeds |
| Brewed drink `consume_action` | ±15 | Elder wine, hawthorn tea, rowan wine, digitalis, antivenom |

## `res/json/items.json` changes

### Plants

| Item | Action | Before | After |
|---|---|---|---|
| `item.plant.rowan` | offering | luck 0 | luck +20 |
| | forage | luck 0 | luck +5 |
| | burn | luck −25 | luck 0 |
| | destroy | luck −10 | luck 0 |
| `item.plant.henbane` | offering / forage | luck 0 | luck +20 / +5 |
| `item.plant.redcampion` | offering | luck 0 | luck +20 |
| `item.plant.elder` | offering / forage | fear 0 | fear −20 / −5 |
| | proximity | fear +2 | fear −1 |
| `item.plant.nettle` | offering / forage | fear 0 | fear −20 / −5 (forage keeps sting health −5) |
| `item.plant.nightshade` | offering / forage | fear 0 | fear −20 / −5 |
| | carry | fear +1 | fear 0 (keeps tachycardia 1) |
| `item.plant.hawthorn` | offering / forage | despair 0 | despair −20 / −5 (forage keeps thorn health −5) |
| | proximity | despair +1 | despair −1 |
| `item.plant.stjohnswort` | offering | despair 0 | despair −20 (keeps infamy −10) |
| | forage | despair 0 | despair −5 |
| `item.plant.yew` | offering / forage | infamy 0 | infamy −20 / −5 |
| `item.plant.burdock` | offering / forage | health 0 | health +20 / +5 |
| `item.plant.foxglove` | offering / forage | health 0 | health +20 / +5 |
| `item.plant.hogweed` | offering / forage | health 0 | health +20 / +5 |

### Edible products (consume)

| Item | Before | After |
|---|---|---|
| `item.rowanberries` | luck 0 | luck +10 |
| `item.henbaneseed` | luck 0 | luck +10 |
| `item.elderberries` | fear 0 | fear −10 |
| `item.nettleleaf` | fear 0 | fear −10 (keeps health +5) |
| `item.nightshadeflower` | fear 0 | fear −10 |
| `item.hawthornflower` | despair 0 | despair −10 |
| `item.stjohnswortflower` | despair −5 | despair −10 |
| `item.yewberry` | infamy 0 | infamy −10 |
| `item.foxgloveflower` | health +3 | health +10 |
| `item.hogweedseed` | health 0 | health +10 |

Carry-action fixes on products:

| Item | Before | After |
|---|---|---|
| `item.nightshadeflower` | fear +1 | fear 0 (keeps tachycardia 1) |
| `item.hawthornflower` | despair +1 | despair −1 (keeps sting health −1) |

### Brewed drinks (consume)

| Item | Before | After |
|---|---|---|
| `item.elderwine` | fear 0 | fear −15 |
| `item.hawthorntea` | despair 0 | despair −15 (keeps fear −20) |
| `item.rowanwine` | luck +10 | luck +15 |
| `item.digitalis` | health 0 | health +15 |
| `item.antivenom` | luck 0 | luck +15 |

### Tools crafted from plant offerings (carry)

| Item | Before | After | Basis |
|---|---|---|---|
| `item.dowsingrod` | luck −1 | luck +1 | Dowsing finds hidden fortune |
| `item.elderflute` | despair +1 | despair −1 | Elder pipes cheer and ward |
| `item.witchesbroom` | infamy +1 | infamy 0, toxidrome `halucinogen` 1 | Henbane flying ointment |

### Unchanged on purpose

- **Sting and thorn health damage:**
  - nettle: carry −3, proximity −1, forage −5
  - nettleleaf: carry −1
  - hawthorn: carry −1, forage −5
  - hawthornflower: carry −1
  - burr: carry −3
- All positive toxidromes on plants and their products.
- Side effects and rewards: Elder Mother, Burryman, lightning, mimic, Azazel, altars, item rewards.

## `res/json/npc.json` changes

NPC spawns no longer add despair. Proximity, collision and projectile actions are unchanged.

| NPC | `spawn_action` before | After |
|---|---|---|
| `npc.adder` | despair +1 | despair 0 |
| `npc.spider` | despair +1 | despair 0 |
| `npc.nightwatchman` | despair +1 | despair 0 |
| `npc.eldermother` | despair +1 | despair 0 (keeps health −1, fear +1) |

## Code change: gloves

Gloves now block only the forage **sting**, not the benefit. Before this change, `PlantSystem::forage_plant()` skipped every forage stat change when the player wore gloves, which would also have cancelled the new forage buffs.

With gloves, the forage action is now copied, `remove_damage()` is applied to the copy, and then its stats and the Grimoire update are applied. This matches the existing carry-path behaviour in `PlayerSystem`.

Files:
- `src/Systems/PlantSystem.cpp`
- `src/Systems/PlantSystem.hpp` (comment)
