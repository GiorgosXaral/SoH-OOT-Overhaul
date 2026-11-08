#ifndef BOSS_OVERHAUL_H
#define BOSS_OVERHAUL_H

/**
 * Boss Overhaul
 *
 * Shared helpers for the "Boss Overhaul" enhancement (Enhancements > Difficulty > Enemies).
 *
 *  - Double Boss HP:     every main boss needs twice the damage to be defeated.
 *  - Extra Boss Attacks: every main boss gets additional attacks, and fights the player more aggressively as its
 *                        health runs low ("rage" phase). Only vanilla actors, effects, sounds and models are used.
 *
 * Both options are enabled by default and can be toggled independently from the menu.
 */

#include <libultraship/bridge/consolevariablebridge.h>
#include "soh/cvar_prefixes.h"

#define BO_HP_ENABLED() (CVarGetInteger(CVAR_ENHANCEMENT("BossOverhaul.DoubleHP"), 1) != 0)
#define BO_ATTACKS_ENABLED() (CVarGetInteger(CVAR_ENHANCEMENT("BossOverhaul.ExtraAttacks"), 1) != 0)

// Scales a boss health constant (the vanilla value is kept when Double Boss HP is disabled).
#define BO_HP(vanilla) (BO_HP_ENABLED() ? ((vanilla) * 2) : (vanilla))

// Convenience for "rage" checks: true when `cur` is at or below half of the boss' (possibly doubled) max health.
#define BO_BELOW_HALF(cur, vanillaMax) ((cur) <= (BO_HP(vanillaMax) / 2))

#endif
