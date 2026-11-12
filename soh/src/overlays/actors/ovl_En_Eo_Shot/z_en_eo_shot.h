#ifndef Z_EN_EO_SHOT_H
#define Z_EN_EO_SHOT_H

#include <libultraship/libultra.h>
#include "global.h"
#include "enemy_overhaul.h"

/**
 * Miniboss & Enemy Overhaul: a projectile, ground wave, shockwave ring, thrown weapon or falling rock. See
 * enemy_overhaul.h for the functions that fire them. Registered in the ActorDB as a custom actor, like Ivan.
 */

typedef struct EnEoShot {
    /* 0x000 */ Actor actor;
    ColliderCylinder collider;
    Actor* owner; // cleared when the owner is destroyed
    u8 mode;      // EoShotMode
    u8 kind;      // EoKind (orbs)
    u8 look;      // EoLook (waves and rings)
    u8 damage;    // ground wave / ring / falling damage (0x10 is one heart)
    u8 gotHit;
    s16 timer;
    s16 life;
    s16 maxLife;
    s16 homing; // orbs: most it turns towards Link each frame, 0 for none
    s16 spin;
    s16 spinStep;
    s16 noHitTimer;
    f32 speed;
    f32 gravity;
    f32 drawScale;
    f32 radius;
    f32 radiusMax;
    f32 radiusStep;
    Vec3f target; // falling: landing spot
    Gfx* dList;   // boomerang / falling model
    f32 dListScale;
    u8 reflectable;             // orbs: Link's sword can send it back
    u8 reflected;               // on its way back to its owner
    EoShotReturnFunc onReturn;  // called when a sent-back orb reaches its owner
} EnEoShot;

#ifdef __cplusplus
extern "C" {
#endif
void EnEoShot_Init(Actor* thisx, PlayState* play);
void EnEoShot_Destroy(Actor* thisx, PlayState* play);
void EnEoShot_Update(Actor* thisx, PlayState* play);
void EnEoShot_Draw(Actor* thisx, PlayState* play);
#ifdef __cplusplus
}
#endif

#endif
