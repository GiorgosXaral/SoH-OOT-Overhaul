/**
 * Miniboss & Enemy Overhaul: summons, helpers, and the encounter extras (companions, spawner tag-alongs, elites and
 * Ganon's Reign). The projectiles live in ovl_En_Eo_Shot; see enemy_overhaul.h for the rules everything follows.
 *
 *  - Companions: some enemies placed in a room bring one or two of their own kind along, and in a few regions a
 *    creature that belongs there (Keese with the night field's Stalchildren, Withered Deku Babas with the forest's
 *    Deku Babas, Fire Keese with the mountain's Tektites and Baby Dodongos, Blue Bubbles with Kakariko's Keese).
 *  - Tag-alongs: the random-encounter spawners (Hyrule Field's Stalchildren, the Leevers, the Tektites, the Wolfos)
 *    sometimes send one more than usual.
 *  - Elites: about one enemy in twenty is bigger, sparkles gold, has twice the health and drops something better.
 *  - Ganon's Reign: once Ganondorf rules (adult Link), companions are twice as common, three times at night, with
 *    more elites at night.
 *
 * Nothing is added to boss rooms, shops, minigames, grottos, houses or the Thieves' Hideout, nor (in dungeons) to a
 * room whose door, chest or gate opens once every enemy is defeated. Extra enemies are ordinary enemies that don't
 * count towards any spawner, never bring companions of their own, and are placed on open floor in sight of the enemy
 * that brought them.
 */

#include "global.h"
#include "enemy_overhaul.h"
#include "overlays/actors/ovl_En_Firefly/z_en_firefly.h"
#include "overlays/actors/ovl_En_Reeba/z_en_reeba.h"
#include "overlays/actors/ovl_En_Poh/z_en_poh.h"
#include "overlays/actors/ovl_En_Bili/z_en_bili.h"

static Vec3f sEoZeroVec = { 0.0f, 0.0f, 0.0f };

/* ---------------------------------------------------------------------------------------------------------------
 * Helpers
 * --------------------------------------------------------------------------------------------------------------- */

s32 Eo_CanAttack(PlayState* play) {
    Player* player = GET_PLAYER(play);

    return !Play_InCsMode(play) && !Player_InBlockingCsMode(play, player) && (gSaveContext.health > 0);
}

s32 Eo_PlayerOnGround(PlayState* play) {
    Player* player = GET_PLAYER(play);

    return (player->actor.bgCheckFlags & 1) && !(player->stateFlags1 & PLAYER_STATE1_ON_HORSE);
}

s32 Eo_HurtPlayer(PlayState* play, Actor* source, u8 damage, s16 yaw, f32 knockback, s32 groundOnly, s32 shock) {
    Player* player = GET_PLAYER(play);

    if ((player->invincibilityTimer != 0) || !Eo_CanAttack(play) || (player->stateFlags1 & PLAYER_STATE1_ON_HORSE) ||
        (player->stateFlags2 & PLAYER_STATE2_GRABBED_BY_ENEMY)) {
        return false;
    }
    if (groundOnly && !Eo_PlayerOnGround(play)) {
        return false;
    }

    Actor_SetPlayerKnockback(play, source, knockback, yaw, knockback * 0.6f,
                  shock ? PLAYER_KNOCKBACK_LARGE_ELECTRIFIED : PLAYER_KNOCKBACK_LARGE, damage);
    return true;
}

s32 Eo_FindFloor(PlayState* play, Vec3f* pos, f32 maxDrop, f32* outY) {
    CollisionPoly* poly;
    Vec3f probe = *pos;
    f32 floorY;

    probe.y += 80.0f;
    floorY = BgCheck_EntityRaycastFloor1(&play->colCtx, &poly, &probe);
    if ((floorY <= BGCHECK_Y_MIN + 1.0f) || ((pos->y - floorY) > maxDrop)) {
        return false;
    }
    *outY = floorY;
    return true;
}

s32 Eo_PosAround(PlayState* play, Vec3f* from, s16 yaw, f32 dist, Vec3f* outPos) {
    Vec3f start = *from;
    Vec3f hitPos;
    CollisionPoly* poly;
    s32 bgId;
    f32 floorY;

    outPos->x = from->x + (Math_SinS(yaw) * dist);
    outPos->y = from->y + 30.0f;
    outPos->z = from->z + (Math_CosS(yaw) * dist);
    start.y += 30.0f;

    if (!BgCheck_EntityLineTest1(&play->colCtx, &start, outPos, &hitPos, &poly, true, false, false, true, &bgId) &&
        Eo_FindFloor(play, outPos, 150.0f, &floorY)) {
        outPos->y = floorY;
        return true;
    }

    *outPos = *from;
    return false;
}

s32 Eo_PosBehindPlayer(PlayState* play, f32 dist, Vec3f* outPos) {
    Player* player = GET_PLAYER(play);

    return Eo_PosAround(play, &player->actor.world.pos, player->actor.shape.rot.y + 0x8000, dist, outPos);
}

void Eo_Puff(PlayState* play, Vec3f* pos, f32 scale) {
    Vec3f puffPos = *pos;

    puffPos.y += 20.0f * scale;
    EffectSsDeadDb_Spawn(play, &puffPos, &sEoZeroVec, &sEoZeroVec, (s16)(70.0f * scale), 6, 250, 250, 250, 255, 150,
                         150, 150, 1, 10, false);
    SoundSource_PlaySfxAtFixedWorldPos(play, pos, 20, NA_SE_EN_EXTINCT);
}

void Eo_DarkPuff(PlayState* play, Vec3f* pos, f32 scale) {
    Vec3f puffPos = *pos;
    Vec3f accel = { 0.0f, 0.3f, 0.0f };
    s32 i;

    for (i = 0; i < 3; i++) {
        puffPos = *pos;
        puffPos.x += Rand_CenteredFloat(30.0f * scale);
        puffPos.y += (15.0f + Rand_ZeroFloat(30.0f)) * scale;
        puffPos.z += Rand_CenteredFloat(30.0f * scale);
        EffectSsDeadDb_Spawn(play, &puffPos, &sEoZeroVec, &accel, (s16)(60.0f * scale), 8, 170, 110, 230, 255, 40,
                             0, 70, 1, 12, false);
    }
    SoundSource_PlaySfxAtFixedWorldPos(play, pos, 20, NA_SE_EV_FANTOM_WARP_S);
}

/* ---------------------------------------------------------------------------------------------------------------
 * Summons
 * --------------------------------------------------------------------------------------------------------------- */

#define EO_SUMMON_MAX 32

typedef struct EoSummonEntry {
    Actor* actor;
    Actor* owner;
} EoSummonEntry;

static EoSummonEntry sEoSummons[EO_SUMMON_MAX];
static s32 sEoSpawningExtra = 0; // set while an extra enemy (summon, companion, tag-along) is being spawned

static Actor* Eo_SpawnExtra(PlayState* play, Actor* anchor, s16 actorId, Vec3f* pos, s16 rotY, s16 params) {
    Actor* actor;

    sEoSpawningExtra++;
    actor = Actor_Spawn(&play->actorCtx, play, actorId, pos->x, pos->y, pos->z, 0, rotY, 0, params);
    sEoSpawningExtra--;

    if ((actor != NULL) && (anchor != NULL)) {
        actor->room = anchor->room;
    }
    return actor;
}

Actor* EoSummon_Spawn(PlayState* play, Actor* owner, s16 actorId, Vec3f* pos, s16 rotY, s16 params) {
    Actor* actor;
    s32 i;

    for (i = 0; i < EO_SUMMON_MAX; i++) {
        if (sEoSummons[i].actor == NULL) {
            break;
        }
    }
    if (i == EO_SUMMON_MAX) {
        return NULL;
    }

    actor = Eo_SpawnExtra(play, owner, actorId, pos, rotY, params);
    if (actor != NULL) {
        sEoSummons[i].actor = actor;
        sEoSummons[i].owner = owner;
        Eo_Puff(play, pos, 1.5f);
    }
    return actor;
}

s32 EoSummon_Count(Actor* owner) {
    s32 i;
    s32 count = 0;

    for (i = 0; i < EO_SUMMON_MAX; i++) {
        if ((sEoSummons[i].actor != NULL) && (sEoSummons[i].owner == owner) &&
            (sEoSummons[i].actor->update != NULL)) {
            count++;
        }
    }
    return count;
}

s32 EoSummon_IsSummon(Actor* actor) {
    s32 i;

    for (i = 0; i < EO_SUMMON_MAX; i++) {
        if ((sEoSummons[i].actor == actor) && (actor->update != NULL)) {
            return true;
        }
    }
    return false;
}

void EoSummon_KillAll(Actor* owner, PlayState* play) {
    s32 i;

    for (i = 0; i < EO_SUMMON_MAX; i++) {
        if ((sEoSummons[i].actor != NULL) && (sEoSummons[i].owner == owner)) {
            if (sEoSummons[i].actor->update != NULL) {
                if (play != NULL) {
                    Eo_Puff(play, &sEoSummons[i].actor->world.pos, 2.0f);
                }
                Actor_Kill(sEoSummons[i].actor);
            }
            sEoSummons[i].owner = NULL;
        }
    }
}

/* ---------------------------------------------------------------------------------------------------------------
 * Regions and Ganon's Reign
 * --------------------------------------------------------------------------------------------------------------- */

typedef enum EoRegion {
    /* 0 */ EO_REGION_NONE,
    /* 1 */ EO_REGION_FIELD,
    /* 2 */ EO_REGION_FOREST,
    /* 3 */ EO_REGION_MOUNTAIN,
    /* 4 */ EO_REGION_WATER,
    /* 5 */ EO_REGION_DESERT,
    /* 6 */ EO_REGION_SHADOW, // Kakariko, the graveyard, the well and the Shadow Temple
    /* 7 */ EO_REGION_CASTLE, // Ganon's Castle and Tower
} EoRegion;

#define EO_REGION_ANY 0xFF

typedef struct EoSceneInfo {
    s16 sceneId;
    u8 region;
    u8 isDungeon;
} EoSceneInfo;

static EoSceneInfo sEoScenes[] = {
    { SCENE_HYRULE_FIELD, EO_REGION_FIELD, false },
    { SCENE_LAKE_HYLIA, EO_REGION_WATER, false },
    { SCENE_OUTSIDE_GANONS_CASTLE, EO_REGION_CASTLE, false },
    { SCENE_KOKIRI_FOREST, EO_REGION_FOREST, false },
    { SCENE_LOST_WOODS, EO_REGION_FOREST, false },
    { SCENE_SACRED_FOREST_MEADOW, EO_REGION_FOREST, false },
    { SCENE_DEKU_TREE, EO_REGION_FOREST, true },
    { SCENE_FOREST_TEMPLE, EO_REGION_FOREST, true },
    { SCENE_DEATH_MOUNTAIN_TRAIL, EO_REGION_MOUNTAIN, false },
    { SCENE_DEATH_MOUNTAIN_CRATER, EO_REGION_MOUNTAIN, false },
    { SCENE_DODONGOS_CAVERN, EO_REGION_MOUNTAIN, true },
    { SCENE_FIRE_TEMPLE, EO_REGION_MOUNTAIN, true },
    { SCENE_ZORAS_RIVER, EO_REGION_WATER, false },
    { SCENE_ZORAS_FOUNTAIN, EO_REGION_WATER, false },
    { SCENE_JABU_JABU, EO_REGION_WATER, true },
    { SCENE_WATER_TEMPLE, EO_REGION_WATER, true },
    { SCENE_ICE_CAVERN, EO_REGION_WATER, true },
    { SCENE_GERUDO_VALLEY, EO_REGION_DESERT, false },
    { SCENE_HAUNTED_WASTELAND, EO_REGION_DESERT, false },
    { SCENE_DESERT_COLOSSUS, EO_REGION_DESERT, false },
    { SCENE_SPIRIT_TEMPLE, EO_REGION_DESERT, true },
    { SCENE_GERUDO_TRAINING_GROUND, EO_REGION_DESERT, true },
    { SCENE_KAKARIKO_VILLAGE, EO_REGION_SHADOW, false },
    { SCENE_GRAVEYARD, EO_REGION_SHADOW, false },
    { SCENE_BOTTOM_OF_THE_WELL, EO_REGION_SHADOW, true },
    { SCENE_SHADOW_TEMPLE, EO_REGION_SHADOW, true },
    { SCENE_INSIDE_GANONS_CASTLE, EO_REGION_CASTLE, true },
    { SCENE_GANONS_TOWER, EO_REGION_CASTLE, true },
};

static EoSceneInfo* EoScene_Get(PlayState* play) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sEoScenes); i++) {
        if (sEoScenes[i].sceneId == play->sceneNum) {
            return &sEoScenes[i];
        }
    }
    return NULL;
}

// 0 as a child or with the option off, 1 in Ganon's reign (adult Link), 2 in Ganon's reign at night.
static s32 Eo_ReignLevel(void) {
    if (!EO_GANONS_REIGN_ON() || !LINK_IS_ADULT) {
        return 0;
    }
    return IS_NIGHT ? 2 : 1;
}

static s32 Eo_Roll(s32 percent) {
    percent *= (Eo_ReignLevel() + 1);
    percent = CLAMP_MAX(percent, 90);
    return Rand_ZeroFloat(100.0f) < percent;
}

/* ---------------------------------------------------------------------------------------------------------------
 * Companions
 * --------------------------------------------------------------------------------------------------------------- */

#define EO_PARAMS_COPY -1

typedef enum EoPlacement {
    /* 0 */ EO_PLACE_GROUND, // on the floor, about level with the enemy
    /* 1 */ EO_PLACE_AIR,    // at the enemy's height (plus the rule's lift), above some floor
    /* 2 */ EO_PLACE_WATER,  // on the same water surface as the enemy
} EoPlacement;

typedef struct EoCompanionRule {
    s16 triggerId;
    s16 companionId;
    s32 params; // EO_PARAMS_COPY to copy the trigger's
    u8 region;  // EO_REGION_ANY or one region
    u8 chance;  // percent, before Ganon's Reign
    u8 minCount;
    u8 maxCount;
    u8 placement;
    u8 nightOnly;
    s16 lift;
} EoCompanionRule;

static EoCompanionRule sEoCompanionRules[] = {
    // Their own kind
    { ACTOR_EN_FIREFLY, ACTOR_EN_FIREFLY, EO_PARAMS_COPY, EO_REGION_ANY, 35, 1, 2, EO_PLACE_AIR, false, 0 },
    { ACTOR_EN_SKB, ACTOR_EN_SKB, EO_PARAMS_COPY, EO_REGION_ANY, 30, 1, 2, EO_PLACE_GROUND, false, 0 },
    { ACTOR_EN_TITE, ACTOR_EN_TITE, EO_PARAMS_COPY, EO_REGION_ANY, 30, 1, 1, EO_PLACE_GROUND, false, 0 },
    { ACTOR_EN_OKUTA, ACTOR_EN_OKUTA, EO_PARAMS_COPY, EO_REGION_ANY, 25, 1, 1, EO_PLACE_WATER, false, 0 },
    { ACTOR_EN_REEBA, ACTOR_EN_REEBA, EO_PARAMS_COPY, EO_REGION_ANY, 30, 1, 2, EO_PLACE_GROUND, false, 0 },
    { ACTOR_EN_DEKUBABA, ACTOR_EN_DEKUBABA, EO_PARAMS_COPY, EO_REGION_ANY, 25, 1, 1, EO_PLACE_GROUND, false, 0 },
    { ACTOR_EN_KAREBABA, ACTOR_EN_KAREBABA, EO_PARAMS_COPY, EO_REGION_ANY, 25, 1, 1, EO_PLACE_GROUND, false, 0 },
    { ACTOR_EN_CROW, ACTOR_EN_CROW, EO_PARAMS_COPY, EO_REGION_ANY, 40, 1, 2, EO_PLACE_AIR, false, 0 },
    { ACTOR_EN_POH, ACTOR_EN_POH, EO_PARAMS_COPY, EO_REGION_ANY, 25, 1, 1, EO_PLACE_AIR, false, 0 },
    { ACTOR_EN_FZ, ACTOR_EN_FZ, EO_PARAMS_COPY, EO_REGION_ANY, 20, 1, 1, EO_PLACE_GROUND, false, 0 },
    { ACTOR_EN_SB, ACTOR_EN_SB, EO_PARAMS_COPY, EO_REGION_ANY, 25, 1, 1, EO_PLACE_GROUND, false, 0 },
    { ACTOR_EN_DODOJR, ACTOR_EN_DODOJR, EO_PARAMS_COPY, EO_REGION_ANY, 30, 1, 2, EO_PLACE_GROUND, false, 0 },
    { ACTOR_EN_NY, ACTOR_EN_NY, EO_PARAMS_COPY, EO_REGION_ANY, 25, 1, 1, EO_PLACE_GROUND, false, 0 },
    { ACTOR_EN_BILI, ACTOR_EN_BILI, EO_PARAMS_COPY, EO_REGION_ANY, 30, 1, 2, EO_PLACE_AIR, false, 0 },
    // Creatures that belong where they are found
    { ACTOR_EN_SKB, ACTOR_EN_FIREFLY, KEESE_NORMAL_FLY, EO_REGION_FIELD, 20, 1, 2, EO_PLACE_AIR, true, 60 },
    { ACTOR_EN_DEKUBABA, ACTOR_EN_KAREBABA, 0, EO_REGION_FOREST, 20, 1, 1, EO_PLACE_GROUND, false, 0 },
    { ACTOR_EN_TITE, ACTOR_EN_FIREFLY, KEESE_FIRE_FLY, EO_REGION_MOUNTAIN, 20, 1, 1, EO_PLACE_AIR, false, 80 },
    { ACTOR_EN_DODOJR, ACTOR_EN_FIREFLY, KEESE_FIRE_FLY, EO_REGION_MOUNTAIN, 20, 1, 1, EO_PLACE_AIR, false, 80 },
    { ACTOR_EN_FZ, ACTOR_EN_FIREFLY, KEESE_ICE_FLY, EO_REGION_WATER, 25, 1, 2, EO_PLACE_AIR, false, 80 },
    { ACTOR_EN_REEBA, ACTOR_EN_CROW, 0, EO_REGION_DESERT, 15, 1, 1, EO_PLACE_AIR, false, 120 },
    { ACTOR_EN_FIREFLY, ACTOR_EN_POH, EN_POH_NORMAL, EO_REGION_SHADOW, 15, 1, 1, EO_PLACE_AIR, true, 20 },
};

// Variants of an enemy that must not be copied (perched, giant, dummies, special-purpose ones).
static s32 EoCompanion_CanCopy(Actor* actor) {
    switch (actor->id) {
        case ACTOR_EN_FIREFLY:
            return ((actor->params & 0x7FFF) != KEESE_FIRE_PERCH) && ((actor->params & 0x7FFF) != KEESE_NORMAL_PERCH);
        case ACTOR_EN_OKUTA:
            return (actor->params & 0xFF) == 0; // an Octorok, not a projectile
        case ACTOR_EN_REEBA:
            return actor->params == LEEVER_SMALL;
        case ACTOR_EN_POH:
            return actor->params == EN_POH_NORMAL; // not the composer brothers or the rupee Poe
        case ACTOR_EN_FZ:
            return actor->params >= 0;
        case ACTOR_EN_NY:
            return actor->params == 0; // not a dummy
        case ACTOR_EN_BILI:
            return actor->params == EN_BILI_TYPE_NORMAL; // not one split off a Bari
        case ACTOR_EN_KAREBABA:
            return actor->params == 0;
        default:
            return true;
    }
}

/**
 * True when defeating every enemy in `room` opens a door, reveals a chest or does something else.
 */
static s32 EoRoom_HasClearReward(PlayState* play, s8 room) {
    s32 category;
    Actor* actor;

    if (Flags_GetClear(play, room)) {
        return false;
    }

    for (category = 0; category < ACTORCAT_MAX; category++) {
        for (actor = play->actorCtx.actorLists[category].head; actor != NULL; actor = actor->next) {
            switch (actor->id) {
                case ACTOR_DOOR_SHUTTER: {
                    s32 type = (actor->params >> 6) & 0xF;

                    if ((type == 1) || (type == 7)) { // SHUTTER_FRONT_CLEAR, SHUTTER_FRONT_SWITCH_BACK_CLEAR
                        u16 index = (u16)actor->params >> 10;

                        if ((index >= play->transiActorCtx.numActors) ||
                            (play->transiActorCtx.list[index].sides[0].room == room) ||
                            (play->transiActorCtx.list[index].sides[1].room == room)) {
                            return true;
                        }
                    }
                    break;
                }

                case ACTOR_EN_BOX: {
                    s32 type = (actor->params >> 12) & 0xF;

                    if ((actor->room == room) && ((type == 1) || (type == 7))) { // ENBOX_TYPE_ROOM_CLEAR_*
                        return true;
                    }
                    break;
                }

                case ACTOR_BG_MORI_BIGST:
                case ACTOR_BG_ICE_SHUTTER:
                case ACTOR_BG_HIDAN_CURTAIN:
                case ACTOR_BG_HIDAN_SYOKU:
                case ACTOR_BG_BDAN_OBJECTS:
                case ACTOR_EN_BLKOBJ:
                    if ((actor->room == room) || (actor->room < 0)) {
                        return true;
                    }
                    break;

                default:
                    break;
            }
        }
    }
    return false;
}

/**
 * Finds a spot 90-170 units from `anchor` with nothing in between. Returns false when every try is blocked.
 */
static s32 EoCompanion_FindSpot(PlayState* play, Actor* anchor, u8 placement, s16 lift, Vec3f* out) {
    s32 try;

    for (try = 0; try < 6; try++) {
        s16 yaw = (s16)Rand_Next();
        f32 dist = 90.0f + Rand_ZeroFloat(80.0f);
        Vec3f from = anchor->world.pos;
        Vec3f spot = anchor->world.pos;
        Vec3f hitPos;
        CollisionPoly* poly;
        s32 bgId;
        f32 floorY;
        f32 surfaceY;
        WaterBox* waterBox;

        spot.x += Math_SinS(yaw) * dist;
        spot.z += Math_CosS(yaw) * dist;
        from.y += 25.0f;
        spot.y += 25.0f;

        if (BgCheck_EntityLineTest1(&play->colCtx, &from, &spot, &hitPos, &poly, true, false, false, true, &bgId)) {
            continue;
        }
        spot.y -= 25.0f;

        if (placement == EO_PLACE_WATER) {
            if (!WaterBox_GetSurface1(play, &play->colCtx, spot.x, spot.z, &surfaceY, &waterBox) ||
                (fabsf(surfaceY - anchor->world.pos.y) > 40.0f)) {
                continue;
            }
            spot.y = anchor->world.pos.y;
        } else if (placement == EO_PLACE_AIR) {
            if (!Eo_FindFloor(play, &spot, 900.0f, &floorY)) {
                continue;
            }
            spot.y += lift;
        } else {
            if (!Eo_FindFloor(play, &spot, 120.0f, &floorY) || (fabsf(floorY - anchor->world.pos.y) > 60.0f)) {
                continue;
            }
            // Land enemies stay on land
            if (WaterBox_GetSurface1(play, &play->colCtx, spot.x, spot.z, &surfaceY, &waterBox) &&
                (surfaceY > floorY + 10.0f)) {
                continue;
            }
            spot.y = floorY;
        }

        *out = spot;
        return true;
    }
    return false;
}

static void EoCompanion_Roll(PlayState* play, Actor* actor, EoSceneInfo* scene) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sEoCompanionRules); i++) {
        EoCompanionRule* rule = &sEoCompanionRules[i];
        s32 count;
        s32 n;
        s32 spawned = false;

        if ((rule->triggerId != actor->id) || ((rule->region != EO_REGION_ANY) && (rule->region != scene->region))) {
            continue;
        }
        if (rule->nightOnly && !IS_NIGHT) {
            continue;
        }
        if (!EoCompanion_CanCopy(actor)) {
            continue;
        }
        // Creatures of other kinds only come along outside dungeons
        if ((rule->companionId != actor->id) && scene->isDungeon) {
            continue;
        }
        if (!Eo_Roll(rule->chance)) {
            continue;
        }

        count = rule->minCount + (s32)Rand_ZeroFloat(rule->maxCount - rule->minCount + 1);
        count = CLAMP_MAX(count, rule->maxCount);
        if (Eo_ReignLevel() == 2) {
            count++;
        }

        for (n = 0; n < count; n++) {
            Vec3f spot;

            if (EoCompanion_FindSpot(play, actor, rule->placement, rule->lift, &spot)) {
                s16 params = (rule->params == EO_PARAMS_COPY) ? actor->params : (s16)rule->params;

                if (Eo_SpawnExtra(play, actor, rule->companionId, &spot, actor->shape.rot.y, params) != NULL) {
                    spawned = true;
                }
            }
        }

        if (spawned) {
            return; // one group per enemy
        }
    }
}

/* ---------------------------------------------------------------------------------------------------------------
 * Tag-alongs for random-encounter spawners
 * --------------------------------------------------------------------------------------------------------------- */

#define EO_TAG_QUEUE_MAX 6
#define EO_TAG_ALIVE_MAX 3

typedef struct EoTagAlong {
    Actor* trigger;
    s16 delay;
    s16 actorId;
    s16 params;
    u8 placement;
    s16 lift;
} EoTagAlong;

static EoTagAlong sEoTagQueue[EO_TAG_QUEUE_MAX];
static Actor* sEoTagAlive[EO_TAG_ALIVE_MAX];

static s32 EoTag_AliveCount(void) {
    s32 i;
    s32 count = 0;

    for (i = 0; i < EO_TAG_ALIVE_MAX; i++) {
        if ((sEoTagAlive[i] != NULL) && (sEoTagAlive[i]->update != NULL)) {
            count++;
        }
    }
    return count;
}

static void EoTag_Queue(Actor* trigger, s16 delay, s16 actorId, s16 params, u8 placement, s16 lift) {
    s32 i;

    for (i = 0; i < EO_TAG_QUEUE_MAX; i++) {
        if (sEoTagQueue[i].trigger == NULL) {
            sEoTagQueue[i].trigger = trigger;
            sEoTagQueue[i].delay = delay;
            sEoTagQueue[i].actorId = actorId;
            sEoTagQueue[i].params = params;
            sEoTagQueue[i].placement = placement;
            sEoTagQueue[i].lift = lift;
            return;
        }
    }
}

static void EoTag_Roll(PlayState* play, Actor* actor) {
    Actor* spawner = actor->parent;

    if ((spawner == NULL) || (spawner->id != ACTOR_EN_ENCOUNT1) || (EoTag_AliveCount() >= EO_TAG_ALIVE_MAX)) {
        return;
    }

    switch (actor->id) {
        case ACTOR_EN_SKB:
            // Hyrule Field at night: a second Stalchild claws its way up, or a Keese swoops in
            if (Eo_Roll(25)) {
                if (Rand_ZeroOne() < 0.6f) {
                    EoTag_Queue(actor, 40, ACTOR_EN_SKB, actor->params, EO_PLACE_GROUND, 0);
                } else {
                    EoTag_Queue(actor, 40, ACTOR_EN_FIREFLY, KEESE_NORMAL_FLY, EO_PLACE_AIR, 70);
                }
            }
            break;

        case ACTOR_EN_REEBA:
            if ((actor->params == LEEVER_SMALL) && Eo_Roll(20)) {
                EoTag_Queue(actor, 30, ACTOR_EN_REEBA, LEEVER_SMALL, EO_PLACE_GROUND, 0);
            }
            break;

        case ACTOR_EN_TITE:
            if (Eo_Roll(25)) {
                EoTag_Queue(actor, 30, ACTOR_EN_TITE, actor->params, EO_PLACE_GROUND, 0);
            }
            break;

        case ACTOR_EN_WF:
            // Wolfos hunt in pairs
            if (Eo_Roll(25)) {
                EoTag_Queue(actor, 60, ACTOR_EN_WF, actor->params, EO_PLACE_GROUND, 0);
            }
            break;

        default:
            break;
    }
}

static void EoTag_Update(PlayState* play, Actor* trigger) {
    s32 i;

    for (i = 0; i < EO_TAG_QUEUE_MAX; i++) {
        EoTagAlong* tag = &sEoTagQueue[i];
        Vec3f spot;
        s32 slot;
        Actor* spawned;

        if (tag->trigger != trigger) {
            continue;
        }
        if (tag->delay > 0) {
            tag->delay--;
            continue;
        }
        if (!Eo_CanAttack(play)) {
            continue;
        }

        tag->trigger = NULL;

        for (slot = 0; slot < EO_TAG_ALIVE_MAX; slot++) {
            if ((sEoTagAlive[slot] == NULL) || (sEoTagAlive[slot]->update == NULL)) {
                break;
            }
        }
        if ((slot == EO_TAG_ALIVE_MAX) || !EoCompanion_FindSpot(play, trigger, tag->placement, tag->lift, &spot)) {
            continue;
        }

        spawned = Eo_SpawnExtra(play, NULL, tag->actorId, &spot,
                                Math_Vec3f_Yaw(&spot, &GET_PLAYER(play)->actor.world.pos), tag->params);
        if (spawned != NULL) {
            sEoTagAlive[slot] = spawned;
        }
    }
}

/* ---------------------------------------------------------------------------------------------------------------
 * Elites
 * --------------------------------------------------------------------------------------------------------------- */

#define EO_ELITE_MAX 16
#define EO_ELITE_SCALE 1.25f

typedef struct EoElite {
    Actor* actor;
    s16 timer;
} EoElite;

static EoElite sEoElites[EO_ELITE_MAX];
static s32 sEoEliteCount = 0;

static s16 sEoEliteIds[] = {
    ACTOR_EN_FIREFLY, ACTOR_EN_SKB,  ACTOR_EN_TITE,    ACTOR_EN_OKUTA, ACTOR_EN_REEBA,  ACTOR_EN_DEKUBABA,
    ACTOR_EN_KAREBABA, ACTOR_EN_CROW, ACTOR_EN_POH,    ACTOR_EN_FZ,    ACTOR_EN_SB,     ACTOR_EN_DODOJR,
    ACTOR_EN_WF,      ACTOR_EN_RD,   ACTOR_EN_RR,      ACTOR_EN_MB,    ACTOR_EN_PEEHAT, ACTOR_EN_DODONGO,
    ACTOR_EN_BILI,    ACTOR_EN_NY,   ACTOR_EN_ANUBICE, ACTOR_EN_ST,
};

static s32 EoElite_Find(Actor* actor) {
    s32 i;

    if (sEoEliteCount == 0) {
        return -1;
    }
    for (i = 0; i < EO_ELITE_MAX; i++) {
        if (sEoElites[i].actor == actor) {
            return i;
        }
    }
    return -1;
}

s32 EoElite_Is(Actor* actor) {
    return EoElite_Find(actor) >= 0;
}

static void EoElite_Roll(PlayState* play, Actor* actor) {
    s32 i;
    s32 slot = -1;
    f32 chance = (Eo_ReignLevel() == 2) ? 10.0f : 5.0f;

    for (i = 0; i < ARRAY_COUNT(sEoEliteIds); i++) {
        if (sEoEliteIds[i] == actor->id) {
            break;
        }
    }
    if ((i == ARRAY_COUNT(sEoEliteIds)) || !EoCompanion_CanCopy(actor) || (actor->colChkInfo.health == 0) ||
        (Rand_ZeroFloat(100.0f) >= chance)) {
        return;
    }

    for (i = 0; i < EO_ELITE_MAX; i++) {
        if (sEoElites[i].actor == NULL) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        return;
    }

    sEoElites[slot].actor = actor;
    sEoElites[slot].timer = 0;
    sEoEliteCount++;
    actor->colChkInfo.health = CLAMP_MAX(actor->colChkInfo.health * 2, 250);
}

static void EoElite_Remove(s32 index) {
    sEoElites[index].actor = NULL;
    if (sEoEliteCount > 0) {
        sEoEliteCount--;
    }
}

static void EoElite_Sparkle(PlayState* play, EoElite* elite) {
    static Color_RGBA8 sPrim = { 255, 255, 180, 255 };
    static Color_RGBA8 sEnv = { 255, 190, 0, 0 };
    Actor* actor = elite->actor;
    Vec3f pos;
    Vec3f vel = { 0.0f, 0.6f, 0.0f };
    Vec3f accel = { 0.0f, 0.0f, 0.0f };

    elite->timer++;
    if ((elite->timer % 6) != 0) {
        return;
    }

    pos.x = actor->world.pos.x + Rand_CenteredFloat(40.0f);
    pos.y = actor->world.pos.y + 10.0f + Rand_ZeroFloat(40.0f);
    pos.z = actor->world.pos.z + Rand_CenteredFloat(40.0f);
    EffectSsKiraKira_SpawnDispersed(play, &pos, &vel, &accel, &sPrim, &sEnv, 1500, 16);
}

/* ---------------------------------------------------------------------------------------------------------------
 * Hooks
 * --------------------------------------------------------------------------------------------------------------- */

#define EO_PENDING_MAX 96

static Actor* sEoPending[EO_PENDING_MAX];
static s32 sEoPendingCount = 0;

static s32 Eo_IsCandidate(s16 id) {
    s32 i;

    if (id == ACTOR_EN_WF) {
        return true; // a tag-along trigger
    }
    for (i = 0; i < ARRAY_COUNT(sEoCompanionRules); i++) {
        if (sEoCompanionRules[i].triggerId == id) {
            return true;
        }
    }
    for (i = 0; i < ARRAY_COUNT(sEoEliteIds); i++) {
        if (sEoEliteIds[i] == id) {
            return true;
        }
    }
    return false;
}

void EnemyOverhaul_OnActorInit(Actor* actor) {
    s32 i;

    if ((gPlayState == NULL) || (sEoSpawningExtra > 0)) {
        return; // summons, companions and tag-alongs never bring anyone
    }
    if (!EO_ENCOUNTERS_ON() && !EO_ELITES_ON()) {
        return;
    }
    if ((actor->update == NULL) || !Eo_IsCandidate(actor->id)) {
        return;
    }

    // Decided on its first update, once the rest of the room (doors, chests) is loaded and its parent is known
    for (i = 0; i < EO_PENDING_MAX; i++) {
        if (sEoPending[i] == NULL) {
            sEoPending[i] = actor;
            sEoPendingCount++;
            return;
        }
    }
}

static void Eo_ProcessNewEnemy(PlayState* play, Actor* actor) {
    EoSceneInfo* scene = EoScene_Get(play);

    if ((scene == NULL) || (actor->update == NULL)) {
        return;
    }

    // Enemy Randomizer already reshuffles every spawn; leave its rooms alone
    if (CVarGetInteger(CVAR_ENHANCEMENT("RandomizedEnemies"), 0)) {
        return;
    }

    if (EO_ELITES_ON()) {
        EoElite_Roll(play, actor);
    }

    if (!EO_ENCOUNTERS_ON() || (scene->isDungeon && !EO_ENCOUNTERS_DUNGEONS_ON())) {
        return;
    }

    if (actor->parent != NULL) {
        // Spawned by something else: a random-encounter spawner's enemy can get a tag-along
        EoTag_Roll(play, actor);
        return;
    }

    if (EoRoom_HasClearReward(play, actor->room)) {
        return;
    }

    EoCompanion_Roll(play, actor, scene);
}

void EnemyOverhaul_OnActorUpdate(Actor* actor) {
    PlayState* play = gPlayState;
    s32 i;

    if (play == NULL) {
        return;
    }

    if (sEoPendingCount > 0) {
        for (i = 0; i < EO_PENDING_MAX; i++) {
            if (sEoPending[i] == actor) {
                sEoPending[i] = NULL;
                sEoPendingCount--;
                Eo_ProcessNewEnemy(play, actor);
                break;
            }
        }
    }

    if (sEoEliteCount > 0) {
        i = EoElite_Find(actor);
        if (i >= 0) {
            if (!EO_ELITES_ON()) {
                EoElite_Remove(i);
            } else {
                EoElite_Sparkle(play, &sEoElites[i]);
            }
        }
    }

    for (i = 0; i < EO_TAG_QUEUE_MAX; i++) {
        if (sEoTagQueue[i].trigger == actor) {
            EoTag_Update(play, actor);
            break;
        }
    }
}

void EnemyOverhaul_OnActorPreDraw(Actor* actor) {
    if ((sEoEliteCount > 0) && (EoElite_Find(actor) >= 0)) {
        Matrix_Scale(EO_ELITE_SCALE, EO_ELITE_SCALE, EO_ELITE_SCALE, MTXMODE_APPLY);
    }
}

void EnemyOverhaul_OnActorKill(Actor* actor) {
    // ITEM00_FLEXIBLE picks whatever Link is shortest of (hearts, magic, arrows, bombs...)
    static s16 sEliteDrops[] = { ITEM00_RUPEE_RED, ITEM00_RUPEE_PURPLE, ITEM00_FLEXIBLE, ITEM00_FLEXIBLE };
    s32 i = EoElite_Find(actor);

    if (i < 0) {
        return;
    }

    if ((gPlayState != NULL) && (actor->colChkInfo.health == 0)) {
        Item_DropCollectible(gPlayState, &actor->world.pos,
                             sEliteDrops[(s32)Rand_ZeroFloat(ARRAY_COUNT(sEliteDrops)) % ARRAY_COUNT(sEliteDrops)]);
    }
    EoElite_Remove(i);
}

void EnemyOverhaul_OnActorDestroy(Actor* actor) {
    s32 i;

    EoShot_ForgetOwner(actor);

    for (i = 0; i < EO_SUMMON_MAX; i++) {
        if (sEoSummons[i].actor == actor) {
            sEoSummons[i].actor = NULL;
            sEoSummons[i].owner = NULL;
        }
    }

    // Anything this actor summoned goes with it
    EoSummon_KillAll(actor, NULL);

    for (i = 0; i < EO_SUMMON_MAX; i++) {
        if ((sEoSummons[i].actor != NULL) && (sEoSummons[i].owner == NULL)) {
            sEoSummons[i].actor = NULL;
        }
    }

    if (sEoPendingCount > 0) {
        for (i = 0; i < EO_PENDING_MAX; i++) {
            if (sEoPending[i] == actor) {
                sEoPending[i] = NULL;
                sEoPendingCount--;
            }
        }
    }

    i = EoElite_Find(actor);
    if (i >= 0) {
        EoElite_Remove(i);
    }

    for (i = 0; i < EO_TAG_QUEUE_MAX; i++) {
        if (sEoTagQueue[i].trigger == actor) {
            sEoTagQueue[i].trigger = NULL;
        }
    }
    for (i = 0; i < EO_TAG_ALIVE_MAX; i++) {
        if (sEoTagAlive[i] == actor) {
            sEoTagAlive[i] = NULL;
        }
    }
}
