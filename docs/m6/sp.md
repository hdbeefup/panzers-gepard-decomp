# M6-SP: the HUD unit selection panel

Branch `m6-sp` from `m6-int` (cb28bc3). Census 2572 -> 2574 lifted, 1316 SWINE-shared, 137 stubs (unchanged).

## Lifted (src/panzers/hud.cpp)

- **0x56b3e0** `SelectionSummary`: the whole selection summary (0x13c bytes on Update's stack):
  heroes, the command flags of the selected units, the shared order kind, behaviour / stance,
  equipment slots, the panel unit (single bit-1, else single bit-0 unit), the 16-unit list,
  and the panel unit's name, stars, XP, HP, ammo, cargo, driver, three weapon slots, armour,
  thermostat. Reads only (getters GetRank, GetHitPoints, Slot_54, SGunner GetWeaponType /
  GetDamage / GetPGunner).
- **0x628430 (piece 0x6294dd..0x62b842)** `HudSelectionPanel`: hides the unit display each frame,
  single selection: name, stars, picture (race font), state icon, stored units / empty seats
  (Units2), driver + weapon icons (Crew), HP / ammo / cargo / XP / thermostat icons and texts,
  armour texts; multiple selection: the 16 SUnitButton icons; the command buttons the
  selection allows, the automatic markers, equipment slots (Equip1/2 + view +0x2a58/+0x2a5c),
  unchecking (MouseMode != 4), the current-order highlight frame (byte table 0x62c440),
  behaviour / stance checked states. Replaces the old "empty panel" block.
- **0x6216b0 (clicks)** in PzHudAction: selection icon (select, Shift toggles), hero photo
  (select, double click 0.75 s also centres World +0xa4), panel unit icon (select + centre),
  stored-unit icon (Pkt 0x31(slot, shift) when the carrier's +0x2e9).
- **0x5cfe30 tail** (src/world/unitregistry.cpp, shared): the display names +0x68 / +0x70 / +0x78
  from units.ini (as market.cpp's LoadUnitDisplayNames describes); before this the mission's
  registry had no names, so the panel name was empty.

## Not lifted

- The production-building panel (UnitType 0x1a, skirmish HQ: 0x6297f8..0x629c03, product
  buttons via 0x548390/0x548500/0x625990 and the queue texts); only the name is shown.
- Hints/tooltips of the panel (0x543bf0 / 0x543a60: "Driver: Active", "Damage: %g + %.02f",
  "Uses equipment"...): the recompile's HUD has no tooltip display.
- market.cpp still fills the names too (harmless duplicate; the owner can drop it).

## Test hook

`PZ_M6_SELECT="frame:mode,..."` (inert unless set): from that logic frame selects own units,
mode `tank[N]` / `squad[N]` / `gun[N]` (N-th class 0 / 5 / 0xb,0xc,9 unit) or `all`.

## Verified (screenshots, scratchpad\m6sp\shots)

- tc1 replay (Training Camp with the bought army) + `400:tank`: Panzer III F panel matches
  HD's m3ref rec_g3 (name, stars, picture, 392/400, 124/131, 227/1000, 46/140, armour
  19/30 30/30 30/30 21/21, driver/cannon/MG/MG icons, the two stored-unit boxes, buttons).
- German 1: SdKfz 223 single, Medics squad, mixed group of 10 (16-icon grid).
- Training Camp via PZ_M5_AUTO has no own units (nothing bought), so it was checked via tc1.
- regress.ps1: menu 1181 / 0 mismatches, tc1 3901 / 3901 equal, tutorial 2012 / 2012 equal,
  all exits closed.
