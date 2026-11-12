/*
 * File: z_en_eo_shot.c
 * Overlay: ovl_En_Eo_Shot (custom actor, registered in the ActorDB)
 * Description: Miniboss & Enemy Overhaul projectiles: orbs, ground waves, shockwave rings, thrown weapons and
 *              falling rocks. Everything is drawn with gameplay_keep's glow and the owners' own models.
 */

#include "z_en_eo_shot.h"
#include "objects/gameplay_keep/gameplay_keep.h"

#define FLAGS (ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED)

s16 gEnEoShotId = -1;

#define EO_SHOT_MAX 96

static EnEoShot* sEoShots[EO_SHOT_MAX];

typedef struct EoKindInfo {
    u8 effect;  // 0 none, 1 fire, 2 ice, 3 shock, 4 knockback
    u8 damage;  // 0x10 is one heart
    s16 radius; // hitbox
    s16 life;
    f32 drawScale;
    Color_RGB8 prim;
    Color_RGB8 env;
} EoKindInfo;

static EoKindInfo sEoKindInfo[EO_KIND_MAX] = {
    /* EO_KIND_FIRE  */ { 1, 0x10, 20, 110, 0.045f, { 255, 230, 140 }, { 255, 70, 0 } },
    /* EO_KIND_ICE   */ { 2, 0x10, 20, 110, 0.045f, { 235, 250, 255 }, { 70, 160, 255 } },
    /* EO_KIND_SHOCK */ { 3, 0x10, 20, 110, 0.045f, { 255, 255, 210 }, { 120, 150, 255 } },
    /* EO_KIND_DARK  */ { 0, 0x10, 20, 120, 0.050f, { 230, 170, 255 }, { 110, 0, 170 } },
    /* EO_KIND_LIGHT */ { 0, 0x10, 22, 120, 0.055f, { 255, 255, 235 }, { 255, 220, 90 } },
    /* EO_KIND_WATER */ { 4, 0x08, 20, 90, 0.045f, { 220, 245, 255 }, { 40, 120, 255 } },
    /* EO_KIND_EMBER */ { 1, 0x08, 16, 28, 0.030f, { 255, 210, 110 }, { 255, 40, 0 } },
    /* EO_KIND_MUD   */ { 0, 0x08, 18, 90, 0.040f, { 205, 170, 120 }, { 90, 60, 20 } },
};

static ColliderCylinderInit sCylinderInit = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_ENEMY,
        AC_NONE,
        OC1_NONE,
        OC2_TYPE_1,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK0,
        { 0xFFCFFFFF, 0x00, 0x10 },
        { 0x00000000, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_NONE,
        OCELEM_NONE,
    },
    { 20, 40, -20, { 0, 0, 0 } },
};

/* ---------------------------------------------------------------------------------------------------------------
 * Actor
 * --------------------------------------------------------------------------------------------------------------- */

void EnEoShot_Init(Actor* thisx, PlayState* play) {
    EnEoShot* this = (EnEoShot*)thisx;
    s32 i;

    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, &this->actor, &sCylinderInit);
    this->actor.flags &= ~(ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE);
    this->actor.room = -1; // follows Link through room changes instead of being unloaded mid-flight
    this->actor.gravity = 0.0f;
    this->actor.shape.shadowDraw = NULL;
    this->dListScale = 1.0f;
    this->drawScale = 0.045f;
    this->maxLife = this->life = 110;
    this->spin = (s16)Rand_Next();
    Actor_SetScale(&this->actor, 0.01f);

    for (i = 0; i < EO_SHOT_MAX; i++) {
        if (sEoShots[i] == NULL) {
            sEoShots[i] = this;
            return;
        }
    }
    // The table is full: this one simply never gets an owner cleared, so it gives up its owner right away
    this->owner = NULL;
}

void EnEoShot_Destroy(Actor* thisx, PlayState* play) {
    EnEoShot* this = (EnEoShot*)thisx;
    s32 i;

    Collider_DestroyCylinder(play, &this->collider);
    for (i = 0; i < EO_SHOT_MAX; i++) {
        if (sEoShots[i] == this) {
            sEoShots[i] = NULL;
        }
    }
}

static s32 EoShot_IsRegistered(EnEoShot* shot) {
    s32 i;

    for (i = 0; i < EO_SHOT_MAX; i++) {
        if (sEoShots[i] == shot) {
            return true;
        }
    }
    return false;
}

void EoShot_ForgetOwner(Actor* actor) {
    s32 i;

    for (i = 0; i < EO_SHOT_MAX; i++) {
        if ((sEoShots[i] != NULL) && (sEoShots[i]->owner == actor)) {
            sEoShots[i]->owner = NULL;
        }
    }
}

s32 EoShot_CountOwned(Actor* owner, s32 mode) {
    s32 i;
    s32 count = 0;

    for (i = 0; i < EO_SHOT_MAX; i++) {
        if ((sEoShots[i] != NULL) && (sEoShots[i]->owner == owner) && (sEoShots[i]->mode == mode) &&
            (sEoShots[i]->actor.update != NULL)) {
            count++;
        }
    }
    return count;
}

static EnEoShot* EoShot_Spawn(PlayState* play, Actor* owner, s32 mode, s32 kind, Vec3f* pos) {
    EnEoShot* shot;
    EoKindInfo* info;

    if ((play == NULL) || (pos == NULL)) {
        return NULL;
    }
    EnEoShot_Register();
    if (gEnEoShotId < 0) {
        return NULL;
    }

    shot = (EnEoShot*)Actor_Spawn(&play->actorCtx, play, gEnEoShotId, pos->x, pos->y, pos->z, 0, 0, 0, mode);
    if (shot == NULL) {
        return NULL;
    }

    if ((kind < 0) || (kind >= EO_KIND_MAX)) {
        kind = EO_KIND_FIRE;
    }
    info = &sEoKindInfo[kind];

    shot->owner = EoShot_IsRegistered(shot) ? owner : NULL;
    shot->mode = mode;
    shot->kind = kind;
    shot->maxLife = shot->life = info->life;
    shot->drawScale = info->drawScale;
    shot->collider.dim.radius = info->radius;
    shot->collider.dim.height = info->radius * 2;
    shot->collider.dim.yShift = -info->radius;
    shot->collider.info.toucher.damage = info->damage;
    shot->collider.info.toucher.effect = info->effect;
    shot->actor.prevPos = *pos;
    return shot;
}

static void EoShot_SetHitbox(EnEoShot* shot, s16 radius, s16 height, s16 yShift, u8 damage, u8 effect) {
    shot->collider.dim.radius = radius;
    shot->collider.dim.height = height;
    shot->collider.dim.yShift = yShift;
    shot->collider.info.toucher.damage = damage;
    shot->collider.info.toucher.effect = effect;
}

void EoShot_SetHoming(Actor* shot, s16 step) {
    if (shot != NULL) {
        ((EnEoShot*)shot)->homing = step;
    }
}

void EoShot_SetImpactRadius(Actor* shot, f32 radius) {
    if (shot != NULL) {
        ((EnEoShot*)shot)->radius = radius;
    }
}

void EoShot_SetReflectable(Actor* shot, EoShotReturnFunc onReturn) {
    EnEoShot* this = (EnEoShot*)shot;

    if (this == NULL) {
        return;
    }
    this->reflectable = true;
    this->onReturn = onReturn;
    // Sword attacks (and the shield, which only stops it) can now touch it, like Ganondorf's light ball
    this->collider.base.acFlags = AC_ON | AC_TYPE_PLAYER;
    this->collider.info.bumperFlags = BUMP_ON;
    this->collider.info.bumper.dmgFlags = 0x0D900740 | 0x00100000;
}

void EoShot_SetDamage(Actor* shot, u8 damage, u8 effect) {
    if (shot != NULL) {
        ((EnEoShot*)shot)->collider.info.toucher.damage = damage;
        ((EnEoShot*)shot)->collider.info.toucher.effect = effect;
    }
}

/* ---------------------------------------------------------------------------------------------------------------
 * Firing
 * --------------------------------------------------------------------------------------------------------------- */

void Eo_PlayerAimPoint(PlayState* play, Vec3f* out) {
    Player* player = GET_PLAYER(play);

    *out = player->actor.world.pos;
    out->y += LINK_IS_ADULT ? 35.0f : 22.0f;
}

static void EoShot_AimVelocity(Vec3f* from, Vec3f* to, f32 speed, s16 yawOffset, Vec3f* outVel) {
    f32 dx = to->x - from->x;
    f32 dy = to->y - from->y;
    f32 dz = to->z - from->z;
    f32 dist = sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
    f32 cosOff = Math_CosS(yawOffset);
    f32 sinOff = Math_SinS(yawOffset);

    if (dist < 1.0f) {
        outVel->x = 0.0f;
        outVel->y = 0.0f;
        outVel->z = speed;
        return;
    }
    dx = (dx / dist) * speed;
    dy = (dy / dist) * speed;
    dz = (dz / dist) * speed;
    outVel->x = (dx * cosOff) + (dz * sinOff);
    outVel->y = dy;
    outVel->z = (dz * cosOff) - (dx * sinOff);
}

Actor* EoShot_Orb(PlayState* play, Actor* owner, s32 kind, Vec3f* pos, Vec3f* vel) {
    EnEoShot* shot = EoShot_Spawn(play, owner, EO_SHOT_ORB, kind, pos);

    if (shot == NULL) {
        return NULL;
    }
    shot->actor.velocity = *vel;
    shot->actor.world.rot.y = shot->actor.shape.rot.y = Math_Atan2S(vel->z, vel->x);
    return &shot->actor;
}

Actor* EoShot_OrbAimed(PlayState* play, Actor* owner, s32 kind, Vec3f* pos, f32 speed, s16 yawOffset) {
    Vec3f target;
    Vec3f vel;

    Eo_PlayerAimPoint(play, &target);
    EoShot_AimVelocity(pos, &target, speed, yawOffset, &vel);
    return EoShot_Orb(play, owner, kind, pos, &vel);
}

void EoShot_OrbFan(PlayState* play, Actor* owner, s32 kind, Vec3f* pos, f32 speed, s32 count, s16 spread) {
    s32 i;

    for (i = 0; i < count; i++) {
        EoShot_OrbAimed(play, owner, kind, pos, speed, (s16)(((i * 2) - (count - 1)) * spread / 2));
    }
}

void EoShot_OrbRing(PlayState* play, Actor* owner, s32 kind, Vec3f* pos, f32 speed, s32 count, s16 baseYaw) {
    Vec3f vel;
    s32 i;

    for (i = 0; i < count; i++) {
        s16 yaw = baseYaw + (s16)(i * (0x10000 / count));

        vel.x = Math_SinS(yaw) * speed;
        vel.y = 0.0f;
        vel.z = Math_CosS(yaw) * speed;
        EoShot_Orb(play, owner, kind, pos, &vel);
    }
}

Actor* EoShot_OrbLob(PlayState* play, Actor* owner, s32 kind, Vec3f* pos, Vec3f* to, f32 frames) {
    Vec3f vel;
    Actor* shot;
    const f32 gravity = -1.0f;

    if (frames < 1.0f) {
        frames = 1.0f;
    }
    vel.x = (to->x - pos->x) / frames;
    vel.z = (to->z - pos->z) / frames;
    // y(t) = vy * t + gravity * t * (t + 1) / 2 (velocity is applied after gravity each frame)
    vel.y = ((to->y - pos->y) - (gravity * frames * (frames + 1.0f) * 0.5f)) / frames;

    shot = EoShot_Orb(play, owner, kind, pos, &vel);
    if (shot != NULL) {
        ((EnEoShot*)shot)->gravity = gravity;
        ((EnEoShot*)shot)->maxLife = ((EnEoShot*)shot)->life = (s16)frames + 30;
    }
    return shot;
}

Actor* EoShot_GroundWave(PlayState* play, Actor* owner, s32 look, Vec3f* pos, s16 yaw, f32 speed, s16 life,
                         u8 damage) {
    EnEoShot* shot = EoShot_Spawn(play, owner, EO_SHOT_GROUND_WAVE, EO_KIND_EMBER, pos);

    if (shot == NULL) {
        return NULL;
    }
    shot->look = look;
    shot->damage = damage;
    shot->speed = speed;
    shot->timer = life;
    shot->actor.world.rot.y = yaw;
    shot->actor.velocity.x = Math_SinS(yaw) * speed;
    shot->actor.velocity.z = Math_CosS(yaw) * speed;
    shot->actor.floorHeight = pos->y;
    EoShot_SetHitbox(shot, 26, 45, 0, damage,
                     (look == EO_LOOK_FIRE) ? 1 : ((look == EO_LOOK_ICE) ? 2 : ((look == EO_LOOK_WATER) ? 4 : 0)));
    return &shot->actor;
}

void EoShot_GroundWaveFan(PlayState* play, Actor* owner, s32 look, Vec3f* pos, s32 count, s16 spread, f32 speed,
                          s16 life, u8 damage) {
    Player* player = GET_PLAYER(play);
    s16 yaw = Math_Vec3f_Yaw(pos, &player->actor.world.pos);
    s32 i;

    for (i = 0; i < count; i++) {
        EoShot_GroundWave(play, owner, look, pos, yaw + (s16)(((i * 2) - (count - 1)) * spread / 2), speed, life,
                          damage);
    }
}

Actor* EoShot_Ring(PlayState* play, Actor* owner, s32 look, Vec3f* pos, f32 radiusMax, f32 radiusStep, u8 damage) {
    EnEoShot* shot = EoShot_Spawn(play, owner, EO_SHOT_RING, EO_KIND_EMBER, pos);

    if (shot == NULL) {
        return NULL;
    }
    shot->look = look;
    shot->damage = damage;
    shot->radius = 20.0f;
    shot->radiusMax = radiusMax;
    shot->radiusStep = radiusStep;
    shot->actor.floorHeight = pos->y;
    return &shot->actor;
}

Actor* EoShot_Boomerang(PlayState* play, Actor* owner, Gfx* dList, f32 scale, Vec3f* pos, s16 yaw, f32 speed,
                        u8 damage) {
    EnEoShot* shot = EoShot_Spawn(play, owner, EO_SHOT_BOOMERANG, EO_KIND_EMBER, pos);

    if (shot == NULL) {
        return NULL;
    }
    shot->dList = dList;
    shot->dListScale = scale;
    shot->speed = speed;
    shot->damage = damage;
    shot->spinStep = 0x2400;
    shot->actor.world.rot.y = yaw;
    EoShot_SetHitbox(shot, 34, 34, -17, damage, 0);
    return &shot->actor;
}

Actor* EoShot_Falling(PlayState* play, Actor* owner, Gfx* dList, f32 scale, Vec3f* spot, f32 height, u8 damage) {
    Vec3f pos = *spot;
    EnEoShot* shot;

    pos.y += height;
    shot = EoShot_Spawn(play, owner, EO_SHOT_FALLING, EO_KIND_EMBER, &pos);
    if (shot == NULL) {
        return NULL;
    }
    shot->dList = dList;
    shot->dListScale = scale;
    shot->damage = damage;
    shot->target = *spot;
    shot->spinStep = (s16)Rand_CenteredFloat(0x1800);
    return &shot->actor;
}

Actor* EoShot_FallingOrb(PlayState* play, Actor* owner, s32 kind, Vec3f* spot, f32 height, u8 damage) {
    Vec3f pos = *spot;
    EnEoShot* shot;

    pos.y += height;
    shot = EoShot_Spawn(play, owner, EO_SHOT_FALLING, kind, &pos);
    if (shot == NULL) {
        return NULL;
    }
    shot->dList = NULL; // drawn as an orb
    shot->damage = damage;
    shot->target = *spot;
    return &shot->actor;
}

/* ---------------------------------------------------------------------------------------------------------------
 * Effects
 * --------------------------------------------------------------------------------------------------------------- */

static void EoShot_Dust(PlayState* play, Vec3f* pos, s16 scale) {
    Vec3f vel = { 0.0f, 1.0f, 0.0f };
    Vec3f accel = { 0.0f, 0.3f, 0.0f };

    func_8002865C(play, pos, &vel, &accel, scale, 15);
}

static void EoShot_LookPuff(PlayState* play, s32 look, Vec3f* pos, f32 scale) {
    static Color_RGBA8 sIcePrim = { 240, 250, 255, 255 };
    static Color_RGBA8 sIceEnv = { 150, 200, 255, 255 };
    Vec3f vel = { 0.0f, 0.0f, 0.0f };
    Vec3f accel = { 0.0f, 0.4f, 0.0f };

    switch (look) {
        case EO_LOOK_FIRE:
            EffectSsKFire_Spawn(play, pos, &vel, &accel, (s16)(18.0f * scale), 0);
            break;

        case EO_LOOK_ICE:
            vel.y = 1.0f;
            func_8002836C(play, pos, &vel, &accel, &sIcePrim, &sIceEnv, (s16)(120.0f * scale), 15, 10);
            break;

        case EO_LOOK_WATER:
            EffectSsGSplash_Spawn(play, pos, NULL, NULL, 0, (s16)(160.0f * scale));
            break;

        case EO_LOOK_DARK:
            EffectSsDeadDb_Spawn(play, pos, &vel, &accel, (s16)(70.0f * scale), 10, 200, 130, 255, 255, 60, 0, 90,
                                 1, 9, false);
            break;

        default:
            EoShot_Dust(play, pos, (s16)(120.0f * scale));
            break;
    }
}

// A trail particle behind an orb.
static void EoShot_Trail(PlayState* play, EnEoShot* this) {
    EoKindInfo* info = &sEoKindInfo[this->kind];
    Vec3f pos = this->actor.world.pos;
    Vec3f vel = { 0.0f, 0.0f, 0.0f };
    Vec3f accel = { 0.0f, 0.0f, 0.0f };
    Color_RGBA8 prim = { info->prim.r, info->prim.g, info->prim.b, 255 };
    Color_RGBA8 env = { info->env.r, info->env.g, info->env.b, 0 };

    pos.x += Rand_CenteredFloat(8.0f);
    pos.y += Rand_CenteredFloat(8.0f);
    pos.z += Rand_CenteredFloat(8.0f);

    switch (this->kind) {
        case EO_KIND_FIRE:
        case EO_KIND_EMBER:
            accel.y = 0.3f;
            EffectSsKFire_Spawn(play, &pos, &vel, &accel, (this->kind == EO_KIND_EMBER) ? 8 : 12, 0);
            break;

        case EO_KIND_ICE:
        case EO_KIND_LIGHT:
            accel.y = -0.1f;
            EffectSsKiraKira_SpawnDispersed(play, &pos, &vel, &accel, &prim, &env, 1200, 12);
            break;

        case EO_KIND_SHOCK:
            if ((this->timer % 4) == 0) {
                EffectSsGSpk_SpawnNoAccel(play, NULL, &pos, &vel, &accel, &prim, &env, 100, 0);
            }
            break;

        case EO_KIND_DARK:
            accel.y = 0.2f;
            EffectSsDeadDb_Spawn(play, &pos, &vel, &accel, 30, 6, 200, 130, 255, 255, 60, 0, 90, 1, 7, false);
            break;

        case EO_KIND_WATER:
            if ((this->timer % 3) == 0) {
                EffectSsBubble_Spawn(play, &pos, 0.0f, 5.0f, 5.0f, 0.08f);
            }
            break;

        default:
            if ((this->timer % 3) == 0) {
                EoShot_Dust(play, &pos, 60);
            }
            break;
    }
}

// The burst when an orb hits something or runs out.
static void EoShot_Burst(PlayState* play, EnEoShot* this) {
    EoKindInfo* info = &sEoKindInfo[this->kind];
    Vec3f pos = this->actor.world.pos;
    Vec3f vel = { 0.0f, 0.0f, 0.0f };
    Vec3f accel = { 0.0f, 0.0f, 0.0f };
    Color_RGBA8 prim = { info->prim.r, info->prim.g, info->prim.b, 255 };
    Color_RGBA8 env = { info->env.r, info->env.g, info->env.b, 255 };
    s32 i;

    switch (this->kind) {
        case EO_KIND_FIRE:
        case EO_KIND_EMBER:
            for (i = 0; i < 3; i++) {
                vel.x = Rand_CenteredFloat(3.0f);
                vel.y = Rand_ZeroFloat(2.0f);
                vel.z = Rand_CenteredFloat(3.0f);
                accel.y = 0.3f;
                EffectSsKFire_Spawn(play, &pos, &vel, &accel, (this->kind == EO_KIND_EMBER) ? 10 : 20, 0);
            }
            SoundSource_PlaySfxAtFixedWorldPos(play, &pos, 20, NA_SE_EV_FLAME_IGNITION);
            break;

        case EO_KIND_ICE:
            EffectSsIceSmoke_Spawn(play, &pos, &vel, &accel, 150);
            SoundSource_PlaySfxAtFixedWorldPos(play, &pos, 20, NA_SE_EV_ICE_BROKEN);
            break;

        case EO_KIND_SHOCK:
            for (i = 0; i < 4; i++) {
                vel.x = Rand_CenteredFloat(6.0f);
                vel.y = Rand_ZeroFloat(4.0f);
                vel.z = Rand_CenteredFloat(6.0f);
                EffectSsGSpk_SpawnNoAccel(play, NULL, &pos, &vel, &accel, &prim, &env, 100, 0);
            }
            SoundSource_PlaySfxAtFixedWorldPos(play, &pos, 20, NA_SE_EN_BIRI_SPARK);
            break;

        case EO_KIND_DARK:
            EffectSsDeadDb_Spawn(play, &pos, &vel, &accel, 90, 10, 200, 130, 255, 255, 60, 0, 90, 1, 10, false);
            SoundSource_PlaySfxAtFixedWorldPos(play, &pos, 20, NA_SE_EN_EXTINCT);
            break;

        case EO_KIND_LIGHT:
            EffectSsKiraKira_SpawnDispersed(play, &pos, &vel, &accel, &prim, &env, 3000, 16);
            SoundSource_PlaySfxAtFixedWorldPos(play, &pos, 20, NA_SE_EN_LIGHT_ARROW_HIT);
            break;

        case EO_KIND_WATER:
            EffectSsGSplash_Spawn(play, &pos, NULL, NULL, 0, 250);
            SoundSource_PlaySfxAtFixedWorldPos(play, &pos, 20, NA_SE_EV_BOMB_DROP_WATER);
            break;

        default:
            for (i = 0; i < 3; i++) {
                EoShot_Dust(play, &pos, 150);
            }
            SoundSource_PlaySfxAtFixedWorldPos(play, &pos, 20, NA_SE_EV_LAND_DIRT);
            break;
    }
}

/* ---------------------------------------------------------------------------------------------------------------
 * Update
 * --------------------------------------------------------------------------------------------------------------- */

static void EoShot_UpdateOrb(EnEoShot* this, PlayState* play) {
    Actor* actor = &this->actor;
    Vec3f from = actor->world.pos;
    Vec3f hitPos;
    CollisionPoly* poly;
    s32 bgId;

    // Sent back by Link's sword: it flies at its owner, and no longer hurts Link
    if (this->reflected) {
        if ((this->owner == NULL) || (this->timer > 90)) {
            this->life = 0;
        } else {
            Vec3f home = this->owner->focus.pos;

            EoShot_AimVelocity(&actor->world.pos, &home, this->speed, 0, &actor->velocity);
            if (Math_Vec3f_DistXYZ(&actor->world.pos, &home) < 55.0f) {
                if (this->onReturn != NULL) {
                    this->onReturn(this->owner, play);
                }
                this->life = 0;
            }
        }
        actor->world.pos.x += actor->velocity.x;
        actor->world.pos.y += actor->velocity.y;
        actor->world.pos.z += actor->velocity.z;
        if ((this->timer % 2) == 0) {
            EoShot_Trail(play, this);
        }
        if (this->life <= 0) {
            EoShot_Burst(play, this);
            Actor_Kill(actor);
        }
        return;
    }

    if (this->reflectable && (this->collider.base.acFlags & AC_HIT)) {
        ColliderInfo* acHitInfo = this->collider.info.acHitInfo;
        Player* player = GET_PLAYER(play);

        this->collider.base.acFlags &= ~AC_HIT;
        if ((acHitInfo != NULL) && (acHitInfo->toucher.dmgFlags & 0x00100000)) {
            // Blocked by the shield
            Audio_PlaySfxGeneral(NA_SE_IT_SHIELD_REFLECT_MG, &player->actor.projectedPos, 4,
                                 &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
            this->life = 0;
        } else {
            // Hit back with the sword
            Audio_PlaySfxGeneral(NA_SE_IT_SWORD_REFLECT_MG, &player->actor.projectedPos, 4,
                                 &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
            this->reflected = true;
            this->timer = 0;
            this->speed = MAX(sqrtf(SQ(actor->velocity.x) + SQ(actor->velocity.y) + SQ(actor->velocity.z)), 10.0f) *
                          1.3f;
            return;
        }
    }

    if ((this->homing != 0) && Eo_CanAttack(play)) {
        Vec3f target;
        f32 speedXZ = sqrtf(SQ(actor->velocity.x) + SQ(actor->velocity.z));
        s16 yaw = Math_Atan2S(actor->velocity.z, actor->velocity.x);

        Eo_PlayerAimPoint(play, &target);
        Math_ScaledStepToS(&yaw, Math_Vec3f_Yaw(&actor->world.pos, &target), this->homing);
        actor->velocity.x = Math_SinS(yaw) * speedXZ;
        actor->velocity.z = Math_CosS(yaw) * speedXZ;
        Math_StepToF(&actor->velocity.y, (target.y - actor->world.pos.y) * 0.08f, 0.5f);
    }

    actor->velocity.y += this->gravity;
    actor->world.pos.x += actor->velocity.x;
    actor->world.pos.y += actor->velocity.y;
    actor->world.pos.z += actor->velocity.z;
    this->spin += 0x1000;

    if ((this->timer % 2) == 0) {
        EoShot_Trail(play, this);
    }

    // Walls, floors and ceilings put it out
    if (BgCheck_EntityLineTest1(&play->colCtx, &from, &actor->world.pos, &hitPos, &poly, true, true, true, true,
                                &bgId)) {
        actor->world.pos = hitPos;
        this->life = 0;
    }

    Collider_UpdateCylinder(actor, &this->collider);
    if (this->collider.base.atFlags & (AT_HIT | AT_BOUNCED)) {
        this->collider.base.atFlags &= ~(AT_HIT | AT_BOUNCED);
        this->life = 0;
    }

    if (this->life <= 0) {
        EoShot_Burst(play, this);
        Actor_Kill(actor);
        return;
    }
    this->life--;
    CollisionCheck_SetAT(play, &play->colChkCtx, &this->collider.base);
    if (this->reflectable) {
        CollisionCheck_SetAC(play, &play->colChkCtx, &this->collider.base);
    }
}

static void EoShot_UpdateGroundWave(EnEoShot* this, PlayState* play) {
    Actor* actor = &this->actor;
    f32 prevFloor = actor->floorHeight;

    actor->world.pos.x += actor->velocity.x;
    actor->world.pos.z += actor->velocity.z;
    actor->world.pos.y = prevFloor + 30.0f;
    Actor_UpdateBgCheckInfo(play, actor, 20.0f, 20.0f, 40.0f, 5);
    actor->world.pos.y = actor->floorHeight;

    // A wall, a gap in the floor or a long drop puts it out
    if ((actor->bgCheckFlags & 8) || (actor->floorHeight <= BGCHECK_Y_MIN + 1.0f) ||
        (fabsf(actor->floorHeight - prevFloor) > 60.0f)) {
        this->timer = 0;
    }

    if ((this->timer % 2) == 0) {
        Vec3f puffPos = actor->world.pos;

        puffPos.x += Rand_CenteredFloat(20.0f);
        puffPos.z += Rand_CenteredFloat(20.0f);
        EoShot_LookPuff(play, this->look, &puffPos, 1.0f);
    }

    if (this->look == EO_LOOK_FIRE) {
        Actor_PlaySfx_Flagged(actor, NA_SE_EV_BURN_OUT - SFX_FLAG);
    }

    Collider_UpdateCylinder(actor, &this->collider);
    if (this->collider.base.atFlags & (AT_HIT | AT_BOUNCED)) {
        this->collider.base.atFlags &= ~(AT_HIT | AT_BOUNCED);
        this->timer = 0;
    }

    if (this->timer <= 0) {
        EoShot_LookPuff(play, this->look, &actor->world.pos, 1.6f);
        Actor_Kill(actor);
        return;
    }
    this->timer--;
    CollisionCheck_SetAT(play, &play->colChkCtx, &this->collider.base);
}

static void EoShot_UpdateRing(EnEoShot* this, PlayState* play) {
    Actor* actor = &this->actor;
    Player* player = GET_PLAYER(play);
    f32 dist;

    this->radius += this->radiusStep;
    if (this->radius >= this->radiusMax) {
        Actor_Kill(actor);
        return;
    }

    if ((this->timer % 2) == 0) {
        s32 count = CLAMP((s32)(this->radius / 45.0f), 8, 28);
        s16 base = (s16)Rand_Next();
        s32 i;

        for (i = 0; i < count; i++) {
            s16 angle = base + (s16)(i * (0x10000 / count));
            Vec3f pos;

            pos.x = actor->world.pos.x + (Math_SinS(angle) * this->radius);
            pos.y = actor->floorHeight + 2.0f;
            pos.z = actor->world.pos.z + (Math_CosS(angle) * this->radius);
            EoShot_LookPuff(play, this->look, &pos, 0.8f);
        }
    }
    this->timer++;

    // One hit per ring, where the ring passes, and only while Link stands on the floor
    if (this->damage != 0) {
        dist = sqrtf(SQ(player->actor.world.pos.x - actor->world.pos.x) +
                     SQ(player->actor.world.pos.z - actor->world.pos.z));

        if ((fabsf(dist - this->radius) < (this->radiusStep + 30.0f)) &&
            (fabsf(player->actor.world.pos.y - actor->floorHeight) < 45.0f) &&
            Eo_HurtPlayer(play, (this->owner != NULL) ? this->owner : actor, this->damage,
                          Math_Vec3f_Yaw(&actor->world.pos, &player->actor.world.pos), 8.0f, true, false)) {
            this->damage = 0;
        }
    }
}

static void EoShot_UpdateBoomerang(EnEoShot* this, PlayState* play) {
    Actor* actor = &this->actor;

    this->timer++;
    this->spin += this->spinStep;
    Actor_PlaySfx_Flagged(actor, NA_SE_IT_BOOMERANG_FLY - SFX_FLAG);

    if ((this->timer < 34) && (this->owner != NULL)) {
        // Out: slowing down, curving a little towards Link
        Player* player = GET_PLAYER(play);

        Math_ScaledStepToS(&actor->world.rot.y, Math_Vec3f_Yaw(&actor->world.pos, &player->actor.world.pos), 0x180);
        this->speed *= 0.95f;
        actor->velocity.x = Math_SinS(actor->world.rot.y) * this->speed;
        actor->velocity.z = Math_CosS(actor->world.rot.y) * this->speed;
        // Comes down to Link's chest height, however high it was thrown from
        actor->velocity.y = CLAMP((player->actor.world.pos.y + 35.0f - actor->world.pos.y) * 0.2f, -6.0f, 6.0f);
    } else if (this->owner != NULL) {
        // Back to its owner's hand
        Vec3f home = this->owner->focus.pos;
        f32 dist = Math_Vec3f_DistXYZ(&actor->world.pos, &home);

        if ((dist < 60.0f) || (this->timer > 200)) {
            Actor_Kill(actor);
            return;
        }
        Math_StepToF(&this->speed, 22.0f, 1.2f);
        EoShot_AimVelocity(&actor->world.pos, &home, this->speed, 0, &actor->velocity);
    } else {
        EoShot_Burst(play, this);
        Actor_Kill(actor);
        return;
    }

    actor->world.pos.x += actor->velocity.x;
    actor->world.pos.y += actor->velocity.y;
    actor->world.pos.z += actor->velocity.z;

    if ((this->timer % 3) == 0) {
        EoShot_Dust(play, &actor->world.pos, 50);
    }

    Collider_UpdateCylinder(actor, &this->collider);
    if (this->collider.base.atFlags & (AT_HIT | AT_BOUNCED)) {
        this->collider.base.atFlags &= ~(AT_HIT | AT_BOUNCED);
        this->timer = MAX(this->timer, 34); // bounced off Link or his shield: it heads home
        this->noHitTimer = 12;              // no second hit for a moment
    }
    if (this->noHitTimer > 0) {
        this->noHitTimer--;
    } else {
        CollisionCheck_SetAT(play, &play->colChkCtx, &this->collider.base);
    }
}

static void EoShot_UpdateFalling(EnEoShot* this, PlayState* play) {
    Actor* actor = &this->actor;
    Player* player = GET_PLAYER(play);

    this->spin += this->spinStep;
    actor->velocity.y -= 1.6f;
    actor->world.pos.y += actor->velocity.y;

    if ((this->dList == NULL) && ((this->timer % 2) == 0)) {
        EoShot_Trail(play, this);
    }

    // The spot it will land on is marked all the way down
    if ((this->timer % 5) == 0) {
        Vec3f mark = this->target;

        mark.x += Rand_CenteredFloat(30.0f);
        mark.z += Rand_CenteredFloat(30.0f);
        EoShot_Dust(play, &mark, 90);
    }
    this->timer++;

    if (actor->world.pos.y <= this->target.y) {
        f32 dist = sqrtf(SQ(player->actor.world.pos.x - this->target.x) +
                         SQ(player->actor.world.pos.z - this->target.z));
        s32 i;

        actor->world.pos.y = this->target.y;
        for (i = 0; i < 4; i++) {
            Vec3f puffPos = this->target;

            puffPos.x += Rand_CenteredFloat(60.0f);
            puffPos.z += Rand_CenteredFloat(60.0f);
            EoShot_Dust(play, &puffPos, 200);
        }
        if (this->dList == NULL) {
            EoShot_Burst(play, this);
        } else {
            SoundSource_PlaySfxAtFixedWorldPos(play, &actor->world.pos, 20, NA_SE_EV_ROCK_BROKEN);
        }

        if ((dist < ((this->radius > 0.0f) ? this->radius : 70.0f)) &&
            (fabsf(player->actor.world.pos.y - this->target.y) < 80.0f)) {
            Eo_HurtPlayer(play, (this->owner != NULL) ? this->owner : actor, this->damage,
                          Math_Vec3f_Yaw(&this->target, &player->actor.world.pos), 6.0f, false, false);
        }
        Actor_Kill(actor);
    }
}

void EnEoShot_Update(Actor* thisx, PlayState* play) {
    EnEoShot* this = (EnEoShot*)thisx;

    // Waves, rings, weapons and rocks vanish in cutscenes (a miniboss dying, a door opening...)
    if (Play_InCsMode(play) && (this->mode != EO_SHOT_ORB)) {
        Actor_Kill(thisx);
        return;
    }

    switch (this->mode) {
        case EO_SHOT_ORB:
            this->timer++;
            EoShot_UpdateOrb(this, play);
            break;
        case EO_SHOT_GROUND_WAVE:
            EoShot_UpdateGroundWave(this, play);
            break;
        case EO_SHOT_RING:
            EoShot_UpdateRing(this, play);
            break;
        case EO_SHOT_BOOMERANG:
            EoShot_UpdateBoomerang(this, play);
            break;
        case EO_SHOT_FALLING:
            EoShot_UpdateFalling(this, play);
            break;
        default:
            Actor_Kill(thisx);
            break;
    }
}

/* ---------------------------------------------------------------------------------------------------------------
 * Draw
 * --------------------------------------------------------------------------------------------------------------- */

// Two counter-rotating glows, like Navi's.
static void EoShot_DrawOrb(EnEoShot* this, PlayState* play) {
    EoKindInfo* info = &sEoKindInfo[this->kind];
    f32 scale = this->drawScale;
    s16 alpha = 255;

    if (this->life < 8) {
        alpha = CLAMP_MIN(this->life, 0) * 32;
    }
    if ((this->kind == EO_KIND_EMBER) && (this->maxLife > 0)) {
        alpha = (alpha * CLAMP_MIN(this->life, 0)) / this->maxLife;
    }

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, info->prim.r, info->prim.g, info->prim.b, (u8)alpha);
    gDPSetEnvColor(POLY_XLU_DISP++, info->env.r, info->env.g, info->env.b, 255);
    Matrix_Translate(this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z, MTXMODE_NEW);
    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
    Matrix_Mult(&play->billboardMtxF, MTXMODE_APPLY);
    Matrix_Push();
    Matrix_RotateZ(BINANG_TO_RAD(this->spin), MTXMODE_APPLY);
    gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)gEffFlash1DL);
    Matrix_Pop();
    Matrix_RotateZ(BINANG_TO_RAD(-this->spin), MTXMODE_APPLY);
    gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)gEffFlash1DL);

    CLOSE_DISPS(play->state.gfxCtx);
}

void EnEoShot_Draw(Actor* thisx, PlayState* play) {
    EnEoShot* this = (EnEoShot*)thisx;

    if ((this->mode == EO_SHOT_ORB) || ((this->mode == EO_SHOT_FALLING) && (this->dList == NULL))) {
        EoShot_DrawOrb(this, play);
    } else if (((this->mode == EO_SHOT_BOOMERANG) || (this->mode == EO_SHOT_FALLING)) && (this->dList != NULL)) {
        OPEN_DISPS(play->state.gfxCtx);

        Gfx_SetupDL_25Opa(play->state.gfxCtx);
        Matrix_Translate(thisx->world.pos.x, thisx->world.pos.y, thisx->world.pos.z, MTXMODE_NEW);
        Matrix_RotateY(BINANG_TO_RAD(this->spin), MTXMODE_APPLY);
        if (this->mode == EO_SHOT_BOOMERANG) {
            Matrix_RotateZ(M_PI / 2, MTXMODE_APPLY);
        } else {
            Matrix_RotateX(BINANG_TO_RAD(this->spin), MTXMODE_APPLY);
        }
        Matrix_Scale(this->dListScale, this->dListScale, this->dListScale, MTXMODE_APPLY);
        // Models borrowed from other actors may call the color display lists their owner sets up: point those
        // segments somewhere harmless
        gSPSegment(POLY_OPA_DISP++, 0x08, (uintptr_t)gEmptyDL);
        gSPSegment(POLY_OPA_DISP++, 0x09, (uintptr_t)gEmptyDL);
        gSPSegment(POLY_OPA_DISP++, 0x0A, (uintptr_t)gEmptyDL);
        gSPSegment(POLY_OPA_DISP++, 0x0B, (uintptr_t)gEmptyDL);
        gSPSegment(POLY_OPA_DISP++, 0x0C, (uintptr_t)gEmptyDL);
        gSPSegment(POLY_OPA_DISP++, 0x0D, (uintptr_t)gEmptyDL);
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 200, 200, 200, 255);
        gDPSetEnvColor(POLY_OPA_DISP++, 40, 40, 40, 255);
        gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_OPA_DISP++, this->dList);

        CLOSE_DISPS(play->state.gfxCtx);
    }
}
