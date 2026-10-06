# Codename Panzers Phase One HD: multiplayer scope

How this was done:

- Static analysis only, of `PANZERS.exe` (2016 Nordic HD, PE timestamp 2016-02-18). I used my own copy of the m1p0 Ghidra project (`scratchpad/mpscope/ghidra`).
- Raw decompiler output is in `scratchpad/mpscope/out/d1..d7.txt`. Caller chains are in `out/chain1.txt` and `out/reach1.txt`, and function counts in `out/count*.txt`.
- New helper scripts are in `scratchpad/mpscope/scripts/`: `refdec.py`, `reach.py`, `chain.py`, `count.py`.
- SWINE sources were read only through `git show` (swine-portable at 95bbcc4 / feat/swine-redux) and by reading `C:\Users\swine\Downloads\swinedecomp`.
- I contacted no network service, and the game was never run.

## 0. Executive summary

The HD multiplayer stack has three layers.

**1. In-game transport: Stormregion's 2004 `SNetworkUDP`.** It is plain WinSock UDP:
- 16-byte header, its own reliability layer, up to 8 hosts.
- The server uses port **5555**, the client **5556**.
- `SMulti` runs on top of it: lobby slots, game settings, map/army upload, lockstep frame data and host migration.
- This code is essentially unchanged from the 2004 retail build. The 2004 Ghidra export `phase1-raw-ghidra-export/test.c` has the same strings and functions.

**2. RakNet (Nordic addition): stock RakNet 4.08x.** RakNet does **not** carry game messages. It is used only for:
- NAT punch-through: `NatPunchthroughClient` against a hard-coded `NatPunchthroughServer` at **176.9.91.194:61111**;
- UPnP;
- three custom messages, `0x87..0x89`;
- a **raw-datagram tunnel**. `NatManager` / `PacketForwarding` take the game's SNetwork UDP datagrams, push them through the punched RakNet socket (port **23022**) with `RakNetSocket2::Send`, and inject the replies back into local forwarder sockets.

**3. Lobby, matchmaking and auth: the RankedGaming (RG) client library v5.0.**
- It speaks plaintext TCP to `gateway.nordic.rankedgaming.com:18600` and `loadbalancer.nordic.rankedgaming.com:1008`. The test host is `ns204252.ovh.net`, and a third option is `127.0.0.1`.
- Nordic re-implemented the *bodies* of `SGameSpy::Init`/`RefreshServers`/the room menus on top of RG (`"RGMasterServer init"`). The old GameSpy "title room" and "staging room" UI is therefore now an RG UI.

**GameSpy:**
- The **peer/chat/QR2/SB path is dead.** `peerInitialize` and `peerSetTitle` are reachable only from `SGameSpy::Connect`, which has no callers apart from its PE export entry.
- GP (`gpiInitialize`) is not referenced at all.
- **Still live:** the GameSpy *stats/persistence* SDK and the available-check, used only by the single-player **TopList** menu (`STopListMenu`, "Upload Score"). That covers `gamestats.gamespy.com`, `%s.available.gamespy.com` and gstats with gamename `cnpanzers`, secret `h3Tod8`, product `cnpanzersphaseone` and game id 0x3bf (959).

**RakNet verdict:**
- Panzers HD and SWINE HD use the same RakNet generation. SWINE uses stock 4.081 (Oculus source, Kite copy); Panzers uses a Nordic copy of stock 4.081/4.082.
- Both use `RAKNET_PROTOCOL_VERSION 6`, the same `OFFLINE_MESSAGE_DATA_ID`, the same MTU table, the same `ID_*` ordinals and `ID_USER_PACKET_ENUM = 0x86`.
- **SWINE's RakNet *library* can be reused unchanged and will interoperate on the wire.**
- **SWINE's RakNet-based game transport (`ConnectionManager` + SWINE `SMulti` messages 0x87..0x99) cannot.** Panzers does not send game data as RakNet messages.

## 1. RakNet version match

### Evidence from PANZERS.exe

| Fingerprint | HD value | Location |
|---|---|---|
| Source paths | `..\..\RakNet\Source\RakPeer.cpp`, `d:\nordicgames\projects\panzers\phaseone\trunk\code\raknet\source\DS_Map.h` etc. | 0x7ee8ac, 0x7f09b8 |
| `OFFLINE_MESSAGE_DATA_ID` | `00 FF FF 00 FE FE FE FE FD FD FD FD 12 34 56 78` | 0x7ee5a8 |
| MTU table that follows | `1492, 1200, 576` (`MAXIMUM_MTU_SIZE` = 1492) | 0x7ee5b8 |
| Protocol version | In the RakPeer offline handler FUN_00502c00, a version mismatch writes `0x19` (ID_INCOMPATIBLE_PROTOCOL_VERSION), then the byte `6`, then the magic → **RAKNET_PROTOCOL_VERSION = 6**. REPLY_1 is ID `6` + magic + GUID + security byte 0 + MTU (capped at 0x5d4). | FUN_00502c00 (~line 900 of `out/d2.txt`) |
| Ping / out-of-band | `Ping()` writes ID 1 or 2, time64, magic, GUID (FUN_00502510). OOB writes 0x0d (ID_OUT_OF_BAND_INTERNAL), time64, magic (FUN_00509530). | |
| Connection IDs | Switch cases 0x10 ACCEPTED, 0x11 ATTEMPT_FAILED, 0x12 ALREADY_CONNECTED, 0x13 NEW_INCOMING, 0x14 NO_FREE, 0x15 DISCONNECTION, 0x16 CONNECTION_LOST, 0x17 BANNED, 0x18 INVALID_PASSWORD, 0x19 INCOMPATIBLE_PROTOCOL, 0x1f/0x20/0x21 REMOTE_*. Log text: `ID_CONNECTION_REQUEST_ACCEPTED to %s with GUID %s`. | FUN_00524cd0, FUN_00525cb0 |
| NAT IDs | 0x3e TARGET_NOT_CONNECTED, 0x3f UNRESPONSIVE, 0x40 CONNECTION_TO_TARGET_LOST, 0x42 PUNCHTHROUGH_FAILED, 0x43 PUNCHTHROUGH_SUCCEEDED. These match the 4.x enum exactly. | FUN_00524e50, FUN_005252b0 |
| User IDs | 0x87 `ID_USER_SEND_CLIENTLIST`, 0x88 `ID_USER_DISCONNECT_CLIENT`, 0x89 `ID_USER_NAT_PEER_BUSY` (= ID_USER_PACKET_ENUM+1..+3) | FUN_00524e50 |
| API shape | `Startup(32, &sd, 1, -99999)` with port 0x59ee = **23022** (FUN_00523cc0). `SetIncomingDatagramEventHandler` at vtbl+0x134. `RNS2RecvStruct` is {data[1492], bytesRead at +0x5d4, ...}. `RNS2_SendParameters`. | |
| RTTI | `RakNetSocket2`, `RNS2_Berkley`, `RNS2_Windows`, `RNS2_Windows_Linux_360`, `RNS2EventHandler` | rtti.txt |

The `RakNetSocket2`/`RNS2_*` socket layer first appeared in RakNet **4.08x**, so this is **4.081 or 4.082**. The last Oculus release, 4.082, made no protocol changes, and both versions use protocol 6. No version string survives, so I cannot tell 4.081 from 4.082. The difference does not matter on the wire.

### SWINE

- `swine-portable` `raknet/CMakeLists.txt` builds `N:/ProjectsCODE/swinehd/RakNet-master/Source/*.cpp`, which is stock **4.081**: `RAKNET_VERSION "4.081"`, `RAKNET_PROTOCOL_VERSION 6`, `MAXIMUM_MTU_SIZE 1492`, the same magic, and `ID_USER_PACKET_ENUM` = 134.
- The original SWINE HD exe was built from `c:\kite\raknet-master\source\`, a Kite copy.
- **So this is not the same *copy*** (one is Kite's, the other Nordic's), **but it is the same upstream release family** with identical wire constants.

### Verdict

- **Library: reuse SWINE's RakNet 4.081 build recipe unchanged.**
  - It interoperates with original Panzers HD RakNet peers and with the punch-through protocol.
  - The `NatPunchthroughServer` in `swinedecomp/serverinfra/natpunch` (stock 4.081) can stand in for the dead 176.9.91.194:61111 server, as long as it runs on port **61111**. The SWINE server README says 60481; the client uses 61111.
- **Transport: do not reuse SWINE's `ConnectionManager`/`SMulti` packet layer** to talk to Panzers. Panzers needs:
  - (a) the SNetworkUDP protocol, byte-exact;
  - (b) for internet play, Nordic's NatManager tunnel semantics (section 2.3).

## 2. Which stacks run, and when

### 2.1 Address and class map

| Module | Range (approx.) | Functions | Key addresses |
|---|---|---|---|
| RakNet 4.08x (static) | 0x4d9000-0x512000 | ~1,400 (includes some STL) | RakPeer offline handler 0x502c00, RunUpdateCycle 0x505b00, NatPunchthroughClient 0x4f3xxx |
| `SMulti` (game session / lockstep) | 0x51dc10-0x522ab0 | 91 | Init/host (creates SNetworkUDP) 0x51f3c0, HandleAppMessages 0x520010, SendFrameData 0x5212a0, SendGameSettings 0x5215e0, SendMap 0x521670, SendMeThisPacketAgain 0x521790, StartHostMigration 0x521e50, GetSlots 0x51f090, vftable 0x7ef038 |
| `NatManager` / NAT handlers / PacketForwarding | 0x522cf0-0x5283e0 | 147 | client handler start 0x523750, host handler start 0x5238a0, RakPeer start 0x523cc0, UPnP 0x523a60, forwarder thread 0x524760, incoming-datagram hook 0x5249b0, NAT packet handler 0x524e50, UpdateInternalIPsForGuid 0x525f20, HostMigrated 0x5235e0, vftable 0x7efa44 |
| `RGMasterServer` / `RGListener` (glue) | 0x528400-0x52c2c0 | 94 | ctor 0x528400, singleton 0x52dbe0, login 0x5292f0, register 0x529330, request game id 0x529460, event dispatcher 0x52b2c0 |
| `SGameSpy` (now RG-backed) | 0x52c320-0x5331f0 | 107 | Connect 0x52cfa0 (**dead**), Init 0x52e050 (RG), InitTitleRoom 0x52e1f0, InitStagingRoom 0x52e120, RefreshServers 0x530270 |
| `SGStats` (GameSpy stats, TopList) | 0x533230-0x534b70 | 18 | ctor 0x533230, connect 0x533f40, snapshot/upload 0x534720 |
| `SNetwork` / `SNetworkUDP` | 0x534bf0-0x5378c0 | 32 | ctor 0x534bf0, WSAStartup 0x536280, GetMessage/Refresh 0x5368c0, Write 0x5374f0, WriteTo 0x5377d0, ManageGuaranteedMessage 0x535f10, unknown-sender handler 0x5365c0, system msgs 0x5366d0, broadcast 0x535550 |
| GameSpy SDK (peer, chat, qr2, sb, gstats, ghttp, gp, natneg) | 0x70d000-0x726000 | ~790 | peerInitialize ≈0x711970, peerSetTitle ≈0x712490, GSIStartAvailableCheck ≈0x70fb10 |
| RankedGaming lib v5.0 | 0x741f10-0x75baf0 | ~505 | RGInterface ctor 0x7426f0, gateway connect 0x742ff0, LB connect 0x745e10, RGLoadBalancer handler 0x746c90, TcpClient connect 0x74c700/0x74cad0, frame parse 0x74cf90/0x750c00/0x750860, sendpacket 0x74fa10/0x74ffe0/0x74fe60/0x7502c0, RGConnection handler 0x7586d0 |
| MP menus | 0x60a710-0x6181f0, 0x635c10-0x6360f0, 0x63b3a0-0x63f2f0, 0x653250-0x654df0 | ~35 SChatRoomMenu + 57 GameSpy list boxes + 21 title/staging rooms + ~43 Pre/LAN/DirectIP/RG menus + 12 SSkirmishChatRoom + TopList 15 | vftables: SMultiPreMenu 0x8070c4, SMultiLANMenu 0x807144, SMultiDirectIPMenu 0x8071c4, SRankedGamingMenu 0x8075c4, SRankedGamingRegisterMenu 0x807644, SGameSpyTitleRoom 0x802df0, SGameSpyStagingRoom 0x802b88, SChatRoomMenu 0x801f98, SSkirmishChatRoomMenu 0x809f50, STopListMenu 0x807444 |

The SSuperWindow entry points are:
- `LoadMultiPreMenu` 0x658a30;
- `LoadGameSpyTitleRoom` 0x658230, which calls `SGameSpyTitleRoom::Create` 0x6169d0 and then `SGameSpy::Init`, i.e. RG;
- `LoadGameSpyStagingRoom` 0x658160;
- `LoadChatRoomView` 0x658050;
- `-host`/`-connect` at 0x6576f0/0x657460.

All of them are dispatched from `SSuperWindow::OnAction` 0x659250.

### 2.2 GameSpy: dead or live?

`out/reach1.txt` and `out/chain1.txt` show the following.

**Dead:**
- `peerInitialize` and `peerSetTitle` are referenced only by `SGameSpy::Connect`. Connect's only references are the EAT entry at 0x8d6e60 and the export.
- The peerchat host string `peerchat.gamespy.com` is reachable only through Connect, or through SDK functions that nothing references.
- `gpiInitialize` has no references at all. GP is linked but unused.
- Some menus (rooms, chat) still reach peer/master/natneg *wrappers*. They do so only through SGameSpy helpers that operate on a `PEER` that is never created, so they do nothing in practice.
  - I did not prove that each wrapper checks for a NULL peer. That is a residual risk if someone re-enables the rooms.
- `SGameSpy`'s constructor (around 0x52be5f) still stores title `cnpanzers` and secret `h3Tod8`. This matches the 2004 export line 42066-42067: `Pz_CopyString(param_1+0x1d,"cnpanzers"); strncpy(local_1c,"h3Tod8",7)`.

**Live:**
- `STopListMenu` create (0x6398f0) calls `SGStats` connect (0x533f40). That runs `GSIStartAvailableCheck("cnpanzers")`, waits for the result, then calls InitStatsConnection, authenticates, and does persistent-data get/set with the callback 0x534090.
- "Upload Score" (0x63c920) calls 0x534720, which sends a gstats snapshot with the keys `hostname`=`cnpanzersphaseone`, `event`, `p_nick`, `nick_index`, `point`, and two short keys at 0x7f1c44 and 0x7f1c48 (one is likely a CD-key hash).
- Strings `Card Game: Upload army update to Gamespy` and `TestGameSpyDeck` show that persistent "deck/army" storage existed. I did not trace whether HD still calls it.
- So the GameSpy backend is only used at runtime by the HD TopList feature, and it fails gracefully when the backend is gone.

### 2.3 What the RG client does

**Endpoints** (RGInterface flags at +0x26c test and +0x26d local):

| Role | Host | Port | Code |
|---|---|---|---|
| Gateway (login, chat, rooms, game list) | `gateway.nordic.rankedgaming.com` / test `ns204252.ovh.net` / local `127.0.0.1` | TCP **18600** (0x48a8) | 0x742ff0 |
| Load balancer (game ids, user verification, results, lobby/chat logging) | `loadbalancer.nordic.rankedgaming.com` / same alternatives | TCP **1008** (0x3f0) | 0x745e10 |
| NAT punch-through (RakNet, not RG) | `176.9.91.194` hard-coded, overridable through the global string at 0x8f1a98. Who writes it was not determined; it is probably supplied by the RG game info. | UDP **61111** | 0x5238a0 / 0x523750 |
| RakNet host port | local | UDP **23022** | 0x523cc0 |

**Configuration set in `RGMasterServer` ctor 0x528400 and singleton 0x52dbe0:**
- API identity strings `"18273849502718345761263748591635"` and `"tqit_self"`, stored at RGInterface+0x274 +0xc/+0x24;
- `setSystem(0xb)` (FUN_007439a0(11)), probably the RG "system/game" id for Panzers;
- locale `"en"`;
- timeout 10000 ms;
- a flag at +0x7d.

`[RGS]` runs its own select()-based `SocketManager`.

**Wire format.** This is the RG v5.0 library: `sendpacket`/`recvpacket` classes, TCP, little-endian.

```
frame  := u32 total_len (includes this 5-byte header)  u8 compressed(=0; zlib not linked → nonzero is fatal)
          body
body   := u16 packet_id   field*
field  := u16 len   bytes[len]
  ints : minimal little-endian byte string (writer 0x750aa0, reader 0x750650 → up to 4 bytes)
  str  : raw bytes (writer 0x74fe60, after a conversion in 0x752580)
  sanity: fields must exactly fill the body ("[ Version 5.0 ] PROTOCOL ERROR @ SANITY CHECK")
```

The header size is `DAT_008e93b8 = 5`. There is **no encryption** in the framing. There are helper codecs, `util::decode` (hex) and `charToExtendedBase` (base-95 alphabet at 0x8962e8), but I did not trace where they are used. Send chunks are at most 0x5b4 bytes.

**Known packet IDs:**

| Dir | ID | Meaning | Fields |
|---|---|---|---|
| C→GW | 1001 (0x3e9) | Login | user, password, a third string from 0x74fe60. That string is probably a hardware unique id: `h.getUniqueID` needs admin rights. |
| C→GW | 1005 (0x3ed) | Register | user, password, repeat/e-mail, ... (4 strings) |
| C→LB | 7001 (0x1b59) | Request game id / host order | 2 ints, a string, and 2 more typed fields |
| GW→C | 0x3eb | CLIENT_LOGIN_FAILURE(msg) | |
| GW→C | 0x3f3 | CLIENT_TIME | |
| GW→C | 0x3f5 | CLIENT_WC3_KEY | This is a library leftover: the RG library is generic and was shared with DotA/WC3 bots. |
| GW→C | 0x3ee, 0x3f1, 0x3f2, 0x3fc, 0x3fe, 0x44c, 0x451, 0x455, 0x458, 0x45d, 0x460, 0x465, 0x515, 0x516, 0x518, 0x5de, 0x5e1, 0x5e2 | Not yet mapped one-to-one to these names: CLIENT_LOGIN_SUCCESS(_WITH_PRIVATE_KEY), CLIENT_VALIDATION_SUCCESS/FAILURE, CLIENT_REGISTER_SUCCESS, CLIENT_SHOW_MESSAGEBOX, CLIENT_CHAT_* (whisper, channel add/count, roomlist refresh), ROOM_PACKET, CLIENT_REMOTE_IP, GAME_PLAYER_JOINED/LEFT, CLIENT_GAME_HOSTED/STARTED(_2)/ENDED, CLIENT_REQUEST_PRIVATE_KEY_VALIDATION, GATEWAY_NOACCESS/USAGE | |
| LB→C | various | UserVerificationSuccess/Failure, InstanceRequested, INTERNAL_AddAccess, StatsSavedSuccessfully, VERIFY_STATE, RGAPI_CONFIRM_GAME_HOSTED/STARTED/UPDATE_DATA/GAMEPLAYER_UPDATE/DELETE/GAME_ENDED, GAMESQL_GAME2_SCORED | |

**Function.** RG provides account login and registration (the "RG Login"/"RG Pass" entries under `[Network options]` in options.ini; I did not verify whether they are stored in plaintext), a chat lobby with rooms, and a game list.
- A game listing is `RGData::RoomGameHosted` with `[DATA_INT]`/`[DATA_STR]` key/value pairs. The host publishes name, IP and GameID.
- RG also handles game-id allocation, per-player verification, start and end confirmation, result/score submission ("ranked"), and lobby/in-game chat logging.
- It does **not** carry game traffic and does **not** do NAT. NAT is RakNet's job.

### 2.4 What the game sends where

**SNetworkUDP** is used for LAN, direct IP, the `-host`/`-connect` command line, and inside the RG tunnel.

Header (16 bytes, LE):

| off | field |
|---|---|
| 0 | i32 seq. -1 = unreliable. Reliable sends carry a per-peer counter and are kept in a resend list (more than 100 pending logs "Too many garanteed messages"). |
| 4 | i16 msgId. Negative = system. |
| 6 | i16 sender slot |
| 8 | u32 ack bitmask (the last 32 messages) |
| 12 | i32 last seq received |

System messages:
- -1 knock (connect request; resent every 500 ms);
- -2 accept (slot id at +0x10);
- -3 refuse;
- -4 keepalive (after 500 ms idle; timeout ~1 s);
- -5 LAN discovery query, broadcast every 1 s to every interface's directed broadcast address;
- -6 discovery reply;
- -8 file chunk (0x3f0 = 1008 bytes per chunk, with callbacks at vtbl+0x18/0x1c).

Other limits: receive buffer 0x2800, at most 8 hosts.

**SMulti app messages** (msgId 1..16, switch in 0x520010):
- 1 MSG_SERVERLOGOUT, which triggers host migration;
- 2..10 slot, settings, team and ready-type lobby messages; 6 is "Game started";
- 11 MSG_ALLSLOTINFO;
- 12 MSG_SLOTINFO (to the server), probably;
- **13: lockstep frame data.** It carries frame N plus N-1 and N-2 (`len/len2/len3`, each ≤1024). The receiver asks again when FrameCount-3 is missing;
- 14 MSG_RESENDFRAMEDATA;
- 15 APP_MSGID_SEND_ME_LEAVING_PLAYER_LASTFRAMES;
- 16 is the default "WHAT IS IT??" case.

Map and army upload use the -8 file stream (UPLOADTYPE_ARMY, `multimaps/`). Exact field layouts for messages 2..12 still need a per-case decompile.

**RakNet / NatManager** (internet games only):
- (1) The host starts RakPeer on UDP 23022, connects to the NAT server, and logs its GUID.
- (2) Clients learn the host's GUID and IP, probably from the RG game data, then run `NatPunchthroughClient::OpenNAT`.
- (3) For each peer, PacketForwarding binds a local UDP socket and creates a forward entry (src, dst, output address, guid, playerID).
- (4) The forwarder thread 0x524760 sends the game's datagrams (≤1492 bytes) **raw** through the RakNet socket.
- (5) `SetIncomingDatagramEventHandler(0x5249b0)` catches incoming non-RakNet datagrams (those not from port 61111) and hands them to the matching local socket.
- An internal port of 23022 is rewritten to game port 5555 ("Setting game port to 5555").
- The host's RakNet also sends `ID_USER_SEND_CLIENTLIST` (0x87, GUID lists for the mesh), `ID_USER_DISCONNECT_CLIENT` (0x88) and `ID_USER_NAT_PEER_BUSY` (0x89).
- Host migration re-runs this through `NatManager::HostMigrated`.

## 3. Panzers game protocol compared with SWINE

| Area | Panzers HD | SWINE HD (swine-portable) | Copy-adaptable? |
|---|---|---|---|
| Transport | SNetworkUDP (own reliability layer, 16-byte header, ports 5555/5556), star topology (server ↔ clients), host migration | RakNet `ConnectionManager` (RELIABLE_ORDERED / UNRELIABLE_SEQUENCED), full P2P mesh on 60500-60503 with TTL relay | **No.** Panzers-only. The 2004 export `test.c` has the same SNetwork code and serves as a naming and structure reference. |
| Lobby / slots | SMulti msgs 2..12, MSG_ALLSLOTINFO, slot ids, team/army/race | `STAGE_MSGID_*` 0x87..0x95 (INTRODUCTION, SETSLOTID, SETSTAGEDATA, GAMESETTINGS, PLAYER_CHANGE*) | **Concept and state machine yes; wire no.** The SWINE SMulti slot model (`SMulti_SlotInfo m_Slots[8]`) is the same lineage. |
| Lockstep | msg 13 frame N/N-1/N-2, msg 14 resend, msg 15 leaving-player frames; `SendFrameData(frameCount, SStream*)` | `APP_MSGID_PACKET_DATA` with lengthFirst/lengthSecond (N, N-1, N-2), RESEND_REQUEST, `SFrameHistory frames[83]`, same `SendFrameData/GetFrameData/HasFrameData` API | **Yes, for the game-logic-facing API and the frame-history logic** (multi.cpp ~1000 lines). Swap the transport underneath. |
| Map/army transfer | SNetwork -8 chunks of 1008 bytes, UPLOADTYPE_ARMY | RakNet FileListTransfer | No (Panzers-only wire). |
| LAN discovery | SNetwork -5/-6 broadcast to 5555 | RakNet AdvertiseSystem on 60500+, LanDiscovery 60600+ | No. |
| Chat (in game) | SMulti message (in the 2..12 range) | `STAGE_MSGID_CHAT` 0x8D | Concept only. |
| NAT | RakNet NatPunchthroughClient, server UDP 61111 | same (nat.kite-games.com:61111) | **Yes.** The RakNet lib and the natpunch server are reusable as-is. |
| Lobby service | RG (TCP 18600/1008) | Kite matchmaker (TCP 9999), optional GameSpy-2001 backend | `IMatchmakingBackend` interface yes (~40 lines). RG protocol is Panzers-only. |
| UPnP | RakNet-side helper (logs `[UPNP]`, miniupnpc-style) | `upnpgateway.cpp` (miniupnpc-like) | Yes. |
| UI | SMultiPreMenu / LAN / DirectIP / RankedGaming / title+staging room / SChatRoomMenu | SWINE menus; `matchlistbox`/`playerlistbox` already imported into the target repo | The list-box widgets yes. The menus are Panzers-only lifts. |

## 4. GameSpy for emulation servers

**What the original (2004 retail) Panzers GameSpy path needs.** HD's version is the same code, but dead.

- Title `cnpanzers`, secret key **`h3Tod8`**. This is the same in the HD exe (immediate at about 0x52be5f) and in the 2004 export.
- Stats product `cnpanzersphaseone`, gstats game id 959 (0x3bf), `http://gamestats.gamespy.com/cnpanzers/`.
- Peer SDK, which includes QR2 + SB under the hood:
  - available check `cnpanzers.available.gamespy.com` UDP 27900;
  - master `cnpanzers.ms%d.gamespy.com` / `cnpanzers.master.gamespy.com`: QR2 heartbeats on UDP 27900 and SB list queries on TCP 28910 with SB encryption keyed by `h3Tod8`;
  - `peerchat.gamespy.com:6667` with CRYPT (gs_peerchat RC4 using the secret key).
- Rooms: title room (`#GSP!cnpanzers`), group rooms, and staging rooms (`#GSP!cnpanzers!<enc ip+port>`). The client also handles the staging-room join errors seen in strings: full, invite-only, banned, bad password.
- Keys seen in strings: `hostname`, `numplayers`, `maxplayers`, `gametype` (Team Match / Domination / Assault / Cooperative), `mapname`, `gamever`, `gamemode` (`openstaging`, `openplaying`, `closedplaying`), plus the CD-key auth (`Authenticating CD key...`, `AuthenticateCDKeyCallback` export 0x52cc60). The full QR2 key list is not determined; it needs the QR2 key-registration callback inside Connect.
- NAT negotiation code is linked and reachable from peer staging. GP is not used.
- In-game transport after the match is set up is still SNetworkUDP (peer only exchanges the host IP:port).

**What the user's SWINE GameSpy work provides** (`swine-portable/network/gamespy_matchmaking.{h,cpp}`, 1,103 + 88 lines; `swinedecomp/serverinfra/gamespy2001/*.py`, about 1,120 lines):

- **Reusable as-is or nearly so:**
  - peerchat client: the CRYPT/705 handshake and `PeerchatCipher` RC4. This is the same scheme in the 2004 peer SDK. Only the gamename and secret need changing to `cnpanzers`/`h3Tod8`.
  - `peerchat.py` server: make `GAME_NAME`/`SECRET_KEY` per-game dicts.
  - `gsmsalg` validate (`GsSecKeyEnc` + `GsBase64`).
  - The `IMatchmakingBackend` shape.
- **Must change for Panzers:**
  - **(1)** Replace the old `\heartbeat\` / `\list\` master protocol with **QR2** (binary UDP 27900: heartbeat 0x03, challenge 0x01, keepalive 0x08, available 0x09) and the **SB server-list** protocol (TCP 28910, encrypted with the game key). Neither exists in the SWINE code.
  - **(2)** Staging-room channel naming and the `\$flags$\`/`@@@NFO` / UTM conventions of the 2004 peer SDK (see strings at 0x8952e4..0x895308).
  - **(3)** If the TopList should work: an available-check responder and a gstats (gamestats TCP 29920) + persist emulation, or simply keep it disabled.
  - **(4)** Panzers UI hooks (SGameSpy listbox/room callbacks) instead of the SWINE UI.
- For a **rebuilt** client, the simplest way to "support GameSpy emulation servers" is an `IMatchmakingBackend` implementation that speaks peerchat + QR2 + SB to OpenSpy-style servers. In-game traffic stays on SNetworkUDP.
  - Original HD clients will **never** use GameSpy matchmaking (dead code), so this path only interoperates with other rebuilt clients and, in principle, with 2004 retail clients if their protocol versions match. That last point is unverified.

## 5. Phased plan

There is a prerequisite for every phase. The target repo has **no gameplay simulation lifted yet**: `world/` is partial, and `game/` and `network/` were excluded at import. Lockstep MP needs deterministic `SGameLogic` frame processing that is bit-identical across clients. For phase (b) it must also be bit-identical to the original exe, including float mode and RNG. Treat "skirmish runs single-player" as the gate before (a) can be played.

### (a) LAN / direct-IP between two rebuilt clients

- **Approach:** a faithful lift of SNetworkUDP and SMulti. They are small, and wire fidelity makes (b) on LAN free.
- **Target files:**
  - `src/network/snetwork.{h,cpp}` (SNetwork/SNetworkUDP, 32 functions);
  - `src/network/multi.{h,cpp}` (SMulti, 91 functions);
  - `src/network/netglobals.cpp`. This replaces the `SMulti::instance` stub in `src/stubs/stub_globals.cpp` and the two-field `SMulti` in `window/playerlistbox.cpp`.
  - `src/panzers/multipremenu.cpp`, `multilanmenu.cpp`, `multidirectipmenu.cpp`, `skirmishchatroom.cpp`, `chatroommenu.cpp` (about 90 functions). SChatRoomMenu is large: 29 KB over 35 functions.
  - Wiring in `superwindow.cpp`: LoadMultiPreMenu, LoadChatRoomView, the OnAction cases, and CreateHostFromCommandLine/ConnectToHostFromCommandLine.
- **Borrowed from SWINE:**
  - the frame-history / lockstep bookkeeping pattern from `network/multi.cpp` (`SFrameHistory`, `GetFrameData` blocking logic, about 600-1000 lines), as a reference rather than a drop-in;
  - `window/matchlistbox`/`playerlistbox` (already in the repo).
  - Use the 2004 `test.c` as the naming reference for SNetwork and SMulti.
- **Estimate:** ~215 functions (32 + 91 + ~90 UI), plus game-logic hooks.
- **Risks:**
  - determinism of the lifted simulation;
  - x64 builds: keep the 16-byte header and the structs packed and 32-bit;
  - the WSAIoctl interface enumeration for broadcast should be replaced with a portable version (getifaddrs / GetAdaptersAddresses);
  - Hungarian-commented edge cases in the reliability layer (resend windows).

### (b) Interop with original Panzers HD clients

- **LAN / direct IP:** comes from (a) once these are byte-exact:
  - the SNetwork header;
  - the knock payload (callback vtbl+0; it may carry the version, player name or CD key; **not yet determined**);
  - message layouts 2..15;
  - the -8 file-chunk stream;
  - simulation determinism.
  - Verify by capturing the original's traffic in a LAN test, which the user would run.
- **Internet (via the RakNet tunnel):**
  - Target files: `src/network/natmanager.{h,cpp}` and `src/network/packetforwarding.{h,cpp}` (~147 functions in the original; a clean re-implementation is maybe 40-60 functions / ~1,200 lines).
  - Link RakNet 4.081 using SWINE's `raknet/CMakeLists.txt` recipe (31 lines, unchanged). Use port 23022, `NatPunchthroughClient`, `SetIncomingDatagramEventHandler`, raw `RNS2` sends, and custom IDs 0x87-0x89.
  - Run SWINE's `serverinfra/natpunch` on UDP 61111.
- **Risks:**
  - Original clients hard-code 176.9.91.194, unless the 0x8f1a98 override is fed in, which was not determined. Reaching a replacement server needs IP-level redirection on the original client's machine, or a byte patch. A hosts-file entry does not work for a literal IP.
  - The forward-entry and playerID-assignment state machine (`NAT_PlayerIDAssigned`, `NAT_RegisterGameConnectedClient`, host migration) must match the original's expectations.
  - The GUID and IP exchange depends on the RG lobby, so it also needs (c).

### (c) RankedGaming path

- **Client.** Do not lift the RG library (~505 functions, generic WC3/DotA code). Write a clean-room client:
  - `src/network/rg/rgclient.{h,cpp}` (framing + socket, ~400 lines), `rgprotocol.h` (packet ids), `rgmasterserver.{h,cpp}` (Panzers glue, ~94 functions in the original);
  - menus `src/panzers/rankedgamingmenu.cpp` and `rankedgamingregistermenu.cpp`;
  - the RG-backed bodies of `SGameSpy` (Init, RefreshServers, title and staging rooms), about 107 functions in the original and maybe half needed.
  - Estimate: about 150 functions, of which about 60 are UI.
- **Server sketch** (Python asyncio, modelled on `swinedecomp/serverinfra/matchmaking`):
  - `rg_gateway.py` on TCP 18600: login 1001 / register 1005 / chat rooms / game list / game hosted-started-ended broadcasts;
  - `rg_loadbalancer.py` on TCP 1008: game id 7001, user verification, result storage;
  - shared `rgframe.py` (u32 len + u8 flag + u16 id + u16-length fields);
  - SQLite accounts. About 1,000-1,500 lines.
  - Original clients reach it by pointing `gateway.nordic.rankedgaming.com` and `loadbalancer.nordic.rankedgaming.com` at it in hosts, since both are resolved with `getaddrinfo`.
- **Work still needed:** map every ID in RGConnection 0x7586d0 and RGLoadBalancer 0x746c90 to its name and field list. That is two big switch functions, about 9 KB together; plan 1-2 RE sessions.
- **Risks:**
  - Unknown semantics of the 3rd login field (hardware id) and of the `18273849...`/`tqit_self` identity.
  - Whether the client verifies anything cryptographically. Nothing seen so far: no TLS, no hashing found in the framing.
  - The NAT IP hard-code from (b).
  - The legal and ethical side of emulating the RG service: offline preservation only.

### (d) GameSpy path for emulation servers

- **Target files:**
  - `src/network/gamespy/` with `peerchat.{h,cpp}` (borrow `PeerchatCipher` + IRC code from `gamespy_matchmaking.cpp`), `qr2.{h,cpp}` (new), `serverbrowser.{h,cpp}` (new, SB + crypt), `gsavailable.cpp` (new);
  - `src/network/sgamespy.{h,cpp}`, re-implementing the 2004 SGameSpy behaviour (use `test.c` as reference);
  - the GameSpy title and staging room menus. They are the same widgets as (c), selected by a backend switch like SWINE's `HD_MATCHMAKING_BACKEND` in `common/hdbeefup.h`, which already exists in the repo.
- **Estimate:** ~120 functions / ~2,500 lines of client code.
- **Server side:** extend `gamespy2001/peerchat.py` with per-game keys and add QR2 + SB servers (~800 lines), or rely on existing OpenSpy-style emulators.
- **Risks:**
  - The SB encryption details and the exact QR2 key set have to come from the 2004 peer SDK behaviour, not from SWINE (which is 2001-era).
  - Original HD clients cannot take part (dead path).
  - 2004 retail interop is unverified.

**Recommended order:**
1. (a), after the skirmish/simulation gate;
2. (b)-LAN;
3. the RG ID mapping, then the (c) server and client together;
4. (b)-internet;
5. (d), which is optional and benefits only rebuilt clients.

## 6. Not determined

- RakNet 4.081 vs 4.082 (wire-identical).
- Who writes the NAT-server override string at 0x8f1a98, and how a client gets the host GUID (probably RG game data).
- The SNetwork knock payload contents (version check?), and the field layouts of SMulti messages 2..12 and 16.
- The full RG packet-ID to name map and the field lists. Also whether `RG Pass` is stored in plaintext, and what the 3rd login field is.
- Whether the GameSpy room wrappers are all NULL-peer safe. Whether HD still uploads the persistent army ("Card Game" strings).
- The QR2 key registration list and the peer staging-room flags of the 2004 build.
- Floating-point and determinism parity between a rebuilt client and the original exe, which is needed for (b).
- Whether any RG or NAT servers are still alive. I made no network contact, by design.
