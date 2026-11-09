# SoH-OOT-Overhaul

A modified version of [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright) 9.3.1 (Dewey), the PC port of *The Legend of Zelda: Ocarina of Time*, with tougher and smarter bosses and enemies, an A Link to the Past style Master Sword beam, and better Epona controls.

Every change can be switched on or off in the in-game menu (press **ESC**). All of them are **on by default**; turning one off gives you the original behavior back.

This is a fan project and is not affiliated with HarbourMasters. You still need your own legally acquired Ocarina of Time ROM.

---

## What's changed

### Master Sword Beam
*Enhancements → Items*

- With full hearts, each normal Master Sword slash throws a spinning blue-white beam of light, like in *A Link to the Past*.
- It hits for exactly the damage of a Master Sword slash, one beam at a time, and disappears when it hits an enemy, a shield or a wall.
- When you're locked on with Z, it aims up or down at the target.
- Jump attacks and spin attacks are unchanged.

### Boss Overhaul
*Enhancements → Difficulty → Enemies* - two switches: **Double HP** and **Extra Attacks**

Every main boss has twice the health, learns new attacks, and becomes more aggressive ("enraged") once badly hurt. Changes take effect the next time a boss fight starts.

| Boss | Health | What's new |
| - | - | - |
| **Gohma** | 10 → 20 hits | Five baby Gohmas on the ceiling instead of three. Lunges forward before the floor slam. Starts attacking from further away (210 instead of 150). Enraged: walks faster. |
| **King Dodongo** | 12 → 24 hits | Fire breath fans out with two extra angled jets. Leaves burning patches while rolling. Enraged: rolls faster, burns longer, and answers every bomb hit with a ring of eight flames. |
| **Barinade** | Tendrils and Bari take 2 hits; core 4 → 8 per phase | Core moves 35% faster and periodically overcharges, swelling its electric aura (55 → 140) to shock anything nearby. Once the tendrils are gone, the zappers charge at double speed. |
| **Phantom Ganon** | 30 → 60 hits | New lightning storm once he's lost a third of his health: two strikes near you, three when badly hurt. Green light marks where each bolt will land, so it can be dodged. Switches to his aggressive endgame pattern at half health. |
| **Volvagia** | 24 → 48 hits | Rocks fall on you when it roars and emerges. Longer chases and rock storms, less idle time, repeated claw swipes. Enraged: rock storms chain straight into a faster chase, with even more rocks. |
| **Morpha** | 20 → 40 hits | Much less downtime between tentacle attacks, faster swing windup, second tentacle after 2 hits instead of 3, faster core. Enraged: every so often the core abandons its pattern and lunges straight at you. |
| **Bongo Bongo** | 36 → 72 hits | Each slam sends a second shockwave, sweeps and claps are faster, and both hands often attack together. Claps become much more frequent as the fight goes on. |
| **Twinrova** | 24 → 48 hits | The sisters need twice as many reflected blasts before merging and fire more often. Merged, she flies and fires faster, recovers sooner, and mixes blast types much earlier. Enraged: chains a second blast immediately. |
| **Ganondorf** | 40 → 80 hits | **Dark storm:** he blinks across the room, throws a fan of five homing magic balls, then calls down three bolts of lightning (four in the second half). Dark motes mark each bolt's landing spot, so keep moving. The tennis rally is no longer all there is. |
| **Ganon** | 30 → 60 hits, then 25 → 50 after he gets back up | Walks faster and fights like the beast in *A Link to the Past*: **leaps** across the arena and lands with a ring of fire; **charges** with both swords (sidestep and he gets stuck in the wall, opening his tail); and **sweeps** the floor with a ring of flame that has one gap to escape through. Enraged: swings 25% faster and adds extra swings to his combo. |

### Miniboss & Enemy Overhaul
*Enhancements → Difficulty → Enemies*

New moves are built only from things already in the game. Original openings are never shortened, anything a miniboss summons vanishes when it falls, and floor shockwaves can be cleared with any jump, sidehop or backflip.

**Minibosses: Double HP** - twice the health for the Dodongo's Cavern Lizalfos, Stalfos, Big Octo, Flare Dancer, Iron Knuckles (Nabooru included), the Gerudo fighters, the Poe Sisters, and the Wolfos and White Wolfos placed in rooms (not the ones roaming Hyrule Field).

**Minibosses: New Attacks**
- **Lizalfos** (Dodongo's Cavern): landing shockwaves, and slashes that carry along the floor
- **Stalfos**: floor-splitting chops, landing shockwaves, and a shield that can shove Link back
- **Big Octo**: water jets while circling, and a wave when it belly-flops
- **Flare Dancer**: fireballs, burning footprints and rings of fire; the core spits fire as it runs
- **Iron Knuckle**: a boomerang axe throw and floor-splitting smashes; the armor bursts off with a shockwave
- **Gerudo fighters**: a dust sweep after the spin, landing shockwaves, and a vanishing strike from behind
- **Poe Sisters**: each throws a ball of her own color
- **Wolfos**: a badly hurt miniboss Wolfos howls for help; White Wolfos claws can freeze

**Dark Link Rework** - twice the health. Throws shadow bolts when kept at a distance (and dodges arrows to answer with one), bursts into a ring of darkness when you stay close, and once badly hurt swings twice as often and rises from the water in darkness. He stands still while winding up either move - a good moment to hit him.

**Dead Hand Rework** - twice the health. Bursts out of the ground with a shockwave and spits balls of cursed darkness (three at once when badly hurt, when he also walks faster). His hands reach further, slam the floor to send tremors at you, and burrow up close if you keep away.

**Enemies: New Tricks**
- **ReDead**: its scream wakes the dead nearby, who chase you further than usual
- **Gibdo**: sudden lunges
- **Like Like**: breathes in with its mouth open, dragging you towards it (Iron Boots hold firm)
- **Octorok**: every third shot is a spread of three rocks
- **Mad Scrub**: every other spit is a spread of three nuts (all can be sent back)
- **Armos**: the last hop of a lunge sends out a shockwave
- **Wallmaster**: a missed grab slams the floor with a shockwave
- **Freezard**: its icy breath ends with a spit of ice shards
- **Poe**: flicks embers from its lantern while you watch it
- **Lizalfos** (outside Dodongo's Cavern): landing shockwaves
- **Dinolfos**: breathe fire at middle distance

**More Encounters** - some enemies bring one or two of their own kind along, and a few regions add a creature that belongs there (Keese with the field's Stalchildren, Withered Deku Babas in the forest, Fire Keese on the mountain...). Never in boss rooms, shops, minigames, grottos, houses or the Thieves' Hideout.
- **In Dungeons** - also adds companions inside dungeons, except in rooms that open once every enemy is defeated.

**Rare Elite Enemies** - about one enemy in twenty is an elite: bigger, sparkling gold, with twice the health. It drops a bonus item when defeated (a red or purple rupee, or whatever you're shortest of).

**Ganon's Reign** - once Ganondorf rules Hyrule (as adult Link), companions are twice as common (three times at night) and elites show up more often at night. Needs More Encounters and/or Rare Elite Enemies.

### Epona Control Overhaul
*Enhancements → Epona*

A port of LT_Schmiddy's "Epona Control Overhaul" mod for *Majora's Mask: Recompiled*.

- **Control Mode**
  - **Tank**: the original controls - the stick steers left and right, pulling back brakes.
  - **Freeform** *(default)*: Epona runs wherever you point the stick, relative to the camera, just like Link on foot. Letting go of the stick brakes.
  - **Hybrid**: Freeform, but switches to Tank while you aim in first person (the bow).
- **Handling sliders**: turn and brake strength for each mode, plus a minimum turn speed for Freeform.
- **Bunny Hood Boost**: wearing the Bunny Hood doubles Epona's speed and turn speed.
- Untick **Epona Control Overhaul** for exactly the original riding.

### Swimming Overhaul
*Enhancements → Swimming*

| Option | What it does |
| - | - |
| **Swim Speed** | 1.0x to 2.0x (default 1.3x) |
| **Smooth Strokes** | Link speeds up steadily while you hold the stick, instead of surging on each stroke and slowing in between |
| **Faster Turning** | Turns about 75% faster, and stops three times quicker when you reverse direction |
| **Zora Tunic Bonus** | 20% faster swimming while wearing the Zora Tunic |
| **Faster Diving** | Sinks about twice as fast while holding A, with full steering underwater instead of half |
| **Faster Resurfacing** | Rises back to the surface about 50% faster |
| **Iron Boots Underwater** | Gets moving and turns about twice as fast underwater; on land the boots are as heavy as ever |

How deep Link can dive still depends on the Silver and Golden Scale. Untick **Swimming Overhaul** for exactly the original swimming.

### Secret Detection
*Enhancements → Secrets*

Undiscovered secrets near Link give off sparkles, and the colour tells you what opens them. Clues stop by themselves once a secret has been found.

| Clue | Opens with | Covers |
| - | - | - |
| **Gold sparkles + dust** | Bombs, bombchus or the hammer | Bombable walls and cracked floors, breakable boulders, hidden grottos |
| **Blue sparkles** | Song of Storms | Hidden grottos that open with the song |
| **Purple sparkles** | Lens of Truth | Fake walls, invisible paths and platforms, invisible chests (paused while the Lens is in use) |
| **White sparkles** | Just look closely | Invisible items hidden in a spot |

- **Clue Strength**: *Subtle* (a few sparkles, close range) or *Obvious* (default: more sparkles from further away, plus dust).
- **Require Stone of Agony**: clues only appear once you own the Stone of Agony (off by default).
- Each clue type has its own switch.

### Threads of Time
*Enhancements → Threads of Time → Open Threads of Time*

A window that follows your journey, so you never need a walkthrough:

- **The Tapestry**: the Spiritual Stones, Medallions and key items, using the game's own icons. Missing ones show as silhouettes and fill in as you play.
- **Next Objective**: where to go and exactly what to do, as a step-by-step checklist that ticks itself as you play.
- **The Timeline**: every story milestone in the Child, Adult and Finale chapters. Finished ones show the play time when you reached them; milestones still ahead show only a spoiler-free hint (tick **Show the whole road** to reveal them).
- **Side Threads**: Epona, the Great Fairies and magic, and Biggoron's Sword, each with steps.
- **Collection**: hearts, Gold Skulltulas and songs.

It follows the original story, so the directions won't match a Randomizer save.

### Instant Text
*Enhancements → Skips & Speed-ups*

- Each text box appears complete at once instead of being typed out. You still press A to go to the next box.
- A character's pause in the middle of a box still waits for A, and Ocarina prompts, Yes/No choices and timed text work as usual.
- Untick **Instant Text** to go back to the normal typing effect and the Text Speed sliders.

---

## How to play (Windows)

### 1. Get the game
Open the [**Actions**](https://github.com/GiorgosXaral/SoH-OOT-Overhaul/actions/workflows/build-overhaul.yml) tab, click the latest successful **build-soh-oot-overhaul** run, and download **soh-oot-overhaul** from the *Artifacts* section at the bottom (you need to be signed in to GitHub).

The download is a complete game folder. You don't need to install the original Ship of Harkinian.

### 2. Check your ROM
You need your own copy of Ocarina of Time. Check that your ROM dump is supported with the [compatibility checker](https://ship.equipment/), or compare its `sha1` hash with the ones in [docs/supportedHashes.json](docs/supportedHashes.json).

### 3. Launch
- Unzip the download into its own folder.
- Run `soh.exe` and select your ROM when asked. On first launch the game builds `oot.o2r` from your ROM; after that it starts straight away.

**Coming from Ship of Harkinian 9.2.3 or 9.3.x?** Copy the `Save` folder and `shipofharkinian.json` from your old folder into the new one to keep your saves and settings.

---

## Controls

### Default keyboard configuration
| N64 | A | B | Z | Start | Analog stick | C buttons | D-Pad |
| - | - | - | - | - | - | - | - |
| Keyboard | X | C | Z | Space | WASD | Arrow keys | TFGH |

### Other shortcuts
| Keys | Action |
| - | - |
| ESC | Toggle menu |
| F2 | Toggle capture mouse input |
| F5 | Save state |
| F6 | Change state |
| F7 | Load state |
| F9 | Toggle Text-to-Speech |
| F11 | Fullscreen |
| Tab | Toggle Alternate assets |
| Ctrl+R | Reset |

### Graphics backends
DirectX 11 is the default on Windows; OpenGL is also available. You can switch in the **Settings** menu (requires a restart). If the game crashes, open `shipofharkinian.json`, find `gfxbackend:""` and change the value to `sdl` to use OpenGL.

## Custom assets
Custom assets are packed in `.otr` archive files. To use them, place them in the `mods` folder. To make your own, see [retro](https://github.com/HarbourMasters64/retro) (OTR generator) and [fast64](https://github.com/HarbourMasters/fast64) (Blender plugin).

---

## Building
Every push to `main` builds a complete Windows game folder automatically with GitHub Actions ([`.github/workflows/build-overhaul.yml`](.github/workflows/build-overhaul.yml)).

To build it yourself, follow the original [building instructions](docs/BUILDING.md).

## Credits
- **Ship of Harkinian** by [HarbourMasters](https://github.com/HarbourMasters/Shipwright), built on [libultraship](https://github.com/Kenix3/libultraship) and the Ocarina of Time decompilation project. Full credits: [docs/CREDITS.md](docs/CREDITS.md).
- **Epona Control Overhaul** originally by LT_Schmiddy for *Majora's Mask: Recompiled*.
- Original Ship of Harkinian links: [website](https://www.shipofharkinian.com/) · [Discord](https://discord.com/invite/shipofharkinian) (for questions about the original game, not this modified version).

More documentation: [Custom Music](docs/CUSTOM_MUSIC.md) · [Controller Mapping](docs/GAME_CONTROLLER_DB.md) · [Modding](docs/MODDING.md) · [Versioning](docs/VERSIONING.md)
