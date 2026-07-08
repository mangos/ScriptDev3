/**
 * ScriptDev3 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013 ScriptDev2 <http://www.scriptdev2.com/>
 * Copyright (C) 2014-2026 MaNGOS <https://www.getmangos.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * World of Warcraft, and all World of Warcraft or Warcraft art, images,
 * and lore are copyrighted by Blizzard Entertainment, Inc.
 */

#ifndef SC_CREATURE_H
#define SC_CREATURE_H

#include "Chat.h"
#include "DBCStores.h"  // Mostly only used for Lookup access, but a few cases really do use the DBC-Stores

// -------------------------------------------------------------------------
// SpellEntry field accessors (cross-expansion compatibility)
//
// ScriptDev3 is a single repository shared by every mangos core (Zero/One/Two/
// Three/Four). mangos-zero (CLASSIC) aligned its SpellEntry struct field names to
// the build-exact 1.12 .dbd names; the other cores keep the legacy field names
// (and the later cores expose GetXxx() accessor methods). These helpers resolve
// each field per expansion so shared scripts compile and behave identically on
// every core - each build only ever compiles its own branch.
// -------------------------------------------------------------------------
inline uint32 SD3_SpellId(SpellEntry const* pSpell)
{
#if defined (CLASSIC)
    return pSpell->ID;
#elif defined (TBC)
    return pSpell->ID;
#else
    return pSpell->Id;
#endif
}

inline uint32 SD3_SpellManaCost(SpellEntry const* pSpell)
{
#if defined (CATA) || defined (MISTS)
    return pSpell->GetManaCost();
#elif defined (CLASSIC)
    return pSpell->ManaCost;
#elif defined (TBC)
    return pSpell->ManaCost;
#else   // WOTLK
    return pSpell->manaCost;
#endif
}

inline uint32 SD3_SpellPowerType(SpellEntry const* pSpell)
{
#if defined (MISTS)
    return pSpell->GetPowerType();
#elif defined (CLASSIC)
    return pSpell->PowerType;
#elif defined (TBC)
    return pSpell->PowerType;
#else   // WOTLK, CATA
    return pSpell->powerType;
#endif
}

inline uint32 SD3_SpellRangeIndex(SpellEntry const* pSpell)
{
#if defined (MISTS)
    return pSpell->GetRangeIndex();
#elif defined (CLASSIC)
    return pSpell->RangeIndex;
#elif defined (TBC)
    return pSpell->RangeIndex;
#else   // WOTLK, CATA
    return pSpell->rangeIndex;
#endif
}

// Effect-array accessors - used only on the non-CATA/MISTS path (CATA/MISTS
// expose GetEffect*ByIndex() with their own SpellEffect null-checks and stay
// inline at the call sites). CATA/MISTS SpellEntry has no EffectImplicitTargetA/
// EffectApplyAuraName member at all, so these helpers are only defined for the
// cores that have those fields (Zero/One/Two).
#if !defined (CATA) && !defined (MISTS)
inline uint32 SD3_SpellEffectImplicitTargetA(SpellEntry const* pSpell, uint8 index)
{
#if defined (CLASSIC)
    return pSpell->ImplicitTargetA[index];
#elif defined (TBC)
    return pSpell->ImplicitTargetA[index];
#else   // WOTLK
    return pSpell->EffectImplicitTargetA[index];
#endif
}

inline uint32 SD3_SpellEffectApplyAuraName(SpellEntry const* pSpell, uint8 index)
{
#if defined (CLASSIC)
    return pSpell->EffectAura[index];
#elif defined (TBC)
    return pSpell->EffectAura[index];
#else   // WOTLK
    return pSpell->EffectApplyAuraName[index];
#endif
}
#endif

// AreaTriggerEntry / SpellRangeEntry accessors (cross-expansion compatibility).
// mangos-zero (CLASSIC) renamed these DBC fields to their .dbd names; the other
// cores keep the legacy names. Same per-expansion resolution as the SpellEntry
// helpers above so shared scripts compile on every core.
inline uint32 SD3_AreaTriggerId(AreaTriggerEntry const* pAt)
{
#if defined (CLASSIC)
    return pAt->ID;
#else
    return pAt->id;
#endif
}

inline float SD3_SpellRangeMin(SpellRangeEntry const* pRange)
{
#if defined (CLASSIC)
    return pRange->RangeMin;
#elif defined (TBC)
    return pRange->RangeMin;
#else
    return pRange->minRange;
#endif
}

inline float SD3_SpellRangeMax(SpellRangeEntry const* pRange)
{
#if defined (CLASSIC)
    return pRange->RangeMax;
#elif defined (TBC)
    return pRange->RangeMax;
#else
    return pRange->maxRange;
#endif
}

// Spell targets used by SelectSpell
enum SelectTarget
{
    SELECT_TARGET_DONTCARE = 0,  // All target types allowed

    SELECT_TARGET_SELF,  // Only Self casting

    SELECT_TARGET_SINGLE_ENEMY,  // Only Single Enemy
    SELECT_TARGET_AOE_ENEMY,  // Only AoE Enemy
    SELECT_TARGET_ANY_ENEMY,  // AoE or Single Enemy

    SELECT_TARGET_SINGLE_FRIEND,  // Only Single Friend
    SELECT_TARGET_AOE_FRIEND,  // Only AoE Friend
    SELECT_TARGET_ANY_FRIEND,  // AoE or Single Friend
};

// Spell Effects used by SelectSpell
enum SelectEffect
{
    SELECT_EFFECT_DONTCARE = 0,  // All spell effects allowed
    SELECT_EFFECT_DAMAGE,  // Spell does damage
    SELECT_EFFECT_HEALING,  // Spell does healing
    SELECT_EFFECT_AURA,  // Spell applies an aura
};

// Equipment states for creatures
enum SCEquip
{
    EQUIP_NO_CHANGE = -1,
    EQUIP_UNEQUIP = 0
};

/// Documentation of CreatureAI functions can be found in MaNGOS source
// Only list them here again to ensure that the interface between SD3 and the core is not changed unnoticed
struct ScriptedAI : public CreatureAI
{
    public:
        explicit ScriptedAI(Creature* pCreature);
        ~ScriptedAI() {}

        // *************
        // CreatureAI Functions
        // *************

        // == Information about AI ========================
        // Get information about the AI
        void GetAIInformation(ChatHandler& reader) override;

        // == Reactions At =================================

        // Called if IsVisible(Unit* pWho) is true at each relative pWho move
        void MoveInLineOfSight(Unit* pWho) override;

        // Called for reaction at enter to combat if not in combat yet (enemy can be nullptr)
        void EnterCombat(Unit* pEnemy) override;

        // Called at stopping attack by any attacker
        void EnterEvadeMode() override;

        // Called when reached home after MoveTargetHome (in evade)
        void JustReachedHome() override {}

        // Called at any Heal received
        void HealedBy(Unit* /*pHealer*/, uint32& /*uiHealedAmount*/) override {}

        // Called at any Damage to any victim (before damage apply)
        void DamageDeal(Unit* /*pDoneTo*/, uint32& /*uiDamage*/) override {}

        // Called at any Damage from any attacker (before damage apply)
        void DamageTaken(Unit* /*pDealer*/, uint32& /*uiDamage*/) override {}

        // Called at creature death
        void JustDied(Unit* /*pKiller*/) override {}

        // Called when the corpse of this creature gets removed
        void CorpseRemoved(uint32& /*uiRespawnDelay*/) override {}

        // Called when a summoned creature is killed
        void SummonedCreatureJustDied(Creature* /*pSummoned*/) override {}

        // Called at creature killing another unit
        void KilledUnit(Unit* /*pVictim*/) override {}

        // Called when owner of m_creature (if m_creature is PROTECTOR_PET) kills a unit
        void OwnerKilledUnit(Unit* /*pVictim*/) override {}

        // Called when the creature successfully summons a creature
        void JustSummoned(Creature* /*pSummoned*/) override {}

        // Called when the creature successfully summons a gameobject
        void JustSummoned(GameObject* /*pGo*/) override {}

        // Called when a summoned creature gets TemporarySummon::UnSummon ed
        void SummonedCreatureDespawn(Creature* /*pSummoned*/) override {}

        // Called when hit by a spell
        void SpellHit(Unit* /*pCaster*/, const SpellEntry* /*pSpell*/) override {}

        // Called when spell hits creature's target
        void SpellHitTarget(Unit* /*pTarget*/, const SpellEntry* /*pSpell*/) override {}

        // Called when the creature is target of hostile action: swing, hostile spell landed, fear/etc)
        /// This will by default result in reattacking, if the creature has no victim
        void AttackedBy(Unit* pAttacker) override { CreatureAI::AttackedBy(pAttacker); }

        // Called when creature is respawned (for resetting variables)
        void JustRespawned() override;

        // Called at waypoint reached or point movement finished
        void MovementInform(uint32 /*uiMovementType*/, uint32 /*uiData*/) override {}

        // Called if a temporary summoned of m_creature reach a move point
        void SummonedMovementInform(Creature* /*pSummoned*/, uint32 /*uiMotionType*/, uint32 /*uiData*/) override {}

        // Called at text emote receive from player
        void ReceiveEmote(Player* /*pPlayer*/, uint32 /*uiEmote*/) override {}

        // Called at each attack of m_creature by any victim
        void AttackStart(Unit* pWho) override;

        // Called at World update tick
        void UpdateAI(const uint32) override;

        // Called when an AI Event is received
        void ReceiveAIEvent(AIEventType /*eventType*/, Creature* /*pSender*/, Unit* /*pInvoker*/, uint32 /*uiMiscValue*/) override {}

        // == State checks =================================

        // Check if unit is visible for MoveInLineOfSight
        bool IsVisible(Unit* pWho) const override;

        // Called when victim entered water and creature can not enter water
        bool canReachByRangeAttack(Unit* pWho) override { return CreatureAI::canReachByRangeAttack(pWho); }

        // *************
        // Variables
        // *************

        // *************
        // Pure virtual functions
        // *************

        /**
         * This is a SD3 internal function, that every AI must implement
         * Usually used to reset combat variables
         * Called by default on creature evade and respawn
         * In most scripts also called in the constructor of the AI
         */
        void Reset() override {}

        /// Called at creature EnterCombat with an enemy

        /**
         * This is a SD3 internal function
         * Called by default on creature EnterCombat with an enemy
         */
        virtual void Aggro(Unit* /*pWho*/) {}

        // *************
        // AI Helper Functions
        // *************

        // Start movement toward victim
        void DoStartMovement(Unit* pVictim, float fDistance = 0, float fAngle = 0);

        // Start no movement on victim
        void DoStartNoMovement(Unit* pVictim);

        // Stop attack of current victim
        void DoStopAttack();

        // Cast spell by Id
        void DoCast(Unit* pTarget, uint32 uiSpellId, bool bTriggered = false);

        // Cast spell by spell info
        void DoCastSpell(Unit* pTarget, SpellEntry const* pSpellInfo, bool bTriggered = false);

        // Plays a sound to all nearby players
        void DoPlaySoundToSet(WorldObject* pSource, uint32 uiSoundId);

        // Drops all threat to 0%. Does not remove enemies from the threat list
        void DoResetThreat();

        // Teleports a player without dropping threat (only teleports to same map)
        void DoTeleportPlayer(Unit* pUnit, float fX, float fY, float fZ, float fO);

        // Returns friendly unit with the most amount of hp missing from max hp
        Unit* DoSelectLowestHpFriendly(float fRange, uint32 uiMinHPDiff = 1);

        // Returns a list of friendly CC'd units within range
        std::list<Creature*> DoFindFriendlyCC(float fRange);

        // Returns a list of all friendly units missing a specific buff within range
        std::list<Creature*> DoFindFriendlyMissingBuff(float fRange, uint32 uiSpellId);

        // Return a player with at least minimumRange from m_creature
        Player* GetPlayerAtMinimumRange(float fMinimumRange);

        // Spawns a creature relative to m_creature
        Creature* DoSpawnCreature(uint32 uiId, float fX, float fY, float fZ, float fAngle, uint32 uiType, uint32 uiDespawntime);

        // Returns spells that meet the specified criteria from the creatures spell list
        SpellEntry const* SelectSpell(Unit* pTarget, int32 uiSchool, int32 iMechanic, SelectTarget selectTargets, uint32 uiPowerCostMin, uint32 uiPowerCostMax, float fRangeMin, float fRangeMax, SelectEffect selectEffect);

        // Checks if you can cast the specified spell
        bool CanCast(Unit* pTarget, SpellEntry const* pSpell, bool bTriggered = false);

        void SetEquipmentSlots(bool bLoadDefault, int32 iMainHand = EQUIP_NO_CHANGE, int32 iOffHand = EQUIP_NO_CHANGE, int32 iRanged = EQUIP_NO_CHANGE);

        bool EnterEvadeIfOutOfCombatArea(const uint32 uiDiff);

        // a wrapper for aggro list browsing
        Unit *SelectAttackTarget(AttackingTarget type, uint32 position, uint32 spellId = 0, uint32 selectFlags = 0) { return m_creature->SelectAttackingTarget(type, position, spellId, selectFlags); }

    private:
        uint32 m_uiEvadeCheckCooldown;
};

/**
 * @struct Scripted_NoMovementAI
 * @brief AI for creatures that do not move during combat.
 */
struct Scripted_NoMovementAI : public ScriptedAI
{
    Scripted_NoMovementAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        SetCombatMovement(false);
    }

    void GetAIInformation(ChatHandler& reader) override;

    // Called at each attack of m_creature by any victim
    void AttackStart(Unit* pWho) override;
};

#endif
