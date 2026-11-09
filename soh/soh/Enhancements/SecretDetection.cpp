// SoH-OOT-Overhaul: Secret Detection
//
// Shows clues near undiscovered secrets using sparkle and dust effects already in the game; no new assets.
// Every secret here is its own actor, and a destroyed wall, opened grotto or opened chest is either never
// spawned again or changes state, so a clue only shows while its secret is still waiting to be found.
//
// The clue's colour says what opens it:
//   gold + dust  - explosives (bombable walls and floors, breakable boulders, bomb/hammer grottos)
//   blue         - Song of Storms grottos
//   purple       - the Lens of Truth (fake walls, invisible paths and platforms, invisible chests)
//   white        - invisible collectables

#include <libultraship/bridge/consolevariablebridge.h>

#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"

extern "C" {
#include "z64.h"
#include "macros.h"
#include "variables.h"
#include "functions.h"
extern PlayState* gPlayState;
}

#define SD_CVAR_ENABLED CVAR_ENHANCEMENT("SecretDetection.Enabled")
#define SD_CVAR_EXPLOSIVE CVAR_ENHANCEMENT("SecretDetection.Explosive")
#define SD_CVAR_SONG CVAR_ENHANCEMENT("SecretDetection.Song")
#define SD_CVAR_LENS CVAR_ENHANCEMENT("SecretDetection.Lens")
#define SD_CVAR_ITEMS CVAR_ENHANCEMENT("SecretDetection.Items")
#define SD_CVAR_STRENGTH CVAR_ENHANCEMENT("SecretDetection.Strength") // 0 = Subtle, 1 = Obvious
#define SD_CVAR_NEED_AGONY CVAR_ENHANCEMENT("SecretDetection.RequireStoneOfAgony")

typedef enum {
    SECRET_EXPLOSIVE,
    SECRET_SONG,
    SECRET_LENS,
    SECRET_ITEM,
} SecretKind;

static bool SecretDetection_On() {
    return CVarGetInteger(SD_CVAR_ENABLED, 1) != 0;
}

static bool SecretDetection_KindOn(const char* cvar) {
    return SecretDetection_On() && CVarGetInteger(cvar, 1) != 0;
}

// Spawns one pulse of clues at a secret, if Link is close enough and it's this secret's turn.
static void SecretDetection_Pulse(Actor* actor, SecretKind kind) {
    PlayState* play = gPlayState;
    const bool obvious = CVarGetInteger(SD_CVAR_STRENGTH, 1) != 0;
    const f32 range = obvious ? 500.0f : 250.0f;
    const u32 period = obvious ? 20 : 40;
    const s32 sparkles = obvious ? 3 : 2;

    if (play == NULL) {
        return;
    }
    if (CVarGetInteger(SD_CVAR_NEED_AGONY, 0) && !CHECK_QUEST_ITEM(QUEST_STONE_OF_AGONY)) {
        return;
    }
    if (actor->xyzDistToPlayerSq > range * range) {
        return;
    }
    // Spread secrets across frames so several nearby ones don't all pulse together
    if (((play->gameplayFrames + (u32)((uintptr_t)actor >> 4)) % period) != 0) {
        return;
    }

    // Sparkle colours per kind, in SecretKind order: gold, blue, purple, white
    static const Color_RGBA8 kPrim[] = {
        { 255, 230, 120, 255 }, { 170, 220, 255, 255 }, { 230, 170, 255, 255 }, { 255, 255, 255, 255 }
    };
    static const Color_RGBA8 kEnv[] = { { 255, 140, 0, 0 }, { 40, 90, 255, 0 }, { 140, 0, 200, 0 }, { 200, 200, 200, 0 } };
    Color_RGBA8 prim = kPrim[kind];
    Color_RGBA8 env = kEnv[kind];

    Vec3f vel = { 0.0f, 0.4f, 0.0f };
    Vec3f accel = { 0.0f, 0.02f, 0.0f };

    for (s32 i = 0; i < sparkles; i++) {
        Vec3f pos = actor->world.pos;
        pos.x += Rand_CenteredFloat(70.0f);
        pos.z += Rand_CenteredFloat(70.0f);
        pos.y += 10.0f + Rand_ZeroOne() * 60.0f;
        EffectSsKiraKira_SpawnDispersed(play, &pos, &vel, &accel, &prim, &env, 1200, 24);
    }

    // Explosive secrets also puff a little dust, like a crack about to give way
    if ((kind == SECRET_EXPLOSIVE) && obvious) {
        Color_RGBA8 dust = { 170, 150, 120, 255 };
        Vec3f dustVel = { Rand_CenteredFloat(1.0f), 0.8f, Rand_CenteredFloat(1.0f) };
        Vec3f dustAccel = { 0.0f, 0.05f, 0.0f };
        Vec3f pos = actor->world.pos;

        pos.x += Rand_CenteredFloat(50.0f);
        pos.z += Rand_CenteredFloat(50.0f);
        pos.y += 5.0f + Rand_ZeroOne() * 30.0f;
        func_8002829C(play, &pos, &dustVel, &dustAccel, &dust, &dust, 50, 5);
    }
}

static void SecretDetection_OnExplosive(void* actor) {
    SecretDetection_Pulse((Actor*)actor, SECRET_EXPLOSIVE);
}

// Lens secrets only need a clue while the Lens of Truth isn't already showing them
static void SecretDetection_OnLens(void* actor) {
    if ((gPlayState != NULL) && !gPlayState->actorCtx.lensActive) {
        SecretDetection_Pulse((Actor*)actor, SECRET_LENS);
    }
}

static void SecretDetection_OnItem(void* actor) {
    SecretDetection_Pulse((Actor*)actor, SECRET_ITEM);
}

// Hidden grottos keep bits 0x300 of their params until opened: 0x200 = bomb/hammer, 0x100 = Song of Storms.
static void SecretDetection_OnGrotto(void* refActor) {
    Actor* actor = (Actor*)refActor;

    if ((actor->params & 0x200) != 0) {
        if (CVarGetInteger(SD_CVAR_EXPLOSIVE, 1)) {
            SecretDetection_Pulse(actor, SECRET_EXPLOSIVE);
        }
    } else if ((actor->params & 0x100) != 0) {
        if (CVarGetInteger(SD_CVAR_SONG, 1)) {
            SecretDetection_Pulse(actor, SECRET_SONG);
        }
    }
}

// An unopened invisible chest carries ACTOR_FLAG_REACT_TO_LENS; it's cleared as the chest opens.
static void SecretDetection_OnChest(void* refActor) {
    Actor* actor = (Actor*)refActor;

    if ((actor->flags & ACTOR_FLAG_REACT_TO_LENS) && (gPlayState != NULL) && !gPlayState->actorCtx.lensActive) {
        SecretDetection_Pulse(actor, SECRET_LENS);
    }
}

// Shadow Temple statues: only the bombable skull wall (1) and the bombable rubble (2) are secrets.
// (Init has already reduced params to the type by the time the actor updates.)
static void SecretDetection_OnHakaZou(void* refActor) {
    Actor* actor = (Actor*)refActor;

    if ((actor->params == 1) || (actor->params == 2)) {
        SecretDetection_Pulse(actor, SECRET_EXPLOSIVE);
    }
}

// Shadow Trial: only the invisible path (type 0) is a secret; it carries ACTOR_FLAG_REACT_TO_LENS from Init.
static void SecretDetection_OnDarkmeiro(void* refActor) {
    Actor* actor = (Actor*)refActor;

    if (((actor->params & 0xFF) == 0) && (gPlayState != NULL) && !gPlayState->actorCtx.lensActive) {
        SecretDetection_Pulse(actor, SECRET_LENS);
    }
}

static void RegisterSecretDetection() {
    const bool explosive = SecretDetection_KindOn(SD_CVAR_EXPLOSIVE);
    const bool lens = SecretDetection_KindOn(SD_CVAR_LENS);
    const bool items = SecretDetection_KindOn(SD_CVAR_ITEMS);
    const bool grottos = SecretDetection_On() && (CVarGetInteger(SD_CVAR_EXPLOSIVE, 1) || CVarGetInteger(SD_CVAR_SONG, 1));

    // Bombable walls and floors, and breakable boulders
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_BREAKWALL, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_BOMBWALL, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_HIDAN_KOWARERUKABE, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_MIZU_BWALL, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_SPOT08_BAKUDANKABE, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_SPOT11_BAKUDANKABE, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_SPOT17_BAKUDANKABE, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_JYA_BOMBIWA, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_JYA_BOMBCHUIWA, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_SPOT16_BOMBSTONE, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_OBJ_BOMBIWA, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_OBJ_HAMISHI, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_HAKA_ZOU, explosive, SecretDetection_OnHakaZou);

    // Hidden grottos (explosive or Song of Storms, decided per grotto)
    COND_ID_HOOK(OnActorUpdate, ACTOR_DOOR_ANA, grottos, SecretDetection_OnGrotto);

    // Fake walls, invisible paths and platforms, invisible chests
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_HAKA_MEGANE, lens, SecretDetection_OnLens);
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_HAKA_MEGANEBG, lens, SecretDetection_OnLens);
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_GND_NISEKABE, lens, SecretDetection_OnLens);
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_MENKURI_NISEKABE, lens, SecretDetection_OnLens);
    COND_ID_HOOK(OnActorUpdate, ACTOR_EN_BOX, lens, SecretDetection_OnChest);
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_GND_DARKMEIRO, lens, SecretDetection_OnDarkmeiro);

    // Invisible collectables
    COND_ID_HOOK(OnActorUpdate, ACTOR_EN_WONDER_ITEM, items, SecretDetection_OnItem);
}

static RegisterShipInitFunc initFunc(RegisterSecretDetection, { SD_CVAR_ENABLED, SD_CVAR_EXPLOSIVE, SD_CVAR_SONG,
                                                                SD_CVAR_LENS, SD_CVAR_ITEMS });
