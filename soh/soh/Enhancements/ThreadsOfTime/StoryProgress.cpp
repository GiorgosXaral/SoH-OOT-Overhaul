#include "StoryProgress.h"

#include <libultraship/bridge/consolevariablebridge.h>
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "z64.h"
#include "macros.h"
#include "variables.h"
#include "soh/Enhancements/gameplaystats.h"
}

namespace StoryProgress {

// ---- Save-file checks -------------------------------------------------------------------------

static bool Ev(int32_t flag) {
    return GET_EVENTCHKINF(flag) != 0;
}

static bool Quest(int32_t questItem) {
    return CHECK_QUEST_ITEM(questItem) != 0;
}

static bool Has(int32_t item) {
    return INV_CONTENT(item) == item;
}

static bool HasOcarina() {
    return INV_CONTENT(ITEM_OCARINA_FAIRY) != ITEM_NONE;
}

static bool HasHookshot() {
    return INV_CONTENT(ITEM_HOOKSHOT) != ITEM_NONE;
}

static bool HasLongshot() {
    return INV_CONTENT(ITEM_HOOKSHOT) == ITEM_LONGSHOT;
}

static bool OwnsEquip(int32_t type, int32_t inv) {
    return CHECK_OWNED_EQUIP(type, inv) != 0;
}

static bool BossKey(int32_t scene) {
    return CHECK_DUNGEON_ITEM(DUNGEON_KEY_BOSS, scene) != 0;
}

static bool BottleHolds(int32_t item) {
    for (int32_t i = 0; i < 4; i++) {
        if (gSaveContext.inventory.items[SLOT_BOTTLE_1 + i] == item) {
            return true;
        }
    }
    return false;
}

// Adult trading sequence: done once Link holds a later item in the chain, or Biggoron's Sword.
static bool TradedPast(int32_t item) {
    const int32_t current = gSaveContext.inventory.items[SLOT_TRADE_ADULT];
    return OwnsEquip(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_BIGGORON) || ((current != ITEM_NONE) && (current > item));
}

static bool Stamped(int32_t timestamp) {
    return gSaveContext.ship.stats.itemTimestamp[timestamp] != 0;
}

// ---- The story --------------------------------------------------------------------------------

static std::vector<Milestone> BuildMainMilestones() {
    std::vector<Milestone> m;

    // ================================ Child ================================

    m.push_back({ Chapter::Child,
                  "Sword and Shield",
                  "A Kokiri's first steps",
                  "Kokiri Forest",
                  {
                      { "Find the Kokiri Sword: in the south-west corner of Kokiri Forest, crawl through the small "
                        "hole at the foot of the cliff (press A), dodge the rolling boulder in the passage, and "
                        "open the chest at the far end.",
                        [] { return OwnsEquip(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_KOKIRI); } },
                      { "Collect 40 Rupees and buy the Deku Shield at the Kokiri Shop. Cut grass, lift rocks, "
                        "climb to the high ledges, and hop across the three marked platforms in the pond "
                        "(5 Rupees each time; go inside a house and come back to do it again).",
                        [] { return OwnsEquip(EQUIP_TYPE_SHIELD, EQUIP_INV_SHIELD_DEKU); } },
                      { "Equip both (Start, then the Equipment screen) and walk past Mido, who blocks the path to "
                        "the Great Deku Tree in the north of the village.",
                        [] { return Ev(EVENTCHKINF_SHOWED_MIDO_SWORD_SHIELD); } },
                  },
                  [] { return Ev(EVENTCHKINF_SHOWED_MIDO_SWORD_SHIELD) || Quest(QUEST_KOKIRI_EMERALD); },
                  -1 });

    m.push_back({ Chapter::Child,
                  "Inside the Great Deku Tree",
                  "A curse within the old tree",
                  "Inside the Great Deku Tree",
                  {
                      { "Talk to the Great Deku Tree; he opens his mouth so you can go inside.",
                        [] { return Ev(EVENTCHKINF_DEKU_TREE_OPENED_MOUTH); } },
                      { "Get the Fairy Slingshot: climb the ladder (not the vines) and follow the path around the "
                        "trunk to a door. Beat the Deku Scrub by holding R to bounce its nuts back, then hop "
                        "quickly across the sinking platform to the chest.",
                        [] { return Has(ITEM_SLINGSHOT); } },
                      { "Shoot down the ladder above the doorway and the Skulltulas off the vines with the "
                        "Slingshot, then climb higher. To reach the basement, go back to the lowest floor and jump "
                        "into the big web in the middle of the room.",
                        nullptr },
                      { "In the basement, step on the switch to light a torch, light a Deku Stick from it, and burn "
                        "the web on the wall to get through.",
                        nullptr },
                      { "Shoot the eye above the next door with the Slingshot. Dive to press the switch under the "
                        "water, ride the floating platform, push the block and keep going. Burn the web on the "
                        "floor with a lit Deku Stick (rolling onto it helps) and drop through.",
                        nullptr },
                      { "Three Deku Scrubs guard Gohma's room. Defeat them in the order 2, 3, 1: the middle one "
                        "first, then the right one, then the left one.",
                        nullptr },
                      { "Queen Gohma: look up (C-Up) to find her on the ceiling. When her eye turns red, shoot it "
                        "with the Slingshot and slash her while she's stunned. Repeat.",
                        [] { return Quest(QUEST_KOKIRI_EMERALD); } },
                  },
                  [] { return Quest(QUEST_KOKIRI_EMERALD); },
                  ITEM_KOKIRI_EMERALD });

    m.push_back({ Chapter::Child,
                  "Leaving the Forest",
                  "Beyond the forest",
                  "Kokiri Forest",
                  {
                      { "Leave Kokiri Forest by the bridge in the south-east. Someone is waiting there for you.",
                        [] { return HasOcarina(); } },
                  },
                  [] { return HasOcarina(); },
                  ITEM_OCARINA_FAIRY });

    m.push_back({ Chapter::Child,
                  "The Princess of Destiny",
                  "A dream of a princess",
                  "Hyrule Castle",
                  {
                      { "Cross Hyrule Field north to Hyrule Castle Town. The drawbridge is raised at night, so get "
                        "there by day (or wait by the gate until morning).",
                        [] { return Ev(EVENTCHKINF_ENTERED_HYRULE_CASTLE); } },
                      { "Take the road north from the Market up to Hyrule Castle. Talk to Malon by the vines to get "
                        "the Weird Egg. It hatches at sunrise.",
                        [] { return Ev(EVENTCHKINF_SPOKE_TO_CHILD_MALON_AT_CASTLE_OR_MARKET); } },
                      { "Wake Talon, asleep to the right of the castle gate: put the hatched Chicken on a C button "
                        "and use it on him.",
                        [] { return Ev(EVENTCHKINF_TALON_WOKEN_IN_CASTLE); } },
                      { "Push and pull the two crates marked with a bull into the moat so they stack, climb onto "
                        "them, and jump across to the hole marked with a Triforce in the castle wall. Crawl through.",
                        nullptr },
                      { "Sneak through the courtyard: move when the guards look away, and hide behind the fountains "
                        "and statues. Zelda is in the garden at the end.",
                        [] { return Ev(EVENTCHKINF_OBTAINED_ZELDAS_LETTER); } },
                      { "Talk to Zelda; she gives you her letter. Impa then takes you out of the castle and teaches "
                        "you Zelda's Lullaby.",
                        [] { return Quest(QUEST_SONG_LULLABY); } },
                  },
                  [] { return Quest(QUEST_SONG_LULLABY); },
                  ITEM_SONG_LULLABY });

    m.push_back({ Chapter::Child,
                  "The Goron's Ruby",
                  "Trouble beneath the mountain",
                  "Death Mountain and Dodongo's Cavern",
                  {
                      { "Go east across Hyrule Field to Kakariko Village. Put Zelda's Letter on a C button and show "
                        "it to the guard at the gate in the north of the village; he lets you up Death Mountain.",
                        [] { return Ev(EVENTCHKINF_ENTERED_DEATH_MOUNTAIN_TRAIL); } },
                      { "Learn Saria's Song: in Kokiri Forest, enter the Lost Woods (north) and follow the music; it "
                        "gets louder on the right path, and a wrong turn sends you back to the start. Beat the "
                        "Wolfos at the gate of the Sacred Forest Meadow, cross the maze, and play your Ocarina with "
                        "Saria on her stump.",
                        [] { return Quest(QUEST_SONG_SARIA); } },
                      { "Climb Death Mountain Trail to Goron City (take the right branch near the flag).",
                        [] { return Ev(EVENTCHKINF_ENTERED_GORON_CITY); } },
                      { "On the bottom floor of Goron City, stand in front of Darunia's door and play Zelda's "
                        "Lullaby. Inside, play Saria's Song: Darunia dances and gives you the Goron's Bracelet, so "
                        "you can pick up Bomb Flowers.",
                        [] { return CUR_UPG_VALUE(UPG_STRENGTH) >= 1; } },
                      { "On Death Mountain Trail, find the Bomb Flower on the small ledge where a Goron sits, above "
                        "the cave entrance. Pull it up and drop it over the edge onto the boulder blocking Dodongo's "
                        "Cavern.",
                        [] { return Ev(EVENTCHKINF_BOMBED_DODONGOS_CAVERN_ENTRANCE); } },
                      { "Inside, blow open cracked walls with Bomb Flowers (or let a Baby Dodongo explode next to "
                        "them), and light torches with a Deku Stick.",
                        nullptr },
                      { "Get the Bomb Bag: after the second Lizalfos fight on the upper level, climb the ledge on "
                        "the west side of the next room and open the big chest.",
                        [] { return Has(ITEM_BOMB); } },
                      { "Open the giant Dodongo skull: from the bridge above it, drop a bomb into each of its eyes. "
                        "Its mouth opens.",
                        nullptr },
                      { "Inside the skull, push the big block into the hole, then bomb the middle of the floor to "
                        "drop into King Dodongo's chamber.",
                        nullptr },
                      { "King Dodongo: when he breathes in, throw a bomb into his open mouth, then slash him while "
                        "he's stunned. Grab the Bomb Flowers in the corners if you run out of bombs.",
                        [] { return Quest(QUEST_GORON_RUBY); } },
                  },
                  [] { return Quest(QUEST_GORON_RUBY); },
                  ITEM_GORON_RUBY });

    m.push_back({ Chapter::Child,
                  "The Zora's Sapphire",
                  "A princess and a giant fish",
                  "Zora's Domain and Jabu-Jabu's Belly",
                  {
                      { "Zora's River starts on the east side of Hyrule Field, just south of the way into Kakariko. "
                        "Bomb the boulders blocking it and follow the river upstream.",
                        nullptr },
                      { "At the waterfall at the end, stand on the Triforce symbol and play Zelda's Lullaby, then "
                        "jump through to Zora's Domain.",
                        [] { return Ev(EVENTCHKINF_ENTERED_ZORAS_DOMAIN); } },
                      { "Win the Silver Scale: take the left path in King Zora's room to the high diving spot, pay "
                        "the Zora 20 Rupees, and collect all 5 Rupees he throws in before time runs out.",
                        [] { return CUR_UPG_VALUE(UPG_SCALE) >= 1; } },
                      { "Dive into the small tunnel at the bottom of Zora's Domain's main pool; it leads to Lake "
                        "Hylia. Just outside the tunnel, look for bubbles and dive for the bottle: a letter from "
                        "Princess Ruto.",
                        [] { return BottleHolds(ITEM_LETTER_RUTO) || Ev(EVENTCHKINF_KING_ZORA_MOVED); } },
                      { "Go back to King Zora, put the letter on a C button and show it to him. He moves aside, "
                        "opening the way to Zora's Fountain.",
                        [] { return Ev(EVENTCHKINF_KING_ZORA_MOVED); } },
                      { "Catch a fish: use the empty bottle on the fish in the shallow water near the Zora Shop.",
                        [] { return BottleHolds(ITEM_FISH) || Ev(EVENTCHKINF_OFFERED_FISH_TO_JABU_JABU); } },
                      { "At Zora's Fountain, stand in front of Lord Jabu-Jabu and use the fish on him.",
                        [] { return Ev(EVENTCHKINF_OFFERED_FISH_TO_JABU_JABU); } },
                      { "Inside, find Princess Ruto, pick her up and carry her with you; put her down on floor "
                        "switches to hold them.",
                        nullptr },
                      { "Get the Boomerang: take the right path at the fork, step on the switch while carrying "
                        "Ruto, and defeat the four Stingers to open the chest.",
                        [] { return Has(ITEM_BOOMERANG); } },
                      { "Destroy the tentacles with the Boomerang (four hits each) to open the way, then drop down "
                        "the north-west pit. Throw Ruto onto the central ledge, beat Big Octo (Boomerang it from "
                        "behind, then slash), and carry on up to the boss door.",
                        nullptr },
                      { "Barinade: Boomerang the three cords holding it to the ceiling, then stun it with the "
                        "Boomerang and strike. When jellyfish spin around it, Boomerang them away first.",
                        [] { return Quest(QUEST_ZORA_SAPPHIRE); } },
                  },
                  [] { return Quest(QUEST_ZORA_SAPPHIRE); },
                  ITEM_ZORA_SAPPHIRE });

    m.push_back({ Chapter::Child,
                  "The Door of Time",
                  "Three stones, one door",
                  "Hyrule Castle Town",
                  {
                      { "With all three Spiritual Stones, go back to Hyrule Castle Town. At the drawbridge something "
                        "is thrown into the moat: dive in to get the Ocarina of Time.",
                        [] { return Has(ITEM_OCARINA_TIME); } },
                      { "Go to the Temple of Time (north-east of the Market, past the Happy Mask Shop). Stand in "
                        "front of the altar and play the Song of Time.",
                        [] { return Ev(EVENTCHKINF_OPENED_THE_DOOR_OF_TIME); } },
                  },
                  [] { return Ev(EVENTCHKINF_OPENED_THE_DOOR_OF_TIME); },
                  ITEM_OCARINA_TIME });

    m.push_back({ Chapter::Child,
                  "The Master Sword",
                  "A blade that waits",
                  "Temple of Time",
                  {
                      { "Walk through the open door at the back of the Temple of Time and pull the Master Sword "
                        "from its pedestal.",
                        [] { return Ev(EVENTCHKINF_TIME_TRAVELED_TO_ADULT); } },
                  },
                  [] { return Ev(EVENTCHKINF_TIME_TRAVELED_TO_ADULT); },
                  ITEM_MEDALLION_LIGHT });

    // ================================ Adult ================================

    m.push_back({ Chapter::Adult,
                  "The Hookshot",
                  "A race against a ghost",
                  "Kakariko Graveyard",
                  {
                      { "Leave the Temple of Time and head east across Hyrule Field to Kakariko Village. (Getting "
                        "Epona now makes travel much faster: see Side Threads.)",
                        nullptr },
                      { "In the Graveyard, find the gravestone with flowers in front of it in the upper, back part "
                        "(not the one you opened as a child). Grab it with A, pull it back, and drop into the hole.",
                        nullptr },
                      { "Follow Dampé's ghost through the tunnels and keep up with him; he always takes the same "
                        "route. Win the race to get the Hookshot.",
                        [] { return HasHookshot(); } },
                  },
                  [] { return HasHookshot(); },
                  ITEM_HOOKSHOT });

    m.push_back({ Chapter::Adult,
                  "The Forest Temple",
                  "A forest held in silence",
                  "Sacred Forest Meadow and the Forest Temple",
                  {
                      { "Go through Kokiri Forest into the Lost Woods: from the entrance go right, left, right, left "
                        "to find Mido, and play Saria's Song to make him move.",
                        nullptr },
                      { "In the Sacred Forest Meadow, sneak past or defeat the Moblins (sidestep the shockwaves from "
                        "the big one's club). Where you met Saria, Sheik teaches you the Minuet of Forest.",
                        [] { return Quest(QUEST_SONG_MINUET); } },
                      { "Hookshot to the tree branch above the platform and walk into the Forest Temple.",
                        nullptr },
                      { "Find the Fairy Bow: shoot the grey eye above the door of the twisted corridor to straighten "
                        "it, then beat the Stalfos in the room beyond; the Bow is in the big chest.",
                        [] { return Has(ITEM_BOW); } },
                      { "Hunt the four Poe Sisters: shoot each painting of a sister with arrows until she appears, "
                        "then defeat her; each one lights a torch in the main hall. The last one, Meg, hides among "
                        "copies: the real one spins around once more.",
                        nullptr },
                      { "The Boss Key is in the big chest in the room before the twisted corridor (straighten it "
                        "first).",
                        [] { return BossKey(SCENE_FOREST_TEMPLE); } },
                      { "With all four torches lit, the elevator appears in the middle of the main hall. Ride it "
                        "down, then push the walls to turn the basement until the way to the boss door opens.",
                        nullptr },
                      { "Phantom Ganon: shoot the real rider (the brighter one) with an arrow as he comes out of a "
                        "painting. Then knock his energy balls back with your sword until one hits him, and strike.",
                        [] { return Quest(QUEST_MEDALLION_FOREST); } },
                  },
                  [] { return Quest(QUEST_MEDALLION_FOREST); },
                  ITEM_MEDALLION_FOREST });

    m.push_back({ Chapter::Adult,
                  "The Prelude of Light",
                  "A song of return",
                  "Temple of Time",
                  {
                      { "Go back to the Temple of Time. Sheik teaches you the Prelude of Light, which brings you "
                        "back here from anywhere.",
                        [] { return Quest(QUEST_SONG_PRELUDE); } },
                      { "From now on you can travel between ages: put the Master Sword back in its pedestal to "
                        "become a child, and pull it out again to grow up. Some things can only be done as a child.",
                        nullptr },
                  },
                  [] { return Quest(QUEST_SONG_PRELUDE); },
                  ITEM_SONG_PRELUDE });

    m.push_back({ Chapter::Adult,
                  "The Fire Temple",
                  "A dragon wakes",
                  "Death Mountain Crater and the Fire Temple",
                  {
                      { "Climb to Goron City; it's almost empty. Stop the Goron rolling around the middle floor by "
                        "putting a bomb in his path, then talk to him. He gives you the Goron Tunic: wear it to "
                        "survive the heat.",
                        [] { return OwnsEquip(EQUIP_TYPE_TUNIC, EQUIP_INV_TUNIC_GORON); } },
                      { "In Darunia's room (bottom floor), pull the big statue at the back away from the wall: it "
                        "hides a shortcut to Death Mountain Crater.",
                        nullptr },
                      { "In the crater, Hookshot across the broken bridge (aim at the wooden post) and meet Sheik, "
                        "who teaches you the Bolero of Fire. The Fire Temple is just beyond.",
                        [] { return Quest(QUEST_SONG_BOLERO); } },
                      { "Inside, free the trapped Gorons by stepping on the switches by their cells; each one leaves "
                        "a Small Key.",
                        nullptr },
                      { "Get the Megaton Hammer: step on the switch and race up to the chest on the small ledge "
                        "before the flames come back.",
                        [] { return Has(ITEM_HAMMER); } },
                      { "Use the Hammer on rusted switches, face statues and the big pillar. The Boss Key is in the "
                        "chest after the second Flare Dancer.",
                        [] { return BossKey(SCENE_FIRE_TEMPLE); } },
                      { "Volvagia: when its head pops out of a hole, hit it with the Megaton Hammer, then slash it. "
                        "Watch the floor for the shadows of falling rocks.",
                        [] { return Quest(QUEST_MEDALLION_FIRE); } },
                  },
                  [] { return Quest(QUEST_MEDALLION_FIRE); },
                  ITEM_MEDALLION_FIRE });

    m.push_back({ Chapter::Adult,
                  "The Water Temple",
                  "A frozen kingdom",
                  "Zora's Fountain, Lake Hylia and the Water Temple",
                  {
                      { "Go up Zora's River to Zora's Domain, now frozen, and on to Zora's Fountain. Hop across the "
                        "ice floes to the cave on the north side: the Ice Cavern.",
                        nullptr },
                      { "In the Ice Cavern, fill your bottles with Blue Fire to melt red ice. At the end, beat the "
                        "White Wolfos for the Iron Boots; Sheik then teaches you the Serenade of Water.",
                        [] { return OwnsEquip(EQUIP_TYPE_BOOTS, EQUIP_INV_BOOTS_IRON); } },
                      { "Pour Blue Fire over the frozen King Zora; he gives you the Zora Tunic, which lets you "
                        "breathe underwater.",
                        [] { return OwnsEquip(EQUIP_TYPE_TUNIC, EQUIP_INV_TUNIC_ZORA); } },
                      { "Go to Lake Hylia (the Serenade of Water takes you there). In the middle of the dried-up "
                        "lake, wear the Iron Boots to sink to the temple door, and Hookshot the crystal switch above "
                        "it.",
                        nullptr },
                      { "Change the water level by playing Zelda's Lullaby at the three Triforce symbols: lowest on "
                        "the south wall (near where you meet Ruto), middle on the first ledge of the central tower, "
                        "highest in the room off the top of the tower.",
                        nullptr },
                      { "Beat Dark Link in the misty room with a lone tree (don't lock on, just swing; the Hammer "
                        "or magic also work). The Longshot is in the chest beyond.",
                        [] { return HasLongshot(); } },
                      { "The Boss Key is at the end of the western route, reached with the Longshot.",
                        [] { return BossKey(SCENE_WATER_TEMPLE); } },
                      { "Raise the water to the top, go behind the central tower and through the spiked corridor to "
                        "the boss. Morpha: Longshot its core out of the water and slash it.",
                        [] { return Quest(QUEST_MEDALLION_WATER); } },
                  },
                  [] { return Quest(QUEST_MEDALLION_WATER); },
                  ITEM_MEDALLION_WATER });

    m.push_back({ Chapter::Adult,
                  "The Eye of Truth",
                  "Something hidden beneath the village",
                  "Kakariko Village and the Bottom of the Well",
                  {
                      { "Get magic: at the very top of Death Mountain Trail, bomb the wall to the left of the owl. "
                        "Inside, play Zelda's Lullaby on the Triforce; the Great Fairy gives you a Magic Meter.",
                        [] { return gSaveContext.isMagicAcquired != 0; } },
                      { "As an adult, visit the windmill in Kakariko. The man there plays a song he learned from a "
                        "boy with an ocarina: learn the Song of Storms from him.",
                        [] { return Quest(QUEST_SONG_STORMS); } },
                      { "Become a child (put the Master Sword back), go into the windmill and play the Song of "
                        "Storms. The windmill spins faster and the village well drains.",
                        [] { return Ev(EVENTCHKINF_DRAINED_WELL_IN_KAKARIKO); } },
                      { "Climb down into the well. Many walls here are fake: walk into them. In the main room, play "
                        "Zelda's Lullaby on the big Triforce on the floor to lower the water.",
                        nullptr },
                      { "Dead Hand: let one of the white hands grab you to make it appear, wiggle free, then hit its "
                        "head when it comes close. The Lens of Truth is in the chest afterwards.",
                        [] { return Has(ITEM_LENS); } },
                  },
                  [] { return Has(ITEM_LENS); },
                  ITEM_LENS });

    m.push_back({ Chapter::Adult,
                  "The Shadow Temple",
                  "Fire over Kakariko",
                  "Kakariko Graveyard and the Shadow Temple",
                  {
                      { "Go to Kakariko Village as an adult; Sheik teaches you the Nocturne of Shadow.",
                        [] { return Quest(QUEST_SONG_NOCTURNE); } },
                      { "Get Din's Fire as a child: along the path up to Hyrule Castle, bomb the boulder at the end "
                        "of a side path to uncover a hole. Inside, play Zelda's Lullaby on the Triforce for the "
                        "Great Fairy.",
                        [] { return Has(ITEM_DINS_FIRE); } },
                      { "Warp with the Nocturne of Shadow. Light every torch around the platform with Din's Fire to "
                        "open the temple.",
                        nullptr },
                      { "Use the Lens of Truth often: walls, platforms and enemies may be invisible. The Hover Boots "
                        "are in the chest after Dead Hand.",
                        [] { return OwnsEquip(EQUIP_TYPE_BOOTS, EQUIP_INV_BOOTS_HOVER); } },
                      { "Ride the ghost ship: stand on its Triforce symbol and play Zelda's Lullaby, defeat the "
                        "Stalfos, and jump off before it sinks.",
                        nullptr },
                      { "Burn the wooden spiked walls with Din's Fire to reach the Boss Key, then shoot the Bomb "
                        "Flowers by the statue with an arrow to drop a bridge across.",
                        [] { return BossKey(SCENE_SHADOW_TEMPLE); } },
                      { "Bongo Bongo: wear the Hover Boots. Shoot one hand with an arrow and the other quickly "
                        "after, use the Lens of Truth to see its eye, then shoot the eye and slash.",
                        [] { return Quest(QUEST_MEDALLION_SHADOW); } },
                  },
                  [] { return Quest(QUEST_MEDALLION_SHADOW); },
                  ITEM_MEDALLION_SHADOW });

    m.push_back({ Chapter::Adult,
                  "The Spirit Temple",
                  "Sands of the Colossus",
                  "Gerudo Valley, the Haunted Wasteland and the Spirit Temple",
                  {
                      { "Cross the broken bridge in Gerudo Valley by jumping it on Epona, or with the Longshot.",
                        nullptr },
                      { "At the Gerudo Fortress you'll be caught and locked up: Longshot out of the window. Sneak "
                        "through the fortress and free the four carpenters; beat the guard near each cell for its "
                        "key.",
                        [] { return GET_EVENTCHKINF_CARPENTERS_FREE_ALL() != 0; } },
                      { "After the last carpenter you get the Gerudo Membership Card. Ask the guard on top of the "
                        "big gate at the north to open it.",
                        [] { return Quest(QUEST_GERUDO_CARD); } },
                      { "Haunted Wasteland: Longshot across the river of sand, follow the flags to the stone "
                        "building, then use the Lens of Truth and follow the ghost guide to Desert Colossus.",
                        [] { return Ev(EVENTCHKINF_ENTERED_DESERT_COLOSSUS); } },
                      { "At the Colossus, Sheik teaches you the Requiem of Spirit.",
                        [] { return Quest(QUEST_SONG_REQUIEM); } },
                      { "Come back as a child (Master Sword back, then play the Requiem). Talk to Nabooru and crawl "
                        "through the small hole. The Silver Gauntlets are in the chest after the Iron Knuckle.",
                        [] { return CUR_UPG_VALUE(UPG_STRENGTH) >= 2; } },
                      { "Return as an adult with the Requiem. The Mirror Shield is in the chest after the second "
                        "Iron Knuckle.",
                        [] { return OwnsEquip(EQUIP_TYPE_SHIELD, EQUIP_INV_SHIELD_MIRROR); } },
                      { "Reflect sunlight with the Mirror Shield onto the sun faces to open the way. The Boss Key "
                        "comes after you shoot the golden eye and hit the switch.",
                        [] { return BossKey(SCENE_SPIRIT_TEMPLE); } },
                      { "Reflect light onto the giant statue's face, then Longshot to the boss door. Twinrova: "
                        "reflect each sister's beam at the other; once they merge, absorb three blasts of the same "
                        "kind with the shield, release them, and strike.",
                        [] { return Quest(QUEST_MEDALLION_SPIRIT); } },
                  },
                  [] { return Quest(QUEST_MEDALLION_SPIRIT); },
                  ITEM_MEDALLION_SPIRIT });

    // ================================ Finale ================================

    m.push_back({ Chapter::Finale,
                  "The Light Arrows",
                  "Six sages awaken",
                  "Temple of Time",
                  {
                      { "With all six Medallions, go back to the Temple of Time. Someone is waiting for you there.",
                        [] { return Has(ITEM_ARROW_LIGHT); } },
                  },
                  [] { return Has(ITEM_ARROW_LIGHT); },
                  ITEM_ARROW_LIGHT });

    m.push_back({ Chapter::Finale,
                  "Ganon's Castle",
                  "A bridge of light",
                  "Ganon's Castle",
                  {
                      { "Go to where Hyrule Castle used to stand (north of the Market). The sages build a rainbow "
                        "bridge to Ganon's Castle.",
                        [] { return Ev(EVENTCHKINF_RAINBOW_BRIDGE_BUILT); } },
                      { "Six barriers block the tower; each trial ends by shooting a Light Arrow at the orb at its "
                        "end.",
                        nullptr },
                      { "Shadow trial (purple): light the torch with a Fire Arrow, cross with the Hover Boots and the "
                        "Lens of Truth, and hit the rusted switch with the Hammer. The Golden Gauntlets are in a "
                        "chest here.",
                        [] { return Ev(EVENTCHKINF_COMPLETED_SHADOW_TRIAL); } },
                      { "Forest trial (green): light the torches with Din's Fire and shoot a Fire Arrow at the torch "
                        "above the door; collect the Silver Rupees with the Hover Boots and Longshot.",
                        [] { return Ev(EVENTCHKINF_COMPLETED_FOREST_TRIAL); } },
                      { "Water trial (blue): melt the red ice with Blue Fire from a bottle and hit the rusted switch "
                        "with the Hammer.",
                        [] { return Ev(EVENTCHKINF_COMPLETED_WATER_TRIAL); } },
                      { "Fire trial (red): collect all the Silver Rupees; the last one needs the Golden Gauntlets.",
                        [] { return Ev(EVENTCHKINF_COMPLETED_FIRE_TRIAL); } },
                      { "Spirit trial (orange): hit the crystal switch with a Bombchu, burn the webs with Fire "
                        "Arrows, and reflect sunlight onto the right sun with the Mirror Shield.",
                        [] { return Ev(EVENTCHKINF_COMPLETED_SPIRIT_TRIAL); } },
                      { "Light trial (yellow): lift the boulder at its entrance with the Golden Gauntlets, play "
                        "Zelda's Lullaby for a key, and use the Lens of Truth to find the real way.",
                        [] { return Ev(EVENTCHKINF_COMPLETED_LIGHT_TRIAL); } },
                      { "With all six barriers gone, the barrier around the tower disappears. Climb the tower.",
                        [] { return Ev(EVENTCHKINF_DISPELLED_GANONS_TOWER_BARRIER); } },
                  },
                  [] { return Ev(EVENTCHKINF_DISPELLED_GANONS_TOWER_BARRIER); },
                  -1 });

    m.push_back({ Chapter::Finale,
                  "Ganondorf",
                  "The King of Evil",
                  "Ganon's Tower",
                  {
                      { "Knock his energy balls back with your sword until one stuns him, then shoot him with a "
                        "Light Arrow and strike while he's down. If you run low, break the pots on the lower level.",
                        [] { return Stamped(TIMESTAMP_DEFEAT_GANONDORF); } },
                      { "Escape the collapsing castle with Zelda. When fire traps her, defeat the Stalfos to free "
                        "her.",
                        nullptr },
                  },
                  [] { return Stamped(TIMESTAMP_DEFEAT_GANONDORF); },
                  TIMESTAMP_DEFEAT_GANONDORF });

    m.push_back({ Chapter::Finale,
                  "Ganon",
                  "The final battle",
                  "Ruins of Ganon's Castle",
                  {
                      { "Your sword is knocked out of reach. Stun Ganon with Light Arrows, get behind him and hit his "
                        "tail (the Megaton Hammer or Biggoron's Sword work best).",
                        nullptr },
                      { "When Zelda tells you, run and grab the Master Sword, then finish him.",
                        [] { return Stamped(TIMESTAMP_DEFEAT_GANON); } },
                  },
                  [] { return Stamped(TIMESTAMP_DEFEAT_GANON); },
                  TIMESTAMP_DEFEAT_GANON });

    return m;
}

static std::vector<SideThread> BuildSideThreads() {
    std::vector<SideThread> s;

    s.push_back({ "Epona",
                  {
                      { "As a child, after Talon has gone home, visit Lon Lon Ranch in the middle of Hyrule Field. "
                        "Talk to Malon in the corral and play your Ocarina with her to learn Epona's Song.",
                        [] { return Quest(QUEST_SONG_EPONA); } },
                      { "As an adult, pay Ingo 10 Rupees to ride and play Epona's Song to call her. Beat Ingo in two "
                        "races, then jump the ranch wall to escape with her.",
                        [] { return Ev(EVENTCHKINF_EPONA_OBTAINED); } },
                  },
                  [] { return Ev(EVENTCHKINF_EPONA_OBTAINED); } });

    s.push_back({ "Great Fairies and Magic",
                  {
                      { "Magic Meter: at the top of Death Mountain Trail, bomb the wall to the left of the owl and "
                        "play Zelda's Lullaby on the Triforce inside.",
                        [] { return gSaveContext.isMagicAcquired != 0; } },
                      { "Din's Fire (child): bomb the boulder at the end of a side path along the road up to Hyrule "
                        "Castle, and play Zelda's Lullaby inside.",
                        [] { return Has(ITEM_DINS_FIRE); } },
                      { "Farore's Wind (child): at Zora's Fountain, bomb the light-coloured boulder in the "
                        "south-east corner, and play Zelda's Lullaby inside.",
                        [] { return Has(ITEM_FARORES_WIND); } },
                      { "Double Magic: in Death Mountain Crater, cross the broken bridge with the Hookshot and smash "
                        "the boulders in front of the cave with the Megaton Hammer.",
                        [] { return gSaveContext.isDoubleMagicAcquired != 0; } },
                      { "Nayru's Love: at Desert Colossus, bomb the cracked wall in the rocks to the north.",
                        [] { return Has(ITEM_NAYRUS_LOVE); } },
                      { "Double Defense: outside Ganon's Castle, lift the huge stone with the Golden Gauntlets.",
                        [] { return gSaveContext.isDoubleDefenseAcquired != 0; } },
                  },
                  [] {
                      return gSaveContext.isDoubleMagicAcquired && gSaveContext.isDoubleDefenseAcquired &&
                             Has(ITEM_DINS_FIRE) && Has(ITEM_FARORES_WIND) && Has(ITEM_NAYRUS_LOVE);
                  } });

    s.push_back({ "Biggoron's Sword",
                  {
                      { "As an adult, the Cucco Lady in Kakariko gives you a Pocket Egg. It hatches overnight.",
                        [] { return TradedPast(ITEM_POCKET_EGG - 1); } },
                      { "Wake Talon in his house in Kakariko with the Pocket Cucco, then give it back to the Cucco "
                        "Lady for Cojiro, a blue Cucco.",
                        [] { return TradedPast(ITEM_POCKET_CUCCO); } },
                      { "Give Cojiro to the young man sitting in the Lost Woods (near the entrance) for an Odd "
                        "Mushroom.",
                        [] { return TradedPast(ITEM_COJIRO); } },
                      { "Hurry: take the Odd Mushroom to the old woman in the shop behind the Kakariko Potion Shop "
                        "for an Odd Potion.",
                        [] { return TradedPast(ITEM_ODD_MUSHROOM); } },
                      { "Hurry: take the Odd Potion back to the same spot in the Lost Woods; the Kokiri girl there "
                        "gives you a Poacher's Saw.",
                        [] { return TradedPast(ITEM_ODD_POTION); } },
                      { "Give the saw to the carpenters' boss in Gerudo Valley for a Broken Goron's Sword.",
                        [] { return TradedPast(ITEM_SAW); } },
                      { "Take the broken sword to Biggoron at the top of Death Mountain for a Prescription.",
                        [] { return TradedPast(ITEM_SWORD_BROKEN); } },
                      { "Show the Prescription to King Zora for an Eyeball Frog.",
                        [] { return TradedPast(ITEM_PRESCRIPTION); } },
                      { "Hurry: take the frog to the scientist in the Lake Hylia laboratory for the World's Finest "
                        "Eye Drops.",
                        [] { return TradedPast(ITEM_FROG); } },
                      { "Hurry: take the Eye Drops to Biggoron for a Claim Check.",
                        [] { return TradedPast(ITEM_EYEDROPS); } },
                      { "Show Biggoron the Claim Check and come back after three days (sleep in Kakariko, or play "
                        "the Sun's Song) for Biggoron's Sword.",
                        [] { return OwnsEquip(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_BIGGORON); } },
                  },
                  [] { return OwnsEquip(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_BIGGORON); } });

    return s;
}

const std::vector<Milestone>& MainMilestones() {
    static const std::vector<Milestone> milestones = BuildMainMilestones();
    return milestones;
}

const std::vector<SideThread>& SideThreads() {
    static const std::vector<SideThread> threads = BuildSideThreads();
    return threads;
}

int32_t CurrentMilestoneIndex() {
    const auto& milestones = MainMilestones();
    for (size_t i = 0; i < milestones.size(); i++) {
        if (!milestones[i].done()) {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

uint32_t MilestoneTime(const Milestone& milestone) {
    if ((milestone.timestamp < 0) || (milestone.timestamp >= TIMESTAMP_MAX)) {
        return 0;
    }
    return gSaveContext.ship.stats.itemTimestamp[milestone.timestamp];
}

} // namespace StoryProgress
