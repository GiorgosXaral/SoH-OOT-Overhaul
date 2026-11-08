#ifndef Z_EN_M_THUNDER_H
#define Z_EN_M_THUNDER_H

#include <libultraship/libultra.h>
#include "global.h"

struct EnMThunder;

typedef void (*EnMThunderActionFunc)(struct EnMThunder*, PlayState*);

typedef struct EnMThunder {
    /* 0x0000 */ Actor actor;
    /* 0x014C */ ColliderCylinder collider;
    /* 0x0198 */ LightNode* lightNode;
    /* 0x019C */ LightInfo lightInfo;
    /* 0x01AC */ f32 unk_1AC;
    /* 0x01B0 */ f32 unk_1B0;
    /* 0x01B0 */ f32 unk_1B4;
    /* 0x01B0 */ f32 unk_1B8;
    /* 0x01BC */ f32 unk_1BC;
    /* 0x01C0 */ EnMThunderActionFunc actionFunc;
    /* 0x01C4 */ u16 unk_1C4;
    /* 0x01C6 */ u8 unk_1C6;
    /* 0x01C7 */ u8 unk_1C7;
    /* 0x01C8 */ u8 unk_1C8;
    /* 0x01C9 */ u8 unk_1C9;
    /* 0x01CA */ u8 unk_1CA;
    // Master Sword beam (Enhancements > Items > Master Sword Beam), only used when params == ENMTHUNDER_SWORD_BEAM_PARAMS
    s16 beamTimer;
    s16 beamSpin;
} EnMThunder;

// An En_M_Thunder spawned with these params is a Master Sword beam instead of a spin attack. Vanilla params are
// (magic cost << 8) | (sword + 1), which never reach this value.
#define ENMTHUNDER_SWORD_BEAM_PARAMS 0x7B7B

// Called by Player when a sword swing becomes active: fires a Master Sword beam if the conditions are met.
void EnMThunder_TrySwordBeam(PlayState* play, Player* player);

#endif
