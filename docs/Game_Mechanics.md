# AntGame Mechanics

## Inferred Game Mechanics (from C++ server source)
- Map: 500×500 (smallMap), nests at corners, ~18,990 food tiles (never respawn)
- Config: timeScale=4 (was 8), startingFood=60, foodYield=1, foodTheftYield=3, all other mods=1
- `hungerRate` is parsed from config but never applied — food drain is simply `foodCount -= delta` with no multiplier
- Food drain: foodCount -= delta each frame (delta = realtime_elapsed × timeScale). ~4 food/real-sec at timeScale=4
- Nest death: foodCount <= 0 → nest eliminated, rank = remaining_nests - dead_nests
- Max ants: 255 (hard cap in server: ants.size() >= 255 blocks newAnt)
- Winner: last nest standing. Rank by order of elimination
- Ranking tiebreak: timeLasted += winner.foodCount (more food = lasted longer)
- AntType attributes inaccessible before connect() — must hardcode constants
- Python module reads Ant::antTypes[] from map.hpp — must rebuild on type changes
- Must rebuild BOTH server AND module on type changes: `make && touch source/AntGamemodule.cpp && make module`

## Ant Types (Rebalanced in map.hpp — base stats × mod=1)
| Type    | dmg  | rate | cost | hp   | speed | capacity | range |
|---------|------|------|------|------|-------|----------|-------|
| DEFAULT | 1.0  | 0.1  | 5.0  | 5.0  | 10.0  | 3.0      | 10.0  |
| GLASS   | 3.0  | 0.1  | 5.0  | 2.0  | 14.0  | 2.0      | 14.0  |
| TANK    | 0.75 | 0.1  | 5.0  | 15.0 | 8.0   | 5.0      | 8.0   |
| PEASANT | 1.0  | 0.1  | 5.0  | 1.0  | 20.0  | 5.0      | 10.0  |

### Server Rebalance History
- GLASS: {3.0, 0.1, 5.0, 2.0, 14.0, 2.0, 14.0} (was {2.0, 0.1, 5.0, 2.5, 10.0, 3.0, 10.0})
- TANK: {0.75, 0.1, 5.0, 15.0, 8.0, 5.0, 8.0} (was {0.5, 0.1, 5.0, 10.0, 10.0, 3.0, 10.0})

## Combat Analysis (with new balance stats)
- GLASS vs DEFAULT 1v1: Both kill in 2 hits, but GLASS has 14 range vs 10 — free hits before close. 1.4× faster, can kite. In groups, GLASS dominates (30 DPS vs 10 DPS per 5 ants)
- TANK vs DEFAULT 1v1: TANK needs 7 hits, DEFAULT needs 15. TANK wins decisively
- TANK vs GLASS 1v1: TANK needs 3 hits, GLASS needs 20. TANK crushes. But GLASS can kite with speed+range advantage
- TANK+GLASS combo: TANKs absorb damage while GLASS deals 3× damage from behind
- Attack rate: realtime_elapsed × timeScale vs attackRate × antType.rate. At timeScale=4: attacks every 0.025 real-sec

## Callback Reference
- `gameStart`: called once at start. 5 DEFAULT ants already exist
- `gameFrame`: called every frame. Main logic loop
- `antWait`: called per-ant per-frame when ant has no commands (idle)
- `antDeliver`: called when ant delivers food to nest
- `antGrab`: called when ant picks up food tile
- `antNew`: called when new ant created
- `antHurt`: called when ant takes damage
- `antHit`: **DEAD CALLBACK** — `hitAnts` deque is populated when AINTERACT succeeds on an enemy, but the callback loop was never implemented. Registering this does nothing
- `antDeath`: called when ant dies. Ant is frozen snapshot; do NOT send commands
- `antFull`: called when ant reaches max carry capacity. Same code path as `antGrab`/`antDeliver` — no crash evident in current source

## API Gotchas
- `nearestEnemy()` returns None only when no enemy ants exist on the map — there is no range/visibility limit (scans all ants globally)
- `nearestFreeFood()` returns the nearest food tile that no **friendly** ant (same nest only) has a pending `take` command on. Returns None when no unclaimed food exists
- `ant.target`: returns the ant referenced by a FOLLOW, CFOLLOW, or AINTERACT command — i.e., the ant being chased or attacked. Returns None if no such command exists or target is dead
- Dead ants in `antDeath` are stack copies (not live pointers). All command functions early-return on dead ants — sending commands is a no-op with a warning
- `flagDict.get(ant.id)` needed — callback order is: onFrame → onNewAnt → onHurt → onGrab → onDeliver → onFull → onWait → onDeath. Ants created last frame exist in `agc.me.ants` during onFrame but haven't had onNewAnt called yet
- `followAttack(ant)`: sends WALK to target's position + AINTERACT (attack). Auto-re-issued each frame by the client to chase. No range limit for issuing — will chase across entire map. `followAttack(None)` silently returns None (no-op)
- Food theft at enemy nests: Python-side `take`/`goTake` rejects the command unless ant has room for full `foodTheftYield` (3). Server-side uses `min(foodTheftYield, capacity - foodCarry)` but this never activates under normal conditions. Ant must walk to nest tile and use `take`/`goTake`
- Building ants in `onFrame` drains food too aggressively (builds every frame vs only when food arrives in `onDeliver`)
- Re-targeting the same ant in `onWait` every frame can trap ALL ants in permanent combat with no economy

## Strategic Lessons
- **Keep gathering during attack phase.** Do NOT send all ants to fight. Keep PEASANTs (or equivalent gatherers) collecting food during the all-in so you can sustain food reserves and continuously replace combat losses. menacingFast does this — only DEFAULT ants attack while PEASANTs keep gathering, which is why it still has 2000+ food at game end. Sending all ants (including PEASANTs with 1 HP) results in food dropping to zero and no ability to rebuild.
- **Raiding bleeds economy.** Sending small squads to raid during the economy phase costs ~150 food per cycle (30 ants × 5 food each) and prevents food accumulation. Pure economy (no raids) reaches 255 ants faster.
- **Army composition matters more than army size.** GLASS/TANK army at 180 ants has comparable total DPS to a DEFAULT army at 250 ants, but with better survivability from TANK HP pools.
- **PEASANT economy is ~3.3× more efficient than DEFAULT.** PEASANT speed 20 + capacity 5 vs DEFAULT speed 10 + capacity 3. A 2:1 DEFAULT:PEASANT ratio starves the economy.
- **Never recall attackers mid-fight.** Pulling combat ants back (e.g., when count drops below a threshold) interrupts winning engagements that would snowball. Let attacks play out.
