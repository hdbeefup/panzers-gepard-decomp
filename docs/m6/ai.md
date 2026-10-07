# M6-AI: the AI group answering an attack (SWorld 0x5d68e0)

## Lifted

- `SWorld::AIGroupUnitAttacked` **0x5d68e0** (was a STUB_LOG in src/game/squadcombat.cpp; now
  src/world/aigroup.cpp). Read against the disassembly, in HD order:
  1. Tactic 5 groups ignore attacks.
  2. Tactic 2 / 4 groups with AttackMoveTimer 0: timer 15, `AttackMoveTo` (0x5d95b0) the attacker.
  3. Tactic 4: one world LCG draw, `(int)(r * (1/32768) * 20.0) == 0` (1 in 20) calls the victim
     player's support (not free, counters World+0x19c..0x1ac): tac bomber (armoured, standing
     attacker), else paratroopers while any are left (no tac bombers), then artillery or heavy bomber
     (soft, standing attacker). Same argument lists as HD (-1.0f altitude, p6 1, 0, 0.0f).
  4. Once per group (HasCalledForHelp; tactic 2 groups already attacking skip): tactic 4 recon call,
     then the enemy strength within 40 m of the attacker (0x5642d0, heap order): classes 0 / 0xb /
     0xc / 5, not wrecked / unplaced / +0x110, not the victim's side, player kind not 2 / 3 / 4,
     armed; hit points (x87 `GetHitPoints * HP + (float)sum`, fstp dword, cvttss2si) or 100 per squad
     member, +500 UnitType 0x17, +600 class 0xb, +500 (and "armoured") ArmourType 2. Own strength
     = `SAIGroup::Strength` 0x5e74e0.
  5. Tactic 1: enemy <= own: attack-move ("Last man standing!"); else the nearest allied reachable
     group in both help ranges (anti-tank strength needed against armour; tactic 2, then 4, groups
     preferred, best score 10000), the attacked group EC_Moves (queue 1) to its first unit and, if it
     is tactic 2, it attack-moves to the attacker (attacked group Status 2, Timer 50).
     Other tactics: enemy > 3 * own and tactic != 0: retreat (EC_Move) to the nearest allied group,
     "OMG! WTF?!" if none; enemy > own: the nearest allied tactic-2 group gets 0x570720 +
     AttackMoveTo, "Defending" if none; else "Enemy is too weak".
  The texts go to SGameLogic 0x568ae0, an empty function in HD.
- `SAIGroup::Strength` **0x5e74e0**, `SAIGroup::AntiTankStrength` **0x5e7110** (gunner 0 weapon
  type 1; squads: first member's gunner type 1 or 3), the radius search **0x5642d0** (static).
- `SSingleUnit::EC_Move` 0x5ac4b0: the "move backwards" branch called STUB_LOG and returned; HD calls
  the unit's +0xb0 (`EC_MoveReverse`, lifted in M3-I2). Now it does (the retreat orders reach it).

Recompile-only: `PZ_M6_AILOG=1` logs `PZM6 AI ...` lines (every call: frame, unit, attacker,
group, tactic, status, HasCalledForHelp, AttackMoveTimer; the strengths; the HD debug texts).
Inert when unset.

## Census

master 2531 lifted / 1316 SWINE-shared / 139 stubs (61 lifted with a STUB_LOG) ->
2535 / 1316 / 137 (60).

## Verified

Mission sweep (`sweep.ps1`, 2400 frames, PZ_M6_AILOG=1): Russian 3, Russian 9, Allied 7,
Russian 4, Allied 8 all OK (no panic / crash / warning box), 0x5d68e0 no longer in the stub lists.
Paths seen in the log: "Attack move!" (tactic 2 groups, timer 15 counting down by Refresh),
"Defending" (Allied 8 Vedok3 / Vedok4, Russian 4 defend1: enemy > own, no tactic-2 ally in range),
"Enemy is too weak" (Russian 3), the tactic-1 helper search with no helper (Russian 3
csarnoknalnemet1scout: enemy 2200 / own 200, silent return as HD). Not reached in 2400 frames: the
support calls (1 in 20 per attack on a tactic-4 group), Retreat, "Call support!", Last man standing.

The tc1 and Tutorial replays never call 0x5d68e0 (no PZM6 line in their logs), so they are no
oracle for it.

## Reference the original could give

No recording exists of an AI-group mission. Wanted: the original HD exe, New Game, Russian
campaign, mission 4, Normal, briefing and market passed with no purchases, no orders given, a replay
recorded through about frame 1000 (the in-game replay recording, as for tc1), plus the original's
per-frame world CRC if it can be captured as for tc1. Our build calls 0x5d68e0 there from frame 743
(group defend1 "Defending", group "nemet1c1 tamadocsapat" "Attack move!"); playing that replay
with `-m3 -packetplay` and PZ_M2_CRC=1 would check this lift frame by frame. Allied 8 (frames
1620..1780, "Defending" and "Attack move!") is the second candidate.
