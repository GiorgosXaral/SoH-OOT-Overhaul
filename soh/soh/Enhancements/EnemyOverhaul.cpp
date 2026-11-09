#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include "soh/ActorDB.h"

extern "C" {
#include "functions.h"
#include "enemy_overhaul.h"
#include "src/overlays/actors/ovl_En_Eo_Shot/z_en_eo_shot.h"
}

// Adds the overhaul's shot actor (projectiles, ground waves, shockwave rings, thrown weapons, falling rocks) to the
// actor list the first time one is needed, the same way Ivan is added.
extern "C" void EnEoShot_Register(void) {
    if (gEnEoShotId >= 0) {
        return;
    }
    ActorDBInit entry = {
        "En_Eo_Shot",
        "Enemy Overhaul Shot",
        -1,
        ACTORCAT_MISC,
        (ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED),
        OBJECT_GAMEPLAY_KEEP,
        sizeof(EnEoShot),
        (ActorFunc)EnEoShot_Init,
        (ActorFunc)EnEoShot_Destroy,
        (ActorFunc)EnEoShot_Update,
        (ActorFunc)EnEoShot_Draw,
        nullptr,
    };
    gEnEoShotId = ActorDB::Instance->AddEntry(entry).entry.id;
}

// Miniboss & Enemy Overhaul: hooks. The mechanics themselves live in the actors, z_enemy_overhaul.c and
// ovl_En_Eo_Shot; this only routes the game's actor events to them.
static void RegisterEnemyOverhaul() {
    const bool encountersOrElites = EO_ENCOUNTERS_ON() || EO_ELITES_ON();

    // Shots and summons are always kept tidy, whatever the toggles, so nothing is left pointing at a freed actor
    COND_HOOK(OnActorDestroy, true, [](void* actor) { EnemyOverhaul_OnActorDestroy((Actor*)actor); });

    // Companions, tag-alongs, elites and Ganon's Reign
    COND_HOOK(OnActorInit, encountersOrElites, [](void* actor) { EnemyOverhaul_OnActorInit((Actor*)actor); });
    COND_HOOK(OnActorUpdate, encountersOrElites, [](void* actor) { EnemyOverhaul_OnActorUpdate((Actor*)actor); });
    COND_HOOK(OnActorKill, EO_ELITES_ON(), [](void* actor) { EnemyOverhaul_OnActorKill((Actor*)actor); });
}

static RegisterShipInitFunc initFunc(RegisterEnemyOverhaul,
                                     { EO_CVAR_MINIBOSS_HP, EO_CVAR_MINIBOSS_ATTACKS, EO_CVAR_DARK_LINK,
                                       EO_CVAR_DEAD_HAND, EO_CVAR_ENEMIES, EO_CVAR_ENCOUNTERS,
                                       EO_CVAR_ENCOUNTERS_DUNGEONS, EO_CVAR_ELITES, EO_CVAR_GANONS_REIGN });
