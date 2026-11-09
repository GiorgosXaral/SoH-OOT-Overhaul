#include "z_en_m_thunder.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "overlays/ovl_Arrow_Light/ovl_Arrow_Light.h"

#define FLAGS 0

void EnMThunder_Init(Actor* thisx, PlayState* play);
void EnMThunder_Destroy(Actor* thisx, PlayState* play);
void EnMThunder_Update(Actor* thisx, PlayState* play);
void EnMThunder_Draw(Actor* thisx, PlayState* play);

void EnMThunder_AdjustEnvLights(PlayState* play, f32 intensity);
void EnMThunder_ChargingSpinAttack(EnMThunder* this, PlayState* play);
void EnMThunder_SpinAttacking(EnMThunder* this, PlayState* play);

const ActorInit En_M_Thunder_InitVars = {
    ACTOR_EN_M_THUNDER,
    ACTORCAT_ITEMACTION,
    FLAGS,
    OBJECT_GAMEPLAY_KEEP,
    sizeof(EnMThunder),
    (ActorFunc)EnMThunder_Init,
    (ActorFunc)EnMThunder_Destroy,
    (ActorFunc)EnMThunder_Update,
    (ActorFunc)EnMThunder_Draw,
    NULL,
};

static ColliderCylinderInit sCylinderInit = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_PLAYER,
        AC_NONE,
        OC1_NONE,
        OC2_TYPE_1,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK2,
        { 0x00000001, 0x00, 0x00 },
        { 0xFFCFFFFF, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NONE,
        BUMP_ON,
        OCELEM_ON,
    },
    { 200, 200, 0, { 0, 0, 0 } },
};

static u32 sSpinAttackDmgFlags[] = { 0x01000000, 0x00400000, 0x00800000 };
static u32 sJumpAttackDmgFlags[] = { 0x08000000, 0x02000000, 0x04000000 };

static u16 sSfxIds[] = {
    NA_SE_IT_ROLLING_CUT_LV2,
    NA_SE_IT_ROLLING_CUT_LV1,
    NA_SE_IT_ROLLING_CUT_LV2,
    NA_SE_IT_ROLLING_CUT_LV1,
};

void EnMThunder_SetupAction(EnMThunder* this, EnMThunderActionFunc actionFunc) {
    this->actionFunc = actionFunc;
}

/* ---------------------------------------------------------------------------------------------------------------
 * Master Sword beam (Enhancements > Items > Master Sword Beam)
 *
 * Like A Link to the Past: while Link's hearts are full, every normal Master Sword slash fires a spinning beam of
 * light. Only one beam flies at a time. It hits exactly like a Master Sword slash (same damage flags, same damage), so
 * every enemy takes the damage its own damage table gives the Master Sword. Drawn with the Light Arrow's glow, so it
 * needs no new assets.
 * --------------------------------------------------------------------------------------------------------------- */

#define SWORD_BEAM_SPEED 22.0f
#define SWORD_BEAM_LIFE 26 // frames, about 570 units of range

static ColliderCylinderInit sSwordBeamCylinderInit = {
    {
        COLTYPE_NONE,
        AT_ON | AT_TYPE_PLAYER,
        AC_NONE,
        OC1_NONE,
        OC2_TYPE_1,
        COLSHAPE_CYLINDER,
    },
    {
        ELEMTYPE_UNK2,
        { DMG_SLASH_MASTER, 0x00, 0x02 },
        { 0xFFCFFFFF, 0x00, 0x00 },
        TOUCH_ON | TOUCH_SFX_NORMAL,
        BUMP_NONE,
        OCELEM_NONE,
    },
    { 28, 36, -18, { 0, 0, 0 } },
};

static s32 EnMThunder_IsSwordBeam(EnMThunder* this) {
    return this->actor.params == ENMTHUNDER_SWORD_BEAM_PARAMS;
}

static s32 EnMThunder_SwordBeamFlying(PlayState* play) {
    Actor* actor = play->actorCtx.actorLists[ACTORCAT_ITEMACTION].head;

    while (actor != NULL) {
        if ((actor->id == ACTOR_EN_M_THUNDER) && (actor->params == ENMTHUNDER_SWORD_BEAM_PARAMS) &&
            (actor->update != NULL)) {
            return true;
        }
        actor = actor->next;
    }
    return false;
}

void EnMThunder_TrySwordBeam(PlayState* play, Player* player) {
    Vec3f pos;
    s16 pitch = 0;
    s16 yaw;

    if ((play == NULL) || (player == NULL) || (player->actor.id != ACTOR_PLAYER)) {
        return;
    }
    if (!CVarGetInteger(CVAR_ENHANCEMENT("MasterSwordBeam"), 1)) {
        return;
    }
    if (player->heldItemAction != PLAYER_IA_SWORD_MASTER) {
        return;
    }
    // Normal slashes, stabs and combo finishers only: jump attacks and spin attacks stay vanilla
    if (!((player->meleeWeaponAnimation <= PLAYER_MWA_STAB_COMBO_2H) ||
          (player->meleeWeaponAnimation == PLAYER_MWA_BACKSLASH_RIGHT) ||
          (player->meleeWeaponAnimation == PLAYER_MWA_BACKSLASH_LEFT))) {
        return;
    }
    if (gSaveContext.health < gSaveContext.healthCapacity) {
        return;
    }
    if (EnMThunder_SwordBeamFlying(play)) {
        return;
    }

    yaw = player->actor.shape.rot.y;
    pos = player->bodyPartsPos[PLAYER_BODYPART_WAIST];
    pos.x += Math_SinS(yaw) * 25.0f;
    pos.y += 12.0f;
    pos.z += Math_CosS(yaw) * 25.0f;

    // When locked on, aim up or down at the target
    if (player->focusActor != NULL) {
        pitch = Math_Vec3f_Pitch(&pos, &player->focusActor->focus.pos);
        pitch = CLAMP(pitch, -0x2000, 0x2000);
    }

    Actor_Spawn(&play->actorCtx, play, ACTOR_EN_M_THUNDER, pos.x, pos.y, pos.z, pitch, yaw, 0,
                ENMTHUNDER_SWORD_BEAM_PARAMS);
}

static void EnMThunder_SwordBeamInit(EnMThunder* this, PlayState* play) {
    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, &this->actor, &sSwordBeamCylinderInit);
    Lights_PointNoGlowSetInfo(&this->lightInfo, this->actor.world.pos.x, this->actor.world.pos.y,
                              this->actor.world.pos.z, 170, 255, 255, 150);
    this->lightNode = LightContext_InsertLight(play, &play->lightCtx, &this->lightInfo);

    this->actor.shape.rot = this->actor.world.rot;
    this->actor.velocity.x = Math_CosS(this->actor.world.rot.x) * Math_SinS(this->actor.world.rot.y) * SWORD_BEAM_SPEED;
    this->actor.velocity.y = -Math_SinS(this->actor.world.rot.x) * SWORD_BEAM_SPEED;
    this->actor.velocity.z = Math_CosS(this->actor.world.rot.x) * Math_CosS(this->actor.world.rot.y) * SWORD_BEAM_SPEED;
    this->actor.room = -1;
    this->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    this->beamTimer = SWORD_BEAM_LIFE;
    this->beamSpin = 0;
    this->isUsingMagic = 0;
    Actor_SetScale(&this->actor, 0.01f);
    Audio_PlayActorSound2(&this->actor, NA_SE_IT_ROLLING_CUT_LV1);
}

static void EnMThunder_SwordBeamBurst(EnMThunder* this, PlayState* play) {
    EffectSsHitMark_SpawnFixedScale(play, 0, &this->actor.world.pos);
    Actor_Kill(&this->actor);
}

static void EnMThunder_SwordBeamUpdate(EnMThunder* this, PlayState* play) {
    static Color_RGBA8 sSparklePrim = { 255, 255, 255, 255 };
    static Color_RGBA8 sSparkleEnv = { 60, 160, 255, 0 };
    static Vec3f sZeroVec = { 0.0f, 0.0f, 0.0f };
    Vec3f sparklePos;

    if (Play_InCsMode(play) || (this->beamTimer <= 0)) {
        Actor_Kill(&this->actor); // out of range: just vanishes
        return;
    }
    this->beamTimer--;

    // Anything the beam touched last frame (an enemy, a shield, a switch) ends it
    if (this->collider.base.atFlags & (AT_HIT | AT_BOUNCED)) {
        EnMThunder_SwordBeamBurst(this, play);
        return;
    }

    Actor_UpdatePos(&this->actor);
    Actor_UpdateBgCheckInfo(play, &this->actor, 15.0f, 15.0f, 15.0f, 7);
    if (this->actor.bgCheckFlags & (1 | 8 | 0x10)) { // floor, wall or ceiling
        EnMThunder_SwordBeamBurst(this, play);
        return;
    }

    this->beamSpin += 0x1C00;

    Collider_UpdateCylinder(&this->actor, &this->collider);
    CollisionCheck_SetAT(play, &play->colChkCtx, &this->collider.base);

    sparklePos.x = this->actor.world.pos.x + Rand_CenteredFloat(20.0f);
    sparklePos.y = this->actor.world.pos.y + Rand_CenteredFloat(20.0f);
    sparklePos.z = this->actor.world.pos.z + Rand_CenteredFloat(20.0f);
    EffectSsKiraKira_SpawnSmall(play, &sparklePos, &sZeroVec, &sZeroVec, &sSparklePrim, &sSparkleEnv);

    Lights_PointNoGlowSetInfo(&this->lightInfo, this->actor.world.pos.x, this->actor.world.pos.y,
                              this->actor.world.pos.z, 170, 255, 255, 150);
}

static void EnMThunder_SwordBeamDraw(EnMThunder* this, PlayState* play) {
    u32 frames = play->state.frames;
    s32 i;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, 200, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 0, 110, 255, 128);

    // Two crossed streaks of the Light Arrow glow, spinning about the direction of travel
    for (i = 0; i < 2; i++) {
        Matrix_Translate(this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z, MTXMODE_NEW);
        Matrix_RotateY(this->actor.shape.rot.y * (M_PI / 0x8000), MTXMODE_APPLY);
        Matrix_RotateX(this->actor.shape.rot.x * (M_PI / 0x8000), MTXMODE_APPLY);
        Matrix_RotateZ((this->beamSpin + (i * 0x4000)) * (M_PI / 0x8000), MTXMODE_APPLY);
        Matrix_Scale(0.01f, 0.01f, 0.01f, MTXMODE_APPLY);
        Matrix_Scale((i == 0) ? 2.4f : 1.6f, (i == 0) ? 4.5f : 3.0f, 2.0f, MTXMODE_APPLY);
        Matrix_Translate(0.0f, -700.0f, 0.0f, MTXMODE_APPLY);
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_XLU_DISP++, sMaterialDL);
        gSPDisplayList(POLY_XLU_DISP++,
                       Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 511 - (frames * 5) % 512, 0, 4, 32, 1,
                                          511 - (frames * 10) % 512, 511 - (frames * 30) % 512, 8, 16, -5, 0, -10,
                                          -30));
        gSPDisplayList(POLY_XLU_DISP++, sModelDL);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

void EnMThunder_Init(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    EnMThunder* this = (EnMThunder*)thisx;
    Player* player = GET_PLAYER(play);

    if (EnMThunder_IsSwordBeam(this)) {
        EnMThunder_SwordBeamInit(this, play);
        return;
    }

    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, &this->actor, &sCylinderInit);
    this->swordType = (this->actor.params & 0xFF) - 1;
    Lights_PointNoGlowSetInfo(&this->lightInfo, this->actor.world.pos.x, this->actor.world.pos.y,
                              this->actor.world.pos.z, 255, 255, 255, 0);
    this->lightNode = LightContext_InsertLight(play, &play->lightCtx, &this->lightInfo);
    this->collider.dim.radius = 0;
    this->collider.dim.height = 40;
    this->collider.dim.yShift = -20;
    this->followPlayerTimer = 8;
    this->spinTrailTexScroll = 0.0f;
    this->actor.world.pos = player->bodyPartsPos[0];
    this->spinAttackTimer = 0.0f;
    this->dimmingIntensity = 0.0f;
    this->actor.shape.rot.y = player->actor.shape.rot.y + 0x8000;
    this->actor.room = -1;
    Actor_SetScale(&this->actor, 0.1f);
    this->isUsingMagic = 0;

    if (player->stateFlags2 & PLAYER_STATE2_SPIN_ATTACKING) {
        if (!gSaveContext.isMagicAcquired || (gSaveContext.magicState != MAGIC_STATE_IDLE) ||
            (((this->actor.params & 0xFF00) >> 8) &&
             !(Magic_RequestChange(play, (this->actor.params & 0xFF00) >> 8, MAGIC_CONSUME_NOW)))) {
            Audio_PlaySfxGeneral(NA_SE_IT_ROLLING_CUT, &player->actor.projectedPos, 4, &gSfxDefaultFreqAndVolScale,
                                 &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
            Audio_PlaySfxGeneral(NA_SE_IT_SWORD_SWING_HARD, &player->actor.projectedPos, 4, &gSfxDefaultFreqAndVolScale,
                                 &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
            Actor_Kill(&this->actor);
            return;
        }

        player->stateFlags2 &= ~PLAYER_STATE2_SPIN_ATTACKING;
        this->isUsingMagic = 1;
        this->collider.info.toucher.dmgFlags = sSpinAttackDmgFlags[this->swordType];
        this->attackStrength = 1;
        this->targetScale = ((this->swordType == 1) ? 2 : 4);
        EnMThunder_SetupAction(this, EnMThunder_SpinAttacking);
        this->followPlayerTimer = 8;
        Audio_PlaySfxGeneral(NA_SE_IT_ROLLING_CUT_LV1, &player->actor.projectedPos, 4, &gSfxDefaultFreqAndVolScale,
                             &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
        this->spinAttackTimer = 1.0f;
    } else {
        EnMThunder_SetupAction(this, EnMThunder_ChargingSpinAttack);
    }
    this->actor.child = NULL;
}

void EnMThunder_Destroy(Actor* thisx, PlayState* play) {
    EnMThunder* this = (EnMThunder*)thisx;

    if (EnMThunder_IsSwordBeam(this)) {
        Collider_DestroyCylinder(play, &this->collider);
        LightContext_RemoveLight(play, &play->lightCtx, this->lightNode);
        return;
    }

    if (this->isUsingMagic != 0) {
        Magic_Reset(play);
    }

    Collider_DestroyCylinder(play, &this->collider);
    EnMThunder_AdjustEnvLights(play, 0.0f);
    LightContext_RemoveLight(play, &play->lightCtx, this->lightNode);
}

void EnMThunder_AdjustEnvLights(PlayState* play, f32 intensity) {
    Environment_AdjustLights(play, intensity, 850.0f, 0.2f, 0.0f);
}

void EnMThunder_EmptySpinAttack(EnMThunder* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (player->stateFlags2 & PLAYER_STATE2_SPIN_ATTACKING) {
        if (player->meleeWeaponAnimation >= 0x18) {
            Audio_PlaySfxGeneral(NA_SE_IT_ROLLING_CUT, &player->actor.projectedPos, 4, &gSfxDefaultFreqAndVolScale,
                                 &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
            Audio_PlaySfxGeneral(NA_SE_IT_SWORD_SWING_HARD, &player->actor.projectedPos, 4, &gSfxDefaultFreqAndVolScale,
                                 &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
        }

        Actor_Kill(&this->actor);
        return;
    }

    if (!(player->stateFlags1 & PLAYER_STATE1_CHARGING_SPIN_ATTACK)) {
        Actor_Kill(&this->actor);
    }
}

void EnMThunder_ChargingSpinAttack(EnMThunder* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    Actor* child = this->actor.child;

    this->spinChargePercent = player->unk_858;
    this->actor.world.pos = player->bodyPartsPos[0];
    this->actor.shape.rot.y = player->actor.shape.rot.y + 0x8000;

    if (this->isUsingMagic == 0) {
        if (player->unk_858 >= 0.1f) {
            if ((gSaveContext.magicState != MAGIC_STATE_IDLE) ||
                (((this->actor.params & 0xFF00) >> 8) &&
                 !(Magic_RequestChange(play, (this->actor.params & 0xFF00) >> 8, MAGIC_CONSUME_WAIT_PREVIEW)))) {
                EnMThunder_EmptySpinAttack(this, play);
                EnMThunder_SetupAction(this, EnMThunder_EmptySpinAttack);
                this->chargeAlpha = 0;
                this->dimmingIntensity = 0.0;
                this->spinAttackTimer = 0.0f;
                return;
            }

            this->isUsingMagic = 1;
        }
    }

    if (player->unk_858 >= 0.1f) {
        Rumble_Request(0.0f, (s32)(player->unk_858 * 150.0f) & 0xFF, 2, (s32)(player->unk_858 * 150.0f) & 0xFF);
    }

    if (player->stateFlags2 & PLAYER_STATE2_SPIN_ATTACKING) {
        if ((child != NULL) && (child->update != NULL)) {
            child->parent = NULL;
        }

        if (player->unk_858 <= 0.15f) {
            if ((player->unk_858 >= 0.1f) && (player->meleeWeaponAnimation >= 0x18)) {
                Audio_PlaySfxGeneral(NA_SE_IT_ROLLING_CUT, &player->actor.projectedPos, 4, &gSfxDefaultFreqAndVolScale,
                                     &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
                Audio_PlaySfxGeneral(NA_SE_IT_SWORD_SWING_HARD, &player->actor.projectedPos, 4,
                                     &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
            }
            Actor_Kill(&this->actor);
            return;
        } else {
            player->stateFlags2 &= ~PLAYER_STATE2_SPIN_ATTACKING;
            if ((this->actor.params & 0xFF00) >> 8) {
                gSaveContext.magicState = MAGIC_STATE_CONSUME_SETUP;
            }
            if (player->unk_858 < 0.85f) {
                this->collider.info.toucher.dmgFlags = sSpinAttackDmgFlags[this->swordType];
                this->attackStrength = 1;
                this->targetScale = ((this->swordType == 1) ? 2 : 4);
            } else {
                this->collider.info.toucher.dmgFlags = sJumpAttackDmgFlags[this->swordType];
                this->attackStrength = 0;
                this->targetScale = ((this->swordType == 1) ? 4 : 8);
            }

            EnMThunder_SetupAction(this, EnMThunder_SpinAttacking);
            this->followPlayerTimer = 8;
            Audio_PlaySfxGeneral(sSfxIds[this->attackStrength], &player->actor.projectedPos, 4,
                                 &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
            this->spinAttackTimer = 1.0f;
            return;
        }
    }

    if (!(player->stateFlags1 & PLAYER_STATE1_CHARGING_SPIN_ATTACK)) {
        if (this->actor.child != NULL) {
            this->actor.child->parent = NULL;
        }
        Actor_Kill(&this->actor);
        return;
    }

    if (player->unk_858 > 0.15f) {
        this->chargeAlpha = 255;
        if (this->actor.child == NULL) {
            Actor_SpawnAsChild(&play->actorCtx, &this->actor, play, ACTOR_EFF_DUST, this->actor.world.pos.x,
                               this->actor.world.pos.y, this->actor.world.pos.z, 0, this->actor.shape.rot.y, 0,
                               this->swordType + 2);
        }
        this->dimmingIntensity += ((((player->unk_858 - 0.15f) * 1.5f) - this->dimmingIntensity) * 0.5f);

    } else if (player->unk_858 > .1f) {
        this->chargeAlpha = (s32)((player->unk_858 - .1f) * 255.0f * 20.0f);
        this->spinAttackTimer = (player->unk_858 - .1f) * 10.0f;
    } else {
        this->chargeAlpha = 0;
    }

    if (player->unk_858 > 0.85f) {
        func_800F4254(&player->actor.projectedPos, 2);
    } else if (player->unk_858 > 0.15f) {
        func_800F4254(&player->actor.projectedPos, 1);
    } else if (player->unk_858 > 0.1f) {
        func_800F4254(&player->actor.projectedPos, 0);
    }

    if (Play_InCsMode(play)) {
        Actor_Kill(&this->actor);
    }
}

void EnMThunder_UpdateSpinAttack(EnMThunder* this, PlayState* play) {
    if (this->followPlayerTimer < 2) {
        if (this->chargeAlpha < 40) {
            this->chargeAlpha = 0;
        } else {
            this->chargeAlpha -= 40;
        }
    }

    this->spinTrailTexScroll += 2.0f * this->spinAttackAlpha;

    if (this->dimmingIntensity < this->spinAttackTimer) {
        this->dimmingIntensity += ((this->spinAttackTimer - this->dimmingIntensity) * 0.1f);
    } else {
        this->dimmingIntensity = this->spinAttackTimer;
    }
}

void EnMThunder_SpinAttacking(EnMThunder* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (Math_StepToF(&this->spinAttackTimer, 0.0f, 1 / 16.0f)) {
        Actor_Kill(&this->actor);
    } else {
        Math_SmoothStepToF(&this->actor.scale.x, (s32)this->targetScale, 0.6f, 0.8f, 0.0f);
        Actor_SetScale(&this->actor, this->actor.scale.x);
        this->collider.dim.radius = (this->actor.scale.x * 25.0f);
        Collider_UpdateCylinder(&this->actor, &this->collider);
        CollisionCheck_SetAT(play, &play->colChkCtx, &this->collider.base);
    }

    if (this->followPlayerTimer > 0) {
        this->actor.world.pos.x = player->bodyPartsPos[0].x;
        this->actor.world.pos.z = player->bodyPartsPos[0].z;
        this->followPlayerTimer--;
    }

    if (this->spinAttackTimer > 0.6f) {
        this->spinAttackAlpha = 1.0f;
    } else {
        this->spinAttackAlpha = this->spinAttackTimer * (5.0f / 3.0f);
    }

    EnMThunder_UpdateSpinAttack(this, play);

    if (Play_InCsMode(play)) {
        Actor_Kill(&this->actor);
    }
}

void EnMThunder_Update(Actor* thisx, PlayState* play) {
    EnMThunder* this = (EnMThunder*)thisx;
    f32 blueRadius;
    s32 redGreen;

    if (EnMThunder_IsSwordBeam(this)) {
        EnMThunder_SwordBeamUpdate(this, play);
        return;
    }

    this->actionFunc(this, play);
    EnMThunder_AdjustEnvLights(play, this->dimmingIntensity);
    blueRadius = this->spinAttackTimer;
    redGreen = (u32)(blueRadius * 255.0f) & 0xFF;
    Lights_PointNoGlowSetInfo(&this->lightInfo, this->actor.world.pos.x, this->actor.world.pos.y,
                              this->actor.world.pos.z, redGreen, redGreen, (u32)(blueRadius * 100.0f),
                              (s32)(blueRadius * 800.0f));
}

void EnMThunder_Draw(Actor* thisx, PlayState* play2) {
    static f32 sSpinChargeScale[] = { 0.1f, 0.15f, 0.2f, 0.25f, 0.3f, 0.25f, 0.2f, 0.15f };
    PlayState* play = play2;
    EnMThunder* this = (EnMThunder*)thisx;
    Player* player = GET_PLAYER(play);
    f32 phi_f14;
    s32 phi_t1;

    if (EnMThunder_IsSwordBeam(this)) {
        EnMThunder_SwordBeamDraw(this, play);
        return;
    }

    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    Matrix_Scale(0.02f, 0.02f, 0.02f, MTXMODE_APPLY);
    gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    switch (this->attackStrength) {
        case 0:
        case 1:
            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScrollEx(
                           play->state.gfxCtx, 0, 0xFF - ((u8)(s32)(this->spinTrailTexScroll * 30) & 0xFF), 0, 0x40,
                           0x20, 1, 0xFF - ((u8)(s32)(this->spinTrailTexScroll * 20) & 0xFF), 0, 8, 8, -30, 0, -20, 0));
            break;
    }

    switch (this->attackStrength) {
        case 0:
            if (CVarGetInteger(CVAR_COSMETIC("SpinAttack.Level2Primary.Changed"), 0)) {
                Color_RGB8 color =
                    CVarGetColor24(CVAR_COSMETIC("SpinAttack.Level2Primary.Value"), (Color_RGB8){ 255, 255, 170 });
                gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, color.r, color.g, color.b, (u8)(this->spinAttackAlpha * 255));
            } else {
                gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, 255, 255, 170, (u8)(this->spinAttackAlpha * 255));
            }
            gSPDisplayList(POLY_XLU_DISP++, gSpinAttack3DL);
            gSPDisplayList(POLY_XLU_DISP++, gSpinAttack4DL);
            break;
        case 1:
            if (CVarGetInteger(CVAR_COSMETIC("SpinAttack.Level1Primary.Changed"), 0)) {
                Color_RGB8 color =
                    CVarGetColor24(CVAR_COSMETIC("SpinAttack.Level1Primary.Value"), (Color_RGB8){ 170, 255, 255 });
                gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, color.r, color.g, color.b, (u8)(this->spinAttackAlpha * 255));
            } else {
                gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, 170, 255, 255, (u8)(this->spinAttackAlpha * 255));
            }
            gSPDisplayList(POLY_XLU_DISP++, gSpinAttack1DL);
            gSPDisplayList(POLY_XLU_DISP++, gSpinAttack2DL);
            break;
    }

    Matrix_Mult(&player->mf_9E0, MTXMODE_NEW);

    switch (this->swordType) {
        case 1:
            Matrix_Translate(0.0f, 220.0f, 0.0f, MTXMODE_APPLY);
            Matrix_Scale(-0.7f, -0.6f, -0.4f, MTXMODE_APPLY);
            Matrix_RotateX(16384.0f, MTXMODE_APPLY);
            break;
        case 0:
            Matrix_Translate(0.0f, 300.0f, -100.0f, MTXMODE_APPLY);
            Matrix_Scale(-1.2f, -1.0f, -0.7f, MTXMODE_APPLY);
            Matrix_RotateX(16384.0f, MTXMODE_APPLY);
            break;
        case 2:
            Matrix_Translate(200.0f, 350.0f, 0.0f, MTXMODE_APPLY);
            Matrix_Scale(-1.8f, -1.4f, -0.7f, MTXMODE_APPLY);
            Matrix_RotateX(16384.0f, MTXMODE_APPLY);
            break;
    }

    if (this->spinChargePercent >= 0.85f) {
        phi_f14 = (sSpinChargeScale[(play->gameplayFrames & 7)] * 6.0f) + 1.0f;
        if (CVarGetInteger(CVAR_COSMETIC("SpinAttack.Level2Primary.Changed"), 0)) {
            Color_RGB8 color =
                CVarGetColor24(CVAR_COSMETIC("SpinAttack.Level2Primary.Value"), (Color_RGB8){ 255, 255, 170 });
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, color.r, color.g, color.b, this->chargeAlpha);
        } else {
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, 255, 255, 170, this->chargeAlpha);
        }
        if (CVarGetInteger(CVAR_COSMETIC("SpinAttack.Level2Secondary.Changed"), 0)) {
            Color_RGB8 color =
                CVarGetColor24(CVAR_COSMETIC("SpinAttack.Level2Secondary.Value"), (Color_RGB8){ 255, 100, 0 });
            gDPSetEnvColor(POLY_XLU_DISP++, color.r, color.g, color.b, 128);
        } else {
            gDPSetEnvColor(POLY_XLU_DISP++, 255, 100, 0, 128);
        }
        phi_t1 = 0x28;
    } else {
        phi_f14 = (sSpinChargeScale[play->gameplayFrames & 7] * 2.0f) + 1.0f;
        if (CVarGetInteger(CVAR_COSMETIC("SpinAttack.Level1Primary.Changed"), 0)) {
            Color_RGB8 color =
                CVarGetColor24(CVAR_COSMETIC("SpinAttack.Level1Primary.Value"), (Color_RGB8){ 170, 255, 255 });
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, color.r, color.g, color.b, this->chargeAlpha);
        } else {
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, 170, 255, 255, this->chargeAlpha);
        }
        if (CVarGetInteger(CVAR_COSMETIC("SpinAttack.Level1Secondary.Changed"), 0)) {
            Color_RGB8 color =
                CVarGetColor24(CVAR_COSMETIC("SpinAttack.Level1Secondary.Value"), (Color_RGB8){ 0, 100, 255 });
            gDPSetEnvColor(POLY_XLU_DISP++, color.r, color.g, color.b, 128);
        } else {
            gDPSetEnvColor(POLY_XLU_DISP++, 0, 100, 255, 128);
        }
        phi_t1 = 0x14;
    }
    Matrix_Scale(1.0f, phi_f14, phi_f14, MTXMODE_APPLY);
    gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    gSPSegment(POLY_XLU_DISP++, 0x09,
               Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, (play->gameplayFrames * 5) & 0xFF, 0, 0x20, 0x20, 1,
                                  (play->gameplayFrames * 20) & 0xFF, (play->gameplayFrames * phi_t1) & 0xFF, 8, 8, 5,
                                  0, 20, phi_t1));

    gSPDisplayList(POLY_XLU_DISP++, gSpinAttackChargingDL);

    CLOSE_DISPS(play->state.gfxCtx);
}
