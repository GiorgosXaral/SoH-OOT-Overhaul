#ifndef ENEMY_OVERHAUL_H
#define ENEMY_OVERHAUL_H

/**
 * Miniboss & Enemy Overhaul (Enhancements > Difficulty > Enemies)
 *
 * Builds on the Boss Overhaul and follows the same rules:
 *  - nothing new is loaded: every attack is made of vanilla actors, effects, sounds and display lists (Ship of
 *    Harkinian loads assets by name, so an enemy or a model doesn't have to belong to the room it shows up in);
 *  - vanilla pauses, recovery times and weak point windows are never shortened;
 *  - anything an enemy summons dies with it, so a "defeat all enemies" door can't be kept shut;
 *  - quest rules (Poe Sisters' torches, Gerudo keys, Big Poes, the Kokiri Sword skip...) keep working.
 *
 * Every toggle is on by default and is read while the game runs.
 */

#include "z64.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include "soh/cvar_prefixes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EO_CVAR_MINIBOSS_HP CVAR_ENHANCEMENT("EnemyOverhaul.MinibossHP")
#define EO_CVAR_MINIBOSS_ATTACKS CVAR_ENHANCEMENT("EnemyOverhaul.MinibossAttacks")
#define EO_CVAR_DARK_LINK CVAR_ENHANCEMENT("EnemyOverhaul.DarkLink")
#define EO_CVAR_DEAD_HAND CVAR_ENHANCEMENT("EnemyOverhaul.DeadHand")
#define EO_CVAR_ENEMIES CVAR_ENHANCEMENT("EnemyOverhaul.Enemies")
#define EO_CVAR_ENCOUNTERS CVAR_ENHANCEMENT("EnemyOverhaul.Encounters")
#define EO_CVAR_ENCOUNTERS_DUNGEONS CVAR_ENHANCEMENT("EnemyOverhaul.EncountersInDungeons")
#define EO_CVAR_ELITES CVAR_ENHANCEMENT("EnemyOverhaul.Elites")
#define EO_CVAR_GANONS_REIGN CVAR_ENHANCEMENT("EnemyOverhaul.GanonsReign")

#define EO_MB_HP_ON() (CVarGetInteger(EO_CVAR_MINIBOSS_HP, 1) != 0)
#define EO_MB_ATTACKS_ON() (CVarGetInteger(EO_CVAR_MINIBOSS_ATTACKS, 1) != 0)
#define EO_DARK_LINK_ON() (CVarGetInteger(EO_CVAR_DARK_LINK, 1) != 0)
#define EO_DEAD_HAND_ON() (CVarGetInteger(EO_CVAR_DEAD_HAND, 1) != 0)
#define EO_ENEMIES_ON() (CVarGetInteger(EO_CVAR_ENEMIES, 1) != 0)
#define EO_ENCOUNTERS_ON() (CVarGetInteger(EO_CVAR_ENCOUNTERS, 1) != 0)
#define EO_ENCOUNTERS_DUNGEONS_ON() (CVarGetInteger(EO_CVAR_ENCOUNTERS_DUNGEONS, 1) != 0)
#define EO_ELITES_ON() (CVarGetInteger(EO_CVAR_ELITES, 1) != 0)
#define EO_GANONS_REIGN_ON() (CVarGetInteger(EO_CVAR_GANONS_REIGN, 1) != 0)

// Scales a miniboss' vanilla health constant, or a threshold compared against health: 2x.
#define EO_HP2(vanilla) (EO_MB_HP_ON() ? ((vanilla) * 2) : (vanilla))
// Dark Link and Dead Hand have their own toggles.
#define EO_DARK_LINK_HP(vanilla) (EO_DARK_LINK_ON() ? ((vanilla) * 2) : (vanilla))
#define EO_DEAD_HAND_HP(vanilla) (EO_DEAD_HAND_ON() ? ((vanilla) * 2) : (vanilla))

// True when `cur` is at or below num/den of `max`.
#define EO_AT_OR_BELOW(cur, max, num, den) (((s32)(cur) * (den)) <= ((s32)(max) * (num)))

/* ---------------------------------------------------------------------------------------------------------------
 * Shots (ovl_En_Eo_Shot): projectiles, ground waves, shockwave rings, thrown weapons and falling rocks that any enemy
 * can use. The shot is its own small actor (registered in the ActorDB like Ivan), never counted as an enemy.
 * --------------------------------------------------------------------------------------------------------------- */

typedef enum EoShotMode {
    /* 0 */ EO_SHOT_ORB,         // a glowing ball of fire, ice, lightning, darkness, light, water or mud
    /* 1 */ EO_SHOT_GROUND_WAVE, // a hitbox racing along the floor, trailing dust / flames / frost / water / darkness
    /* 2 */ EO_SHOT_RING,        // an expanding ring on the floor: hurts Link where it passes, unless he is airborne
    /* 3 */ EO_SHOT_BOOMERANG,   // a spinning display list that flies out and comes back to its owner
    /* 4 */ EO_SHOT_FALLING,     // a display list (rock...) or an orb dropping from above onto a marked spot
} EoShotMode;

typedef enum EoKind {
    /* 0 */ EO_KIND_FIRE,  // burns
    /* 1 */ EO_KIND_ICE,   // freezes
    /* 2 */ EO_KIND_SHOCK, // shocks
    /* 3 */ EO_KIND_DARK,
    /* 4 */ EO_KIND_LIGHT,
    /* 5 */ EO_KIND_WATER, // knocks back, half a heart
    /* 6 */ EO_KIND_EMBER, // a small, short-lived flame that drifts
    /* 7 */ EO_KIND_MUD,   // half a heart
    /* 8 */ EO_KIND_MAX
} EoKind;

typedef enum EoLook {
    /* 0 */ EO_LOOK_DUST,
    /* 1 */ EO_LOOK_FIRE,
    /* 2 */ EO_LOOK_ICE,
    /* 3 */ EO_LOOK_WATER,
    /* 4 */ EO_LOOK_DARK,
} EoLook;

struct EnEoShot;

// Actor id of the shot, set when the ActorDB registers it.
extern s16 gEnEoShotId;

// Number of live shots of `mode` fired by `owner`.
s32 EoShot_CountOwned(Actor* owner, s32 mode);

// An orb flying with `vel`. Returns the shot actor, or NULL.
Actor* EoShot_Orb(PlayState* play, Actor* owner, s32 kind, Vec3f* pos, Vec3f* vel);
// An orb aimed at Link's chest, turned sideways by `yawOffset`.
Actor* EoShot_OrbAimed(PlayState* play, Actor* owner, s32 kind, Vec3f* pos, f32 speed, s16 yawOffset);
// A fan of `count` orbs aimed at Link, `spread` apart.
void EoShot_OrbFan(PlayState* play, Actor* owner, s32 kind, Vec3f* pos, f32 speed, s32 count, s16 spread);
// A flat ring of `count` orbs flying outwards.
void EoShot_OrbRing(PlayState* play, Actor* owner, s32 kind, Vec3f* pos, f32 speed, s32 count, s16 baseYaw);
// An orb lobbed onto `to`, landing after `frames` frames.
Actor* EoShot_OrbLob(PlayState* play, Actor* owner, s32 kind, Vec3f* pos, Vec3f* to, f32 frames);
// Makes an orb turn towards Link by up to `step` each frame.
void EoShot_SetHoming(Actor* shot, s16 step);
// Overrides an orb's damage (0x10 is one heart) and hit effect (0 none, 1 fire, 2 ice, 3 shock, 4 knockback).
void EoShot_SetDamage(Actor* shot, u8 damage, u8 effect);

// A wave racing along the floor from `pos` in direction `yaw`.
Actor* EoShot_GroundWave(PlayState* play, Actor* owner, s32 look, Vec3f* pos, s16 yaw, f32 speed, s16 life,
                         u8 damage);
// A fan of ground waves aimed at Link.
void EoShot_GroundWaveFan(PlayState* play, Actor* owner, s32 look, Vec3f* pos, s32 count, s16 spread, f32 speed,
                          s16 life, u8 damage);
// An expanding ring on the floor around `pos`. Link clears it with any jump, sidehop or backflip.
Actor* EoShot_Ring(PlayState* play, Actor* owner, s32 look, Vec3f* pos, f32 radiusMax, f32 radiusStep, u8 damage);
// A spinning weapon that flies out along `yaw` and comes back to `owner`.
Actor* EoShot_Boomerang(PlayState* play, Actor* owner, Gfx* dList, f32 scale, Vec3f* pos, s16 yaw, f32 speed,
                        u8 damage);
// Something falling from `height` above `spot`, the spot marked with dust during the whole fall.
Actor* EoShot_Falling(PlayState* play, Actor* owner, Gfx* dList, f32 scale, Vec3f* spot, f32 height, u8 damage);
// The same with an orb of `kind` instead of a model.
Actor* EoShot_FallingOrb(PlayState* play, Actor* owner, s32 kind, Vec3f* spot, f32 height, u8 damage);

// Forgets `actor` as the owner of any live shot (called when it is destroyed).
void EoShot_ForgetOwner(Actor* actor);

/* ---------------------------------------------------------------------------------------------------------------
 * Summons: vanilla enemies called in by another enemy. They vanish in a puff of smoke with their summoner.
 * --------------------------------------------------------------------------------------------------------------- */

Actor* EoSummon_Spawn(PlayState* play, Actor* owner, s16 actorId, Vec3f* pos, s16 rotY, s16 params);
s32 EoSummon_Count(Actor* owner);
void EoSummon_KillAll(Actor* owner, PlayState* play);
// True for an actor spawned by EoSummon_Spawn that is still alive.
s32 EoSummon_IsSummon(Actor* actor);

/* ---------------------------------------------------------------------------------------------------------------
 * Hooks (called from EnemyOverhaul.cpp and Actor_Draw)
 * --------------------------------------------------------------------------------------------------------------- */

void EnemyOverhaul_OnActorInit(Actor* actor);
void EnemyOverhaul_OnActorUpdate(Actor* actor);
void EnemyOverhaul_OnActorKill(Actor* actor);
void EnemyOverhaul_OnActorDestroy(Actor* actor);
// Called by Actor_Draw right after the actor's model matrix is set up.
void EnemyOverhaul_OnActorPreDraw(Actor* actor);
// True while the actor was rolled as an elite.
s32 EoElite_Is(Actor* actor);

/* ---------------------------------------------------------------------------------------------------------------
 * Small helpers
 * --------------------------------------------------------------------------------------------------------------- */

// Hurts and knocks Link back unless he is invincible, in a cutscene or not on the ground (when `groundOnly`).
s32 Eo_HurtPlayer(PlayState* play, Actor* source, u8 damage, s16 yaw, f32 knockback, s32 groundOnly, s32 shock);
// True while Link is standing on the ground (not jumping, sidehopping, backflipping, climbing or riding).
s32 Eo_PlayerOnGround(PlayState* play);
// The floor under (or a little above) `pos`, if it is within `maxDrop` below it.
s32 Eo_FindFloor(PlayState* play, Vec3f* pos, f32 maxDrop, f32* outY);
// A point `dist` behind Link on the floor, with no wall in between. False (and Link's position) when there is none.
s32 Eo_PosBehindPlayer(PlayState* play, f32 dist, Vec3f* outPos);
// A point `dist` from `from` towards `yaw` on the floor with no wall in between.
s32 Eo_PosAround(PlayState* play, Vec3f* from, s16 yaw, f32 dist, Vec3f* outPos);
// A puff of smoke, for summons that come and go.
void Eo_Puff(PlayState* play, Vec3f* pos, f32 scale);
// A puff of dark mist, for teleports.
void Eo_DarkPuff(PlayState* play, Vec3f* pos, f32 scale);
// True when no cutscene is running and Link can be attacked.
s32 Eo_CanAttack(PlayState* play);
// Link's chest.
void Eo_PlayerAimPoint(PlayState* play, Vec3f* out);

#ifdef __cplusplus
}
#endif

#endif
