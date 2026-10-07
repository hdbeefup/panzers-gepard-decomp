# M6: playtest fixes, drawn cut-scenes, AI, weather, the selection panel, partial lifts

Parallel parts, merged and checked together; each has its notes in `docs/m6/`.

| part | notes | what works now |
|---|---|---|
| `.4d` cut-scenes (CD) | `m6/cd.md` | The animated cut-scene scene draws with its camera cuts (0x56f0d0, 0x582380, 0x5652d0, SModel::ApplyCamera 0x6d62e0, GetAmbient 0x6d78e0) |
| Army making (AR) | `m6/ar.md` | Skirmish New / Edit army opens the market in multi mode, Save Army (0x64a180) writes the army file, the room lists it, the units are placed in the game |
| AI under attack (AI) | `m6/ai.md` | SWorld 0x5d68e0 with SAIGroup::Strength / AntiTankStrength: attack-move, support calls, defend / retreat / help (`PZ_M6_AILOG=1`) |
| Weather (WX) | `m6/wx.md` | Rain, snow, lens flares, the rain sound, the soldier blob shadow (shadows off), the scene line passes |
| Roads (RD) | `m6/rd.md` | STerrain::UpdateRoad 0x6f95b0 lifted: no zig-zag strips at road ends and bends (playtest, Allied 8) |
| Selection decal (fix-sel) | `m6/fsel.md` | Terrain effect decals rotate the texture as HD (0x6f6640): the selection frame follows the heading (playtest) |
| reccmp triage (RC) | `m6/rc.md` | 150 short pairs classified; marker fixes; hero photos and group icons, map ambient sounds, minimap objective markers, squad board elements after a load |
| Selection panel (SP) | `m6/sp.md` | The HUD panel for one or many selected units (0x56b3e0, 0x628430 panel part, clicks in 0x6216b0); unit display names from units.ini |
| Partial lifts (PL) | `m6/pl.md` | 61 → 36 lifted bodies with a STUB_LOG: unit speech and order acknowledgements, combat music, building orders, vehicle entry and towing, model fire effects, the in-game Options menu, ShowBriefing |
| Russian 4 reference (OR) | `m6/or.md` | One recording of the original (`scratchpad\m6ref`); campaign replays start correctly (`LoadReplay` loads the mission properties and objectives) |
| Water (WA) | `m6/wa.md` | Plan only: the HD layouts and order for lakes and rivers |

## Checks on the merged tree

- Census: 2531 → 2635 lifted, 1316 SWINE-shared (unchanged), 139 → 102 stubs; lifted bodies that still
  contain a STUB_LOG 61 → 38.
- Menu CRC 0 / 1181, Training Camp replay 3901 / 3901, Tutorial replay 2012 / 2012, clean exits.

## Open

- **Russian 4 replay** (`scratchpad\m6ref\r4.rec`): frame 0 equal, frame 1 differs. The sapper squads'
  lay-mines order handler 0x59abf0 and move-along-path 0x59aef0 are not lifted; the static block map at a
  campaign start differs in 18,248 cells (bit 0x200 vs 0x400). The AI lift (from frame 743) can be checked
  once those match.
- Water: lakes and rivers (`m6/wa.md`; lakes write the world's water heights, so the Tutorial replay is the
  oracle).
- The wire / cable system (5 partials, hit in Allied 1 and German 10).
- The production-building panel, HUD tooltips, message line fade-out.
- Original screenshots wanted (listed in the m6 notes): selection frame of a turning vehicle, the
  `.4d` cut-scene lighting, snow / rain density.
