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

/**
 * ScriptData
 * SDName:      Gilneas City
 * SD%Complete: 0 Work in progress...
 * SDComment:
 * SDCategory:  Gilneas City
 * EndScriptData
 */

/**
 * ContentData
 * npc_prince_liam_greymane_phase1     - Fully Scripted
 * npc_prince_liam_greymane_phase2     - Fully Scripted
 * rampaging_worgen                    - Enrage Spell Scripted
 * frightened_citizen_quest
 * EndContentData
 */

#include "precompiled.h"
#include "escort_ai.h"
#include <cmath>

// Cataclysm carries the full Worgen starting experience. It rests on core
// support only MaNGOS Three has (phase ids and terrain swaps, vehicle seats,
// authored paths, formations, sparring floors), so every other core keeps
// the original Gilneas City scripts in the #else branch below.
#if defined(CATA)
#include "TemporarySummon.h"
#include "Vehicle.h"
#include "movement/MoveSplineInit.h"
#include "movement/MoveSpline.h"

// A storyline bark plays to an AUDIENCE: a living, non-GM player in the
// speaker's own phase, close enough to hear. The stock loops barked into
// the void forever - and bled straight through to phase-blind GM
// listeners running the city battle two phases away.
static bool HasStoryAudience(Creature* pSpeaker)
{
    Map::PlayerList const& lPlayers = pSpeaker->GetMap()->GetPlayers();

    for (Map::PlayerList::const_iterator itr = lPlayers.begin();
         itr != lPlayers.end(); ++itr)
    {
        Player* pPlayer = itr->getSource();

        if (pPlayer && pPlayer->IsAlive() && !pPlayer->isGameMaster() &&
            pPlayer->InSamePhase(pSpeaker) &&
            pPlayer->IsWithinDistInMap(pSpeaker, 45.0f))
        {
            return true;
        }
    }

    return false;
}

/*######
# npc_prince_liam_greymane_phase1
######*/

enum {
    SAY_STORYLINE1        = -1654001,
    SAY_STORYLINE2        = -1654002,
    SAY_STORYLINE3        = -1654003
};

struct npc_prince_liam_greymane_phase1 : public CreatureScript
{
    npc_prince_liam_greymane_phase1() : CreatureScript("npc_prince_liam_greymane_phase1") {}

    struct npc_prince_liam_greymane_phase1AI : public ScriptedAI
    {
        npc_prince_liam_greymane_phase1AI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_uiNpcFlags = pCreature->GetUInt32Value(UNIT_NPC_FLAGS);
            Reset();
        }

        ObjectGuid m_casterGuid;
        uint32 m_uiNpcFlags;
        uint32 m_uiSayStoryDelay;
        uint32 m_uiSayStoryTimer;
        int m_uiSayStoryLast;

        bool m_bCanSayStory;

        void Reset() override
        {
            m_casterGuid.Clear();

            m_uiSayStoryDelay = 15000;
            m_uiSayStoryTimer = m_uiSayStoryDelay;
            m_uiSayStoryLast = 0;

            m_bCanSayStory = true;

            m_creature->SetUInt32Value(UNIT_NPC_FLAGS, m_uiNpcFlags);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_bCanSayStory && HasStoryAudience(m_creature))
            {
                m_uiSayStoryTimer = m_uiSayStoryDelay;

                // Random switch between 3 texts
                switch (m_uiSayStoryLast)
                {
                    case 0: DoScriptText(SAY_STORYLINE1, m_creature); m_uiSayStoryLast++; break;
                    case 1: DoScriptText(SAY_STORYLINE2, m_creature); m_uiSayStoryLast++; break;
                    case 2: DoScriptText(SAY_STORYLINE3, m_creature); m_uiSayStoryLast = 0; m_uiSayStoryTimer = 60000; break;
                    //case 3: DoScriptText(SAY_HELP4, m_creature); break;
                }

                m_bCanSayStory = false;
            }

            if (m_uiSayStoryTimer < uiDiff)
            {
                m_bCanSayStory = true;
                m_uiSayStoryTimer = m_uiSayStoryDelay;
            }
            else
            {
                m_uiSayStoryTimer -= uiDiff;
            }
        }

    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_prince_liam_greymane_phase1AI(pCreature);
    }
};

/*######
# npc_prince_liam_greymane_phase2
######*/

enum {
    SAY_STORYLINE4        = -1654004,
    SAY_STORYLINE5        = -1654005,
    SAY_STORYLINE6        = -1654006,
    SAY_STORYLINE7        = -1654007,
    SAY_STORYLINE8        = -1654008,

    SAY_STORY_DELAY        = 30000,
    SPELL_SHOOT            = 50092,
    SPELL_CNOCKING        = 67869,

    QUEST_EVAQUATE_THE_MERCHANT_SQUARE = 14098
};

struct npc_prince_liam_greymane_phase2 : public CreatureScript
{
    npc_prince_liam_greymane_phase2() : CreatureScript("npc_prince_liam_greymane_phase2") {}

    struct npc_prince_liam_greymane_phase2AI : public ScriptedAI
    {
        npc_prince_liam_greymane_phase2AI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            Reset();
        }

        uint32 m_uiNpcFlags;
        uint32 m_uiSayStoryTimer;

        Unit* lastVictim;

        int m_uiSayStoryLast;

        bool m_bCanSayStory;
        bool IsSelfRooted;

        void Reset() override
        {
            m_uiSayStoryTimer = SAY_STORY_DELAY;
            m_uiSayStoryLast = 0;
            lastVictim = nullptr;

            m_bCanSayStory = true;
            IsSelfRooted = false;

        }

        void Aggro(Unit* who) override
        {
            if (!InMeleeReach(*m_creature, *who) && !m_creature->CanUseEquippedWeapon(RANGED_ATTACK))
            {
                IsSelfRooted = true;
            }
        }

        void AttackedBy(Unit* pAttacker) override
        {
            // Check if Liam is attacking who attack he and don't jump on multiple attackers
            if (m_creature->getVictim() && (m_creature->getVictim() == pAttacker || lastVictim == pAttacker))
            {
                return;
            }

            if (m_creature->IsFriendlyTo(pAttacker))
            {
                return;
            }

            lastVictim = pAttacker;
            AttackStart(pAttacker);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // Ready to say emote
            if (m_bCanSayStory && HasStoryAudience(m_creature))
            {
                m_uiSayStoryTimer = SAY_STORY_DELAY;

                // Sequence switch between 3 texts
                switch (m_uiSayStoryLast)
                {
                    case 0: DoScriptText(SAY_STORYLINE4, m_creature); m_uiSayStoryLast++; break;
                    case 1: DoScriptText(SAY_STORYLINE5, m_creature); m_uiSayStoryLast++; break;
                    case 2: DoScriptText(SAY_STORYLINE6, m_creature); m_uiSayStoryLast++; break;
                    case 3: DoScriptText(SAY_STORYLINE7, m_creature); m_uiSayStoryLast++; break;
                    case 4: DoScriptText(SAY_STORYLINE8, m_creature); m_uiSayStoryLast = 0; break;
                    //case 3: DoScriptText(SAY_HELP4, m_creature); break;
                }

                m_bCanSayStory = false;
            }

            if (m_uiSayStoryTimer < uiDiff)
            {
                m_bCanSayStory = true;
                m_uiSayStoryTimer = SAY_STORY_DELAY;
            }
            else
            {
                m_uiSayStoryTimer -= uiDiff;
            }

            // Return since we have no target
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Equipped ranged weapon usable and not close to victim
            if (CombatDistanceBetween(*m_creature, *m_creature->getVictim(), false) > 0 && m_creature->CanUseEquippedWeapon(RANGED_ATTACK) )
            {
                // Make sure our attack is ready
                if (m_creature->isAttackReady(RANGED_ATTACK))
                {
                    // Use spell instead of normal ranged attack that seem not working. TODO: check ranged attack!
                    DoCastSpellIfCan(m_creature->getVictim(), SPELL_SHOOT);
                    //m_creature->AttackerStateUpdate(m_creature->getVictim(),RANGED_ATTACK);
                    m_creature->resetAttackTimer(RANGED_ATTACK);
                }

            }
            else if (InMeleeReach(*m_creature, *m_creature->getVictim()))
            {
                // If we are within range melee the target
                // Make sure our attack is ready
                if (m_creature->isAttackReady())
                {
                    printf("---> %s melee is ready %s! \n",m_creature->GetName(), m_creature->getVictim()->GetName());
                    m_creature->AttackerStateUpdate(m_creature->getVictim());

                    m_creature->resetAttackTimer();
                }

            }
            else if (IsSelfRooted)
            {
                // Cancel our current spell and then allow movement again
                m_creature->InterruptNonMeleeSpells(false);
                IsSelfRooted = false;
            }

        }

    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_prince_liam_greymane_phase2AI(pCreature);
    }
};

/*######
# rampaging_worgen
######*/

enum {
    SPELL_ENRAGE          = 8599,
    SPELL_ENRAGE_DELAY    = 120000
};

struct rampaging_worgen : public CreatureScript
{
    rampaging_worgen() : CreatureScript("rampaging_worgen") {}

    struct rampaging_worgenAI : public ScriptedAI
    {
        rampaging_worgenAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 spellCooldown;      // This variable acts like the global cooldown that players have (2 minute)
        bool IsSelfRooted;
        bool m_bWakeOnFirstTick;   ///< Summoned: go for whoever knocked, once live

        void Reset() override
        {
            spellCooldown = 0;
            IsSelfRooted = false;

            // The template spawns this one asleep: UNIT_FLAG_PASSIVE so it
            // ignores everything around it, and UNIT_FLAG_OOC_NOT_ATTACKABLE
            // so nothing can hit it first. That is deliberate - the same entry
            // stands about as scenery - so only the copy a Merchant Square door
            // throws out gets woken.
            m_bWakeOnFirstTick = m_creature->IsTemporarySummon();

            if (m_bWakeOnFirstTick)
            {
                m_creature->RemoveFlag(UNIT_FIELD_FLAGS,
                    UNIT_FLAG_PASSIVE | UNIT_FLAG_OOC_NOT_ATTACKABLE);
            }
        }

        void Aggro(Unit* who) override
        {
            if (!InMeleeReach(*m_creature, *who))
            {
                IsSelfRooted = true;
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            // Picking a target has to wait for the first tick. Reset() runs
            // while the AI is still being built inside AIM_Initialize, before
            // the creature is in the world, and attacking from there sends the
            // assistance call walking a grid this creature is not in yet.
            if (m_bWakeOnFirstTick)
            {
                m_bWakeOnFirstTick = false;

                TemporarySummon* pTemporary = (TemporarySummon*)m_creature;

                if (Player* pPlayer = m_creature->GetMap()->GetPlayer(pTemporary->GetSummonerGuid()))
                {
                    if (pPlayer->IsAlive())
                    {
                        AttackStart(pPlayer);
                    }
                }
            }

            // Always decrease our spell cooldown first
            if (spellCooldown > diff)
            {
                spellCooldown -= diff;
            }
            else
            {
                spellCooldown = 0;
            }

            // Enrage timer (only enrage when we are alive and  in combat)
            if (m_creature->IsInCombat() && m_creature->IsAlive())
            {
                // Cast enrage spell if less than 70% hp ONLY and not casted already
                if (m_creature->GetHealthPercent() < 70.0f && !spellCooldown)
                {
                    DoCastSpellIfCan(m_creature, SPELL_ENRAGE);
                    spellCooldown = SPELL_ENRAGE_DELAY;
                }

            }

            // Return since we have no target
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Return if we already cast a spell
            if (m_creature->IsNonMeleeSpellCasted(false))
            {
                return;
            }

            // If we are within range melee the target
            if (InMeleeReach(*m_creature, *m_creature->getVictim()))
            {
                // Make sure our attack is ready
                if (m_creature->isAttackReady())
                {
                    m_creature->AttackerStateUpdate(m_creature->getVictim());

                    m_creature->resetAttackTimer();
                }
            }
            else if (IsSelfRooted)
            {
                // Cancel our current spell and then allow movement again
                m_creature->InterruptNonMeleeSpells(false);
                IsSelfRooted = false;
            }

        }

    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new rampaging_worgenAI(pCreature);
    }
};

/** ######
 *  ## creature_frightened_citizen_quest
 *  ######*/
enum {
    SAY_ON_ESCAPE1             = -1654009,
    SAY_ON_ESCAPE2             = -1654010,
    SAY_ON_ESCAPE3             = -1654011,
    SAY_ON_ESCAPE4             = -1654012,
    SAY_ON_ESCAPE5             = -1654013,
    SAY_ON_ESCAPE6             = -1654014,
    SAY_ON_ESCAPE7             = -1654015,
    SAY_ON_ESCAPE_DELAY        = 12000,

    QUEST_EVACUATE_MERCHANT_SQUARE  = 14098,
    GO_MERCHANT_SQUARE_DOOR         = 195327,


    // Blizzard's own pair: most doors give up a citizen alone, some turn out a
    // rampaging worgen along with them.
    SPELL_JUST_CITIZEN              = 68087,
    SPELL_CITIZEN_AND_WORGEN        = 68070,
    SPELL_SUMMON_WORGEN             = 80281,

    // One marker stands at each of the fourteen doors, and the quest counts
    // these rather than the door itself - which is why knocking alone never
    // credited anything.
    NPC_EVACUATION_MARKER_FIRST     = 35830,
    NPC_EVACUATION_MARKER_NEAR      = 35010,
    NPC_EVACUATION_MARKER_FAR       = 35011,

    POINT_MARKER_FIRST              = 1,
    POINT_MARKER_NEAR               = 2,
    POINT_MARKER_FAR                = 3,

    CITIZEN_CREDIT_DELAY            = 1000,
    CITIZEN_DEPART_DELAY            = 2000,
    // Gone while still running, a few strides past the edge of the square.
    // Waiting for the far marker parks them all on one spot.
    CITIZEN_EXIT_DESPAWN            = 5000,
};

struct frightened_citizen_quest : public CreatureScript
{
    frightened_citizen_quest() : CreatureScript("frightened_citizen_quest") {}

    struct frightened_citizen_questAI : public ScriptedAI
    {
        frightened_citizen_questAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiSayOnEscapeTimer;
        bool m_bCanSayOnEscape;

        bool m_bEvacuating;      ///< Knocked out of a door, not standing scenery
        uint32 m_uiCreditTimer;  ///< Runs once the citizen reaches its doorway marker
        uint32 m_uiDepartTimer;  ///< A beat after that, before it runs for the exit
        uint32 m_uiExitTimer;    ///< Despawns mid-run once the exit leg has begun

        void Reset() override
        {
            m_uiSayOnEscapeTimer = SAY_ON_ESCAPE_DELAY;
            m_bCanSayOnEscape = true;

            m_uiCreditTimer = 0;
            m_uiDepartTimer = 0;
            m_uiExitTimer = 0;

            // Both entries are also spawned around the square as scenery, and
            // those keep their idle shouting. Only the ones a door summons
            // evacuate.
            m_bEvacuating = m_creature->IsTemporarySummon();

            if (!m_bEvacuating)
            {
                return;
            }

            m_bCanSayOnEscape = false;

            // Head for the marker at this doorway. Blizzard put one at each
            // door, so the nearest is this citizen's own.
            if (Creature* pMarker = GetClosestCreatureWithEntry(
                    m_creature, NPC_EVACUATION_MARKER_FIRST, 20.0f))
            {
                m_creature->GetMotionMaster()->MovePoint(POINT_MARKER_FIRST,
                    pMarker->Where().X(), pMarker->Where().Y(), pMarker->Where().Z());
            }
        }

        /// Sends the citizen on towards the edge of the square, and out.
        void RunForTheExit()
        {
            m_uiExitTimer = CITIZEN_EXIT_DESPAWN;

            if (Creature* pFar = GetClosestCreatureWithEntry(
                    m_creature, NPC_EVACUATION_MARKER_FAR, 50.0f))
            {
                m_creature->GetMotionMaster()->MovePoint(POINT_MARKER_FAR,
                    pFar->Where().X(), pFar->Where().Y(), pFar->Where().Z());
            }
            else if (Creature* pNear = GetClosestCreatureWithEntry(
                         m_creature, NPC_EVACUATION_MARKER_NEAR, 100.0f))
            {
                m_creature->GetMotionMaster()->MovePoint(POINT_MARKER_NEAR,
                    pNear->Where().X(), pNear->Where().Y(), pNear->Where().Z());
            }
            else
            {
                m_creature->ForcedDespawn();
            }
        }

        /// Second leg from a near marker. The far marker is well over 50 yards
        /// from every near one, so the short search above would only ever find
        /// the near marker the citizen is already standing on - and loop there.
        void RunForTheFarMarker()
        {
            if (Creature* pFar = GetClosestCreatureWithEntry(
                    m_creature, NPC_EVACUATION_MARKER_FAR, 500.0f))
            {
                m_creature->GetMotionMaster()->MovePoint(POINT_MARKER_FAR,
                    pFar->Where().X(), pFar->Where().Y(), pFar->Where().Z());
            }
            else
            {
                m_creature->ForcedDespawn();
            }
        }

        /// Credits whoever knocked, and lets the citizen say its piece.
        void CreditRescuer()
        {
            if (!m_creature->IsTemporarySummon())
            {
                return;
            }

            TemporarySummon* pTemporary = (TemporarySummon*)m_creature;

            if (Player* pPlayer = m_creature->GetMap()->GetPlayer(pTemporary->GetSummonerGuid()))
            {
                // The quest counts the doorway marker, never the door or the
                // citizen, so this is the only thing that advances it.
                pPlayer->KilledMonsterCredit(NPC_EVACUATION_MARKER_FIRST);
            }

            switch (urand(0, 6))
            {
                case 0: DoScriptText(SAY_ON_ESCAPE1, m_creature); break;
                case 1: DoScriptText(SAY_ON_ESCAPE2, m_creature); break;
                case 2: DoScriptText(SAY_ON_ESCAPE3, m_creature); break;
                case 3: DoScriptText(SAY_ON_ESCAPE4, m_creature); break;
                case 4: DoScriptText(SAY_ON_ESCAPE5, m_creature); break;
                case 5: DoScriptText(SAY_ON_ESCAPE6, m_creature); break;
                case 6: DoScriptText(SAY_ON_ESCAPE7, m_creature); break;
            }
        }

        void MovementInform(uint32 uiMovementType, uint32 uiPointId) override
        {
            if (uiMovementType != POINT_MOTION_TYPE || !m_bEvacuating)
            {
                return;
            }

            switch (uiPointId)
            {
                case POINT_MARKER_FIRST:
                    m_uiCreditTimer = CITIZEN_CREDIT_DELAY;
                    break;
                case POINT_MARKER_NEAR:
                    RunForTheFarMarker();
                    break;
                case POINT_MARKER_FAR:
                    m_creature->ForcedDespawn();
                    break;
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_bEvacuating)
            {
                if (m_uiCreditTimer)
                {
                    if (m_uiCreditTimer <= uiDiff)
                    {
                        m_uiCreditTimer = 0;
                        m_uiDepartTimer = CITIZEN_DEPART_DELAY;
                        CreditRescuer();
                    }
                    else
                    {
                        m_uiCreditTimer -= uiDiff;
                    }
                }
                else if (m_uiDepartTimer)
                {
                    if (m_uiDepartTimer <= uiDiff)
                    {
                        m_uiDepartTimer = 0;
                        RunForTheExit();
                    }
                    else
                    {
                        m_uiDepartTimer -= uiDiff;
                    }
                }
                else if (m_uiExitTimer)
                {
                    if (m_uiExitTimer <= uiDiff)
                    {
                        m_uiExitTimer = 0;
                        m_creature->ForcedDespawn();
                    }
                    else
                    {
                        m_uiExitTimer -= uiDiff;
                    }
                }

                return;
            }

            // Ready to say emote
            if (m_bCanSayOnEscape)
            {
                m_uiSayOnEscapeTimer = SAY_ON_ESCAPE_DELAY;

                // switch between 7 texts randomly
                switch (urand(0, 6))
                {
                    case 0: DoScriptText(SAY_ON_ESCAPE1, m_creature); break;
                    case 1: DoScriptText(SAY_ON_ESCAPE2, m_creature); break;
                    case 2: DoScriptText(SAY_ON_ESCAPE3, m_creature); break;
                    case 3: DoScriptText(SAY_ON_ESCAPE4, m_creature); break;
                    case 4: DoScriptText(SAY_ON_ESCAPE5, m_creature); break;
                    case 5: DoScriptText(SAY_ON_ESCAPE6, m_creature); break;
                    case 6: DoScriptText(SAY_ON_ESCAPE7, m_creature); break;
                }

                m_bCanSayOnEscape = false;
            }

            if (m_uiSayOnEscapeTimer < uiDiff)
            {
                m_bCanSayOnEscape = true;
                m_uiSayOnEscapeTimer = SAY_ON_ESCAPE_DELAY;
            }
            else
            {
                m_uiSayOnEscapeTimer -= uiDiff;
            }

        }

    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new frightened_citizen_questAI(pCreature);
    }
};

/*######
## go_merchant_square_door
######*/

// Knocking on a Merchant Square door should turn a citizen out of the house,
// and about a third of the time a worgen comes with them.
//
// None of that is in the game object itself: 195327 is a goober whose spell
// field is empty, and the credit a goober hands out is for its own entry,
// while the quest counts creature 35830. So the door opened, played its
// animation, and could never advance anything. The two summon spells below are
// Blizzard's own; casting them from the player makes the player the summoner,
// which is what lets the citizen credit them on arrival.
struct go_merchant_square_door : public GameObjectScript
{
    go_merchant_square_door() : GameObjectScript("go_merchant_square_door") {}

    bool OnUse(Player* pPlayer, GameObject* pGo) override
    {
        if (!pPlayer || !pGo)
        {
            return false;
        }

        if (pPlayer->GetQuestStatus(QUEST_EVACUATE_MERCHANT_SQUARE) != QUEST_STATUS_INCOMPLETE)
        {
            return false;
        }

        if (urand(0, 2))
        {
            pPlayer->CastSpell(pPlayer, SPELL_JUST_CITIZEN, true);
        }
        else
        {
            pPlayer->CastSpell(pPlayer, SPELL_CITIZEN_AND_WORGEN, true);

            // 68070 is meant to reach the worgen half through a TRIGGER_SPELL
            // effect, but that effect's target resolves to the door rather
            // than to a unit, and EffectTriggerSpell drops every non-unit
            // case. Casting 80281 directly gives the pair Blizzard intended.
            pPlayer->CastSpell(pPlayer, SPELL_SUMMON_WORGEN, true);
        }

        // false: the core still runs its own goober handling, so the door
        // opens and closes on its own timer as before.
        return false;
    }
};

/*######
# npc_gilnean_crow
######*/

// Lieutenant Walden's body is the first thing Lockdown! sends the player to,
// and nine crows are staged on it. They should scatter as the player closes
// in, wheel overhead a moment, then climb away into the sky. The same staging
// is repeated at the bridge crossing below King Genn's camp.
//
// Entry 50260 is spawned 96 times across Gilneas; the rest are ambient scenery
// spread over the zone and must keep their idle wander. The gate in Reset()
// keeps this behaviour to the birds actually standing on a staged flock.
enum
{
    CROW_STAGED_RANGE       = 15,      ///< Covers one flock and no more
    CROW_FLUSH_RANGE        = 12,      ///< How close the player gets before they break
    CROW_CIRCLE_POINTS      = 16,      ///< Control points of the lap, not stops

    CROW_TAKEOFF_DELAY_MAX  = 1100,    ///< Stagger, ms, so the flock breaks raggedly
    CROW_CIRCLE_TIME        = 2000,    ///< Time overhead, ms, before they leave

    POINT_ID_TAKEOFF        = 1,
};

/// A flock staged on something worth scavenging, and the point it circles.
struct CrowStagingSite
{
    float fX;
    float fY;
};

// A bird belongs to a site if it spawned within CROW_STAGED_RANGE of one.
// The flocks are far enough apart that the ranges cannot overlap, so the first
// match is the only match.
static const CrowStagingSite CROW_STAGING_SITES[] =
{
    { -1405.0f, 1447.0f },             ///< Lieutenant Walden's body, Lockdown!
    { -1616.5f, 1311.8f },             ///< The bridge below King Genn's camp
    { -1799.4f, 1549.2f },             ///< The yard north of the Military District
    { -1834.7f, 2289.6f },             ///< The stagecoach at Duskhaven, In Need of Ingredients
    { -1845.8f, 2323.7f },             ///< The road above the stagecoach
    { -1920.9f, 2404.9f },             ///< The invasion hill at Duskhaven
    { -1782.2f, 2382.0f }              ///< The rise east of Duskhaven
};

// Every crow gets its own orbit. Sharing one radius, height and starting angle
// is what makes nine birds read as one machine: they converge into a single
// ring and hold formation. Spreading these per bird turns the same code into a
// loose, milling flock.
static const float CROW_RADIUS_MIN  = 3.0f;
static const float CROW_RADIUS_MAX  = 8.0f;
static const float CROW_HEIGHT_MIN  = 3.5f;   ///< Low: they circle carrion, not a mountain
static const float CROW_HEIGHT_MAX  = 7.0f;
static const float CROW_BOB         = 0.6f;   ///< Amplitude of the rise and fall
static const float CROW_SPEED_MIN   = 3.4f;   ///< Varied, so equal orbits still drift apart
static const float CROW_SPEED_MAX   = 4.8f;

// The break itself. A bird flushed off a corpse throws itself away from the
// thing that startled it and gains height as it goes; climbing straight up
// from the spot reads as an elevator, not as flight. So the first hop is
// outward as well as upward, and carries the whole climb: it ends on the ring,
// which lets the circling spline below start clean.
static const float CROW_LAUNCH_MIN  = 3.5f;   ///< Outward distance of the break
static const float CROW_LAUNCH_MAX  = 6.0f;

// Leaving. Once they have wheeled overhead they climb out hard and are gone;
// the despawn is timed to land after the climb, by which point the bird is far
// enough up and out that nobody sees it wink out.
static const float CROW_DEPART_SPEED_MIN = 6.0f;  ///< Faster than the lap, not a rocket
static const float CROW_DEPART_SPEED_MAX = 8.0f;
static const float CROW_DEPART_DIST_MIN = 60.0f;
static const float CROW_DEPART_DIST_MAX = 90.0f;
static const float CROW_DEPART_RISE_MIN = 45.0f;
static const float CROW_DEPART_RISE_MAX = 70.0f;
static const float CROW_DEPART_SPREAD   = 0.6f;  ///< Bearing fan, radians

struct npc_gilnean_crow : public CreatureScript
{
    npc_gilnean_crow() : CreatureScript("npc_gilnean_crow") {}

    struct npc_gilnean_crowAI : public ScriptedAI
    {
        npc_gilnean_crowAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        bool m_bOnCorpse;         ///< Staged on Walden, rather than ambient scenery
        bool m_bFlushed;          ///< Startled; never flushes twice
        bool m_bAirborne;         ///< Past the break, climbing or circling
        bool m_bDeparting;        ///< Gone: climbing out, despawn already armed
        uint32 m_uiTakeoffTimer;  ///< Stagger before this bird breaks
        uint32 m_uiCircleTimer;   ///< Time left overhead; zero until the lap starts
        float m_fRadius;          ///< This bird's own orbit
        float m_fHeight;
        float m_fAngle;           ///< Where it joins the lap
        float m_fAngleStep;       ///< Signed, so birds circle both ways
        float m_fSpeed;
        float m_fCentreZ;
        float m_fFleeX;           ///< Unit vector away from whoever startled it
        float m_fFleeY;
        float m_fLaunchDist;      ///< How far out the break carries it
        float m_fStageX;          ///< Centre of this bird's flock, and of its lap
        float m_fStageY;

        void Reset() override
        {
            m_bOnCorpse = false;
            m_fStageX = 0.0f;
            m_fStageY = 0.0f;

            for (uint32 i = 0; i < sizeof(CROW_STAGING_SITES) / sizeof(CROW_STAGING_SITES[0]); ++i)
            {
                const float fDX = m_creature->Where().X() - CROW_STAGING_SITES[i].fX;
                const float fDY = m_creature->Where().Y() - CROW_STAGING_SITES[i].fY;

                if ((fDX * fDX + fDY * fDY) <=
                    float(CROW_STAGED_RANGE * CROW_STAGED_RANGE))
                {
                    m_bOnCorpse = true;
                    m_fStageX = CROW_STAGING_SITES[i].fX;
                    m_fStageY = CROW_STAGING_SITES[i].fY;
                    break;
                }
            }

            m_bFlushed = false;
            m_bAirborne = false;
            m_bDeparting = false;
            m_uiTakeoffTimer = 0;
            m_uiCircleTimer = 0;
            m_fCentreZ = m_creature->Where().Z();

            // Everything below is per bird. Shared values are what made the
            // flock look mechanical.
            m_fRadius = frand(CROW_RADIUS_MIN, CROW_RADIUS_MAX);
            m_fHeight = frand(CROW_HEIGHT_MIN, CROW_HEIGHT_MAX);
            m_fAngle  = frand(0.0f, 2.0f * M_PI_F);
            m_fSpeed  = frand(CROW_SPEED_MIN, CROW_SPEED_MAX);
            m_fLaunchDist = frand(CROW_LAUNCH_MIN, CROW_LAUNCH_MAX);

            // Overwritten the moment something actually startles it.
            m_fFleeX = 0.0f;
            m_fFleeY = 0.0f;

            m_fAngleStep = (2.0f * M_PI_F) / float(CROW_CIRCLE_POINTS);
            if (urand(0, 1))
            {
                m_fAngleStep = -m_fAngleStep;
            }
        }

        /// Puts the crow on one closed spline it then flies without further input.
        void LaunchCircle()
        {
            // Join the lap wherever the break actually left the bird. Launch()
            // overwrites the first vertex of the path with the creature's live
            // position, so the ring has to be built around that point or the
            // spline starts by yanking it somewhere else.
            const float fDX = m_creature->Where().X() - m_fStageX;
            const float fDY = m_creature->Where().Y() - m_fStageY;

            m_fAngle = atan2(fDY, fDX);
            m_fRadius = sqrt(fDX * fDX + fDY * fDY);

            // Where the break landed sets the orbit, within reason.
            if (m_fRadius < CROW_RADIUS_MIN)
            {
                m_fRadius = CROW_RADIUS_MIN;
            }
            else if (m_fRadius > CROW_RADIUS_MAX)
            {
                m_fRadius = CROW_RADIUS_MAX;
            }

            Movement::PointsArray path;
            path.reserve(CROW_CIRCLE_POINTS);

            for (uint32 i = 0; i < CROW_CIRCLE_POINTS; ++i)
            {
                const float fA = m_fAngle + m_fAngleStep * float(i);

                // A slow rise and fall, twice per lap. The height offset has to
                // be a function of position: a per-point random jitter is white
                // noise, so consecutive heights are unrelated and the bird
                // sawtooths. Phrasing it against the entry angle also puts the
                // offset at exactly zero on the first and last vertex, so the
                // seam where the cycle closes is invisible.
                const float fBob = CROW_BOB * sin(2.0f * (fA - m_fAngle));

                path.push_back(Geometry::Vector3(
                    m_fStageX + m_fRadius * cos(fA),
                    m_fStageY + m_fRadius * sin(fA),
                    m_fCentreZ + m_fHeight + fBob));
            }

            // One cyclic Catmull-Rom spline, launched once and never revisited.
            // Issuing a fresh MovePoint per vertex instead would make each leg
            // a separate spline the bird has to arrive at and stop on, sixteen
            // times a lap, and those stops read as a stutter. Handing over the
            // whole closed curve lets the client interpolate it continuously.
            Movement::MoveSplineInit init(*m_creature);
            init.MovebyPath(path);
            init.SetSmooth();
            init.SetFly();
            init.SetCyclic();
            init.SetVelocity(m_fSpeed);
            init.Launch();

            // Overhead from here. The spline runs unattended, so this timer is
            // the only thing left to tick.
            m_uiCircleTimer = CROW_CIRCLE_TIME;
        }

        /// Takes the crow out of the scene: a hard climb away, then despawn.
        void Depart()
        {
            m_bDeparting = true;

            const float fDX = m_creature->Where().X() - m_fStageX;
            const float fDY = m_creature->Where().Y() - m_fStageY;

            // Outward from wherever it had got to on the lap, fanned a little
            // so the flock disperses instead of filing off along one line.
            const float fBearing = atan2(fDY, fDX) +
                                   frand(-CROW_DEPART_SPREAD, CROW_DEPART_SPREAD);
            const float fDist = frand(CROW_DEPART_DIST_MIN, CROW_DEPART_DIST_MAX);
            const float fRise = frand(CROW_DEPART_RISE_MIN, CROW_DEPART_RISE_MAX);

            Movement::MoveSplineInit init(*m_creature);
            init.MoveTo(m_creature->Where().X() + fDist * cos(fBearing),
                        m_creature->Where().Y() + fDist * sin(fBearing),
                        m_creature->Where().Z() + fRise);
            init.SetFly();
            init.SetVelocity(frand(CROW_DEPART_SPEED_MIN, CROW_DEPART_SPEED_MAX));

            // Launch() hands back the flight time, so the bird can be removed
            // at the exact moment it finishes the climb - high, distant, and
            // nowhere near anyone's screen.
            const int32 iFlightTime = init.Launch();

            m_creature->ForcedDespawn(iFlightTime > 0 ? uint32(iFlightTime) : 1);
        }

        void MoveInLineOfSight(Unit* pWho) override
        {
            if (!m_bOnCorpse || m_bFlushed || !pWho)
            {
                return;
            }

            if (pWho->GetTypeId() != TYPEID_PLAYER)
            {
                return;
            }

            if (m_creature->Where().DistanceTo(pWho->Where()) > float(CROW_FLUSH_RANGE))
            {
                return;
            }

            // Which way it throws itself: directly away from the player, taken
            // now while we still have them, rather than at takeoff.
            const float fAwayX = m_creature->Where().X() - pWho->Where().X();
            const float fAwayY = m_creature->Where().Y() - pWho->Where().Y();
            const float fLen = sqrt(fAwayX * fAwayX + fAwayY * fAwayY);

            if (fLen < 0.1f)
            {
                // Player is standing on it; no meaningful direction to flee.
                const float fPick = frand(0.0f, 2.0f * M_PI_F);
                m_fFleeX = cos(fPick);
                m_fFleeY = sin(fPick);
            }
            else
            {
                m_fFleeX = fAwayX / fLen;
                m_fFleeY = fAwayY / fLen;
            }

            // Startled, but not yet moving: a short per-bird delay means the
            // flock comes off the body raggedly instead of as one snap.
            m_bFlushed = true;
            m_uiTakeoffTimer = urand(1, CROW_TAKEOFF_DELAY_MAX);
        }

        void MovementInform(uint32 uiMovementType, uint32 uiData) override
        {
            // The break is the only leg that reports; everything after it is
            // one continuous spline that never arrives anywhere.
            if (uiMovementType != POINT_MOTION_TYPE || uiData != POINT_ID_TAKEOFF)
            {
                return;
            }

            LaunchCircle();
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // Critters, so there is no combat to run. Two timers is the whole
            // of it: the stagger before the break, and the spell spent overhead
            // before leaving. The circling itself needs no upkeep.
            if (!m_bFlushed || m_bDeparting)
            {
                return;
            }

            if (m_bAirborne)
            {
                // Still climbing out of the break; the lap has not begun.
                if (!m_uiCircleTimer)
                {
                    return;
                }

                if (m_uiCircleTimer > uiDiff)
                {
                    m_uiCircleTimer -= uiDiff;
                    return;
                }

                m_uiCircleTimer = 0;
                Depart();
                return;
            }

            if (m_uiTakeoffTimer > uiDiff)
            {
                m_uiTakeoffTimer -= uiDiff;
                return;
            }

            m_bAirborne = true;

            // Break off the idle wander before anything else, or the random
            // generator keeps fighting the climb.
            m_creature->GetMotionMaster()->MoveIdle();
            m_creature->SetLevitate(true);

            // Out and up together, ending level with the ring: a cyclic spline
            // repeats its first vertex every lap, so finishing the climb here
            // is what stops the bird dipping back down to the launch height
            // once per circuit.
            m_creature->GetMotionMaster()->MovePoint(
                POINT_ID_TAKEOFF,
                m_creature->Where().X() + m_fFleeX * m_fLaunchDist,
                m_creature->Where().Y() + m_fFleeY * m_fLaunchDist,
                m_fCentreZ + m_fHeight,
                false);
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_gilnean_crowAI(pCreature);
    }
};

/*######
# npc_prison_roof_worgen
######*/

// `By the Skin of His Teeth` (14154) stages its waves on the rooftops around
// Stoneward Prison, then leaves them there. The quest's summon script drops
// them 53 to 67 yards from Lord Darius Crowley - well outside any aggro
// radius - so they landed on neighbouring roofs and watched the fight they
// were summoned for. They are meant to come across the gap.
//
// Pathing cannot carry them: the roofs are separate structures with drops
// between them. A jump is the right primitive anyway, because it is what the
// scene calls for, and it ignores navmesh connectivity entirely.
//
// Entries 35167, 35170, 35188 and 35456 also stand around Gilneas as ordinary
// spawns, so the leap is gated on IsTemporarySummon(): only the copies the
// quest script throws out cross to the roof.
enum
{
    ROOF_LEAP_DELAY_MIN     = 700,     ///< Stagger, ms, so a wave lands ragged
    ROOF_LEAP_DELAY_MAX     = 2200,

    ROOF_ACQUIRE_INTERVAL   = 1000,    ///< ms between looks for someone to fight
    ROOF_MAX_ON_PLAYER      = 1,       ///< How many may press the player at once

    NPC_TOBIAS_MISTMANTLE   = 35124,   ///< Holds the roof alongside Crowley

    POINT_ID_ROOF_LEAP      = 1,
    POINT_ID_ROOF_RUN       = 2,
};

// The roof is 84 yards end to end, so a worgen landing at one end can be most
// of that from someone fighting at the other. The search has to cover it.
static const float ROOF_AGGRO_RANGE = 90.0f;  ///< How far it will look for a defender

// They do not land in the defenders' laps. The jump carries a worgen onto the
// roof short of them and it covers the last stretch on foot: an arc that ended
// in melee would read as a teleport, and the ones staged furthest out would
// cross sixty yards in a single impossible bound.
//
// The roof is a long, narrow strip rather than a disc. Surveyed in game at its
// two far ends and one side edge, all sitting on Z 52.287:
//     (-1705.16, 1475.66) and (-1646.33, 1415.91)   84 yards apart
//     (-1669.23, 1451.18)                           8.4 yards off the centreline
// so it measures roughly 84 by 14, with Crowley near the middle. An offset
// applied radially therefore runs out of roof sideways long before it does
// lengthways, which dropped anything arriving across the short axis into mid
// air. The landing offset is taken ALONG the strip for that reason, and the
// scatter across it is deliberately small.
static const float ROOF_AXIS_X      = 0.7016f;  ///< Unit vector down the roof's length
static const float ROOF_AXIS_Y      = -0.7126f;
static const float ROOF_ALONG_MAX   = 16.0f;  ///< Landing offset along the strip
static const float ROOF_SIDE_MAX    = 3.0f;   ///< Scatter across it, kept well inside the edge
static const float ROOF_RUN_IN_DIST = 12.0f;  ///< Closer than this and it just runs
static const float ROOF_LEAP_SPEED  = 16.0f;  ///< Horizontal, reads as a charge not a float
static const float ROOF_LEAP_ARC    = 10.0f;  ///< Clearance over the gap

/// A staging site whose landing had to be surveyed rather than derived.
struct RoofLeapOverride
{
    float fStageX;                 ///< Where the quest script puts the wave
    float fStageY;
    float fLandX;                  ///< Where it must come down instead
    float fLandY;
    float fLandZ;
};

// One staged approach, straight from the 18019 capture: a wave appears at
// the BACK of a neighbouring roof, runs its own roof to a fixed jump-off
// point on the edge, and leaps from there. The capture shows two distinct
// arcs - the house runts hop the gap under normal gravity (JumpGravity
// 19.3), the tower worgen soar sixty yards down a long flat one (3.5-4.9) -
// so each stage carries its own peak. A stage whose edge equals its centre
// has no run (the south roof: its line to Crowley runs ALONG the roof).
struct RoofStage
{
    float fStageX;                 ///< Match centre for the summon cluster
    float fStageY;
    float fEdgeX;                  ///< The surveyed jump-off point on the edge
    float fEdgeY;
    float fEdgeZ;
    float fEdgeJitter;             ///< Spread along the edge, so a wave fans out
    float fLandX;                  ///< Landing centre on the prison roof
    float fLandY;
    float fLandZ;
    float fLandJitter;
    float fArc;                    ///< Leap peak; low = the long glide
    bool  bGallop;                 ///< All-fours gallop (Running Wild) vs the sneak
};

/// The skinny Gilneas worgen ride the player character model, whose
/// gallop animations are reachable only through the player-only
/// self-mount - on an NPC the client falls back to a stiff rider pose.
/// The classic creature worgen model does the whole 2010 scene natively:
/// it RUNS ON ALL FOURS and STANDS UP on two legs to fight. The house
/// wave wears it for the assault.
static const uint32 DISPLAY_ROOF_GALLOPER = 26787;

static const RoofStage ROOF_STAGES[] =
{
    // The north house roof - the runt waves GALLOP (2010 footage)
    { -1719.0f, 1523.0f,  -1718.0f, 1495.0f, 56.40f, 3.0f,  -1695.5f, 1463.5f, 52.29f, 5.0f, 11.0f, true },
    // The high tower - the soaring alpha slinks to the edge
    { -1620.2f, 1510.0f,  -1639.3f, 1491.2f, 67.30f, 1.0f,  -1694.0f, 1461.0f, 52.29f, 1.5f,  7.5f, false },
    // The tower edge - the finale wave slinks, then glides
    { -1627.6f, 1489.3f,  -1639.5f, 1478.5f, 65.50f, 2.0f,  -1676.5f, 1449.0f, 52.29f, 5.0f,  4.6f, false },
    // The south roof - surveyed landing, no run
    { -1651.1f, 1381.4f,  -1651.1f, 1381.4f, 51.77f, 0.0f,  -1656.6f, 1403.2f, 52.74f, 1.0f, 10.0f, false },
};

static const float ROOF_RUN_SPEED   = 8.0f;    ///< The dash pace (retail run legs: 8.0 yd/s)
static const float ROOF_STAGE_MATCH = 25.0f;   ///< Spawn-to-stage match radius
static const float ROOF_RUN_MIN     = 4.0f;    ///< Nearer than this to the edge: skip the run

/*######
# npc_lord_darius_prison
######*/

// Lord Darius Crowley holds the prison roof STANDING HIS GROUND. The
// capture shows him planted mid-roof answering the wave with Left Hook
// (67825, a knockback punch), Snap Kick (67827, a knockback kick) and
// Demoralizing Shout (61044); default chase AI instead had him
// sprinting around the roof after whatever held aggro.
enum
{
    SPELL_CROWLEY_LEFT_HOOK     = 67825,
    SPELL_CROWLEY_SNAP_KICK     = 67827,
    SPELL_CROWLEY_DEMO_SHOUT    = 61044,
};

struct npc_lord_darius_prison : public CreatureScript
{
    npc_lord_darius_prison() : CreatureScript("npc_lord_darius_prison") {}

    struct npc_lord_darius_prisonAI : public ScriptedAI
    {
        npc_lord_darius_prisonAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            SetCombatMovement(false);
        }

        uint32 m_uiHookTimer;
        uint32 m_uiKickTimer;
        uint32 m_uiShoutTimer;

        void Reset() override
        {
            m_uiHookTimer = urand(8000, 14000);
            m_uiKickTimer = urand(16000, 24000);
            m_uiShoutTimer = urand(30000, 45000);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            if (m_uiHookTimer <= uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CROWLEY_LEFT_HOOK) == CAST_OK)
                {
                    m_uiHookTimer = urand(12000, 18000);
                }
            }
            else
            {
                m_uiHookTimer -= uiDiff;
            }

            if (m_uiKickTimer <= uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_CROWLEY_SNAP_KICK) == CAST_OK)
                {
                    m_uiKickTimer = urand(20000, 30000);
                }
            }
            else
            {
                m_uiKickTimer -= uiDiff;
            }

            if (m_uiShoutTimer <= uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_CROWLEY_DEMO_SHOUT) == CAST_OK)
                {
                    m_uiShoutTimer = urand(45000, 60000);
                }
            }
            else
            {
                m_uiShoutTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_lord_darius_prisonAI(pCreature);
    }
};

struct npc_prison_roof_worgen : public CreatureScript
{
    npc_prison_roof_worgen() : CreatureScript("npc_prison_roof_worgen") {}

    struct npc_prison_roof_worgenAI : public ScriptedAI
    {
        npc_prison_roof_worgenAI(Creature* pCreature)
            : ScriptedAI(pCreature), m_bEntranceDone(false) {}

        bool m_bEntranceDone;     ///< Landed once; an evade must never replay the entrance
        bool m_bLeaps;            ///< A wave copy, rather than an ambient spawn
        bool m_bAcross;           ///< Down on the roof; nothing may touch movement before this
        uint8 m_uiRunLeg;         ///< 0 none, 1 sidestep leg running, 2 edge leg running
        bool m_bLegDone;          ///< A run leg finished; the next tick advances the chain
        const RoofStage* m_pStage; ///< The staged approach this copy uses, if any
        uint32 m_uiLeapDelay;     ///< Counts down to the run (or leap) it opens with
        uint32 m_uiAcquireTimer;  ///< Retries finding a defender once it is across

        void Reset() override
        {
            m_bLeaps = m_creature->IsTemporarySummon() && !m_bEntranceDone;
            m_bAcross = false;
            m_uiRunLeg = 0;
            m_bLegDone = false;
            m_pStage = NULL;
            m_uiLeapDelay = m_bLeaps ? urand(ROOF_LEAP_DELAY_MIN, ROOF_LEAP_DELAY_MAX) : 0;
            m_uiAcquireTimer = 0;

            // Launch-2010 footage (the only surviving record - the MoP 18019
            // re-staging AND TC both dropped it): these worgen move on all
            // fours and only stand up to fight. The wave wears the feral
            // worgen from its FIRST tick - the model whose walk is the
            // four-legged prowl, whose run is the gallop, and whose combat
            // stance is upright. Ambient street copies keep the skinny look.
            if (m_bLeaps)
            {
                m_creature->SetDisplayId(DISPLAY_ROOF_GALLOPER);
            }

        }

        /// On two legs to fight - the pose drops the moment combat starts,
        /// and an evade re-applies it through Reset.
        void Aggro(Unit* /*pWho*/) override
        {
            m_creature->RemoveByteFlag(UNIT_FIELD_BYTES_1, 2, 0x02);
        }

        /// Crowley summons the waves, so he marks the roof they are coming for.
        /// Read live rather than stored, so the landing follows the spawn.
        Creature* GetSummoner()
        {
            TemporarySummon* pTemporary = (TemporarySummon*)m_creature;

            return m_creature->GetMap()->GetAnyTypeCreature(pTemporary->GetSummonerGuid());
        }

        /// The captured entrance: match the summon to its stage, run the
        /// roof to the jump-off point, and leap from there. A summon off
        /// every surveyed stage keeps the old derived leap.
        void StartApproach()
        {
            float fBest = ROOF_STAGE_MATCH * ROOF_STAGE_MATCH;

            for (uint32 i = 0; i < sizeof(ROOF_STAGES) / sizeof(ROOF_STAGES[0]); ++i)
            {
                const RoofStage& rStage = ROOF_STAGES[i];
                const float fSX = m_creature->Where().X() - rStage.fStageX;
                const float fSY = m_creature->Where().Y() - rStage.fStageY;
                const float fDist = fSX * fSX + fSY * fSY;

                // Nearest stage, not first: the tower's two stages overlap.
                if (fDist < fBest)
                {
                    fBest = fDist;
                    m_pStage = &rStage;
                }
            }

            if (m_pStage)
            {
                const float fEX = m_creature->Where().X() - m_pStage->fEdgeX;
                const float fEY = m_creature->Where().Y() - m_pStage->fEdgeY;

                if ((fEX * fEX + fEY * fEY) > (ROOF_RUN_MIN * ROOF_RUN_MIN))
                {
                    m_creature->addUnitState(UNIT_STAT_IGNORE_PATHFINDING);
                    m_creature->SetWalk(false);


                    // Driver-managed MovePoint legs, NOT raw splines: every
                    // driver leg (MovePoint, chase) animates the gallop
                    // correctly on the client, while every raw script spline
                    // here walked no matter its flags - the proven mechanism
                    // wins. The ridge geometry stays: outer spawn points
                    // sidestep onto the ridge line first (a straight diagonal
                    // off it leaves the pitched surface and renders as the
                    // falling animation), then run it down to the edge.
                    if (m_pStage->bGallop &&
                        fabs(m_creature->Where().X() - m_pStage->fEdgeX) > 2.0f)
                    {
                        m_uiRunLeg = 1;
                        m_creature->GetMotionMaster()->MovePoint(POINT_ID_ROOF_RUN,
                            m_pStage->fEdgeX, m_creature->Where().Y(),
                            m_creature->Where().Z(), false);
                        return;
                    }

                    m_uiRunLeg = 2;
                    m_creature->GetMotionMaster()->MovePoint(POINT_ID_ROOF_RUN,
                        m_pStage->fEdgeX + frand(-1.0f, 1.0f),
                        m_pStage->fEdgeY + frand(-m_pStage->fEdgeJitter, m_pStage->fEdgeJitter),
                        m_pStage->fEdgeZ, false);
                    return;
                }
            }

            Leap();
        }

        /// One arc onto the near edge of the roof, not into the defenders.
        void Leap()
        {
            Creature* pSummoner = GetSummoner();

            if (!pSummoner)
            {
                return;
            }

            // The staged copies leap from their surveyed edge onto their
            // surveyed landing; the arc is the capture's, not a guess.
            if (m_pStage)
            {
                m_creature->GetMotionMaster()->MoveJump(
                    m_pStage->fLandX + frand(-m_pStage->fLandJitter, m_pStage->fLandJitter),
                    m_pStage->fLandY + frand(-m_pStage->fLandJitter, m_pStage->fLandJitter),
                    m_pStage->fLandZ,
                    ROOF_LEAP_SPEED, m_pStage->fArc, POINT_ID_ROOF_LEAP);
                return;
            }

            const float fDX = m_creature->Where().X() - pSummoner->Where().X();
            const float fDY = m_creature->Where().Y() - pSummoner->Where().Y();

            // Anything staged this close is already on the roof and has a
            // clear run in; leaping would only look like a stutter.
            if (sqrt(fDX * fDX + fDY * fDY) <= ROOF_RUN_IN_DIST)
            {
                m_bAcross = true;
                m_bEntranceDone = true;
                RunIn();
                return;
            }

            // Keep only the component that runs down the length of the roof,
            // so a worgen still lands on the side it was staged from, but one
            // arriving across the short axis is pulled onto the centreline
            // instead of over the edge.
            float fAlong = fDX * ROOF_AXIS_X + fDY * ROOF_AXIS_Y;

            if (fAlong > ROOF_ALONG_MAX)
            {
                fAlong = ROOF_ALONG_MAX;
            }
            else if (fAlong < -ROOF_ALONG_MAX)
            {
                fAlong = -ROOF_ALONG_MAX;
            }

            // Scatter to one side only. The surveyed edge lies that way and
            // Crowley stands nearer the opposite one, so a signed jitter would
            // spend half its range walking off the closer edge.
            const float fSide = frand(0.0f, ROOF_SIDE_MAX);

            m_creature->GetMotionMaster()->MoveJump(
                pSummoner->Where().X() + ROOF_AXIS_X * fAlong - ROOF_AXIS_Y * fSide,
                pSummoner->Where().Y() + ROOF_AXIS_Y * fAlong + ROOF_AXIS_X * fSide,
                pSummoner->Where().Z(),
                ROOF_LEAP_SPEED, ROOF_LEAP_ARC, POINT_ID_ROOF_LEAP);
        }

        /// Lord Darius Crowley and Tobias Mistmantle are the two holding the
        /// roof. Whichever currently has fewer worgen on it takes the next
        /// one, so a wave splits between them instead of burying one.
        Creature* GetDefender()
        {
            Creature* pCandidates[2] =
            {
                GetSummoner(),                  // Crowley, who summons the waves
                GetClosestCreatureWithEntry(m_creature, NPC_TOBIAS_MISTMANTLE, ROOF_AGGRO_RANGE)
            };

            Creature* pBest = NULL;
            size_t uiBestLoad = 0;

            for (uint32 i = 0; i < 2; ++i)
            {
                if (!pCandidates[i] || !pCandidates[i]->IsAlive())
                {
                    continue;
                }

                const size_t uiLoad = pCandidates[i]->getAttackers().size();

                if (!pBest || uiLoad < uiBestLoad)
                {
                    pBest = pCandidates[i];
                    uiBestLoad = uiLoad;
                }
            }

            return pBest;
        }

        /// The last stretch, on foot.
        ///
        /// Landing beside the defenders is NOT enough to start the fight.
        /// MoveInLineOfSight only fires when something moves INTO view, and a
        /// jump moves the worgen while the defenders stand still - so nothing
        /// re-triggers it and the worgen sits where it came down. The target
        /// has to be taken deliberately.
        void RunIn()
        {
            m_creature->SetWalk(false);

            Creature* pSummoner = GetSummoner();
            Player* pPlayer = GetPlayerAtMinimumRange(ROOF_AGGRO_RANGE);

            // One at a time on the player, no more. This is a defence of a
            // roof, not a duel: a wave that piles onto a single level 4 buries
            // them, and the quest only asks that the line holds for two
            // minutes. `getAttackers` is the live count of who is already on
            // them, so the next worgen steps in as soon as the current one
            // dies - the cap follows the fight rather than being decided once.
            if (pPlayer && pPlayer->getAttackers().size() < ROOF_MAX_ON_PLAYER)
            {
                AttackStart(pPlayer);
                return;
            }

            // Everything else goes for the two defending the roof, which is
            // what they are there for - Crowley is rank 1 with a 35x damage
            // multiplier, an elite built to hold this. They thin the wave
            // while the player handles a handful.
            if (Creature* pDefender = GetDefender())
            {
                AttackStart(pDefender);
                return;
            }

            if (!pSummoner)
            {
                return;
            }

            // Nobody left worth reaching; close on the roof and look again.
            m_creature->GetMotionMaster()->MovePoint(0,
                pSummoner->Where().X(), pSummoner->Where().Y(), pSummoner->Where().Z());
        }

        void MovementInform(uint32 uiMovementType, uint32 uiPointId) override
        {
            // A run leg ended; the next tick advances the chain (starting
            // movement inside the generator Finalize gets wiped).
            if (uiMovementType == POINT_MOTION_TYPE && uiPointId == POINT_ID_ROOF_RUN)
            {
                m_bLegDone = true;
                return;
            }

            if (uiMovementType != EFFECT_MOTION_TYPE || uiPointId != POINT_ID_ROOF_LEAP)
            {
                return;
            }

            // Deliberately does NOT move the worgen. This runs inside
            // EffectMovementGenerator::Finalize, which restores movement the
            // moment it returns - MoveChase if there is a victim, otherwise
            // MotionMaster::Initialize(), and that wipes anything started
            // here. Landing only arms the next tick, which is a safe context.
            m_bAcross = true;
            m_bEntranceDone = true;
            m_uiAcquireTimer = 0;

            // The leash anchor moves to the DEFENSE, not the landing: the
            // south stage comes down 51 yards from Crowley, and with
            // ThreatRadius at 40 an anchor on the landing flips
            // IsOutOfThreatArea every tick - chase, evade home, re-acquire,
            // chase - the halfway ping-pong the log shows. Anchored on the
            // fight itself, the whole defended stretch is in-area.
            Creature* pAnchor = GetSummoner();
            m_creature->SetSpawn(Geometry::Vector3(
                pAnchor ? pAnchor->GetPositionX() : -1675.7f,
                pAnchor ? pAnchor->GetPositionY() : 1445.8f,
                pAnchor ? pAnchor->GetPositionZ() : 52.29f),
                m_creature->GetOrientation());
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // Deferred to the first tick on purpose. Reset() runs inside
            // AIM_Initialize, before the creature is in the world, and moving
            // from there acts on a grid it has not been added to yet.
            if (m_uiLeapDelay)
            {
                if (m_uiLeapDelay <= uiDiff)
                {
                    m_uiLeapDelay = 0;
                    StartApproach();
                }
                else
                {
                    m_uiLeapDelay -= uiDiff;
                }
            }

            // Advance the run chain from a safe tick context.
            if (m_bLegDone)
            {
                m_bLegDone = false;

                if (m_uiRunLeg == 1 && m_pStage)
                {
                    // On the ridge; run it down to the edge.
                    m_uiRunLeg = 2;
                    m_creature->GetMotionMaster()->MovePoint(POINT_ID_ROOF_RUN,
                        m_pStage->fEdgeX + frand(-1.0f, 1.0f),
                        m_pStage->fEdgeY + frand(-m_pStage->fEdgeJitter, m_pStage->fEdgeJitter),
                        m_pStage->fEdgeZ, false);
                }
                else if (m_uiRunLeg == 2)
                {
                    m_uiRunLeg = 0;
                    Leap();
                }
            }

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                // Across and idle. Gated on having actually landed: taking a
                // target mid-flight would replace the jump spline with a
                // ground chase and walk the worgen off the roof it was
                // supposed to clear.
                //
                // Retried rather than done once on landing, because the
                // defenders may still be running back up the roof and a worgen
                // that came down behind them has to keep looking.
                if (m_bEntranceDone)
                {
                    if (m_uiAcquireTimer <= uiDiff)
                    {
                        m_uiAcquireTimer = ROOF_ACQUIRE_INTERVAL;
                        RunIn();
                    }
                    else
                    {
                        m_uiAcquireTimer -= uiDiff;
                    }
                }

                return;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_prison_roof_worgenAI(pCreature);
    }
};

/*######
# npc_josiah_worgen
######*/

// Handing in `The Rebel Lord's Arsenal` (14159) turns Josiah Avery. The quest
// carries RewSpellCast 67352, this core stubs SPELL_EFFECT_FORCE_CAST into a
// DBS_ON_SPELL script, and that script phases the player, puts this worgen in
// the human's place and has it hurl them back up the cellar. Lorna Crowley
// then puts it down, which is the part that cannot be written as data:
// `db_scripts` resolves a buddy as either the source or the target of a cast,
// never one of each, and this shot needs Lorna firing at the worgen while the
// script's own source and target are both the player.
enum
{
    NPC_LORNA_CROWLEY       = 35378,
    SPELL_GET_SHOT          = 67349,   ///< Blizzard's own; a bare dummy, scripted below

    JOSIAH_LUNGE_DELAY      = 400,     ///< ms after it appears: turn and spring
    JOSIAH_SHOT_DELAY       = 1400,    ///< ms; she fires while it is still in the air
    JOSIAH_KILL_DELAY       = 300,     ///< ms after the rifle comes up: the round lands
    JOSIAH_SEARCH_RANGE     = 60,      ///< She waits at the stairs, well within this
};

// The lunge is aimed at the player rather than past them, and stops short so
// the worgen is caught coming in rather than landing on top of them.
//
// The arc is deliberately shallow. `MoveJump` takes max_height as the peak
// above the straight line between the two points, so anything generous turns a
// twenty yard spring into a lob; kept low, the same call reads as a flat
// charge that happens to leave the ground.
static const float JOSIAH_LUNGE_SPEED = 14.0f;
static const float JOSIAH_LUNGE_ARC   = 2.0f;
static const float JOSIAH_LUNGE_STOP  = 3.0f;   ///< How far short of the player it comes down

struct npc_josiah_worgen : public CreatureScript
{
    npc_josiah_worgen() : CreatureScript("npc_josiah_worgen") {}

    struct npc_josiah_worgenAI : public ScriptedAI
    {
        npc_josiah_worgenAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiLungeTimer;    ///< Counts down to the spring; 0 once made
        uint32 m_uiShotTimer;     ///< Counts down to Lorna's shot; 0 once fired
        uint32 m_uiKillTimer;     ///< Counts from the rifle coming up to the round landing

        void Reset() override
        {
            m_uiLungeTimer = JOSIAH_LUNGE_DELAY;
            m_uiShotTimer = JOSIAH_SHOT_DELAY;
            m_uiKillTimer = 0;
        }

        /// It turns on whoever Lorna just hauled clear, and springs at them.
        void Lunge()
        {
            Player* pPlayer = GetPlayerAtMinimumRange(float(JOSIAH_SEARCH_RANGE));

            if (!pPlayer)
            {
                return;
            }

            m_creature->SetFacingToObject(pPlayer);

            const float fDX = m_creature->Where().X() - pPlayer->Where().X();
            const float fDY = m_creature->Where().Y() - pPlayer->Where().Y();
            const float fDist = sqrt(fDX * fDX + fDY * fDY);

            if (fDist <= JOSIAH_LUNGE_STOP)
            {
                return;
            }

            const float fScale = JOSIAH_LUNGE_STOP / fDist;

            m_creature->GetMotionMaster()->MoveJump(
                pPlayer->Where().X() + fDX * fScale,
                pPlayer->Where().Y() + fDY * fScale,
                pPlayer->Where().Z(),
                JOSIAH_LUNGE_SPEED, JOSIAH_LUNGE_ARC);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiLungeTimer)
            {
                if (m_uiLungeTimer <= uiDiff)
                {
                    m_uiLungeTimer = 0;
                    Lunge();
                }
                else
                {
                    m_uiLungeTimer -= uiDiff;
                }
            }

            if (m_uiShotTimer)
            {
                if (m_uiShotTimer <= uiDiff)
                {
                    m_uiShotTimer = 0;
                    GetShot();
                }
                else
                {
                    m_uiShotTimer -= uiDiff;
                }
            }

            if (m_uiKillTimer)
            {
                if (m_uiKillTimer <= uiDiff)
                {
                    m_uiKillTimer = 0;
                    TakeTheRound();
                }
                else
                {
                    m_uiKillTimer -= uiDiff;
                }
            }

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            DoMeleeAttackIfReady();
        }

        /// Lorna turns and fires.
        ///
        /// The bang belongs to the animation, not to the spell. 67349's visual
        /// 14267 is a missile and a target impact and nothing else - every cast
        /// side kit on it is zero - and no `Shoot` spell in the table carries a
        /// cast sound either, because a gun is heard when the model plays its
        /// rifle attack. So she has to actually perform the shot; 52052
        /// `Monster - Gun, Rifle, Worgen` is already in her ranged slot.
        void GetShot()
        {
            Creature* pLorna =
                GetClosestCreatureWithEntry(m_creature, NPC_LORNA_CROWLEY, float(JOSIAH_SEARCH_RANGE));

            if (!pLorna || !pLorna->IsAlive())
            {
                return;
            }

            pLorna->SetFacingToObject(m_creature);
            pLorna->HandleEmoteCommand(EMOTE_ONESHOT_ATTACKRIFLE);

            m_uiKillTimer = JOSIAH_KILL_DELAY;
        }

        /// The round arrives a beat behind the rifle, and that is the end of
        /// Josiah Avery.
        void TakeTheRound()
        {
            Creature* pLorna =
                GetClosestCreatureWithEntry(m_creature, NPC_LORNA_CROWLEY, float(JOSIAH_SEARCH_RANGE));

            if (!pLorna || !pLorna->IsAlive())
            {
                return;
            }

            // The kill lives in spell_josiah_get_shot, on Blizzard's own spell,
            // rather than being dealt here - so what ends Josiah is the shot
            // itself and not a script reaching past it.
            pLorna->CastSpell(m_creature, SPELL_GET_SHOT, true);
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_josiah_worgenAI(pCreature);
    }
};

/*######
# spell_josiah_get_shot
######*/

// 67349 `Get Shot` is Blizzard's own spell for this moment, sitting beside
// `Summon Josiah` and `Pull-to` in the same block of ids. It is a bare
// SPELL_EFFECT_DUMMY: the effect carries no behaviour, because on retail the
// server decided what being shot meant. Here it means Lorna's shot kills him,
// which keeps the corpse hers and gives the player a death to watch rather
// than a creature blinking out.
struct spell_josiah_get_shot : public SpellScript
{
    spell_josiah_get_shot() : SpellScript("spell_josiah_get_shot") {}

    bool EffectDummy(Unit* pCaster, uint32 uiSpellId, SpellEffectIndex uiEffIndex,
                     Object* pTarget, ObjectGuid /*originalCasterGuid*/) override
    {
        if (uiSpellId != SPELL_GET_SHOT || uiEffIndex != EFFECT_INDEX_0)
        {
            return false;
        }

        Creature* pJosiah = pTarget ? pTarget->ToCreature() : NULL;

        if (!pCaster || !pJosiah || !pJosiah->IsAlive())
        {
            return false;
        }

        pCaster->DealDamage(pJosiah, pJosiah->GetHealth(), NULL, DIRECT_DAMAGE,
            SPELL_SCHOOL_MASK_NORMAL, NULL, false);

        return true;
    }
};

/*######
# npc_lorna_crowley
######*/

// `From the Shadows` (14204) sends the player after six Bloodfang Lurkers,
// which wear 5916 `Shadowstalker Stealth` - SPELL_AURA_MOD_STEALTH, amount 1.
// That is weak stealth: `Unit::IsVisibleForOrDetect` gives 10.5 - 1/100 yards,
// so they surface as the player walks up to them. The dog is the quicker way,
// not the only way - it carries 67805 `Attack Lurker`, which leaps to one from
// 30 yards - and no aura is put on the player, because retail shows none.
//
// Blizzard left every piece in the data and nothing joining them. The collar
// (48707) casts 67807 `Summon Gilnean Mastiff` when used, so the player CAN
// call it by hand, and 68234 `Forcecast Mastiff` exists to do it for them on
// accept - but SPELL_EFFECT_FORCE_CAST is stubbed in this core, so accepting
// the quest summoned nothing. The quest's own RewSpellCast, 43511 `Force
// Dismiss Plaguehound`, is stubbed the same way, so the dog was never taken
// back either.
//
// Both ends are handled here, which is also the only place the class pet can
// be dealt with: a hunter or warlock arrives with one already out, and the
// core has the right primitive for setting it aside and lending it back.
enum
{
    QUEST_FROM_THE_SHADOWS       = 14204,
    QUEST_REBEL_LORDS_ARSENAL    = 14159,
    NPC_GILNEAN_MASTIFF_PET      = 35631,
    NPC_BLOODFANG_LURKER         = 35463,
    SPELL_SUMMON_GILNEAN_MASTIFF = 67807,

    SAY_LORNA_MASTIFF            = -1999998,  ///< capture: voiced (19696) at the offer

    MASTIFF_SEARCH_RANGE         = 60,
    LURKER_SWEEP_RANGE           = 160,       ///< the 21 lurk spots all sit within 91 yd of her
};

/**
* The hunt has a window, and it is not the quest log's. In the capture the
* first Lurker springs the player thirty-six seconds BEFORE 14204 is taken -
* it enters the world with the bulk update the 14159 turn-in triggers - and
* the whole population is gone within half a minute of the 14204 turn-in:
* the Krennan ride afterwards passes 0.2 yd over a lurk spot, in the same
* phase, and the server sends nothing. So: open at 14159 rewarded, shut at
* 14204 rewarded, regardless of whether the quest is currently held.
*/
static bool IsLurkerHuntOpen(Player const* pPlayer)
{
    return pPlayer->GetQuestRewardStatus(QUEST_REBEL_LORDS_ARSENAL) &&
           !pPlayer->GetQuestRewardStatus(QUEST_FROM_THE_SHADOWS);
}

struct npc_lorna_crowley : public CreatureScript
{
    npc_lorna_crowley() : CreatureScript("npc_lorna_crowley") {}

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, Quest const* pQuest) override
    {
        if (!pPlayer || !pQuest || pQuest->GetQuestId() != QUEST_FROM_THE_SHADOWS)
        {
            return false;
        }

        // The capture pairs the handoff with her voiced line (sound 19696).
        DoScriptText(SAY_LORNA_MASTIFF, pCreature, pPlayer);

        // Stashed rather than dismissed: the core remembers it and hands it
        // straight back, so a hunter does not lose a pet to a starting quest.
        pPlayer->UnsummonPetTemporaryIfAny();

        pPlayer->CastSpell(pPlayer, SPELL_SUMMON_GILNEAN_MASTIFF, true);

        // A mastiff from an earlier run is kept in `character_pet`, and
        // summoning loads that saved state - corpse included, if a Lurker got
        // it. Whatever turns up is put back on its feet at full health, so a
        // bad run cannot poison every run after it.
        if (Pet* pPet = pPlayer->GetPet())
        {
            if (pPet->GetEntry() == NPC_GILNEAN_MASTIFF_PET)
            {
                if (!pPet->IsAlive())
                {
                    pPet->SetDeathState(ALIVE);
                }

                pPet->SetHealth(pPet->GetMaxHealth());
            }
        }

        // False: the quest's own handling still has to run, not least to hand
        // over the collar the player keeps.
        return false;
    }

    bool OnQuestRewarded(Player* pPlayer, Creature* pCreature, Quest const* pQuest) override
    {
        if (!pPlayer || !pQuest || pQuest->GetQuestId() != QUEST_FROM_THE_SHADOWS)
        {
            return false;
        }

        // The capture clears the district on the turn-in: a straggler that
        // surfaced fourteen seconds after the reward is removed in the same
        // DestroyObjects batch as the mastiffs, and nothing Lurker-shaped is
        // ever sent again. Sweep every lurk spot; the ten-minute respawn
        // restages the hunt for the next character.
        std::list<Creature*> lLurkers;
        GetCreatureListWithEntryInGrid(lLurkers, pCreature,
                                       NPC_BLOODFANG_LURKER, (float)LURKER_SWEEP_RANGE);
        for (std::list<Creature*>::iterator itr = lLurkers.begin(); itr != lLurkers.end(); ++itr)
        {
            (*itr)->ForcedDespawn();
        }

        // PET_SAVE_AS_DELETED, not a despawn: a despawned pet still leaves its
        // row in `character_pet`, and a borrowed quest dog that outlives the
        // quest comes back on the next summon in whatever state it died in -
        // and sits in the slot a hunter's real pet wants.
        if (Pet* pPet = pPlayer->GetPet())
        {
            if (pPet->GetEntry() == NPC_GILNEAN_MASTIFF_PET)
            {
                pPet->Unsummon(PET_SAVE_AS_DELETED, pPlayer);
            }
        }

        pPlayer->ResummonPetTemporaryUnSummonedIfAny();

        return false;
    }
};

/*######
# npc_bloodfang_lurker
######*/

// The sweep above empties the district for the character that finished the
// quest, but the spots restock on their respawn timer for the next one, and a
// respawned Lurker must not jump whoever happens by. The stock AI does
// everything else right - the stealth, the surfacing, the spacing-driven
// one-at-a-time hunt - so the script is nothing but the window on the door.
struct npc_bloodfang_lurker : public CreatureScript
{
    npc_bloodfang_lurker() : CreatureScript("npc_bloodfang_lurker") {}

    struct npc_bloodfang_lurkerAI : public ScriptedAI
    {
        npc_bloodfang_lurkerAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        void MoveInLineOfSight(Unit* pWho) override
        {
            // Outside the hunt it stays a lurker: no spring at a player who
            // has not turned Josiah in yet, none at one who has already
            // cleared the cellar. Being attacked is not gated - a poked
            // wolf fights back.
            if (pWho->GetTypeId() == TYPEID_PLAYER && !IsLurkerHuntOpen((Player*)pWho))
            {
                return;
            }

            ScriptedAI::MoveInLineOfSight(pWho);
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_bloodfang_lurkerAI(pCreature);
    }
};

/*######
# npc_greymanes_horse
######*/

// `Save Krennan Aranas` (14293) is a vehicle ride. Accepting it casts 68221
// `Summon Greymane's Horse`, whose SummonProperties 488 is a vehicle group, so
// `Spell::DoSummonVehicle` boards the caster the moment the horse appears - the
// mount half needs no script at all.
//
// What is missing is everything after that. The horse runs itself to the far
// side of the district, springs the last stretch, and waits under the tree
// while the player uses its one ability, 68219 `Rescue Krennan`, which is
// already on its bar through `creature_template_spells`.
//
// Two details of this core shape the code below.
//
// The ride itself is two splines, not a chain of point moves. Retail (18019
// Gilneas capture) sends the outbound run as ONE SMSG_MONSTER_MOVE with seven
// points over 10,251 ms and the return as ONE with thirteen over 15,870 ms -
// both at 11.67 yd/s, above the horse's own run speed of 9. A point move per
// waypoint stops dead at every corner; `MovebyPath` with a set velocity keeps
// the pace through them, and arrival is read off `movespline->Finalized()`.
//
// `MovementInform` is still used for the jump: it is raised from `Finalize`
// and a movement started there is wiped by the restore that follows, so
// landing only arms a timer and the rest happens from `UpdateAI`.
//
// 68228 - which 68219 triggers - carries SUMMON plus KILL_CREDIT, but the
// vehicle is the caster and the credit effect is TARGET_SELF, and
// `EffectKillCreditPersonal` drops anything that is not a player. The credit is
// therefore given here, to the rider, or the objective would never tick.
enum
{
    NPC_TRAPPED_KRENNAN       = 35753,     ///< Hangs in the tree; also the quest's credit id
    NPC_RESCUED_KRENNAN       = 35907,     ///< Summoned aboard by 68228

    SAY_HORSE_ANNOUNCE        = -1999944,
    SAY_KRENNAN_TRAPPED       = -1999945,
    SAY_KRENNAN_RESCUED       = -1999946,

    HORSE_SEAT_RIDER          = 0,
    HORSE_SEAT_KRENNAN        = 1,

    HORSE_POINT_JUMP          = 200,

    HORSE_MAX_ON_RIDER        = 4,         ///< Attackers left on the rider; the rest are sent home. Retail peaks at four on the horse
    HORSE_MAX_SHED            = 32,        ///< Ceiling on one pass of the cull

    HORSE_POLL_INTERVAL       = 250,       ///< ms between "has anyone boarded" checks
    HORSE_CULL_INTERVAL       = 2000,      ///< ms between attacker culls; see ShedExcessAttackers
    HORSE_ANNOUNCE_DELAY      = 1000,      ///< ms after landing before the rider is told what to press (retail 0.98 s)
    HORSE_KRENNAN_CALL_DELAY  = 500,       ///< ms after that before Krennan calls out (retail 0.48 s)
    HORSE_SEARCH_RANGE        = 30,
    HORSE_KRENNAN_DESPAWN     = 6700,     ///< ms he stands there after saying his piece (retail 6.7 s)
    HORSE_DESPAWN_AFTER_RIDE  = 2600,     ///< ms the horse lingers once the rider is off (retail 2.6 s)
    HORSE_TRAPPED_RESPAWN     = 30,       ///< seconds before the man in the tree is back for the next player
};

/// Spline velocity of both legs, measured: 119.5 yd in 10.251 s out, 184.9 yd
/// in 15.870 s back. Not the template's run speed (9, which retail also has).
static const float HORSE_RIDE_SPEED = 11.667f;


/// Out to the tree. The last leg stops short - the gap is the jump.
static const float HORSE_PATH_OUT[][3] =
{
    { -1797.425f, 1396.941f, 20.07336f },
    { -1788.675f, 1378.441f, 20.07336f },
    { -1780.925f, 1368.941f, 20.07336f },
    { -1767.425f, 1358.191f, 19.82336f },
    { -1746.425f, 1358.691f, 20.07336f },
    { -1726.175f, 1354.191f, 19.82336f },
    { -1709.064f, 1348.535f, 19.78232f }
};

/// The loop back to Lord Godfrey, who takes the quest in.
static const float HORSE_PATH_BACK[][3] =
{
    { -1664.807f, 1345.011f, 15.48499f },
    { -1662.807f, 1354.511f, 15.48499f },
    { -1667.307f, 1362.511f, 15.48499f },
    { -1674.307f, 1363.761f, 15.48499f },
    { -1686.057f, 1355.011f, 15.48499f },
    { -1691.057f, 1347.261f, 15.48499f },
    { -1705.807f, 1350.011f, 19.98499f },
    { -1731.307f, 1360.011f, 19.98499f },
    { -1744.807f, 1370.511f, 20.23499f },
    { -1758.807f, 1389.511f, 19.98499f },
    { -1768.307f, 1410.011f, 19.98499f },
    { -1771.557f, 1423.011f, 19.98499f },
    { -1770.955f, 1430.332f, 19.83506f }
};

static const uint32 HORSE_PATH_OUT_SIZE  = 7;
static const uint32 HORSE_PATH_BACK_SIZE = 13;

static const float HORSE_JUMP_X     = -1676.16f;
static const float HORSE_JUMP_Y     = 1346.19f;
static const float HORSE_JUMP_Z     = 15.1349f;
// Retail: 35.06 yd in 1,965 ms, JumpGravity 12.43 -> peak 6.0 yd. This core's
// MoveJump takes the PEAK HEIGHT; the 14.00968 that used to sit here is
// TrinityCore's vertical *speed* for the same jump and gave a 14 yard arc.
static const float HORSE_JUMP_SPEED = 17.84f;
static const float HORSE_JUMP_ARC   = 6.0f;

enum HorseStage
{
    HORSE_AWAIT_RIDER = 0,   ///< Standing where it was summoned
    HORSE_OUTBOUND,          ///< Running HORSE_PATH_OUT
    HORSE_JUMPING,           ///< In the air, or landed and about to call out
    HORSE_AWAIT_RESCUE,      ///< Under the tree; the player's move
    HORSE_RETURN,            ///< Running HORSE_PATH_BACK
    HORSE_DONE
};

struct npc_greymanes_horse : public CreatureScript
{
    npc_greymanes_horse() : CreatureScript("npc_greymanes_horse") {}

    struct npc_greymanes_horseAI : public ScriptedAI
    {
        npc_greymanes_horseAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiStage;
        bool   m_bRunning;          ///< A ride spline is out; watch for it to finish
        uint32 m_uiAnnounceTimer;
        uint32 m_uiKrennanCallTimer;
        uint32 m_uiPollTimer;
        uint32 m_uiCullTimer;

        void Reset() override
        {
            m_uiStage = HORSE_AWAIT_RIDER;
            m_bRunning = false;
            m_uiAnnounceTimer = 0;
            m_uiKrennanCallTimer = 0;
            m_uiPollTimer = HORSE_POLL_INTERVAL;
            m_uiCullTimer = HORSE_CULL_INTERVAL;

            // The core's default is to take a summoned vehicle away one second
            // after its controller leaves. Retail keeps this one 2.6 s.
            if (VehicleInfo* pKit = m_creature->GetVehicleInfo())
            {
                pKit->SetDespawnDelay(HORSE_DESPAWN_AFTER_RIDE);
            }
        }

        /// It carries the player; nothing it passes is its business.
        void AttackStart(Unit* /*pWho*/) override {}
        void MoveInLineOfSight(Unit* /*pWho*/) override {}

        Player* GetRider() const
        {
            if (VehicleInfo* pKit = m_creature->GetVehicleInfo())
            {
                Unit* pRider = pKit->GetPassenger(HORSE_SEAT_RIDER);

                if (pRider && pRider->GetTypeId() == TYPEID_PLAYER)
                {
                    return (Player*)pRider;
                }
            }

            return NULL;
        }

        /// The whole leg as one spline at the ride's own pace.
        void RunPath(const float pPath[][3], uint32 uiSize)
        {
            m_creature->SetRoot(false);

            // Slot 0 is a placeholder: `Launch` overwrites it with the horse's
            // current position.
            Movement::PointsArray points;
            points.reserve(uiSize + 1);
            points.push_back(Geometry::Vector3(0.0f, 0.0f, 0.0f));

            for (uint32 i = 0; i < uiSize; ++i)
            {
                points.push_back(Geometry::Vector3(pPath[i][0], pPath[i][1], pPath[i][2]));
            }

            Movement::MoveSplineInit init(*m_creature);
            init.MovebyPath(points);
            init.SetWalk(false);
            init.SetVelocity(HORSE_RIDE_SPEED);
            init.Launch();

            m_bRunning = true;
        }

        void MovementInform(uint32 uiType, uint32 uiPointId) override
        {
            // Never start movement from here: this arrives out of Finalize, and
            // the generator restore that follows would discard it.
            if (uiType == EFFECT_MOTION_TYPE && uiPointId == HORSE_POINT_JUMP)
            {
                m_creature->SetRoot(true);
                m_uiAnnounceTimer = HORSE_ANNOUNCE_DELAY;
            }
        }

        /// Keep the ride survivable.
        ///
        /// Phase 4 of the Military District holds 67 Bloodfang Rippers against 21
        /// Gilneas City Guards, and every one of those guards is 100 yards or more
        /// back at the barricade - the stretch this ride crosses, and the tree it
        /// stops at, have no defenders at all. A rider halted under the tree is
        /// therefore swarmed by a dozen of them. Retail does not ask the player to
        /// win that fight, so a few are left on them and the rest are sent home.
        /// Gathers attackers of one unit, keeping the first few and listing the rest.
        void CullAttackersOf(Unit* pWhom, uint32& uiKept, Creature** pExcess,
                             uint32& uiExcess) const
        {
            if (!pWhom)
            {
                return;
            }

            Unit::AttackerSet const& attackers = pWhom->getAttackers();

            for (Unit::AttackerSet::const_iterator itr = attackers.begin();
                 itr != attackers.end() && uiExcess < HORSE_MAX_SHED; ++itr)
            {
                Creature* pAttacker = *itr ? (*itr)->ToCreature() : NULL;

                if (!pAttacker)
                {
                    continue;
                }

                if (uiKept < HORSE_MAX_ON_RIDER)
                {
                    ++uiKept;
                    continue;
                }

                pExcess[uiExcess++] = pAttacker;
            }
        }

        void ShedExcessAttackers()
        {
            Player* pRider = GetRider();

            if (!pRider)
            {
                return;
            }

            Creature* pExcess[HORSE_MAX_SHED];
            uint32 uiKept = 0;
            uint32 uiExcess = 0;

            // Both, and counted as one pool: a Ripper may pick the rider or the
            // mount under them, and letting each have its own allowance is how
            // the whole district ends up piled on the horse.
            CullAttackersOf(pRider, uiKept, pExcess, uiExcess);
            CullAttackersOf(m_creature, uiKept, pExcess, uiExcess);

            // Evading edits the very sets walked above, so it waits until after.
            for (uint32 i = 0; i < uiExcess; ++i)
            {
                if (pExcess[i]->AI())
                {
                    pExcess[i]->AI()->EnterEvadeMode();
                }
            }
        }

        /// Under the tree: tell the player what to press. Krennan follows half
        /// a second later, as he does on retail.
        void Announce()
        {
            Player* pRider = GetRider();

            if (!pRider)
            {
                return;
            }

            DoScriptText(SAY_HORSE_ANNOUNCE, m_creature, pRider);
            m_uiKrennanCallTimer = HORSE_KRENNAN_CALL_DELAY;
        }

        void KrennanCallsOut()
        {
            Player* pRider = GetRider();

            if (!pRider)
            {
                return;
            }

            if (Creature* pTrapped = GetClosestCreatureWithEntry(m_creature,
                    NPC_TRAPPED_KRENNAN, float(HORSE_SEARCH_RANGE)))
            {
                DoScriptText(SAY_KRENNAN_TRAPPED, pTrapped, pRider);
            }
        }

        /// 68228 has put Krennan down beside us; take him aboard.
        void TakeKrennanAboard()
        {
            VehicleInfo* pKit = m_creature->GetVehicleInfo();
            Player* pRider = GetRider();

            if (!pKit || !pRider || pKit->GetPassenger(HORSE_SEAT_KRENNAN))
            {
                return;
            }

            // He belongs to the HORSE, not the rider. 68219 is a vehicle
            // ability, so the vehicle is the caster of the 68228 it triggers,
            // and `Spell::DoSummonGuardian` hands ownership to that caster:
            //     spawnCreature->SetOwnerGuid(m_caster->GetObjectGuid());
            // A guardian is also a pet, and this core keeps pets in the WORLD
            // container while the grid searchers only walk the GRID one, so
            // `GetClosestCreatureWithEntry` cannot see him either way.
            Creature* pKrennan = m_creature->FindGuardianWithEntry(NPC_RESCUED_KRENNAN);

            if (!pKrennan)
            {
                pKrennan = pRider->FindGuardianWithEntry(NPC_RESCUED_KRENNAN);
            }

            if (!pKrennan || pKrennan->IsBoarded())
            {
                return;
            }

            // A guardian arrives ready to fight, and the tree is ringed with
            // Rippers - left alone he wades into them instead of being carried
            // out. He is a passenger from here on, nothing more.
            pKrennan->AttackStop();
            pKrennan->CombatStop();
            pKrennan->SetFlag(UNIT_FIELD_FLAGS,
                UNIT_FLAG_PASSIVE | UNIT_FLAG_NON_ATTACKABLE);

            // He is a guardian, and a guardian's AI picks its own targets from
            // whatever it can reach - the flags above stop him starting a fight
            // on his own account, but not his pet AI reacting to one. Passive
            // is what keeps him a passenger the whole way home.
            if (CharmInfo* pCharm = pKrennan->GetCharmInfo())
            {
                pCharm->SetReactState(REACT_PASSIVE);
            }

            // Put the weapon away; he is being carried out, not going to war.
            pKrennan->SetSheath(SHEATH_STATE_UNARMED);

            // Settle him. Stopping the attack and going passive keeps him from
            // swinging, but a creature still FACES whatever sits in
            // UNIT_FIELD_TARGET, which is what had him turning round in the
            // saddle as though he were about to start something. Clearing the
            // target and idling the movement generator leaves him simply
            // riding, and PACIFIED makes sure nothing talks him back into it.
            pKrennan->SetTargetGuid(ObjectGuid());
            pKrennan->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
            pKrennan->GetMotionMaster()->Clear(false, true);
            pKrennan->GetMotionMaster()->MoveIdle();

            // Idling him once is not enough, and this is what actually had him
            // twisted round in the saddle all the way home.
            //
            // A seated passenger's world pose is only re-derived every half
            // second, and only once the horse has moved a yard
            // (`VehicleInfo::Update`), so on a running mount his position
            // trails the owner's. PetAI reads that as "my owner is out of
            // reach" and restarts `MoveFollow` - onto the very horse he is
            // sitting on. Every follow leg is a spline, and a boarded unit's
            // spline is written back into his SEAT POSE by
            // `Unit::UpdateSplineMovement`, facing included; a follow leg has
            // no facing mode, so what gets stored is its direction of travel.
            // The seat forbids turning, so nothing ever turns him back.
            //
            // None of the combat flags above touch that path - they gate
            // attacking, not following. These two do:
            // DISABLE_MOVE is what PetAI itself checks before its whole
            // stay/follow block, and the state root is what the movement
            // generators respect, `Board` having set only the move flag.
            // TrinityCore roots every passenger this way from the core side.
            pKrennan->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            pKrennan->addUnitState(UNIT_STAT_ROOT);

            // Seat him the way the core seats a vehicle's own accessories.
            // `VehicleInfo::Initialize` does exactly this - summon at the
            // vehicle, then cast the ride spell with the seat index in its
            // basepoints - and accessories sit correctly, which is the whole
            // point: the CONTROL_VEHICLE aura it creates is what the client is
            // actually told about. Calling `Board` by hand attaches him on the
            // server and leaves the client to guess, which is how he ended up
            // inside the horse, facing the wrong way.
            //
            // The teleport matters for the same reason it does for accessories:
            // `Board` derives the seat pose from where the passenger IS, and he
            // is summoned at the man in the tree, high above the saddle.
            pKrennan->NearTeleportTo(m_creature->Where().X(),
                m_creature->Where().Y(), m_creature->Where().Z(),
                m_creature->Where().Facing());

            int32 iSeat = HORSE_SEAT_KRENNAN + 1;
            pKrennan->CastCustomSpell(m_creature, SPELL_RIDE_VEHICLE_HARDCODED,
                &iSeat, NULL, NULL, true);

            // Make every client build him again. A passenger is drawn on the
            // vehicle's seat attachment, and the client learns to do that from
            // the transport block in the CREATE packet - which is why a
            // vehicle's own accessories look right: they are boarded before
            // anyone has ever seen them. Krennan is seen standing on the ground
            // first and boarded afterwards, so without this the client keeps
            // the copy it already has and draws him at a raw offset, half
            // inside the horse.
            pKrennan->UpdateObjectVisibility();

            // The spell's own KILL_CREDIT cannot reach the player - see above.
            pRider->KilledMonsterCredit(NPC_TRAPPED_KRENNAN);

            // The man is off the branch now, so the one still hanging in it has
            // to go; two Krennans in the same courtyard reads as a bug.
            if (Creature* pTrapped = GetClosestCreatureWithEntry(m_creature,
                    NPC_TRAPPED_KRENNAN, float(HORSE_SEARCH_RANGE)))
            {
                // His spawn carries spawntimesecs 300, so despawning him left
                // the tree empty for five minutes and the next player with no
                // one to rescue. Retail phases him away per player; the nearest
                // this server can offer is a short absence.
                pTrapped->SetRespawnDelay(HORSE_TRAPPED_RESPAWN);
                pTrapped->ForcedDespawn();
            }

            m_uiStage = HORSE_RETURN;
            RunPath(HORSE_PATH_BACK, HORSE_PATH_BACK_SIZE);
        }

        /// Keeps the passenger from turning round on the way home.
        ///
        /// Passive, pacified and with his attack stopped he still will not
        /// swing at anything - but a pet's AI goes on looking for something to
        /// point at, and a creature faces whatever sits in UNIT_FIELD_TARGET.
        /// Clearing it each poll is what actually leaves him riding quietly.
        void KeepKrennanIdle()
        {
            VehicleInfo* pKit = m_creature->GetVehicleInfo();

            if (!pKit)
            {
                return;
            }

            Unit* pKrennan = pKit->GetPassenger(HORSE_SEAT_KRENNAN);

            if (pKrennan && !pKrennan->GetTargetGuid().IsEmpty())
            {
                pKrennan->SetTargetGuid(ObjectGuid());
            }
        }

        /// Back at the barricade: everyone off.
        void SetDown()
        {
            m_uiStage = HORSE_DONE;
            m_creature->SetRoot(true);

            VehicleInfo* pKit = m_creature->GetVehicleInfo();

            if (!pKit)
            {
                return;
            }

            // Krennan first, and the man who says goodbye is NOT the guardian
            // who rode here.
            //
            // A pet cannot outlive its owner. `Pet::Update` unsummons the moment
            // `GetOwner()` comes back NULL, and unboarding the rider at the end
            // of this function despawns the horse a second later - so the
            // guardian was destroyed roughly a second after the dismount, every
            // time, well inside the five seconds he is meant to stand there.
            // The diagnostic caught it exactly:
            //     PETDIAG: unsummon Pet (...): owner=NONE ... boarded=0
            // Detaching him from the guardian list does not help either: the
            // owner GUID is what `GetOwner()` reads, and clearing that trips the
            // very same `!owner` test.
            //
            // So the guardian is retired here and a plain timed summon takes his
            // place - summoned by the PLAYER, who is not about to disappear, and
            // on TEMPSPAWN_TIMED_DESPAWN, which answers to nothing but its own
            // clock.
            Player* pRider = GetRider();
            Pet* pKrennanPet = m_creature->FindGuardianWithEntry(NPC_RESCUED_KRENNAN);

            if (Unit* pPassenger = pKit->GetPassenger(HORSE_SEAT_KRENNAN))
            {
                // Where the core would have set him down: a yard off the left
                // flank (seat 1 exits left), on the ground, facing as the horse
                // does. Retail put him at exactly that spot to say his line.
                const float fO = m_creature->Where().Facing();
                const float fX = m_creature->Where().X() + cos(fO + M_PI_F / 2);
                const float fY = m_creature->Where().Y() + sin(fO + M_PI_F / 2);
                const float fZ = m_creature->Where().Z();

                // His own ride aura, not the rider's - the caster-scoped
                // overload leaves the player seated until the last line here.
                m_creature->RemoveSpellsCausingAura(SPELL_AURA_CONTROL_VEHICLE,
                    pPassenger->GetObjectGuid());

                if (pKrennanPet)
                {
                    pKrennanPet->Unsummon(PET_SAVE_AS_DELETED, m_creature);
                }

                if (pRider)
                {
                    if (Creature* pFarewell = pRider->SummonCreature(
                            NPC_RESCUED_KRENNAN, fX, fY, fZ, fO,
                            TEMPSPAWN_TIMED_DESPAWN, HORSE_KRENNAN_DESPAWN))
                    {
                        DoScriptText(SAY_KRENNAN_RESCUED, pFarewell);
                    }
                }
            }

            // The rider LAST, and never through `UnBoard`. The player holds the
            // control seat, and that relationship belongs to the CONTROL_VEHICLE
            // aura: `Aura::HandleAuraControlVehicle` calls `UnBoard` itself when
            // the aura goes. Calling `UnBoard` here as well takes him off twice,
            // and the second pass walks straight into
            //     MANGOS_ASSERT(itr != m_passengers.end())
            // which aborts the process - no crash dump, and the buffered tail of
            // the log goes with it, which is exactly how this presented.
            //
            // `UnBoard` also despawns this horse a second later, so nothing of
            // the HORSE may be touched after this line: it ends the ride.
            //
            // The rider's landing spot is not the core's to pick here. Its
            // default sets a control-seat passenger down off the RIGHT flank;
            // this dismount belongs on the LEFT, beside Krennan. The exit
            // spline the core just launched is re-issued to the left-flank
            // spot before the packet batch flushes, so the client only ever
            // plays the corrected one - same shape, same facing, same exit
            // flag, other side. Half a yard forward keeps him at Krennan's
            // shoulder instead of on top of him.
            const float fEndO = m_creature->Where().Facing();
            const float fEndX = m_creature->Where().X() + cos(fEndO) * 0.5f +
                                cos(fEndO + M_PI_F / 2);
            const float fEndY = m_creature->Where().Y() + sin(fEndO) * 0.5f +
                                sin(fEndO + M_PI_F / 2);
            const float fEndZ = m_creature->Where().Z();

            m_creature->RemoveSpellsCausingAura(SPELL_AURA_CONTROL_VEHICLE);

            if (pRider)
            {
                Movement::MoveSplineInit init(*pRider);
                init.MoveTo(fEndX, fEndY, fEndZ);
                init.SetFacing(fEndO);
                init.SetExitVehicle();
                init.Launch();
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiAnnounceTimer)
            {
                if (m_uiAnnounceTimer <= uiDiff)
                {
                    m_uiAnnounceTimer = 0;
                    m_uiStage = HORSE_AWAIT_RESCUE;
                    Announce();
                }
                else
                {
                    m_uiAnnounceTimer -= uiDiff;
                }
            }

            if (m_uiKrennanCallTimer)
            {
                if (m_uiKrennanCallTimer <= uiDiff)
                {
                    m_uiKrennanCallTimer = 0;
                    KrennanCallsOut();
                }
                else
                {
                    m_uiKrennanCallTimer -= uiDiff;
                }
            }

            // A ride spline has run its course: the outbound one hands over
            // to the jump, the return one to the dismount.
            if (m_bRunning && m_creature->movespline->Finalized())
            {
                m_bRunning = false;

                if (m_uiStage == HORSE_OUTBOUND)
                {
                    m_uiStage = HORSE_JUMPING;
                    m_creature->GetMotionMaster()->MoveJump(HORSE_JUMP_X,
                        HORSE_JUMP_Y, HORSE_JUMP_Z, HORSE_JUMP_SPEED,
                        HORSE_JUMP_ARC, HORSE_POINT_JUMP);
                }
                else if (m_uiStage == HORSE_RETURN)
                {
                    SetDown();
                }
            }

            if (m_uiPollTimer <= uiDiff)
            {
                m_uiPollTimer = HORSE_POLL_INTERVAL;

                if (m_uiStage == HORSE_AWAIT_RIDER)
                {
                    if (GetRider())
                    {
                        m_uiStage = HORSE_OUTBOUND;
                        RunPath(HORSE_PATH_OUT, HORSE_PATH_OUT_SIZE);
                    }
                }
                else if (m_uiStage == HORSE_AWAIT_RESCUE)
                {
                    TakeKrennanAboard();
                }
                else if (m_uiStage == HORSE_RETURN)
                {
                    KeepKrennanIdle();
                }
            }
            else
            {
                m_uiPollTimer -= uiDiff;
            }

            // Deliberately slower than the boarding poll. Culling is visible -
            // a Ripper walks in, is sent home, and comes straight back - so
            // running it every tick turns the whole courtyard into a stutter.
            // Once the rescue works this window is only a few seconds anyway.
            if (m_uiCullTimer <= uiDiff)
            {
                m_uiCullTimer = HORSE_CULL_INTERVAL;

                if (m_uiStage != HORSE_AWAIT_RIDER && m_uiStage != HORSE_DONE)
                {
                    ShedExcessAttackers();
                }
            }
            else
            {
                m_uiCullTimer -= uiDiff;
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_greymanes_horseAI(pCreature);
    }
};

/*######
# npc_bloodfang_ripper_cannon
######*/

// The pack the Military District cannon fires on.
//
// This was first built as a waypoint patrol in to the cannon and back, which
// cannot work: a patrol runs on its own clock, so where the pack stood when a
// player turned the quest in was luck, and half the time they were already at
// the far end before a shell was fired. `MovementType` 2 also respawns a
// creature at its CURRENT waypoint rather than its spawn, so once killed they
// came back at the impact point instead of the muster.
//
// The retail capture shows no patrol at all - each ripper makes ONE long move
// as the scene starts, 57 yards down to 24. So they stand at the muster and
// charge on cue.
//
// The cue is the camera. 50420 `Gilneas - Cannon Camera` is summoned by 93522
// only for this shot and lives about seven seconds, which makes its presence an
// exact marker for "the scene is running" - no new plumbing, and it cannot
// fire early because nothing else ever spawns it.
enum
{
    NPC_CANNON_CAMERA        = 50420,
    NPC_COMMANDEERED_CANNON  = 35914,

    RIPPER_WATCH_RANGE       = 150,   ///< yards; the camera sits well inside this
    RIPPER_POLL_INTERVAL     = 500,   ///< ms between checks for the camera
    RIPPER_CHARGE_DIST       = 30,    ///< yards forward; muster to impact is 29.7
    RIPPER_POINT_CHARGE      = 1,
};

struct npc_bloodfang_ripper_cannon : public CreatureScript
{
    npc_bloodfang_ripper_cannon() : CreatureScript("npc_bloodfang_ripper_cannon") {}

    struct npc_bloodfang_ripper_cannonAI : public ScriptedAI
    {
        npc_bloodfang_ripper_cannonAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            SetRunning();
        }

        uint32 m_uiPollTimer;
        bool   m_bCharging;

        /// The second argument is the point: `asDefault` is what sets
        /// UNIT_STAT_RUNNING, and that state - not the movement flag - is what
        /// the movement generators read when they decide a leg's pace.
        void SetRunning()
        {
            m_creature->SetWalk(false, true);
        }

        void Reset() override
        {
            SetRunning();
            m_uiPollTimer = RIPPER_POLL_INTERVAL;
            m_bCharging = false;

            // Out of sight until there is a shot to be part of. Thirteen of
            // them loitering in the street between runs is not what the scene
            // is for, and hiding them is cheaper and steadier than despawning:
            // a despawn would respawn on its own timer and flicker.
            m_creature->SetVisibility(VISIBILITY_OFF);
        }

        /// Forward, keeping formation.
        ///
        /// Each ripper advances along ITS OWN facing rather than turning to
        /// point at the cannon. Aiming every one of them at a single object
        /// converges their paths - a front eight yards wide at fifty yards out
        /// arrives less than four wide at twenty - and thirteen bodies land on
        /// top of each other. Translating them all by the same vector keeps the
        /// spacing they started with, and the spawn orientation already holds
        /// the direction of the run, so the DB decides the lane and this only
        /// decides how far.
        void Charge()
        {
            const float fFacing = m_creature->Where().Facing();

            m_creature->GetMotionMaster()->MovePoint(RIPPER_POINT_CHARGE,
                m_creature->Where().X() + cos(fFacing) * float(RIPPER_CHARGE_DIST),
                m_creature->Where().Y() + sin(fFacing) * float(RIPPER_CHARGE_DIST),
                m_creature->Where().Z());
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_bCharging)
            {
                if (m_uiPollTimer <= uiDiff)
                {
                    m_uiPollTimer = RIPPER_POLL_INTERVAL;

                    if (GetClosestCreatureWithEntry(m_creature, NPC_CANNON_CAMERA,
                            float(RIPPER_WATCH_RANGE)))
                    {
                        m_bCharging = true;

                        // They arrive with the camera, not before it.
                        m_creature->SetVisibility(VISIBILITY_ON);
                        Charge();
                    }
                }
                else
                {
                    m_uiPollTimer -= uiDiff;
                }
            }

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_bloodfang_ripper_cannonAI(pCreature);
    }
};

/*######
# npc_crowleys_horse
######*/

// `Sacrifices` (14212) is the second vehicle ride of the Gilneas start, and
// like the first it is only half a spell. Clicking the horse beside Lord
// Darius Crowley casts 67001 `Summon Crowley's Horse`, whose SummonProperties
// 161 is a vehicle group, so `Spell::DoSummonVehicle` seats the player the
// moment the horse appears - and then nothing happens. The route is here.
//
// Every coordinate below was read out of a retail capture of this quest
// (Worgen Hunter, build 18019, Gilneas City) by decoding `SMSG_MONSTER_MOVE`:
// each leg arrives as its own spline whose destination is a plain float
// triple, so the whole ride is recoverable in order. It runs from t=2939.03
// (seated) to t=3018.56 (set down), 79.6 seconds.
//
// The horse is summoned at (-1738.620, 1657.992) and springs the courtyard
// barricade almost at once - there is no lap of the yard first. Retail sends
// that as ONE spline of 1672 ms whose first 359 ms is a run through
// (-1731.941, 1657.576) before it goes ballistic. `MoveJump` always passes 0
// for `SetParabolic`'s time shift, so the jump is built by hand here with the
// run-up as the spline's first vertex and the parabola starting 21% in.
//
// Both jumps peak at exactly 4 yards. Take care with that number: retail's
// field is a gravity, and TrinityCore passes it straight to its own MoveJump,
// but `MoveSpline.cpp:254` computes
//     vertical_acceleration = parabolic_amplitude * 8 / duration^2
// which makes this core's argument the APEX. Feeding it retail's gravity
// (18.5618) throws the horse 18 yards into the air.
//
// The two road legs are ONE spline each, at 12.6 yd/s. Retail actually sends
// them as some thirty short splines, but every one of those is issued while
// the previous is still running, from a point part-way along it - the horse
// never stops - and every one of them measures 12.5-12.6 yd/s. A point move
// per waypoint reproduces the points and loses the pace at every corner;
// `MovebyPath` with the velocity set keeps it. Arrival is read off
// `movespline->Finalized()`.
//
// The rider is let off by removing the vehicle's own CONTROL_VEHICLE auras,
// never by calling `UnBoard` - see the note at the end of the Greymane script
// for what the second unboard costs.
enum
{
    NPC_CROWLEY_MOUNTED       = 35230,     ///< Rides in front, from `vehicle_accessory`

    SAY_CROWLEY_ROUND_UP      = -1999947,
    SAY_CROWLEY_TAUNT         = -1999948,

    CROWLEY_SEAT_RIDER        = 0,         ///< 5106: CAN_CONTROL | CAN_CAST, offset -0.2 (rear)
    CROWLEY_SEAT_CROWLEY      = 1,         ///< 5107: offset +0.4 (front)

    CROWLEY_POLL_INTERVAL     = 250,       ///< ms between "has anyone boarded" checks
    CROWLEY_MOUNT_DELAY       = 2000,      ///< ms held still after boarding (retail: 2.45 s after the summon)
    CROWLEY_SETTLE_DELAY      = 500,       ///< ms after a jump lands before the path resumes (retail 0.39 / 0.55 s)
    CROWLEY_GATHER_DELAY      = 700,       ///< ms at the foot of the second barricade before springing it (retail 0.73 s)
    CROWLEY_SETDOWN_DELAY     = 900,       ///< ms standing at the top before everyone is let off (retail 0.93 s)
    CROWLEY_ROUND_UP_DELAY    = 9800,      ///< ms after boarding for Crowley's first line (retail 9.9 s after seating)
    CROWLEY_TAUNT_DELAY       = 41700,     ///< ms after boarding for Crowley's second line (retail 41.7 s)
    CROWLEY_DESPAWN_DELAY     = 1300,      ///< ms Crowley lingers once the ride is over (retail 1.3 s)
    CROWLEY_HORSE_DESPAWN     = 4200,      ///< ms the horse lingers once the rider is off (retail 4.2 s)
};

/// Spline velocity of both road legs - 7 x 1.8, the template's own run speed.
static const float CROWLEY_RIDE_SPEED = 12.6f;

/// The few strides taken before the courtyard barricade is sprung.
static const float CROWLEY_RUNUP_X = -1731.941f;
static const float CROWLEY_RUNUP_Y = 1657.576f;
static const float CROWLEY_RUNUP_Z = 20.49000f;

/// Out of the courtyard and along the north street, ending where the second
/// jump begins. Points 14-15 and 17-23 are the ramp up and over the barricade;
/// the rest are the legs retail walks.
static const float CROWLEY_PATH_ONE[][3] =
{
    { -1686.120f, 1655.639f, 20.61058f },
    { -1668.156f, 1639.432f, 20.61058f },
    { -1666.630f, 1628.859f, 20.53271f },
    { -1670.760f, 1617.431f, 20.61058f },
    { -1693.648f, 1592.639f, 20.61058f },
    { -1708.017f, 1590.550f, 20.61042f },
    { -1719.366f, 1595.649f, 20.61042f },
    { -1720.566f, 1610.141f, 20.60740f },
    { -1711.898f, 1629.703f, 20.60740f },
    { -1698.149f, 1656.483f, 20.61058f },
    { -1696.424f, 1682.012f, 20.52616f },
    { -1684.675f, 1700.120f, 20.46484f },
    { -1658.309f, 1710.313f, 20.58983f },
    { -1631.478f, 1710.857f, 20.68900f },
    { -1628.728f, 1711.607f, 21.18900f },
    { -1625.477f, 1712.299f, 21.84700f },
    { -1624.161f, 1712.248f, 22.21500f },
    { -1616.911f, 1711.998f, 22.96500f },
    { -1609.161f, 1711.498f, 22.96500f },
    { -1601.911f, 1711.248f, 21.96500f },
    { -1599.911f, 1710.998f, 21.46500f },
    { -1598.161f, 1710.998f, 20.96500f },
    { -1593.661f, 1710.498f, 20.71500f },
    { -1592.095f, 1710.710f, 20.58986f }
};

/// The long loop east and back, ending on the rise beside the quest's taker.
/// Points 1-4 are the curve out of the second landing, 8-12 a corner, and
/// 24-27 plus 29-31 the climb up to Tobias Mistmantle.
static const float CROWLEY_PATH_TWO[][3] =
{
    { -1564.396f, 1704.136f, 20.55000f },
    { -1560.146f, 1698.386f, 20.55000f },
    { -1554.146f, 1691.886f, 20.55000f },
    { -1549.896f, 1687.636f, 20.55000f },
    { -1545.082f, 1684.233f, 20.60992f },
    { -1542.793f, 1656.083f, 20.60992f },
    { -1542.998f, 1640.535f, 20.60992f },
    { -1541.354f, 1637.965f, 20.90000f },
    { -1539.854f, 1636.715f, 21.10000f },
    { -1536.854f, 1633.965f, 21.10000f },
    { -1535.604f, 1632.965f, 20.86000f },
    { -1533.104f, 1631.965f, 20.86000f },
    { -1520.733f, 1618.097f, 20.61054f },
    { -1498.792f, 1621.738f, 20.61054f },
    { -1468.356f, 1630.780f, 20.61054f },
    { -1441.646f, 1627.670f, 20.61054f },
    { -1429.844f, 1616.627f, 20.58984f },
    { -1417.991f, 1586.554f, 20.58984f },
    { -1429.177f, 1548.783f, 20.84849f },
    { -1447.417f, 1532.790f, 20.60095f },
    { -1463.104f, 1535.031f, 20.61054f },
    { -1483.035f, 1544.151f, 20.61054f },
    { -1505.156f, 1568.611f, 20.61053f },
    { -1510.075f, 1572.576f, 23.57500f },
    { -1515.575f, 1574.826f, 26.32500f },
    { -1518.325f, 1576.076f, 26.82500f },
    { -1521.825f, 1577.576f, 26.82500f },
    { -1531.599f, 1581.872f, 26.53872f },
    { -1532.372f, 1580.897f, 26.84800f },
    { -1538.122f, 1576.147f, 29.09800f },
    { -1539.122f, 1575.397f, 29.34800f },
    { -1540.594f, 1574.429f, 29.20671f }
};

static const uint32 CROWLEY_PATH_ONE_SIZE = 24;
static const uint32 CROWLEY_PATH_TWO_SIZE = 32;

/// Over the courtyard barricade. Run-up plus leap: 29.9 yards in 1672 ms, the
/// parabola starting at 359 ms - 21.5% of the way in.
static const float CROWLEY_JUMP_ONE_X = -1714.762f;
static const float CROWLEY_JUMP_ONE_Y = 1673.160f;
static const float CROWLEY_JUMP_ONE_Z = 20.49182f;
static const float CROWLEY_JUMP_ONE_SPEED = 17.87f;
static const float CROWLEY_JUMP_ONE_SHIFT = 0.215f;

/// Over the second, at the end of the north street. 25.5 yards, 1419 ms.
static const float CROWLEY_JUMP_TWO_X = -1566.710f;
static const float CROWLEY_JUMP_TWO_Y = 1708.040f;
static const float CROWLEY_JUMP_TWO_Z = 20.48490f;
static const float CROWLEY_JUMP_TWO_SPEED = 18.0f;

/// Apex above the line, in yards - NOT retail's gravity field. See the note above.
static const float CROWLEY_JUMP_ARC = 4.0f;

enum CrowleyStage
{
    CROWLEY_AWAIT_RIDER = 0,   ///< Standing where it was summoned
    CROWLEY_SETTLING,          ///< Boarded, held still for a beat
    CROWLEY_JUMP_ONE,          ///< Run-up and over the first barricade
    CROWLEY_PATH_ONE_RUN,      ///< Running CROWLEY_PATH_ONE
    CROWLEY_GATHERING,         ///< Stood at the foot of the second barricade
    CROWLEY_JUMP_TWO,          ///< Over the second
    CROWLEY_PATH_TWO_RUN,      ///< Running CROWLEY_PATH_TWO
    CROWLEY_ARRIVED,           ///< At the top; a beat before the dismount
    CROWLEY_DONE
};

struct npc_crowleys_horse : public CreatureScript
{
    npc_crowleys_horse() : CreatureScript("npc_crowleys_horse") {}

    struct npc_crowleys_horseAI : public ScriptedAI
    {
        npc_crowleys_horseAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiStage;
        bool   m_bRunning;          ///< A spline is out; watch for it to finish
        uint32 m_uiActTimer;        ///< ms to the next scripted step; 0 is idle
        uint32 m_uiRoundUpTimer;
        uint32 m_uiTauntTimer;
        uint32 m_uiPollTimer;

        void Reset() override
        {
            m_uiStage = CROWLEY_AWAIT_RIDER;
            m_bRunning = false;
            m_uiActTimer = 0;
            m_uiRoundUpTimer = 0;
            m_uiTauntTimer = 0;
            m_uiPollTimer = CROWLEY_POLL_INTERVAL;

            if (VehicleInfo* pKit = m_creature->GetVehicleInfo())
            {
                pKit->SetDespawnDelay(CROWLEY_HORSE_DESPAWN);
            }
        }

        /// It carries the player; nothing it passes is its business.
        void AttackStart(Unit* /*pWho*/) override {}
        void MoveInLineOfSight(Unit* /*pWho*/) override {}

        Player* GetRider() const
        {
            if (VehicleInfo* pKit = m_creature->GetVehicleInfo())
            {
                Unit* pRider = pKit->GetPassenger(CROWLEY_SEAT_RIDER);

                if (pRider && pRider->GetTypeId() == TYPEID_PLAYER)
                {
                    return (Player*)pRider;
                }
            }

            return NULL;
        }

        Creature* GetCrowley() const
        {
            if (VehicleInfo* pKit = m_creature->GetVehicleInfo())
            {
                Unit* pCrowley = pKit->GetPassenger(CROWLEY_SEAT_CROWLEY);

                if (pCrowley && pCrowley->GetTypeId() == TYPEID_UNIT)
                {
                    return (Creature*)pCrowley;
                }
            }

            return NULL;
        }

        /// A whole road leg as one spline at the ride's pace.
        void RunPath(const float pPath[][3], uint32 uiSize)
        {
            m_creature->SetRoot(false);

            // Slot 0 is a placeholder: `Launch` overwrites it with the horse's
            // current position.
            Movement::PointsArray points;
            points.reserve(uiSize + 1);
            points.push_back(Geometry::Vector3(0.0f, 0.0f, 0.0f));

            for (uint32 i = 0; i < uiSize; ++i)
            {
                points.push_back(Geometry::Vector3(pPath[i][0], pPath[i][1], pPath[i][2]));
            }

            Movement::MoveSplineInit init(*m_creature);
            init.MovebyPath(points);
            init.SetWalk(false);
            init.SetVelocity(CROWLEY_RIDE_SPEED);
            init.Launch();

            m_bRunning = true;
        }

        /// A leap, optionally with a run-up as the first vertex; the parabola
        /// starts fShift of the way through, as retail's first jump does.
        void Spring(const float* pRunUp, float fX, float fY, float fZ,
                    float fSpeed, float fShift)
        {
            m_creature->SetRoot(false);

            Movement::PointsArray points;
            points.push_back(Geometry::Vector3(0.0f, 0.0f, 0.0f));

            if (pRunUp)
            {
                points.push_back(Geometry::Vector3(pRunUp[0], pRunUp[1], pRunUp[2]));
            }

            points.push_back(Geometry::Vector3(fX, fY, fZ));

            Movement::MoveSplineInit init(*m_creature);
            init.MovebyPath(points);
            init.SetParabolic(CROWLEY_JUMP_ARC, fShift);
            init.SetVelocity(fSpeed);
            init.Launch();

            m_bRunning = true;
        }

        /// Ends the ride at Tobias Mistmantle's feet.
        void SetDown()
        {
            m_uiStage = CROWLEY_DONE;
            m_creature->GetMotionMaster()->MoveIdle();

            // Crowley first, and by his own aura, so the rider stays seated
            // until the last line here.
            if (Creature* pCrowley = GetCrowley())
            {
                m_creature->RemoveSpellsCausingAura(SPELL_AURA_CONTROL_VEHICLE,
                    pCrowley->GetObjectGuid());
                pCrowley->ForcedDespawn(CROWLEY_DESPAWN_DELAY);
            }

            // This lets the rider off and despawns the horse a second later, so
            // nothing may be touched after it.
            m_creature->RemoveSpellsCausingAura(SPELL_AURA_CONTROL_VEHICLE);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiRoundUpTimer)
            {
                if (m_uiRoundUpTimer <= uiDiff)
                {
                    m_uiRoundUpTimer = 0;

                    if (Creature* pCrowley = GetCrowley())
                    {
                        DoScriptText(SAY_CROWLEY_ROUND_UP, pCrowley);
                    }
                }
                else
                {
                    m_uiRoundUpTimer -= uiDiff;
                }
            }

            if (m_uiTauntTimer)
            {
                if (m_uiTauntTimer <= uiDiff)
                {
                    m_uiTauntTimer = 0;

                    if (Creature* pCrowley = GetCrowley())
                    {
                        DoScriptText(SAY_CROWLEY_TAUNT, pCrowley);
                    }
                }
                else
                {
                    m_uiTauntTimer -= uiDiff;
                }
            }

            if (m_uiActTimer)
            {
                if (m_uiActTimer <= uiDiff)
                {
                    m_uiActTimer = 0;

                    switch (m_uiStage)
                    {
                        case CROWLEY_SETTLING:
                        {
                            static const float runUp[3] =
                                { CROWLEY_RUNUP_X, CROWLEY_RUNUP_Y, CROWLEY_RUNUP_Z };
                            m_uiStage = CROWLEY_JUMP_ONE;
                            Spring(runUp, CROWLEY_JUMP_ONE_X, CROWLEY_JUMP_ONE_Y,
                                CROWLEY_JUMP_ONE_Z, CROWLEY_JUMP_ONE_SPEED,
                                CROWLEY_JUMP_ONE_SHIFT);
                            break;
                        }
                        case CROWLEY_JUMP_ONE:
                            m_uiStage = CROWLEY_PATH_ONE_RUN;
                            RunPath(CROWLEY_PATH_ONE, CROWLEY_PATH_ONE_SIZE);
                            break;
                        case CROWLEY_GATHERING:
                            m_uiStage = CROWLEY_JUMP_TWO;
                            Spring(NULL, CROWLEY_JUMP_TWO_X, CROWLEY_JUMP_TWO_Y,
                                CROWLEY_JUMP_TWO_Z, CROWLEY_JUMP_TWO_SPEED, 0.0f);
                            break;
                        case CROWLEY_JUMP_TWO:
                            m_uiStage = CROWLEY_PATH_TWO_RUN;
                            RunPath(CROWLEY_PATH_TWO, CROWLEY_PATH_TWO_SIZE);
                            break;
                        case CROWLEY_ARRIVED:
                            SetDown();
                            break;
                        default:
                            break;
                    }
                }
                else
                {
                    m_uiActTimer -= uiDiff;
                }
            }

            // A spline has run its course. Each stage ends standing still for a
            // beat before the next begins, as the capture shows.
            if (m_bRunning && m_creature->movespline->Finalized())
            {
                m_bRunning = false;
                m_creature->SetRoot(true);

                switch (m_uiStage)
                {
                    case CROWLEY_JUMP_ONE:
                    case CROWLEY_JUMP_TWO:
                        m_uiActTimer = CROWLEY_SETTLE_DELAY;
                        break;
                    case CROWLEY_PATH_ONE_RUN:
                        m_uiStage = CROWLEY_GATHERING;
                        m_uiActTimer = CROWLEY_GATHER_DELAY;
                        break;
                    case CROWLEY_PATH_TWO_RUN:
                        m_uiStage = CROWLEY_ARRIVED;
                        m_uiActTimer = CROWLEY_SETDOWN_DELAY;
                        break;
                    default:
                        break;
                }
            }

            if (m_uiPollTimer <= uiDiff)
            {
                m_uiPollTimer = CROWLEY_POLL_INTERVAL;

                if (m_uiStage == CROWLEY_AWAIT_RIDER)
                {
                    if (Player* pRider = GetRider())
                    {
                        // Retail never hands over the reins. Across the whole
                        // eighty second capture the client sends five movement
                        // packets and every one of them acks a jump or the
                        // dismount - Crowley drives, the player throws.
                        //
                        // Seat 0 carries CAN_CONTROL, so `ApplySeatMods` makes
                        // the player the mover regardless, and the first time
                        // he touches a key his movement replaces the running
                        // spline: it is finalized where it stands and the
                        // chain stops for good. Take the reins back.
                        // CAN_CAST is applied separately, so `Throw Torch`
                        // stays on the bar, and `UnBoard` hands the player his
                        // own body back at the end.
                        pRider->SetClientControl(m_creature, 0);

                        m_uiStage = CROWLEY_SETTLING;
                        m_creature->SetRoot(true);
                        m_uiActTimer = CROWLEY_MOUNT_DELAY;

                        // Both lines come well into the ride: the first about
                        // ten seconds in, on the first street; the second a
                        // couple of seconds after the second barricade.
                        m_uiRoundUpTimer = CROWLEY_ROUND_UP_DELAY;
                        m_uiTauntTimer = CROWLEY_TAUNT_DELAY;
                    }
                }
                else if (m_uiStage != CROWLEY_DONE && !GetRider())
                {
                    // He got off, logged out, or died on the way. Nothing left
                    // to carry, so take the horse with him.
                    m_uiStage = CROWLEY_DONE;
                    m_creature->ForcedDespawn();
                }
            }
            else
            {
                m_uiPollTimer -= uiDiff;
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_crowleys_horseAI(pCreature);
    }
};

/*######
# npc_frenzied_stalker
######*/

// `Last Stand` (14222) is fought inside Light's Dawn Cathedral: eight
// Northgate Rebels (41015) hold their posts across the nave and the pack of
// Frenzied Stalkers throws itself at them while the player kills eight.
//
// Left to plain aggro the whole room collapses into one melee ball on the
// first pull - every stalker and every rebel converges on the first fight
// that breaks out. Retail keeps the brawl SPREAD: each defender fights at his
// own post with a wolf on him, and replacements walk in through the door as
// the engaged ones die. This AI reproduces exactly that:
//
//   * A stalker with no target looks every couple of seconds for the nearest
//     rebel with FEWER THAN TWO stalkers on him, walks over - through the
//     door, if that is where it stands - and takes that post. Two wolves per
//     guard, so the fight spreads across all eight posts.
//   * Taking a post re-anchors the leash there (the walk can be over 40
//     yards, which is the spawn-leash radius); death puts the anchor back so
//     the respawn rises at the entrance and walks in like the next wave.
//   * Assist calls are swallowed - they are what piled the room up.
//
// The sparring floors (Rel22_07_058/067) keep the theatre bloodless: rebels
// hold at 85%, stalkers at 80%, and only the player actually kills.
enum
{
    NPC_NORTHGATE_REBEL         = 41015,

    SPELL_REBEL_SHOOT           = 6660,    ///< What the capture's rebels fire at range

    STALKER_SEEK_INTERVAL       = 2000,    ///< ms between looks for a free post
    STALKER_POST_SEARCH_RANGE   = 90,      ///< yd to look for rebels in
    STALKER_PER_POST            = 2,       ///< wolves a single rebel fights at once
};

// The defenders of the same battle. Their spawns already sit on the retail
// posts - the create blocks in the capture match the stock rows to the
// decimal - but a default AI walked them off those posts: chases followed
// whatever they aggroed, assist calls dragged them across the nave, and the
// whole line ended in one knot. A post is HELD: the rebel fights whatever
// stands in front of him and never takes a step. On retail no defender moves
// through the entire 90 second window.
struct npc_northgate_rebel : public CreatureScript
{
    npc_northgate_rebel() : CreatureScript("npc_northgate_rebel") {}

    struct npc_northgate_rebelAI : public ScriptedAI
    {
        npc_northgate_rebelAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        void Reset() override {}

        /// Fight what is biting HIM, and hold the post. The guard is the
        /// point: a bitten neighbour's call-for-help worker invokes
        /// `AttackStart` on every friendly rebel in radius DIRECTLY - not
        /// through the AI-event channel - and accepting those re-points the
        /// whole line at whichever wolf bit somebody last (the diagnostics
        /// caught seven rebels sharing one victim across the room, one of
        /// them with an empty attacker set).
        void AttackStart(Unit* pWho) override
        {
            if (!pWho || !m_creature->CanInitiateAttack())
            {
                return;
            }

            Unit::AttackerSet const& attackers = m_creature->getAttackers();

            if (attackers.find(pWho) == attackers.end())
            {
                return;
            }

            // Already fighting one of his own biters: stay on him.
            if (Unit* pCurrent = m_creature->getVictim())
            {
                if (pCurrent->IsAlive() &&
                    attackers.find(pCurrent) != attackers.end())
                {
                    return;
                }
            }

            if (m_creature->Attack(pWho, true))
            {
                m_creature->AddThreat(pWho);
                m_creature->SetInCombatWith(pWho);
                pWho->SetInCombatWith(m_creature);
            }
        }

        /// Assist calls are what piled the line into one knot.
        void ReceiveAIEvent(AIEventType /*eventType*/, Creature* /*pSender*/,
                            Unit* /*pInvoker*/, uint32 /*uiMiscValue*/) override {}

        /// The nearest thing biting him.
        Unit* PickAttacker() const
        {
            Unit* pBest = NULL;
            float fBest = 0.0f;

            Unit::AttackerSet const& attackers = m_creature->getAttackers();

            for (Unit::AttackerSet::const_iterator itr = attackers.begin();
                 itr != attackers.end(); ++itr)
            {
                Unit* pWho = *itr;

                if (!pWho->IsAlive() || !pWho->IsTargetableForAttack())
                {
                    continue;
                }

                const float fDist = m_creature->GetDistance(pWho);

                if (!pBest || fDist < fBest)
                {
                    pBest = pWho;
                    fBest = fDist;
                }
            }

            return pBest;
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // Neither the base UpdateAI - its Cata arm casts
            // `ReachWithSpellAttack` triggered on EVERY tick when the victim
            // is out of melee reach, a machine gun on a planted creature -
            // NOR `SelectHostileTarget`. The sparring floor keeps every wolf
            // hit at zero damage, so this rebel's THREAT list stays empty;
            // given attackers, an empty list and an idle motion generator,
            // `SelectHostileTarget` refuses to pick anything and the whole
            // update used to die on its return. A planted defender takes his
            // victim from the ATTACKER set instead: whoever is biting him.
            Unit* pVictim = m_creature->getVictim();

            // A victim that is dead - or that is not actually biting him (a
            // call-for-help leftover) - is dropped, and the next real biter
            // adopted.
            if (pVictim)
            {
                Unit::AttackerSet const& attackers = m_creature->getAttackers();

                if (!pVictim->IsAlive() ||
                    attackers.find(pVictim) == attackers.end())
                {
                    m_creature->AttackStop();
                    pVictim = NULL;
                }
            }

            if (!pVictim)
            {
                pVictim = PickAttacker();

                if (pVictim)
                {
                    AttackStart(pVictim);
                }
            }

            if (!pVictim)
            {
                return;
            }

            // Face the fight, the way the core faces every other creature:
            // `SelectHostileTarget` runs `SetInFront(target)` each tick for
            // anything with a threat list, and the planted rebel bypasses that
            // path. Without it his SERVER orientation stays at spawn and the
            // 120-degree arc gate in `UpdateMeleeAttackingState` swallows most
            // of his swings. No packet is needed for the LOOK of it - the
            // 4.3.4 client turns a model towards its UNIT_FIELD_TARGET on its
            // own (the training-dummy note in `Unit::Attack` documents it),
            // and `Attack()` set that field when the victim was adopted.
            m_creature->SetInFront(pVictim);

            if (InMeleeReach(*m_creature, *pVictim))
            {
                DoMeleeAttackIfReady();
            }
            // The rifle, at the rifle's own pace - the ranged attack timer,
            // exactly as the militia AI above gates it.
            else if (m_creature->isAttackReady(RANGED_ATTACK))
            {
                DoCastSpellIfCan(pVictim, SPELL_REBEL_SHOOT);
                m_creature->resetAttackTimer(RANGED_ATTACK);
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_northgate_rebelAI(pCreature);
    }
};

struct npc_frenzied_stalker : public CreatureScript
{
    npc_frenzied_stalker() : CreatureScript("npc_frenzied_stalker") {}

    struct npc_frenzied_stalkerAI : public ScriptedAI
    {
        npc_frenzied_stalkerAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiSeekTimer;

        void Reset() override
        {
            // Staggered, so the room does not move as one man.
            m_uiSeekTimer = urand(500, STALKER_SEEK_INTERVAL);
        }

        /// The nearest rebel with room left in his fight.
        Creature* FindFreePost() const
        {
            std::list<Creature*> lRebels;
            GetCreatureListWithEntryInGrid(lRebels, m_creature,
                NPC_NORTHGATE_REBEL, float(STALKER_POST_SEARCH_RANGE));

            Creature* pBest = NULL;
            float fBest = 0.0f;

            for (std::list<Creature*>::const_iterator itr = lRebels.begin();
                 itr != lRebels.end(); ++itr)
            {
                Creature* pRebel = *itr;

                if (!pRebel->IsAlive())
                {
                    continue;
                }

                uint32 uiOnHim = 0;
                Unit::AttackerSet const& attackers = pRebel->getAttackers();

                for (Unit::AttackerSet::const_iterator at = attackers.begin();
                     at != attackers.end(); ++at)
                {
                    if ((*at) != m_creature && (*at)->GetEntry() == m_creature->GetEntry())
                    {
                        ++uiOnHim;
                    }
                }

                if (uiOnHim >= STALKER_PER_POST)
                {
                    continue;
                }

                const float fDist = m_creature->GetDistance(pRebel);

                if (!pBest || fDist < fBest)
                {
                    pBest = pRebel;
                    fBest = fDist;
                }
            }

            return pBest;
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            // Back to the books: the replacement rises at the entrance and
            // walks in, not on the post this one fell at.
            m_creature->ResetSpawn();
        }

        /// The pile-up was assist calls dragging every wolf in the room into
        /// the first fight; a stalker minds its own post.
        void ReceiveAIEvent(AIEventType /*eventType*/, Creature* /*pSender*/,
                            Unit* /*pInvoker*/, uint32 /*uiMiscValue*/) override {}

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->getVictim())
            {
                if (m_uiSeekTimer <= uiDiff)
                {
                    m_uiSeekTimer = STALKER_SEEK_INTERVAL;

                    if (Creature* pPost = FindFreePost())
                    {
                        // The walk to the post can be longer than the spawn
                        // leash; home is the post now, and JustDied puts it
                        // back for the next one.
                        m_creature->SetSpawn(Geometry::Vector3(
                            pPost->Where().X(), pPost->Where().Y(),
                            pPost->Where().Z()), m_creature->Where().Facing());

                        AttackStart(pPost);
                    }
                }
                else
                {
                    m_uiSeekTimer -= uiDiff;
                }
            }

            ScriptedAI::UpdateAI(uiDiff);
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_frenzied_stalkerAI(pCreature);
    }
};

/*######
# npc_bloodfang_stalker
######*/

// The objective of `Sacrifices` is 30 `Bloodfang Stalker Credit` (35582), and
// retail awards it ON HIT, not on death: in the capture the credit packet for
// 35582 lands 0.79 s after `Throw Torch` goes off, and four arrive together
// when one throw catches four of them. The torch cannot kill anything anyway -
// its damage effect has 0 base points and its burn is 4 damage a tick against
// a 102 hit point stalker - so waiting for a corpse means waiting forever.
//
// The caster of 67063 is the VEHICLE, not the player: the throw arrives as
// CMSG_PET_CAST_SPELL from the vehicle bar, so the credit has to be handed up
// to whoever is in the control seat.
enum
{
    SPELL_THROW_TORCH        = 67063,
    NPC_STALKER_CREDIT       = 35582
};

struct npc_bloodfang_stalker : public CreatureScript
{
    npc_bloodfang_stalker() : CreatureScript("npc_bloodfang_stalker") {}

    struct npc_bloodfang_stalkerAI : public ScriptedAI
    {
        npc_bloodfang_stalkerAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        bool m_bTorched;            ///< One credit per stalker per life

        void Reset() override
        {
            m_bTorched = false;
        }

        void SpellHit(Unit* pCaster, const SpellEntry* pSpell) override
        {
            if (!pCaster || !pSpell || pSpell->ID != SPELL_THROW_TORCH || m_bTorched)
            {
                return;
            }

            Player* pRider = NULL;

            if (VehicleInfo* pKit = pCaster->GetVehicleInfo())
            {
                if (Unit* pSeat = pKit->GetPassenger(0))
                {
                    if (pSeat->GetTypeId() == TYPEID_PLAYER)
                    {
                        pRider = (Player*)pSeat;
                    }
                }
            }

            if (!pRider)
            {
                pRider = pCaster->GetCharmerOrOwnerPlayerOrPlayerItself();
            }

            if (pRider)
            {
                m_bTorched = true;
                pRider->KilledMonsterCredit(NPC_STALKER_CREDIT,
                    m_creature->GetObjectGuid());
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_bloodfang_stalkerAI(pCreature);
    }
};

/*######
# npc_rebel_cannon
######*/

// `By Blood and Ash` (14218) asks for 80 Bloodfang Stalker kills made from the
// Rebel Cannons on Tobias Mistmantle's hill. Retail throws the gunner out the
// moment the last one is counted rather than leaving him sitting in a cannon
// with nothing left to shoot: in the capture the eightieth credit lands at
// t=3077.07 and at t=3077.73 - two thirds of a second later, the shape of a
// server-side poll rather than an event - the cannon's charmer is cleared, its
// faction goes back to 35, the `Ride Vehicle` aura is stripped and the player
// is splined out of the seat. The client asks to fire again at t=3077.59 and
// is simply ignored.
//
// Nothing is cast to do this - no `Eject All Passengers` (51254) appears
// anywhere near it, that spell only ends the horse ride of the previous quest -
// so this is a plain unboard.
enum
{
    QUEST_BY_BLOOD_AND_ASH  = 14218,
    CANNON_POLL_INTERVAL    = 500
};

struct npc_rebel_cannon : public CreatureScript
{
    npc_rebel_cannon() : CreatureScript("npc_rebel_cannon") {}

    struct npc_rebel_cannonAI : public ScriptedAI
    {
        npc_rebel_cannonAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiPollTimer;
        bool m_bManned;             ///< Was somebody in the seat last poll?

        void Reset() override
        {
            m_uiPollTimer = CANNON_POLL_INTERVAL;
            m_bManned = false;
        }

        /// A cannon never swings at anything - deliberately no melee here.
        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiPollTimer > uiDiff)
            {
                m_uiPollTimer -= uiDiff;
                return;
            }

            m_uiPollTimer = CANNON_POLL_INTERVAL;

            VehicleInfo* pKit = m_creature->GetVehicleInfo();
            if (!pKit)
            {
                return;
            }

            // Vehicle 470 has exactly one seat, so the gunner is always seat 0.
            Unit* pSeat = pKit->GetPassenger(0);
            if (!pSeat || pSeat->GetTypeId() != TYPEID_PLAYER)
            {
                // The seat turns with the gunner - VehicleSeat 5206 carries
                // ALLOW_TURNING - so an abandoned cannon keeps whatever heading
                // it was last aimed at. Put it back the way it was planted.
                if (m_bManned)
                {
                    m_bManned = false;
                    m_creature->SetFacingTo(m_creature->Spawn().Facing());
                }

                return;
            }

            m_bManned = true;

            Player* pGunner = (Player*)pSeat;

            if (pGunner->GetQuestStatus(QUEST_BY_BLOOD_AND_ASH) == QUEST_STATUS_COMPLETE)
            {
                // Drop the control aura and let its handler do the unboarding.
                // `VehicleInfo::UnBoard` must NOT be called directly: the only
                // caller in the core is `Aura::HandleAuraControlVehicle`, and
                // going round it leaves `Ride Vehicle` stranded on the vehicle
                // with no passenger. When that aura is removed later - it is
                // tracked, TRACK_AURA_TYPE_CONTROL_VEHICLE, and carries no
                // duration - the handler unboards a passenger that has already
                // gone and `MANGOS_ASSERT(itr != m_passengers.end())` takes the
                // server down, minutes later and nowhere near here.
                m_creature->RemoveSpellsCausingAura(SPELL_AURA_CONTROL_VEHICLE,
                    pGunner->GetObjectGuid());
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_rebel_cannonAI(pCreature);
    }
};

/*######
# npc_forsaken_catapult
######*/

// `Two By Sea` (14382). The Forsaken Catapults ship with the template's
// UNIT_FLAG_NOT_SELECTABLE, so they could never be clicked and the quest was
// unplayable. Retail crews each piece with a Forsaken Machinist and only
// opens it over its gunner's corpse; here the pieces stand open from spawn -
// walk up, click, aim, fire - and the machinists hold the shore beside them
// as ordinary hostiles. The launch itself (68659, aimed by the client) is
// served in core at cast-receive, with the capture's own knockback numbers.
enum
{
    CATAPULT_OPEN_POLL          = 5000,    ///< ms; re-opens the piece if anything re-flags it
    CATAPULT_LEAVE_DELAY        = 9000,    ///< ms from the rider leaving to despawn

    FACTION_FRIENDLY            = 35,
};

struct npc_forsaken_catapult : public CreatureScript
{
    npc_forsaken_catapult() : CreatureScript("npc_forsaken_catapult") {}

    struct npc_forsaken_catapultAI : public ScriptedAI
    {
        npc_forsaken_catapultAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiOpenTimer;
        bool m_bHadRider;

        void Reset() override
        {
            m_uiOpenTimer = CATAPULT_OPEN_POLL;
            m_bHadRider = false;
            OpenForRiders();
        }

        /// Artillery, not a combatant.
        void AttackStart(Unit* /*pWho*/) override {}
        void MoveInLineOfSight(Unit* /*pWho*/) override {}

        /// Selectable and friendly: the spellclick is live.
        void OpenForRiders()
        {
            m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            m_creature->SetFactionTemporary(FACTION_FRIENDLY, TEMPFACTION_NONE);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // The rider comes (the player charms the piece while seated) and
            // goes (the launch drops the charm): retail clears the piece nine
            // seconds after the shot, and the respawn brings a fresh one.
            if (!m_bHadRider)
            {
                if (!m_creature->GetCharmerGuid().IsEmpty())
                {
                    m_bHadRider = true;
                }
            }
            else if (m_creature->GetCharmerGuid().IsEmpty())
            {
                m_creature->ForcedDespawn(CATAPULT_LEAVE_DELAY);
                m_bHadRider = false;
            }

            if (m_uiOpenTimer <= uiDiff)
            {
                m_uiOpenTimer = CATAPULT_OPEN_POLL;

                if (m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                {
                    OpenForRiders();
                }
            }
            else
            {
                m_uiOpenTimer -= uiDiff;
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_forsaken_catapultAI(pCreature);
    }
};

/*######
# spell_call_attack_mastiffs
######*/

// `Leader of the Pack` (14386). The Mastiff Whistle (49240) casts 68682 at
// Dark Ranger Thyala - and failed with "invalid target" because the spell's
// `spell_script_target` row pointed at a stray GAMEOBJECT (176210), one of
// the junk rows this table is known to carry. With the row fixed to Thyala
// the cast resolves, and this script serves the dummy hit the TrinityCore
// way: ten Attack Mastiffs at the shore posts, all on her. Each dog is
// marked as the whistler's own so the pack's kill tags the quest credit
// even if the player never lands a hit.
enum
{
    NPC_ATTACK_MASTIFF          = 36405,

    MASTIFF_DESPAWN             = 60000,   ///< ms, TC's leash on the pack
    MASTIFF_COUNT               = 10,
};

/// TrinityCore's summon posts along the Duskmist shore.
static const float MASTIFF_POSTS[MASTIFF_COUNT][4] =
{
    { -1944.483f, 2656.656f,  1.051441f, 1.691914f  },
    { -1956.602f, 2649.942f,  1.374257f, 1.441419f  },
    { -1973.627f, 2654.836f, -0.6995407f, 1.098437f },
    { -1983.201f, 2662.242f, -1.66652f,  0.8627869f },
    { -1994.557f, 2672.134f, -2.303949f, 0.5766099f },
    { -1949.314f, 2642.024f,  1.299083f, 1.580745f  },
    { -1972.606f, 2639.383f,  1.211673f, 1.217789f  },
    { -1997.009f, 2650.811f, -1.030188f, 0.8184887f },
    { -2006.259f, 2663.115f, -2.00431f,  0.5941383f },
    { -1945.504f, 2653.386f,  1.177739f, 1.675516f  }
};

struct spell_call_attack_mastiffs : public SpellScript
{
    spell_call_attack_mastiffs() : SpellScript("spell_call_attack_mastiffs") {}

    bool EffectDummy(Unit* pCaster, uint32 uiSpellId, SpellEffectIndex uiEffIndex, Object* pTarget, ObjectGuid /*originalCasterGuid*/) override
    {
        if (uiSpellId != 68682 || uiEffIndex != EFFECT_INDEX_0 || !pCaster)
        {
            return false;
        }

        Creature* pRanger = pTarget->ToCreature();

        if (!pRanger)
        {
            return false;
        }

        // The kill is the pack's, the credit the whistler's: tag her now so
        // whichever dog lands the blow, the quest rewards the player.
        pRanger->SetLootRecipient(pCaster);

        // TrinityCore's summon posts, 40-55 yards down the beach - a real
        // charge up the shore, as on retail, on real pathfinding (the mesh
        // is clean: every post paths to her, probed offline). The dogs are
        // marked as the whistler's own, which is also what frees them of
        // the spawn leash - a unit fighting for a player leashes to its
        // master (Creature::IsOutOfThreatArea), exactly TrinityCore's rule,
        // so their shape here matches TC's script: summon and sic.
        for (uint8 i = 0; i < MASTIFF_COUNT; ++i)
        {
            if (Creature* pMastiff = pCaster->SummonCreature(NPC_ATTACK_MASTIFF,
                    MASTIFF_POSTS[i][0], MASTIFF_POSTS[i][1], MASTIFF_POSTS[i][2],
                    MASTIFF_POSTS[i][3], TEMPSPAWN_TIMED_DESPAWN, MASTIFF_DESPAWN))
            {
                pMastiff->SetOwnerGuid(pCaster->GetObjectGuid());

                if (pMastiff->AI())
                {
                    pMastiff->AI()->AttackStart(pRanger);
                }

                pMastiff->AddThreat(pRanger, 100000.0f);

                // The other direction matters just as much: a player-OWNED
                // unit runs the pet combat timer (Unit::Update), which drops
                // combat the moment nobody holds it on a threat list - and a
                // dog still mid-charge has hurt nobody yet. Her reference
                // keeps each runner's combat alive the whole way in.
                pRanger->AddThreat(pMastiff);
            }
        }

        return true;
    }
};

/*######
# npc_dark_ranger_thyala
######*/

// Leader of the Pack (14386), the pack's exit. The whistle's mastiffs are
// 60 s summons and would spend the rest of their leash brawling with the
// shore Forsaken; instead the pack goes with its prey. Only the SUMMONED
// dogs fold away - the shore's resident mastiffs (world spawns of the same
// entry) keep their own fight.
struct npc_dark_ranger_thyala : public CreatureScript
{
    npc_dark_ranger_thyala() : CreatureScript("npc_dark_ranger_thyala") {}

    struct npc_dark_ranger_thyalaAI : public ScriptedAI
    {
        npc_dark_ranger_thyalaAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        void JustDied(Unit* /*pKiller*/) override
        {
            std::list<Creature*> lMastiffs;
            GetCreatureListWithEntryInGrid(lMastiffs, m_creature,
                NPC_ATTACK_MASTIFF, 120.0f);

            for (std::list<Creature*>::iterator itr = lMastiffs.begin();
                 itr != lMastiffs.end(); ++itr)
            {
                if ((*itr)->IsTemporarySummon() && (*itr)->IsAlive())
                {
                    (*itr)->ForcedDespawn(2000);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_dark_ranger_thyalaAI(pCreature);
    }
};

/*######
# npc_lord_godfrey_duskhaven
######*/

// Turning in Leader of the Pack (14386). Retail answers the reward with
// RewSpellCast 69027, a serverside force-cast chain ending in a Cataclysm
// camera-shake self-cast - the earthquake that opens the chapter where
// Duskhaven starts falling into the sea. 69027 exists in no client DBC and
// not in our spell_template, so the hook casts the quake itself. The shake
// comes in one variant per chapter phase, each aura-gated to its own phase
// spell (SpellAuraRestrictions: 80133 needs 68481, 68953 needs 68482,
// 80134 needs 68483 - the capture shows retail firing all three and the
// gates picking); cast the one the player's current phase accepts, which
// at this turn-in is 80133 - the phase swap to 68483 lands after the hook.
struct npc_lord_godfrey_duskhaven : public CreatureScript
{
    npc_lord_godfrey_duskhaven() : CreatureScript("npc_lord_godfrey_duskhaven") {}

    bool OnQuestRewarded(Player* pPlayer, Creature* /*pCreature*/, Quest const* pQuest) override
    {
        if (pQuest->GetQuestId() != 14386)
        {
            return false;
        }

        uint32 uiQuake = pPlayer->HasAura(68481) ? 80133
                       : pPlayer->HasAura(68482) ? 68953
                       : 80134;
        pPlayer->CastSpell(pPlayer, uiQuake, true);         // Cataclysm: the ground answers

        return true;
    }
};

/*######
# npc_drowning_watchman
######*/

// Gasping for Breath (14395). Duskhaven watchmen float drowning in the
// flood; the player clicks one (spellclick 68735) and CARRIES him - the
// rescue aura (296) turns the player into vehicle 520 and the watchman
// rides their back while they swim. TrinityCore ends the ride at an
// areatrigger by the wagon; here it ends the moment the carrier reaches
// dry footing, which is the same shore. Credit 36450, four needed. The
// template shipped UNIT_FLAG_NOT_SELECTABLE (Rel22_07_087 strips it) -
// why no watchman could ever be clicked.
enum
{
    SPELL_RESCUE_WATCHMAN       = 68735,
    NPC_WATCHMAN_CREDIT         = 36450,

    WATCHMAN_BOARD_DELAY        = 500,     ///< ms; the carry aura must land first
    WATCHMAN_POLL               = 1000,
};

struct npc_drowning_watchman : public CreatureScript
{
    npc_drowning_watchman() : CreatureScript("npc_drowning_watchman") {}

    struct npc_drowning_watchmanAI : public ScriptedAI
    {
        npc_drowning_watchmanAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        ObjectGuid m_carrierGuid;
        uint32 m_uiBoardTimer;
        uint32 m_uiPollTimer;
        uint8 m_uiDryPolls;

        void Reset() override
        {
            m_carrierGuid.Clear();
            m_uiBoardTimer = 0;
            m_uiPollTimer = 0;
            m_uiDryPolls = 0;
        }

        /// A drowning man fights water, nothing else.
        void AttackStart(Unit* /*pWho*/) override {}
        void MoveInLineOfSight(Unit* /*pWho*/) override {}

        void SpellHit(Unit* pCaster, const SpellEntry* pSpell) override
        {
            if (pSpell->ID != SPELL_RESCUE_WATCHMAN ||
                !pCaster || pCaster->GetTypeId() != TYPEID_PLAYER ||
                !m_carrierGuid.IsEmpty())
            {
                return;
            }

            m_carrierGuid = pCaster->GetObjectGuid();
            m_uiBoardTimer = WATCHMAN_BOARD_DELAY;

            // Claimed: nobody else grabs him mid-rescue.
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }

        /// Back into the water, clickable again, on any broken rescue.
        void AbortRescue()
        {
            m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            Reset();
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiBoardTimer)
            {
                if (m_uiBoardTimer <= uiDiff)
                {
                    m_uiBoardTimer = 0;

                    Player* pCarrier = m_creature->GetMap()->GetPlayer(m_carrierGuid);

                    if (pCarrier && pCarrier->IsAlive() && pCarrier->IsVehicle())
                    {
                        pCarrier->GetVehicleInfo()->Board(m_creature, 0);
                        m_uiPollTimer = WATCHMAN_POLL;
                    }
                    else
                    {
                        AbortRescue();
                    }
                }
                else
                {
                    m_uiBoardTimer -= uiDiff;
                }
            }

            if (m_uiPollTimer)
            {
                if (m_uiPollTimer <= uiDiff)
                {
                    m_uiPollTimer = WATCHMAN_POLL;

                    Player* pCarrier = m_creature->GetMap()->GetPlayer(m_carrierGuid);

                    // Carry aura gone, carrier gone or dead: back to the
                    // flood (the respawn stands him up at his spot).
                    if (!pCarrier || !pCarrier->IsAlive() || !pCarrier->IsVehicle())
                    {
                        m_uiPollTimer = 0;
                        m_creature->ForcedDespawn(1);
                    }
                    // Dry footing: rescued. A jump off the water surface
                    // clears IsInWater for a moment - retail wants him
                    // HAULED to the shore - so the ground must stay dry,
                    // with no airborne flag, for two polls in a row.
                    else if (!pCarrier->IsInWater() &&
                             !pCarrier->m_movementInfo.HasMovementFlag(
                                 MovementFlags(MOVEFLAG_FALLING | MOVEFLAG_FALLINGFAR)) &&
                             ++m_uiDryPolls >= 2)
                    {
                        m_uiPollTimer = 0;

                        pCarrier->KilledMonsterCredit(NPC_WATCHMAN_CREDIT);
                        pCarrier->RemoveAurasDueToSpell(SPELL_RESCUE_WATCHMAN);

                        m_creature->HandleEmote(EMOTE_ONESHOT_SALUTE);
                        m_creature->ForcedDespawn(2500);
                    }
                    else
                    {
                        // Wet again (or airborne): the shore count restarts.
                        if (pCarrier->IsInWater())
                        {
                            m_uiDryPolls = 0;
                        }
                    }
                }
                else
                {
                    m_uiPollTimer -= uiDiff;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_drowning_watchmanAI(pCreature);
    }
};

/*######
# npc_mountain_horse
######*/

// The Hungry Ettin (14416), the delivery half. The rope (68903, served in
// core) puts a beam and a follow on each lassoed horse; the credit belongs
// to the moment the string reaches Lorna Crowley - the capture shows the
// RIDDEN horse casting 68917 (kill credit 36560) at its rider there, and
// era comments confirm "she will accept the horses in 2s or 3s" and that
// the ridden horse itself counts. When the ride comes within the yard,
// every horse in the string credits and trots into the corral.
enum
{
    NPC_LORNA_ORCHARD           = 36457,   ///< the Duskhaven Lorna (35378 is the city one)
    SPELL_HORSE_CREDIT          = 68917,   ///< kill credit 36560, cast at the rider
    SPELL_HORSE_ROPE                = 46674,   ///< the rope RENDER id the string channels (never cast)

    HORSE_DELIVERY_RANGE        = 30,      ///< yd to Lorna
    HORSE_DELIVERY_POLL         = 2000,
};

struct npc_mountain_horse : public CreatureScript
{
    npc_mountain_horse() : CreatureScript("npc_mountain_horse") {}

    struct npc_mountain_horseAI : public ScriptedAI
    {
        npc_mountain_horseAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiDeliveryTimer;

        void Reset() override
        {
            m_uiDeliveryTimer = HORSE_DELIVERY_POLL;
        }

        /// Horses run from wolves; they do not fight them.
        void AttackStart(Unit* /*pWho*/) override {}
        void MoveInLineOfSight(Unit* /*pWho*/) override {}

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiDeliveryTimer > uiDiff)
            {
                m_uiDeliveryTimer -= uiDiff;
                return;
            }

            m_uiDeliveryTimer = HORSE_DELIVERY_POLL;

            // Only a RIDDEN horse delivers.
            Player* pRider = m_creature->GetMap()->GetPlayer(m_creature->GetCharmerGuid());

            if (!pRider)
            {
                return;
            }

            Creature* pLorna = GetClosestCreatureWithEntry(m_creature,
                NPC_LORNA_ORCHARD, float(HORSE_DELIVERY_RANGE));

            if (!pLorna)
            {
                return;
            }

            // The string first: every horse this one is towing.
            std::list<Creature*> lHorses;
            GetCreatureListWithEntryInGrid(lHorses, m_creature,
                m_creature->GetEntry(), 80.0f);

            for (std::list<Creature*>::iterator itr = lHorses.begin();
                 itr != lHorses.end(); ++itr)
            {
                if ((*itr)->GetChannelObjectGuid() == pRider->GetObjectGuid() &&
                    (*itr)->GetUInt32Value(UNIT_CHANNEL_SPELL) == SPELL_HORSE_ROPE)
                {
                    m_creature->CastSpell(pRider, SPELL_HORSE_CREDIT, true);
                    (*itr)->SetChannelObjectGuid(ObjectGuid());
                    (*itr)->SetUInt32Value(UNIT_CHANNEL_SPELL, 0);
                    (*itr)->ForcedDespawn(2000);
                }
            }

            // Then the one under the saddle: credit, dismount, corral.
            m_creature->CastSpell(pRider, SPELL_HORSE_CREDIT, true);
            m_creature->RemoveAurasDueToSpell(94654);       // the ride: rider off
            m_creature->ForcedDespawn(2500);
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_mountain_horseAI(pCreature);
    }
};

/*######
# npc_chance_the_cat
######*/

// Grandma's Cat (14401). Chance hides under the tree behind the cottage;
// the click casts 68743 `Get Cat` (the spellclick row wrongly carried
// 43689, Rel22_07_091 fixes it), whose retail effect is a server event
// (61 -> 22401): the cat goes into the player's arms - item 49281 - and
// the capture shows Lucius the Cruel (36461) CREATED in that same packet
// burst, the ambush the quest is remembered for.
enum
{
    SPELL_GET_CAT               = 68743,
    ITEM_CHANCE_THE_CAT         = 49281,
    NPC_LUCIUS_THE_CRUEL        = 36461,
    NPC_GRANDMA_WAHL            = 36458,

    SAY_LUCIUS_AMBUSH           = -1999964,
    SAY_GRANDMA_KITTY           = -1999965,

    CAT_EVENT_FALLBACK          = 180000,  ///< ms; unseen orchestrator safety despawn
    LUCIUS_DESPAWN_OOC          = 120000,
    GRANDMA_LINE_DELAY          = 3000,    ///< ms; she storms over first
    CAT_EVENT_POLL              = 1000,
};

/// The capture's create position for the event Grandma - her garden spot.
static const float GRANDMA_EVENT_SPAWN[4] = { -2098.20f, 2351.29f, 7.18f, 5.5f };

struct npc_chance_the_cat : public CreatureScript
{
    npc_chance_the_cat() : CreatureScript("npc_chance_the_cat") {}

    struct npc_chance_the_catAI : public ScriptedAI
    {
        npc_chance_the_catAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        bool m_bTaken;
        uint32 m_uiGrandmaTimer;
        ObjectGuid m_grandmaGuid;
        ObjectGuid m_luciusGuid;

        void Reset() override
        {
            m_bTaken = false;
            m_uiGrandmaTimer = 0;
            m_uiEventPoll = 0;
            m_grandmaGuid.Clear();
            m_luciusGuid.Clear();
        }

        /// A cat. It fights no one.
        void AttackStart(Unit* /*pWho*/) override {}
        void MoveInLineOfSight(Unit* /*pWho*/) override {}

        void SpellHit(Unit* pCaster, const SpellEntry* pSpell) override
        {
            if (pSpell->ID != SPELL_GET_CAT || m_bTaken ||
                !pCaster || pCaster->GetTypeId() != TYPEID_PLAYER)
            {
                return;
            }

            m_bTaken = true;
            Player* pPlayer = (Player*)pCaster;

            pPlayer->StoreNewItemInBestSlots(ITEM_CHANCE_THE_CAT, 1);

            // The ambush: Lucius steps out the moment the cat is lifted.
            if (Creature* pLucius = m_creature->SummonCreature(NPC_LUCIUS_THE_CRUEL,
                    m_creature->Where().X() + 6.0f, m_creature->Where().Y() + 3.0f,
                    m_creature->Where().Z(), 3.6f,
                    TEMPSPAWN_TIMED_OOC_DESPAWN, LUCIUS_DESPAWN_OOC))
            {
                m_luciusGuid = pLucius->GetObjectGuid();
                DoScriptText(SAY_LUCIUS_AMBUSH, pLucius, pPlayer);

                if (pLucius->AI())
                {
                    pLucius->AI()->AttackStart(pPlayer);
                }

                // And nobody touches Grandma's kitty: she storms over from
                // her garden (the capture's exact create position) and
                // joins the fight.
                if (Creature* pGrandma = m_creature->SummonCreature(NPC_GRANDMA_WAHL,
                        GRANDMA_EVENT_SPAWN[0], GRANDMA_EVENT_SPAWN[1],
                        GRANDMA_EVENT_SPAWN[2], GRANDMA_EVENT_SPAWN[3],
                        TEMPSPAWN_TIMED_OOC_DESPAWN, LUCIUS_DESPAWN_OOC))
                {
                    m_grandmaGuid = pGrandma->GetObjectGuid();
                    m_uiGrandmaTimer = GRANDMA_LINE_DELAY;

                    // The event copy is a fighter, not a questgiver: her
                    // cottage self keeps the quests; this one must not
                    // offer a turn-in mid-brawl.
                    pGrandma->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);

                    if (pGrandma->AI())
                    {
                        pGrandma->AI()->AttackStart(pLucius);
                    }

                    pGrandma->AddThreat(pLucius, 1000.0f);
                }
            }

            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            m_creature->SetVisibility(VISIBILITY_OFF);
            m_creature->ForcedDespawn(CAT_EVENT_FALLBACK);
        }

        uint32 m_uiEventPoll;

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiGrandmaTimer)
            {
                if (m_uiGrandmaTimer <= uiDiff)
                {
                    m_uiGrandmaTimer = 0;
                    m_uiEventPoll = CAT_EVENT_POLL;

                    Creature* pGrandma = m_creature->GetMap()->GetCreature(m_grandmaGuid);
                    Creature* pLucius = m_creature->GetMap()->GetCreature(m_luciusGuid);

                    if (pGrandma)
                    {
                        DoScriptText(SAY_GRANDMA_KITTY, pGrandma, pLucius);
                    }
                }
                else
                {
                    m_uiGrandmaTimer -= uiDiff;
                }
            }

            // The fight's end sends everyone home: Grandma back to her
            // cottage self, the unseen cat with her.
            if (m_uiEventPoll)
            {
                if (m_uiEventPoll <= uiDiff)
                {
                    m_uiEventPoll = CAT_EVENT_POLL;

                    Creature* pLucius = m_creature->GetMap()->GetCreature(m_luciusGuid);

                    if (!pLucius || !pLucius->IsAlive())
                    {
                        m_uiEventPoll = 0;

                        if (Creature* pGrandma = m_creature->GetMap()->GetCreature(m_grandmaGuid))
                        {
                            pGrandma->ForcedDespawn(2500);
                        }

                        m_creature->ForcedDespawn(3000);
                    }
                }
                else
                {
                    m_uiEventPoll -= uiDiff;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_chance_the_catAI(pCreature);
    }
};

/*######
# npc_swift_mountain_horse - To Greymane Manor (14465)
######*/

// Accepting To Greymane Manor from Gwen Armstead summons a Swift Mountain
// Horse at her side (retail: player self-casts 69255 -> summon 36741, then
// 69254 boards them) and the horse gallops the player up the mountain
// switchbacks to the manor gate. The route below is the capture's own:
// 131 spline waypoints geometrically chained and thinned to 56 points,
// 403 yards at 13 yd/s = the retail 31-second ride, ending exactly at the
// gate (-1669.9, 2520.1). Queen Mia waits inside for the turn-in.
enum
{
    QUEST_TO_GREYMANE_MANOR     = 14465,
    NPC_SWIFT_MOUNTAIN_HORSE    = 36741,
    SPELL_RIDE_MANOR_HORSE      = 43671,   ///< generic Ride Vehicle seat 0 - retail's 69254 also PHASES
                                           ///< to 32768, which this core's non-stacking phase auras
                                           ///< turn into a vanishing world; the manor population spans
                                           ///< 16384 (mask 180224), so no phase change is needed

    MANOR_RIDE_SPEED_X100       = 1300,    ///< 13.0 yd/s
    MANOR_HORSE_BOARD_DELAY     = 1000,
    MANOR_HORSE_LINGER          = 2500,

    GO_MANOR_ROAD_GATE          = 196863,
    GO_MANOR_UPPER_GATE         = 196401,  ///< the door half; 196864 is its button twin
    GO_MANOR_UPPER_BUTTON       = 196864,
    MANOR_GATE_OPEN_MS          = 15000,
};

/// The estate gate at the first bend, and the upper gate at the manor.
static const float MANOR_GATE_AT[2]       = { -1818.0f, 2333.0f };
static const float MANOR_UPPER_GATE_AT[2] = { -1682.0f, 2508.0f };

static const float MANOR_HORSE_SPAWN[4] = { -1902.45f, 2252.34f, 42.32f, 0.77f };  ///< facing the first bend

static const float MANOR_RIDE_PATH[][3] =
{
    { -1902.45f, 2252.34f, 42.41f },
    { -1851.55f, 2302.07f, 42.00f },
    { -1848.05f, 2305.57f, 41.25f },
    { -1841.55f, 2311.82f, 40.25f },
    { -1838.80f, 2314.82f, 39.50f },
    { -1835.30f, 2318.32f, 39.00f },
    { -1832.42f, 2321.03f, 38.17f },
    { -1828.96f, 2323.23f, 38.18f },
    { -1824.71f, 2326.23f, 37.43f },
    { -1820.46f, 2328.48f, 37.18f },
    { -1817.46f, 2333.48f, 36.93f },
    { -1813.21f, 2337.73f, 36.68f },
    { -1810.71f, 2341.98f, 36.43f },
    { -1804.71f, 2345.98f, 36.43f },
    { -1800.46f, 2350.23f, 36.43f },
    { -1796.46f, 2354.23f, 36.93f },
    { -1792.71f, 2358.23f, 37.93f },
    { -1790.21f, 2361.98f, 38.68f },
    { -1788.46f, 2366.73f, 39.68f },
    { -1785.71f, 2370.23f, 40.68f },
    { -1785.46f, 2374.98f, 41.18f },
    { -1783.21f, 2379.98f, 42.68f },
    { -1782.66f, 2385.56f, 44.47f },
    { -1782.41f, 2389.56f, 45.22f },
    { -1781.91f, 2393.31f, 46.72f },
    { -1781.41f, 2398.31f, 47.97f },
    { -1781.16f, 2402.31f, 49.22f },
    { -1780.66f, 2406.31f, 50.47f },
    { -1780.41f, 2411.31f, 51.72f },
    { -1779.91f, 2416.31f, 53.47f },
    { -1779.41f, 2420.31f, 54.72f },
    { -1777.98f, 2424.74f, 56.25f },
    { -1775.73f, 2429.24f, 57.75f },
    { -1773.23f, 2433.74f, 59.25f },
    { -1769.48f, 2440.49f, 60.75f },
    { -1767.48f, 2444.99f, 62.25f },
    { -1764.98f, 2449.24f, 63.75f },
    { -1760.59f, 2452.65f, 65.65f },
    { -1760.48f, 2458.24f, 67.00f },
    { -1758.47f, 2461.76f, 68.01f },
    { -1755.34f, 2458.40f, 68.40f },
    { -1758.59f, 2454.90f, 66.65f },
    { -1753.34f, 2460.65f, 69.40f },
    { -1735.09f, 2463.90f, 75.40f },
    { -1717.09f, 2466.15f, 81.15f },
    { -1712.59f, 2466.40f, 82.40f },
    { -1708.69f, 2470.01f, 84.99f },
    { -1706.69f, 2473.26f, 86.49f },
    { -1703.94f, 2477.51f, 88.49f },
    { -1701.19f, 2481.51f, 90.24f },
    { -1698.94f, 2484.76f, 91.99f },
    { -1696.19f, 2489.01f, 93.49f },
    { -1693.44f, 2493.01f, 95.24f },
    { -1689.15f, 2498.53f, 96.90f },
    { -1683.90f, 2504.53f, 97.90f },
    { -1669.87f, 2520.10f, 97.98f },
};

struct npc_gwen_armstead : public CreatureScript
{
    npc_gwen_armstead() : CreatureScript("npc_gwen_armstead") {}

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, Quest const* pQuest) override
    {
        if (pQuest->GetQuestId() != QUEST_TO_GREYMANE_MANOR)
        {
            return false;
        }

        if (Creature* pHorse = pCreature->SummonCreature(NPC_SWIFT_MOUNTAIN_HORSE,
                MANOR_HORSE_SPAWN[0], MANOR_HORSE_SPAWN[1], MANOR_HORSE_SPAWN[2],
                MANOR_HORSE_SPAWN[3], TEMPSPAWN_TIMED_OOC_DESPAWN, 120000))
        {
            pPlayer->CastSpell(pHorse, SPELL_RIDE_MANOR_HORSE, true);
        }

        return true;
    }
};

struct npc_swift_mountain_horse : public CreatureScript
{
    npc_swift_mountain_horse() : CreatureScript("npc_swift_mountain_horse") {}

    struct npc_swift_mountain_horseAI : public ScriptedAI
    {
        npc_swift_mountain_horseAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiStartTimer;
        bool m_bRiding;
        bool m_bDone;
        bool m_bGateOpened;
        bool m_bUpperOpened;

        void Reset() override
        {
            m_uiStartTimer = 0;
            m_bRiding = false;
            m_bDone = false;
            m_bGateOpened = false;
            m_bUpperOpened = false;
        }

        /// A ride, not a fight.
        void AttackStart(Unit* /*pWho*/) override {}
        void MoveInLineOfSight(Unit* /*pWho*/) override {}

        void RunPath()
        {
            m_creature->SetRoot(false);

            Movement::PointsArray points;
            points.reserve(56 + 1);
            points.push_back(Geometry::Vector3(0.0f, 0.0f, 0.0f));

            for (uint32 i = 0; i < 56; ++i)
            {
                points.push_back(Geometry::Vector3(MANOR_RIDE_PATH[i][0],
                    MANOR_RIDE_PATH[i][1], MANOR_RIDE_PATH[i][2]));
            }

            Movement::MoveSplineInit init(*m_creature);
            init.MovebyPath(points);
            init.SetWalk(false);
            init.SetVelocity(MANOR_RIDE_SPEED_X100 / 100.0f);
            init.Launch();
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // The rider settles into the saddle, then the gallop.
            if (!m_bRiding && !m_creature->GetCharmerGuid().IsEmpty())
            {
                m_bRiding = true;
                m_uiStartTimer = MANOR_HORSE_BOARD_DELAY;
            }

            if (m_uiStartTimer)
            {
                if (m_uiStartTimer <= uiDiff)
                {
                    m_uiStartTimer = 0;
                    RunPath();
                }
                else
                {
                    m_uiStartTimer -= uiDiff;
                }
            }

            // The estate gate swings open as the horse bears down on it.
            if (m_bRiding && !m_bGateOpened &&
                m_creature->IsWithinDist2d(MANOR_GATE_AT[0], MANOR_GATE_AT[1], 35.0f))
            {
                m_bGateOpened = true;

                if (GameObject* pGate = GetClosestGameObjectWithEntry(m_creature,
                        GO_MANOR_ROAD_GATE, 60.0f))
                {
                    pGate->UseDoorOrButton(MANOR_GATE_OPEN_MS);
                }
            }

            // The upper gate at the manor swings for the last stretch.
            if (m_bRiding && !m_bUpperOpened &&
                m_creature->IsWithinDist2d(MANOR_UPPER_GATE_AT[0], MANOR_UPPER_GATE_AT[1], 40.0f))
            {
                m_bUpperOpened = true;

                if (GameObject* pGate = GetClosestGameObjectWithEntry(m_creature,
                        GO_MANOR_UPPER_GATE, 60.0f))
                {
                    pGate->UseDoorOrButton(MANOR_GATE_OPEN_MS);
                }

                if (GameObject* pButton = GetClosestGameObjectWithEntry(m_creature,
                        GO_MANOR_UPPER_BUTTON, 60.0f))
                {
                    pButton->UseDoorOrButton(MANOR_GATE_OPEN_MS);
                }
            }

            // Journey's end: set the rider down at the gate.
            if (m_bRiding && !m_bDone && m_creature->movespline->Finalized() &&
                m_creature->IsWithinDist3d(MANOR_RIDE_PATH[56 - 1][0],
                    MANOR_RIDE_PATH[56 - 1][1],
                    MANOR_RIDE_PATH[56 - 1][2], 8.0f))
            {
                m_bDone = true;
                m_creature->RemoveAurasDueToSpell(SPELL_RIDE_MANOR_HORSE);
                m_creature->ForcedDespawn(MANOR_HORSE_LINGER);
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_swift_mountain_horseAI(pCreature);
    }
};

/*######
# npc_king_greymane_observatory
######*/

// Alas, Gilneas! (14467). At the observatory telescope the king shows the
// player what became of the city; the capture answers the turn-in with
// SMSG_TRIGGER_CINEMATIC, CinematicID 167 - the flooded-Gilneas sequence.
// The quest carries RewSpellCast 69257, another serverside-only forcecast
// this core has no record of, so the hook triggers the cinematic itself.
struct npc_king_greymane_observatory : public CreatureScript
{
    npc_king_greymane_observatory() : CreatureScript("npc_king_greymane_observatory") {}

    bool OnQuestRewarded(Player* pPlayer, Creature* /*pCreature*/, Quest const* pQuest) override
    {
        if (pQuest->GetQuestId() != 14467)
        {
            return false;
        }

        pPlayer->SendCinematicStart(167);                   // Alas, Gilneas

        return true;
    }

};

/*######
# Exodus (24438) - the caravan to the crash site
######*/

// The parked caravan at the manor gate IS the ride (retail swaps two
// phase copies at the boarding moment; one caravan doing both jobs is
// indistinguishable and has no seam to break). The vehicle_accessory
// tree assembles it: harness 38755 -> horses + coach 44928, the coach
// seating the survivors, seat 0 left for the player. Clicking the coach
// seats the player (43671 - the seat fallback lands on the free seat 0,
// proven in the wild), the coach signals the harness, and the harness
// drives the capture route leg-true: 14.0 yd/s down the switchbacks,
// 7.0 yd/s for the final stretch - the 95-second retail ride - ending
// at the stagecoach crash site where the next chapter begins. The old
// quest StartScript teleported the player straight there; Rel22_07_105
// retires it for this.
enum
{
    NPC_EXODUS_HARNESS          = 38755,
    NPC_EXODUS_COACH            = 44928,
    SPELL_RIDE_EXODUS_COACH     = 43671,

    GO_KINGS_GATE               = 196412,

    EXODUS_START_DELAY          = 1500,
    EXODUS_POLL                 = 500,

    EXODUS_SPEED_RIDE_X100      = 1371,     ///< 1142 yd / 83.3 s across the fast legs
    EXODUS_SPEED_ROLL_X100      = 700,      ///< the empty coach rolling away

    EXODUS_RIDE_FIRST           = 0,        ///< span of the ride: pool[0..63]
    EXODUS_RIDE_COUNT           = 60,
    EXODUS_ROLL_FIRST           = 60,       ///< span of the roll-away
    EXODUS_ROLL_COUNT           = 3,
};

// The capture route: eleven monster-move legs exactly as retail sent
// them (dup packet dropped), each with its own measured speed - 14.0
// yd/s down the switchbacks, 7.0 yd/s for the final stretch. Leg-by-leg
// is not scripting taste, it is the WIRE FORMAT: monster-move packs the
// intermediate points as 11-bit quarter-yard offsets (about 256 yd of
// range); one long spline overflows the packing and the client renders
// garbage mid-air. Every retail leg fits the range.
static const float EXODUS_RIDE_POINTS[][3] =
{
    { -1674.20f, 2518.88f, 97.94f },
    { -1676.20f, 2515.88f, 97.94f },
    { -1678.95f, 2511.63f, 97.94f },
    { -1681.11f, 2508.63f, 97.85f },
    { -1686.14f, 2502.62f, 97.58f },
    { -1693.64f, 2493.12f, 94.83f },
    { -1698.14f, 2486.62f, 92.58f },
    { -1701.14f, 2479.87f, 89.83f },
    { -1704.39f, 2473.37f, 87.08f },
    { -1702.89f, 2460.37f, 83.58f },
    { -1700.39f, 2452.12f, 82.08f },
    { -1699.14f, 2441.37f, 79.33f },
    { -1698.89f, 2429.37f, 75.83f },
    { -1703.14f, 2422.62f, 72.83f },
    { -1714.39f, 2406.87f, 65.58f },
    { -1724.64f, 2392.62f, 61.58f },
    { -1732.14f, 2373.62f, 61.33f },
    { -1736.89f, 2358.37f, 64.58f },
    { -1743.89f, 2341.87f, 68.08f },
    { -1749.89f, 2322.37f, 71.33f },
    { -1757.14f, 2304.37f, 75.58f },
    { -1764.39f, 2290.87f, 78.58f },
    { -1777.39f, 2269.12f, 83.08f },
    { -1794.39f, 2255.12f, 87.08f },
    { -1812.39f, 2245.87f, 89.58f },
    { -1828.39f, 2232.37f, 89.83f },
    { -1843.14f, 2212.37f, 89.83f },
    { -1855.39f, 2194.12f, 89.83f },
    { -1867.39f, 2172.87f, 89.83f },
    { -1869.14f, 2152.87f, 89.83f },
    { -1870.89f, 2137.37f, 89.83f },
    { -1871.39f, 2121.37f, 89.83f },
    { -1871.39f, 2107.62f, 89.83f },
    { -1871.67f, 2094.11f, 89.32f },
    { -1873.98f, 2065.08f, 89.57f },
    { -1877.78f, 2055.42f, 89.32f },
    { -1881.36f, 1990.49f, 89.44f },
    { -1876.03f, 1953.82f, 89.21f },
    { -1881.59f, 1912.88f, 89.19f },
    { -1888.66f, 1905.74f, 89.19f },
    { -1901.20f, 1902.61f, 89.45f },
    { -1911.45f, 1902.11f, 89.45f },
    { -1923.95f, 1901.36f, 89.45f },
    { -1936.45f, 1901.11f, 89.45f },
    { -1947.70f, 1901.11f, 89.45f },
    { -1960.45f, 1901.11f, 89.45f },
    { -1972.45f, 1901.36f, 89.45f },
    { -1988.70f, 1901.36f, 89.45f },
    { -1997.20f, 1902.11f, 89.45f },
    { -2010.45f, 1906.61f, 88.20f },
    { -2024.95f, 1910.36f, 86.20f },
    { -2037.45f, 1911.61f, 82.95f },
    { -2054.95f, 1908.11f, 76.95f },
    { -2070.70f, 1900.61f, 69.20f },
    { -2089.24f, 1888.97f, 57.72f },
    { -2098.98f, 1874.57f, 49.41f },
    { -2109.98f, 1849.57f, 37.91f },
    { -2135.98f, 1821.82f, 23.41f },
    { -2180.73f, 1807.07f, 12.91f },
    { -2213.64f, 1808.83f, 11.83f },
    { -2276.26f, 1788.03f, 11.60f },
    { -2300.13f, 1780.33f, 11.32f },
    { -2319.80f, 1761.53f, 11.18f },
};

struct npc_exodus_coach : public CreatureScript
{
    npc_exodus_coach() : CreatureScript("npc_exodus_coach") {}

    struct npc_exodus_coachAI : public ScriptedAI
    {
        npc_exodus_coachAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        bool m_bSignalled;

        void Reset() override
        {
            m_bSignalled = false;
        }

        void AttackStart(Unit* /*pWho*/) override {}
        void MoveInLineOfSight(Unit* /*pWho*/) override {}

        void UpdateAI(const uint32 /*uiDiff*/) override
        {
            // The wrangler takes seat 0 (the click aura seats them):
            // tell the parent harness to drive.
            if (!m_bSignalled && m_creature->IsBoarded() &&
                m_creature->IsVehicle() &&
                m_creature->GetVehicleInfo()->GetPassenger(0) &&
                m_creature->GetVehicleInfo()->GetPassenger(0)->GetTypeId() == TYPEID_PLAYER)
            {
                m_bSignalled = true;

                Unit* pHarness = (Unit*)m_creature->GetTransportInfo()->GetTransport();

                if (pHarness->GetTypeId() == TYPEID_UNIT &&
                    ((Creature*)pHarness)->AI())
                {
                    ((Creature*)pHarness)->AI()->ReceiveAIEvent(AI_EVENT_CUSTOM_A,
                        m_creature, m_creature, 0);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_exodus_coachAI(pCreature);
    }
};

struct npc_exodus_harness : public CreatureScript
{
    npc_exodus_harness() : CreatureScript("npc_exodus_harness") {}

    struct npc_exodus_harnessAI : public ScriptedAI
    {
        npc_exodus_harnessAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiStartTimer;
        uint32 m_uiEndPoll;
        uint8 m_uiStage;
        bool m_bKingsGate;
        bool m_bDriving;

        void Reset() override
        {
            m_uiStartTimer = 0;
            m_uiEndPoll = 0;
            m_uiStage = 0;
            m_bKingsGate = false;
            m_bDriving = false;
        }

        void AttackStart(Unit* /*pWho*/) override {}
        void MoveInLineOfSight(Unit* /*pWho*/) override {}

        void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/,
                            Unit* /*pInvoker*/, uint32 /*uiMiscValue*/) override
        {
            if (eventType == AI_EVENT_CUSTOM_A && !m_bDriving)
            {
                m_bDriving = true;
                SetCaravanPhase();
                m_uiStartTimer = EXODUS_START_DELAY;
            }
        }

        // The road down crosses several spell_area phase boundaries; the
        // rider phase flips along the way. The caravan must survive every
        // one of them, or the rider floats alone mid-ride.
        void SetCaravanPhase()
        {
            m_creature->SetPhaseMask(PHASEMASK_ANYWHERE, true);

            if (!m_creature->IsVehicle())
            {
                return;
            }

            for (uint8 i = 0; i < 8; ++i)
            {
                Unit* pOuter = m_creature->GetVehicleInfo()->GetPassenger(i);

                if (!pOuter)
                {
                    continue;
                }

                pOuter->SetPhaseMask(PHASEMASK_ANYWHERE, true);

                if (pOuter->IsVehicle())
                {
                    for (uint8 j = 0; j < 8; ++j)
                    {
                        if (Unit* pInner = pOuter->GetVehicleInfo()->GetPassenger(j))
                        {
                            pInner->SetPhaseMask(PHASEMASK_ANYWHERE, true);
                        }
                    }
                }
            }
        }


        // One smooth catmull-rom spline per span. Catmull mode sends the
        // points as full floats - no 11-bit offset packing, so the long
        // span is wire-safe - and the client rounds the corners instead of
        // pausing at every vertex.
        void LaunchSpan(uint32 uiFirst, uint32 uiCount, uint32 uiSpeedX100)
        {
            m_creature->SetRoot(false);

            Movement::PointsArray points;
            points.reserve(uiCount + 1);
            points.push_back(Geometry::Vector3(0.0f, 0.0f, 0.0f));

            for (uint32 i = 0; i < uiCount; ++i)
            {
                const float* pt = EXODUS_RIDE_POINTS[uiFirst + i];
                points.push_back(Geometry::Vector3(pt[0], pt[1], pt[2]));
            }

            Movement::MoveSplineInit init(*m_creature);
            init.MovebyPath(points);
            init.SetSmooth();
            init.SetWalk(false);
            init.SetVelocity(uiSpeedX100 / 100.0f);
            init.Launch();
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiStartTimer)
            {
                if (m_uiStartTimer <= uiDiff)
                {
                    m_uiStartTimer = 0;
                    m_uiEndPoll = EXODUS_POLL;
                    m_uiStage = 1;

                    // The estate gate right in front of the caravan swings
                    // as the horses pull away.
                    if (GameObject* pGate = GetClosestGameObjectWithEntry(m_creature,
                            GO_MANOR_UPPER_GATE, 60.0f))
                    {
                        pGate->UseDoorOrButton(MANOR_GATE_OPEN_MS);
                    }
                    if (GameObject* pButton = GetClosestGameObjectWithEntry(m_creature,
                            GO_MANOR_UPPER_BUTTON, 60.0f))
                    {
                        pButton->UseDoorOrButton(MANOR_GATE_OPEN_MS);
                    }

                    LaunchSpan(EXODUS_RIDE_FIRST, EXODUS_RIDE_COUNT,
                        EXODUS_SPEED_RIDE_X100);
                }
                else
                {
                    m_uiStartTimer -= uiDiff;
                }
            }

            if (m_uiEndPoll)
            {
                if (m_uiEndPoll <= uiDiff)
                {
                    m_uiEndPoll = EXODUS_POLL;

                    // The King's Gate on the plateau swings open as the
                    // caravan bears down on it.
                    if (m_uiStage == 1 && !m_bKingsGate &&
                        m_creature->IsWithinDist3d(-1872.92f, 2073.58f, 89.15f, 60.0f))
                    {
                        m_bKingsGate = true;

                        if (GameObject* pGate = GetClosestGameObjectWithEntry(
                                m_creature, GO_KINGS_GATE, 70.0f))
                        {
                            pGate->UseDoorOrButton(MANOR_GATE_OPEN_MS);
                        }
                    }

                    if (m_uiStage == 1 && m_creature->movespline->Finalized() &&
                        m_creature->IsWithinDist3d(-2213.64f, 1808.83f, 11.83f, 10.0f))
                    {
                        // The crash site: the rider steps down here; the
                        // empty coach rolls on into the distance (the two
                        // slow 7.0 yd/s capture legs) and fades there.
                        m_uiStage = 2;

                        if (Unit* pCoach = m_creature->IsVehicle()
                                ? m_creature->GetVehicleInfo()->GetPassenger(2) : NULL)
                        {
                            pCoach->RemoveAurasDueToSpell(SPELL_RIDE_EXODUS_COACH);
                        }

                        LaunchSpan(EXODUS_ROLL_FIRST, EXODUS_ROLL_COUNT,
                            EXODUS_SPEED_ROLL_X100);
                    }
                    else if (m_uiStage == 2 && m_creature->movespline->Finalized() &&
                        m_creature->IsWithinDist3d(-2319.80f, 1761.53f, 11.18f, 10.0f))
                    {
                        m_uiEndPoll = 0;

                        // Set the rider down at the crash site; the caravan
                        // folds away behind them.
                        std::list<Creature*> lCoach;
                        GetCreatureListWithEntryInGrid(lCoach, m_creature,
                            NPC_EXODUS_COACH, 20.0f);

                        for (std::list<Creature*>::iterator itr = lCoach.begin();
                             itr != lCoach.end(); ++itr)
                        {
                            (*itr)->RemoveAurasDueToSpell(SPELL_RIDE_EXODUS_COACH);
                        }

                        // The static caravan folds away and stands parked
                        // at the gate again two minutes later.
                        m_creature->SetRespawnDelay(120);
                        m_creature->ForcedDespawn(4000);
                    }
                }
                else
                {
                    m_uiEndPoll -= uiDiff;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_exodus_harnessAI(pCreature);
    }
};

/*######
# Losing Your Tail (24616) - the Dark Scout ambush
######*/

// The capture (03:08:27): stepping into areatrigger 6687 with the quest
// roots the player - 70794 (20 s stun) whose forcecast chain summons the
// scout on retail (70795, suppressed here: the static spawn ambushes
// instead) - and the scout winds up 70796, a TEN-SECOND aimed kill shot.
// Belysra's Talisman (item 49944 -> 70797) is the out: its dummy effect
// reaches the scout via spell_script_target, the shot breaks, the freeze
// lifts, and the scout fights on: "I don't need a trap to defeat you."
enum
{
    NPC_DARK_SCOUT              = 37953,
    SPELL_FREEZE_TRAP           = 70794,
    SPELL_TRAP_SHOT             = 70796,
    SPELL_BELYSRA_TALISMAN      = 70797,
    QUEST_LOSING_YOUR_TAIL      = 24616,
    SAY_SCOUT_TRAP_BROKEN       = -1999966,
};

struct npc_dark_scout : public CreatureScript
{
    npc_dark_scout() : CreatureScript("npc_dark_scout") {}

    struct npc_dark_scoutAI : public ScriptedAI
    {
        npc_dark_scoutAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        ObjectGuid m_preyGuid;
        uint32 m_uiShotTimer;
        uint32 m_uiProwlTimer;
        bool m_bAmbushed;
        bool m_bFighting;

        void Reset() override
        {
            m_preyGuid.Clear();
            m_uiShotTimer = 0;
            m_uiProwlTimer = 1000;
            m_bAmbushed = false;
            m_bFighting = false;
            m_creature->SetVisibility(VISIBILITY_OFF);
        }

        void MoveInLineOfSight(Unit* /*pWho*/) override {}

        // One scout serves one prey at a time (retail summons one per
        // player). She is free again once that prey is dead, gone or done
        // with the quest - otherwise the static spawn ambushes the first
        // quester of its life and nobody after.
        bool IsHunting() const
        {
            if (!m_bAmbushed)
            {
                return false;
            }

            Player* pPrey = m_creature->GetMap()->GetPlayer(m_preyGuid);
            return pPrey && pPrey->IsAlive() &&
                   pPrey->GetQuestStatus(QUEST_LOSING_YOUR_TAIL) == QUEST_STATUS_INCOMPLETE;
        }

        void AttackStart(Unit* pWho) override
        {
            // Nothing draws the scout off the trap shot; melee begins only
            // once the ambush is resolved.
            if (m_bFighting)
            {
                ScriptedAI::AttackStart(pWho);
            }
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/,
                            Unit* pInvoker, uint32 /*uiMiscValue*/) override
        {
            if (!pInvoker || pInvoker->GetTypeId() != TYPEID_PLAYER)
            {
                return;
            }

            if (eventType == AI_EVENT_CUSTOM_A && !IsHunting())
            {
                // The trap springs: the scout steps from the shadows and
                // the prey is frozen where they stand. A stale ambush on a
                // prey who is gone is dropped first.
                m_creature->InterruptNonMeleeSpells(false);
                m_bFighting = false;
                m_bAmbushed = true;
                m_preyGuid = pInvoker->GetObjectGuid();
                m_creature->SetVisibility(VISIBILITY_ON);
                m_creature->SetFacingToObject(pInvoker);
                m_creature->CastSpell(pInvoker, SPELL_FREEZE_TRAP, true);
                m_creature->SetInCombatWith(pInvoker);
                m_creature->AddThreat(pInvoker);
                pInvoker->SetInCombatWith(m_creature);
                m_uiShotTimer = 1500;
            }

            // Only the prey's own talisman breaks the trap.
            if (eventType == AI_EVENT_CUSTOM_B && m_bAmbushed &&
                pInvoker->GetObjectGuid() == m_preyGuid)
            {
                // Belysra's talisman breaks the trap: the shot dies on the
                // string and the scout fights on. (The freeze itself is
                // lifted by the spell script, whatever state the scout is
                // in.)
                m_creature->InterruptNonMeleeSpells(false);

                if (!m_bFighting)
                {
                    DoScriptText(SAY_SCOUT_TRAP_BROKEN, m_creature, pInvoker);
                    m_bFighting = true;
                    AttackStart(pInvoker);
                }
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // The prowl: areatrigger 6687 is the retail spring, but its
            // five-yard circle sits on the road - a prey cutting overland
            // walks right past it. The scout also watches on her own.
            if (m_uiProwlTimer && !IsHunting())
            {
                if (m_uiProwlTimer <= uiDiff)
                {
                    m_uiProwlTimer = 1000;

                    Map::PlayerList const& lPlayers = m_creature->GetMap()->GetPlayers();

                    for (Map::PlayerList::const_iterator itr = lPlayers.begin();
                         itr != lPlayers.end(); ++itr)
                    {
                        Player* pPlayer = itr->getSource();

                        if (pPlayer && pPlayer->IsAlive() &&
                            pPlayer->GetQuestStatus(QUEST_LOSING_YOUR_TAIL) == QUEST_STATUS_INCOMPLETE &&
                            !pPlayer->HasAura(SPELL_FREEZE_TRAP) &&
                            pPlayer->IsWithinDistInMap(m_creature, 25.0f))
                        {
                            ReceiveAIEvent(AI_EVENT_CUSTOM_A, m_creature, pPlayer, 0);
                            break;
                        }
                    }
                }
                else
                {
                    m_uiProwlTimer -= uiDiff;
                }
            }

            if (m_uiShotTimer)
            {
                if (m_uiShotTimer <= uiDiff)
                {
                    m_uiShotTimer = 0;

                    if (Player* pPrey = m_creature->GetMap()->GetPlayer(m_preyGuid))
                    {
                        m_creature->CastSpell(pPrey, SPELL_TRAP_SHOT, false);
                    }
                }
                else
                {
                    m_uiShotTimer -= uiDiff;
                }
            }

            // The shot has resolved - landed, or broke with the freeze -
            // and no talisman intervened: fight normally.
            if (m_bAmbushed && !m_bFighting && !m_uiShotTimer &&
                !m_creature->IsNonMeleeSpellCasted(false))
            {
                m_bFighting = true;

                if (Player* pPrey = m_creature->GetMap()->GetPlayer(m_preyGuid))
                {
                    AttackStart(pPrey);
                }
            }

            if (m_bFighting)
            {
                DoMeleeAttackIfReady();
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_dark_scoutAI(pCreature);
    }
};

struct at_dark_scout_ambush : public AreaTriggerScript
{
    at_dark_scout_ambush() : AreaTriggerScript("at_dark_scout_ambush") {}

    bool OnTrigger(Player* pPlayer, AreaTriggerEntry const* /*pAt*/) override
    {
        if (!pPlayer->IsAlive() ||
            pPlayer->GetQuestStatus(QUEST_LOSING_YOUR_TAIL) != QUEST_STATUS_INCOMPLETE ||
            pPlayer->HasAura(SPELL_FREEZE_TRAP))
        {
            return false;
        }

        Creature* pScout = GetClosestCreatureWithEntry(pPlayer, NPC_DARK_SCOUT, 60.0f);

        if (!pScout || !pScout->AI())
        {
            return false;
        }

        pScout->AI()->ReceiveAIEvent(AI_EVENT_CUSTOM_A, pScout, pPlayer, 0);

        return true;
    }
};

struct spell_belysra_talisman : public SpellScript
{
    spell_belysra_talisman() : SpellScript("spell_belysra_talisman") {}

    bool EffectDummy(Unit* pCaster, uint32 uiSpellId, SpellEffectIndex uiEffIndex,
                     Object* pTarget, ObjectGuid /*originalCasterGuid*/) override
    {
        if (uiSpellId != SPELL_BELYSRA_TALISMAN || uiEffIndex != EFFECT_INDEX_1 ||
            !pCaster || !pTarget)
        {
            return false;
        }

        // The unfreeze itself, unconditional: in the 4.3.4 data this dummy
        // IS the escape (the immunity effect's mechanic value never matched
        // the trap; Blizzard later patched in a Cancel Aura effect).
        pCaster->RemoveAurasDueToSpell(SPELL_FREEZE_TRAP);

        Creature* pScout = pTarget->ToCreature();

        if (!pScout || pScout->GetEntry() != NPC_DARK_SCOUT || !pScout->AI())
        {
            return true;
        }

        pScout->AI()->ReceiveAIEvent(AI_EVENT_CUSTOM_B, pScout, pCaster, 0);

        return true;
    }
};

/*######
# Take Back What's Ours (24646) - the Horn of Tal'doren
######*/

// The Worn Coffer with the Mysterious Artifact sits in a camp of ten
// Veteran Dark Rangers. The Horn of Tal'doren (item 50134 -> 71061,
// SPELL_EFFECT_SEND_EVENT 23338) calls the pack: the capture shows
// Tobias Mistmantle plus TEN Tal'doren Trackers appearing around the
// grove and charging the rangers while the player loots. Summoned by
// the player so they share the player's phase; threat is seeded both
// ways (the mastiff lesson: an unfought helper is despawn fodder).
enum
{
    NPC_TOBIAS_EVENT            = 38029,
    NPC_TALDOREN_TRACKER        = 38027,
    NPC_VETERAN_DARK_RANGER     = 38022,
    EVENT_HORN_OF_TALDOREN      = 23338,
};

static const float HORN_PACK_SPAWNS[][3] =
{
    { -2146.12f, 1583.35f, -48.81f },                       // Tobias leads
    { -2126.95f, 1574.38f, -50.44f },
    { -2109.85f, 1575.93f, -49.64f },
    { -2090.00f, 1556.44f, -50.95f },
    { -2174.55f, 1598.85f, -43.74f },
    { -2078.17f, 1587.95f, -45.46f },
    { -2148.19f, 1595.18f, -47.29f },
    { -2091.12f, 1568.63f, -50.86f },
    { -2129.17f, 1616.49f, -43.51f },
    { -2156.54f, 1646.71f, -39.12f },
    { -2160.46f, 1627.84f, -41.74f },
};

struct event_horn_of_taldoren : public MapEventScript
{
    event_horn_of_taldoren() : MapEventScript("event_horn_of_taldoren") {}

    bool OnReceived(uint32 uiEventId, Object* pSource, Object* /*pTarget*/,
                    bool /*bIsStart*/) override
    {
        if (uiEventId != EVENT_HORN_OF_TALDOREN || !pSource ||
            pSource->GetTypeId() != TYPEID_PLAYER)
        {
            return false;
        }

        Player* pPlayer = (Player*)pSource;

        // One pack at a time: a second blast while the first still
        // fights is wind.
        if (GetClosestCreatureWithEntry(pPlayer, NPC_TALDOREN_TRACKER, 80.0f))
        {
            return true;
        }

        for (uint32 i = 0; i < 11; ++i)
        {
            uint32 uiEntry = i == 0 ? NPC_TOBIAS_EVENT : NPC_TALDOREN_TRACKER;

            // Hard timer, not OOC: the sparring floors make this fight
            // unresolvable, so it would never leave combat on its own.
            // Three minutes of theatre, then the pack fades and the
            // rangers reset.
            Creature* pWorgen = pPlayer->SummonCreature(uiEntry,
                HORN_PACK_SPAWNS[i][0], HORN_PACK_SPAWNS[i][1],
                HORN_PACK_SPAWNS[i][2], 1.0f,
                TEMPSPAWN_TIMED_DESPAWN, 180000);

            if (!pWorgen)
            {
                continue;
            }

            if (Creature* pRanger = GetClosestCreatureWithEntry(pWorgen,
                    NPC_VETERAN_DARK_RANGER, 60.0f))
            {
                if (pWorgen->AI())
                {
                    pWorgen->AI()->AttackStart(pRanger);
                }

                pWorgen->AddThreat(pRanger, 1000.0f);
                pRanger->AddThreat(pWorgen);                // inbound ref: no pet-timer despawn
                pRanger->SetInCombatWith(pWorgen);
            }
        }

        return true;
    }
};

/*######
# Neither Human Nor Beast (24593) - the ritual wells
######*/

// The light column belongs to the WELL, not the drinker: the capture
// shows the invisible bunny (35374) standing on each well casting 71200
// (a 12-second glow) on ITSELF when the well is used - the player gets
// nothing. The goober default lands the spell on the clicker instead;
// OnUse returning true suppresses that cast while the core still grants
// the anim, state change, and quest credit. Lyros speaks his blessing
// once, at the well that completes the ritual.
enum
{
    NPC_WELL_GLOW_BUNNY         = 35374,
    NPC_LYROS_SWIFTWIND         = 37870,
    SPELL_WELL_GLOW             = 71200,
    QUEST_NEITHER_HUMAN         = 24593,
    SAY_LYROS_BLESSING          = -1999967,

    GO_WELL_OF_FURY             = 201950,
    GO_WELL_OF_TRANQUILITY      = 201951,
    GO_WELL_OF_BALANCE          = 201952,
};

struct go_ritual_well : public GameObjectScript
{
    go_ritual_well() : GameObjectScript("go_ritual_well") {}

    bool OnUse(Player* pPlayer, GameObject* pGo) override
    {
        // The glow rises from the well itself.
        if (Creature* pBunny = GetClosestCreatureWithEntry(pGo, NPC_WELL_GLOW_BUNNY, 6.0f))
        {
            pBunny->CastSpell(pBunny, SPELL_WELL_GLOW, true);
        }

        // The click completing the ritual draws the blessing from Lyros.
        QuestStatusMap::const_iterator itr =
            pPlayer->getQuestStatusMap().find(QUEST_NEITHER_HUMAN);

        if (itr != pPlayer->getQuestStatusMap().end())
        {
            uint8 uiDone = 0;

            for (int i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
            {
                if (itr->second.m_creatureOrGOcount[i])
                {
                    ++uiDone;
                }
            }

            if (uiDone == 2)                                // this well is the third
            {
                if (Creature* pLyros = GetClosestCreatureWithEntry(pPlayer,
                        NPC_LYROS_SWIFTWIND, 80.0f))
                {
                    DoScriptText(SAY_LYROS_BLESSING, pLyros, pPlayer);
                    pLyros->HandleEmote(EMOTE_ONESHOT_TALK);
                }
            }
        }

        // True: the core skips the goober spell that would beam the
        // player (credit and anim were already granted above the hook).
        return true;
    }
};

/*######
# Return to Stormglen (24673) - the Tal'doren accord
######*/

// Accepting the quest plays the accord (capture 11:21:54): Godfrey
// points and barks his ultimatum; Crowley answers with "does this toad
// speak for you, Genn?"; Greymane casts his transform pair (81768 +
// 81908) and TURNS WORGEN mid-line - "I've come to you as an equal" -
// Godfrey sputters, Crowley pledges his men, and Greymane closes with
// the union of Gilneas. Timings are the capture's own.
enum
{
    NPC_TALDOREN_GODFREY        = 38766,
    NPC_TALDOREN_GREYMANE       = 38767,
    NPC_TALDOREN_LORNA          = 38768,
    QUEST_RETURN_TO_STORMGLEN   = 24673,

    SPELL_GODFREY_POINT         = 72822,
    SPELL_GREYMANE_TRANSFORM_1  = 81768,
    SPELL_GREYMANE_TRANSFORM_2  = 81908,
    SPELL_GREYMANE_CONVERSE     = 72824,

    SAY_ACCORD_GODFREY_ORDER    = -1999968,
    SAY_ACCORD_CROWLEY_TOAD     = -1999969,
    SAY_ACCORD_GREYMANE_EQUAL   = -1999970,
    SAY_ACCORD_GODFREY_GASP     = -1999971,
    SAY_ACCORD_CROWLEY_AYE      = -1999972,
    SAY_ACCORD_GREYMANE_UNITE   = -1999973,
    SAY_ACCORD_DARIUS_LORNA     = -1999974,
};

// The walk-in, straight from the capture: Lorna runs to her father while
// the king and his toad come up the path behind her.
static const float ACCORD_PATHS[][6] =
{
    // entry idx: 0 Lorna, 1 Greymane, 2 Godfrey - start xyz, goal xyz
    { -2089.68f, 1275.03f, -84.46f, -2071.69f, 1277.62f, -85.29f },
    { -2097.90f, 1275.87f, -83.44f, -2077.32f, 1272.58f, -85.25f },
    { -2118.87f, 1290.50f, -80.70f, -2073.30f, 1275.25f, -85.33f },
};

struct npc_darius_taldoren : public CreatureScript
{
    npc_darius_taldoren() : CreatureScript("npc_darius_taldoren") {}

    struct npc_darius_taldorenAI : public ScriptedAI
    {
        npc_darius_taldorenAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiStep;
        uint32 m_uiStepTimer;

        void Reset() override
        {
            m_uiStep = 0;
            m_uiStepTimer = 0;
        }

        Creature* GetActor(uint32 uiEntry)
        {
            return GetClosestCreatureWithEntry(m_creature, uiEntry, 80.0f);
        }

        void SummonActor(uint32 uiEntry, uint32 uiPath)
        {
            if (Creature* pActor = m_creature->SummonCreature(uiEntry,
                    ACCORD_PATHS[uiPath][0], ACCORD_PATHS[uiPath][1],
                    ACCORD_PATHS[uiPath][2], 0.0f,
                    TEMPSPAWN_TIMED_DESPAWN, 120000))
            {
                pActor->SetWalk(false);
                pActor->GetMotionMaster()->MovePoint(1,
                    ACCORD_PATHS[uiPath][3], ACCORD_PATHS[uiPath][4],
                    ACCORD_PATHS[uiPath][5]);
            }
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/,
                            Unit* /*pInvoker*/, uint32 /*uiMiscValue*/) override
        {
            if (eventType == AI_EVENT_CUSTOM_A && !m_uiStepTimer)
            {
                m_uiStep = 1;
                m_uiStepTimer = 500;
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_uiStepTimer)
            {
                return;
            }

            if (m_uiStepTimer > uiDiff)
            {
                m_uiStepTimer -= uiDiff;
                return;
            }

            Creature* pGodfrey = GetActor(NPC_TALDOREN_GODFREY);
            Creature* pGreymane = GetActor(NPC_TALDOREN_GREYMANE);

            switch (m_uiStep)
            {
                case 1:                                     // "Lorna!" - the walk-in
                    DoScriptText(SAY_ACCORD_DARIUS_LORNA, m_creature);
                    SummonActor(NPC_TALDOREN_LORNA, 0);
                    SummonActor(NPC_TALDOREN_GREYMANE, 1);
                    SummonActor(NPC_TALDOREN_GODFREY, 2);
                    m_uiStepTimer = 8000;
                    break;
                case 2:                                     // the ultimatum
                    if (pGodfrey)
                    {
                        pGodfrey->HandleEmote(EMOTE_ONESHOT_POINT);
                        DoScriptText(SAY_ACCORD_GODFREY_ORDER, pGodfrey);
                    }
                    m_uiStepTimer = 13600;
                    break;
                case 3:
                    if (pGodfrey)
                    {
                        pGodfrey->CastSpell(pGodfrey, SPELL_GODFREY_POINT, true);
                    }
                    m_uiStepTimer = 700;
                    break;
                case 4:                                     // "this toad"
                    DoScriptText(SAY_ACCORD_CROWLEY_TOAD, m_creature);
                    m_uiStepTimer = 5500;
                    break;
                case 5:                                     // the king turns worgen
                    if (pGreymane)
                    {
                        pGreymane->CastSpell(pGreymane, SPELL_GREYMANE_TRANSFORM_1, true);
                        pGreymane->CastSpell(pGreymane, SPELL_GREYMANE_TRANSFORM_2, true);
                        pGreymane->SetDisplayId(31177);
                        pGreymane->HandleEmote(EMOTE_ONESHOT_ROAR);
                    }
                    m_uiStepTimer = 200;
                    break;
                case 6:
                    if (pGreymane)
                    {
                        DoScriptText(SAY_ACCORD_GREYMANE_EQUAL, pGreymane);
                    }
                    m_uiStepTimer = 8400;
                    break;
                case 7:                                     // "Impossible!"
                    if (pGodfrey)
                    {
                        DoScriptText(SAY_ACCORD_GODFREY_GASP, pGodfrey);
                    }
                    m_uiStepTimer = 1100;
                    break;
                case 8:
                    if (pGreymane)
                    {
                        pGreymane->CastSpell(pGreymane, SPELL_GREYMANE_CONVERSE, true);
                    }
                    m_uiStepTimer = 400;
                    break;
                case 9:                                     // the pledge
                    m_creature->HandleEmote(EMOTE_ONESHOT_YES);
                    DoScriptText(SAY_ACCORD_CROWLEY_AYE, m_creature);
                    m_uiStepTimer = 10900;
                    break;
                case 10:                                    // the union
                    if (pGreymane)
                    {
                        DoScriptText(SAY_ACCORD_GREYMANE_UNITE, pGreymane);
                        pGreymane->HandleEmote(66);         // capture broadcast emote
                    }
                    m_uiStepTimer = 0;
                    m_uiStep = 0;
                    return;
            }

            ++m_uiStep;
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_darius_taldorenAI(pCreature);
    }

    bool OnQuestAccept(Player* /*pPlayer*/, Creature* pCreature, Quest const* pQuest) override
    {
        if (pQuest->GetQuestId() != QUEST_RETURN_TO_STORMGLEN)
        {
            return false;
        }

        if (pCreature->AI())
        {
            pCreature->AI()->ReceiveAIEvent(AI_EVENT_CUSTOM_A, pCreature, pCreature, 0);
        }

        return true;
    }
};

/*######
# Flank the Forsaken (24677) - Godfrey takes the cliff
######*/

// Accepting the quest at the overlook plays Godfrey's refusal (capture
// 11:44:45): "No... I'd sooner die than have one of your kind for a
// king!" (VO 19631) - then he runs for the edge in two legs and hurls
// himself off (parabolic jump, 51 yards of air down to the rocks) and
// is gone. The static spawn's 60-second respawn brings him back for
// the next taker.
enum
{
    NPC_GODFREY_OVERLOOK        = 37875,
    QUEST_FLANK_THE_FORSAKEN    = 24677,
    SAY_GODFREY_SOONER_DIE      = -1999975,
};

struct npc_godfrey_flank : public CreatureScript
{
    npc_godfrey_flank() : CreatureScript("npc_godfrey_flank") {}

    struct npc_godfrey_flankAI : public ScriptedAI
    {
        npc_godfrey_flankAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiStep;
        uint32 m_uiStepTimer;

        void Reset() override
        {
            m_uiStep = 0;
            m_uiStepTimer = 0;

            // The respawn reuses the creature: kill any spline state the
            // last life left behind, or the create-block replays it.
            if (m_creature->IsInWorld() && m_creature->IsAlive())
            {
                m_creature->InterruptMoving();
                m_creature->GetMotionMaster()->MoveIdle();
                m_creature->SetWalk(true);
            }
        }

        // A pure actor: the default aggro made him pick a fight with the
        // Tempest's Reach watchmen, who killed him before his cue.
        void MoveInLineOfSight(Unit* /*pWho*/) override {}
        void AttackStart(Unit* /*pWho*/) override {}
        void AttackedBy(Unit* /*pAttacker*/) override {}

        void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/,
                            Unit* /*pInvoker*/, uint32 /*uiMiscValue*/) override
        {
            if (eventType == AI_EVENT_CUSTOM_A && !m_uiStepTimer && !m_uiStep)
            {
                m_uiStep = 1;
                m_uiStepTimer = 300;
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_uiStepTimer)
            {
                return;
            }

            if (m_uiStepTimer > uiDiff)
            {
                m_uiStepTimer -= uiDiff;
                return;
            }

            switch (m_uiStep)
            {
                case 1:                                     // the refusal
                    DoScriptText(SAY_GODFREY_SOONER_DIE, m_creature);
                    m_uiStepTimer = 3500;
                    break;
                case 2:                                     // the run for the edge
                    m_creature->SetWalk(false);
                    m_creature->GetMotionMaster()->MovePoint(1,
                        -2050.99f, 953.29f, 70.05f);
                    m_uiStepTimer = 2600;
                    break;
                case 3:
                    m_creature->GetMotionMaster()->MovePoint(2,
                        -2053.59f, 941.74f, 70.05f);
                    m_uiStepTimer = 1800;
                    break;
                case 4:                                     // over the edge
                    // Face the dive line explicitly: an idle beat at the lip
                    // re-faced him toward the audience before the leap.
                    m_creature->SetFacingTo(4.66f);
                    m_creature->GetMotionMaster()->MoveJump(
                        -2054.75f, 893.83f, 19.11f, 14.0f, 10.0f);
                    m_uiStepTimer = 4600;
                    break;
                case 5:                                     // gone on the rocks
                    // NOT StopMoving: that LAUNCHES a one-point spline which
                    // the instant despawn leaves unfinalized on the reused
                    // creature - the respawn create-block then replays it
                    // forever (the walking-in-place ghost). InterruptMoving
                    // finalizes the spline; its stop then no-ops.
                    m_creature->InterruptMoving();
                    m_creature->ForcedDespawn();
                    m_uiStepTimer = 0;
                    m_uiStep = 0;
                    return;
            }

            ++m_uiStep;
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_godfrey_flankAI(pCreature);
    }
};

struct npc_flank_questgiver : public CreatureScript
{
    npc_flank_questgiver() : CreatureScript("npc_flank_questgiver") {}

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, Quest const* pQuest) override
    {
        // The Battle for Gilneas City: Krennan sends the player off.
        if (pQuest->GetQuestId() == 24904)
        {
            if (Creature* pKrennan = GetClosestCreatureWithEntry(pCreature,
                    38553, 40.0f))
            {
                DoScriptText(-1999979, pKrennan, pPlayer);
            }

            return true;
        }

        if (pQuest->GetQuestId() != QUEST_FLANK_THE_FORSAKEN)
        {
            return false;
        }

        // Lorna in Stormglen also offers the quest; from there the cliff
        // is out of sight and the search simply finds no Godfrey.
        if (Creature* pGodfrey = GetClosestCreatureWithEntry(pCreature,
                NPC_GODFREY_OVERLOOK, 100.0f))
        {
            if (pGodfrey->AI())
            {
                pGodfrey->AI()->ReceiveAIEvent(AI_EVENT_CUSTOM_A, pGodfrey,
                    pGodfrey, 0);
            }
        }

        return true;
    }
};

/*######
# Flank the Forsaken (24677) - Lord Hewell's horse
######*/

// Hewell offers "I need a horse." to the quest holder (capture menu
// 11079, npc_text 15408). Selecting it summons a Stout Mountain Horse
// (38765, vehicle 542) and seats the player with 46598 - the capture's
// own boarding spell - and the horse rides the capture route down to
// the Stormglen staging point, sets the rider down, and fades. One
// smooth catmull spline: full-float points, no leg seams.
enum
{
    NPC_LORD_HEWELL             = 38764,
    NPC_STOUT_MOUNTAIN_HORSE    = 38765,
    SPELL_RIDE_STOUT_HORSE      = 46598,
    TEXT_ID_HEWELL              = 15408,

    HEWELL_RIDE_SPEED_X100      = 1360,
    HEWELL_RIDE_POLL            = 500,
    HEWELL_RIDE_COUNT           = 41,
};

#define GOSSIP_ITEM_NEED_HORSE  "I need a horse."

static const float HEWELL_RIDE_PATH[][3] =
{
    { -2041.51f, 973.34f, 70.06f },                         // out the wooden door
    { -2050.30f, 960.15f, 70.12f },
    { -2009.71f, 930.33f, 61.57f },
    { -1993.43f, 897.66f, 57.31f },
    { -1971.22f, 863.58f, 64.44f },
    { -1933.52f, 858.57f, 69.31f },
    { -1906.93f, 863.13f, 77.08f },
    { -1882.33f, 878.22f, 82.18f },
    { -1875.92f, 919.38f, 78.93f },
    { -1877.80f, 950.60f, 77.81f },
    { -1885.67f, 980.57f, 76.02f },
    { -1879.78f, 1017.91f, 68.10f },
    { -1857.93f, 1060.37f, 57.39f },
    { -1849.76f, 1097.92f, 47.65f },
    { -1845.84f, 1137.07f, 38.04f },
    { -1854.08f, 1180.34f, 29.41f },
    { -1859.94f, 1208.52f, 24.02f },
    { -1858.65f, 1219.45f, 22.73f },
    { -1834.74f, 1206.45f, 19.98f },
    { -1807.83f, 1183.70f, 20.16f },
    { -1777.18f, 1168.66f, 22.60f },
    { -1743.69f, 1157.76f, 27.07f },
    { -1709.76f, 1145.05f, 25.92f },
    { -1680.94f, 1130.62f, 26.00f },
    { -1649.49f, 1113.03f, 28.65f },
    { -1614.57f, 1113.56f, 29.70f },
    { -1576.36f, 1120.76f, 30.35f },
    { -1549.19f, 1119.77f, 30.25f },
    { -1533.06f, 1113.01f, 30.12f },
    { -1516.32f, 1100.63f, 29.52f },
    { -1500.91f, 1087.08f, 29.28f },
    { -1478.38f, 1082.03f, 29.72f },
    { -1443.80f, 1082.19f, 29.22f },
    { -1408.47f, 1089.57f, 29.30f },
    { -1384.43f, 1103.10f, 30.02f },
    { -1365.07f, 1114.61f, 31.06f },
    { -1334.67f, 1122.47f, 35.21f },
    { -1321.08f, 1122.87f, 36.68f },
    { -1316.56f, 1132.03f, 37.28f },
    { -1345.35f, 1164.83f, 34.94f },
    { -1362.86f, 1186.38f, 35.09f },
};

struct npc_lord_hewell : public CreatureScript
{
    npc_lord_hewell() : CreatureScript("npc_lord_hewell") {}

    bool OnGossipHello(Player* pPlayer, Creature* pCreature) override
    {
        // The script path skips the core's menu clear: without this, every
        // previous window's rows pile up in the next one.
        pPlayer->PlayerTalkClass->ClearMenus();

        if (pPlayer->IsCurrentQuest(QUEST_FLANK_THE_FORSAKEN))
        {
            pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_NEED_HORSE,
                GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        }

        pPlayer->SEND_GOSSIP_MENU(TEXT_ID_HEWELL, pCreature->GetObjectGuid());

        return true;
    }

    bool OnGossipSelect(Player* pPlayer, Creature* pCreature, uint32 /*uiSender*/,
                        uint32 uiAction) override
    {
        if (uiAction != GOSSIP_ACTION_INFO_DEF + 1)
        {
            return false;
        }

        pPlayer->CLOSE_GOSSIP_MENU();

        // The lifespan clock starts at SUMMON, not at mount: a rider who
        // dawdles before mounting had the old 3-minute timer expire in the
        // final stretch - the horse vanished mid-gallop and the despawn
        // safety-ejected them. Ten minutes is beyond any dawdle; the AI
        // tidies away an unridden horse on its own.
        if (Creature* pHorse = pCreature->SummonCreature(NPC_STOUT_MOUNTAIN_HORSE,
                pCreature->GetPositionX() + 2.0f, pCreature->GetPositionY() + 3.0f,
                pCreature->GetPositionZ(), 4.10f, TEMPSPAWN_TIMED_DESPAWN, 600000))
        {
            pPlayer->CastSpell(pHorse, SPELL_RIDE_STOUT_HORSE, true);
        }

        return true;
    }
};

struct npc_stout_horse_flank : public CreatureScript
{
    npc_stout_horse_flank() : CreatureScript("npc_stout_horse_flank") {}

    struct npc_stout_horse_flankAI : public ScriptedAI
    {
        npc_stout_horse_flankAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiPoll;
        uint32 m_uiUnriddenTimer;
        uint32 m_uiArriveTimer;
        bool m_bDriving;

        void Reset() override
        {
            m_uiPoll = HEWELL_RIDE_POLL;
            m_uiUnriddenTimer = 90000;
            m_uiArriveTimer = 0;
            m_bDriving = false;

            // Nothing on the road may touch the courier: a hostile daze
            // mid-ride kills the spline and strands the rider short.
            m_creature->SetFlag(UNIT_FIELD_FLAGS,
                UNIT_FLAG_PASSIVE | UNIT_FLAG_OOC_NOT_ATTACKABLE);
        }

        void AttackStart(Unit* /*pWho*/) override {}
        void MoveInLineOfSight(Unit* /*pWho*/) override {}
        void AttackedBy(Unit* /*pAttacker*/) override {}

        void LaunchFrom(uint32 uiFirst)
        {
            m_creature->SetRoot(false);

            Movement::PointsArray points;
            points.reserve(HEWELL_RIDE_COUNT - uiFirst + 1);
            points.push_back(Geometry::Vector3(0.0f, 0.0f, 0.0f));

            for (uint32 i = uiFirst; i < HEWELL_RIDE_COUNT; ++i)
            {
                points.push_back(Geometry::Vector3(HEWELL_RIDE_PATH[i][0],
                    HEWELL_RIDE_PATH[i][1], HEWELL_RIDE_PATH[i][2]));
            }

            Movement::MoveSplineInit init(*m_creature);
            init.MovebyPath(points);
            init.SetSmooth();
            init.SetWalk(false);
            init.SetVelocity(HEWELL_RIDE_SPEED_X100 / 100.0f);
            init.Launch();
        }

        uint32 NearestAheadIndex() const
        {
            uint32 uiBest = 0;
            float fBestDist = 1000000.0f;

            for (uint32 i = 0; i < HEWELL_RIDE_COUNT; ++i)
            {
                float fDist = m_creature->GetDistance(HEWELL_RIDE_PATH[i][0],
                    HEWELL_RIDE_PATH[i][1], HEWELL_RIDE_PATH[i][2]);

                if (fDist < fBestDist)
                {
                    fBestDist = fDist;
                    uiBest = i;
                }
            }

            return uiBest + 1 < HEWELL_RIDE_COUNT ? uiBest + 1 : uiBest;
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiPoll > uiDiff)
            {
                m_uiPoll -= uiDiff;
                return;
            }

            m_uiPoll = HEWELL_RIDE_POLL;

            if (!m_bDriving)
            {
                // The rider is seated (the boarding aura charms): ride.
                if (!m_creature->GetCharmerGuid().IsEmpty())
                {
                    m_bDriving = true;
                    LaunchFrom(0);
                }
                else if (m_uiUnriddenTimer <= HEWELL_RIDE_POLL)
                {
                    // Nobody mounted: the stable takes it back.
                    m_creature->ForcedDespawn();
                    m_uiPoll = 60000;
                }
                else
                {
                    m_uiUnriddenTimer -= HEWELL_RIDE_POLL;
                }
            }
            else if (m_creature->movespline->Finalized())
            {
                if (m_creature->IsWithinDist3d(-1362.86f, 1186.38f, 35.09f, 8.0f))
                {
                    // The Livery Outpost. Do NOT eject on the same tick the
                    // server spline ends: the client still renders the final
                    // approach a few yards back, and an immediate dismount
                    // detaches the rider mid-motion - the air-fling. Stand
                    // for a beat so every client draws the horse stopped at
                    // the outpost, then set the rider down, then fade.
                    if (!m_uiArriveTimer)
                    {
                        m_creature->InterruptMoving();
                        m_uiArriveTimer = 2000;
                    }
                    else if (m_uiArriveTimer <= HEWELL_RIDE_POLL)
                    {
                        m_creature->RemoveAurasDueToSpell(SPELL_RIDE_STOUT_HORSE);
                        m_creature->ForcedDespawn(3000);
                        m_uiPoll = 60000;
                    }
                    else
                    {
                        m_uiArriveTimer -= HEWELL_RIDE_POLL;
                    }
                }
                else
                {
                    // The spline died short of the outpost (whatever cut
                    // it): pick the road back up from the nearest point
                    // ahead and finish the delivery.
                    LaunchFrom(NearestAheadIndex());
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_stout_horse_flankAI(pCreature);
    }
};

/*######
# Liberation Day (24575) - the freed villagers
######*/

// Unlocking a Ball and Chain must FREE someone: the capture shows the
// villager beside it thanking the player (one of three lines) and
// walking off. The GO use hook (credit is granted by the core before
// it) hands the release to the villager, who speaks, walks a few yards
// clear, and fades; the shared 60-second respawn cycle re-chains the
// pair for the next liberator.
enum
{
    NPC_ENSLAVED_VILLAGER       = 37694,
    GO_BALL_AND_CHAIN           = 201775,

    SAY_FREED_THANKS            = -1999976,
    SAY_FREED_CURSE             = -1999977,
    SAY_FREED_PAY               = -1999978,
};

struct npc_enslaved_villager : public CreatureScript
{
    npc_enslaved_villager() : CreatureScript("npc_enslaved_villager") {}

    struct npc_enslaved_villagerAI : public ScriptedAI
    {
        npc_enslaved_villagerAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiFadeTimer;
        uint32 m_uiChainTimer;

        void Reset() override
        {
            m_uiFadeTimer = 0;
            m_uiChainTimer = 0;
        }

        void AttackStart(Unit* /*pWho*/) override {}
        void MoveInLineOfSight(Unit* /*pWho*/) override {}

        void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/,
                            Unit* pInvoker, uint32 /*uiMiscValue*/) override
        {
            if (eventType != AI_EVENT_CUSTOM_A || m_uiFadeTimer)
            {
                return;
            }

            switch (urand(0, 2))
            {
                case 0: DoScriptText(SAY_FREED_THANKS, m_creature, pInvoker); break;
                case 1: DoScriptText(SAY_FREED_CURSE, m_creature, pInvoker); break;
                case 2: DoScriptText(SAY_FREED_PAY, m_creature, pInvoker); break;
            }

            // Tools down: the addon re-arms the mining pose on respawn,
            // so the freed copy just clears it.
            m_creature->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_ONESHOT_NONE);

            // The chain is spent: consume it a beat later (the core's use
            // flow would overwrite a state change made inside the hook),
            // and the shared 60s respawn brings the pair back together.
            m_uiChainTimer = 1000;

            // A few steps into freedom, away from the chain.
            float fAngle = frand(0.0f, 2 * M_PI_F);
            m_creature->SetWalk(false);
            m_creature->GetMotionMaster()->MovePoint(1,
                m_creature->GetPositionX() + 9.0f * cos(fAngle),
                m_creature->GetPositionY() + 9.0f * sin(fAngle),
                m_creature->GetPositionZ());

            m_uiFadeTimer = 12000;
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_uiFadeTimer)
            {
                return;
            }

            if (m_uiChainTimer)
            {
                if (m_uiChainTimer <= uiDiff)
                {
                    m_uiChainTimer = 0;

                    if (GameObject* pChain = GetClosestGameObjectWithEntry(
                            m_creature, GO_BALL_AND_CHAIN, 10.0f))
                    {
                        pChain->SetLootState(GO_JUST_DEACTIVATED);
                    }
                }
                else
                {
                    m_uiChainTimer -= uiDiff;
                }
            }

            if (m_uiFadeTimer <= uiDiff)
            {
                m_uiFadeTimer = 0;
                m_creature->InterruptMoving();
                m_creature->ForcedDespawn();
            }
            else
            {
                m_uiFadeTimer -= uiDiff;
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_enslaved_villagerAI(pCreature);
    }
};

struct go_ball_and_chain : public GameObjectScript
{
    go_ball_and_chain() : GameObjectScript("go_ball_and_chain") {}

    bool OnUse(Player* pPlayer, GameObject* pGo) override
    {
        if (pPlayer->GetQuestStatus(24575) != QUEST_STATUS_INCOMPLETE)
        {
            return false;
        }

        if (Creature* pVillager = GetClosestCreatureWithEntry(pGo,
                NPC_ENSLAVED_VILLAGER, 10.0f))
        {
            if (pVillager->AI())
            {
                pVillager->AI()->ReceiveAIEvent(AI_EVENT_CUSTOM_A, pVillager,
                    pPlayer, 0);
            }
        }

        return false;                                       // core flow continues
    }
};

/*######
# The Battle for Gilneas City (24904) - Stage 1: the rally
######*/

// The army already stands mustered in phase 262144 (69485 carries the
// player in at Flank the Forsaken's turn-in; the whole cast is static
// spawns). Stage 1 is the rally, straight from the capture: Krennan's
// send-off at the accept, and Liam's never-surrender speech - six
// voiced verses (19623-19628), FOR GILNEAS with the crowd roar
// (19651 + 22584), and the Attack! that opens the push.
enum
{
    NPC_LIAM_BATTLE             = 38218,
    NPC_KRENNAN_BATTLE          = 38553,
    QUEST_BATTLE_FOR_GILNEAS    = 24904,

    SAY_BATTLE_KRENNAN_FRAY     = -1999979,
    SAY_BATTLE_SPEECH_1         = -1999980,
    SAY_BATTLE_SPEECH_2         = -1999981,
    SAY_BATTLE_SPEECH_3         = -1999982,
    SAY_BATTLE_SPEECH_4         = -1999983,
    SAY_BATTLE_SPEECH_5         = -1999984,
    SAY_BATTLE_SPEECH_6         = -1999985,
    SAY_BATTLE_FOR_GILNEAS      = -1999986,
    SAY_BATTLE_ATTACK           = -1999987,
    SAY_BATTLE_PUSH             = -1999988,
    SAY_BATTLE_TIME_UP          = -1999989,
    SAY_BATTLE_PREVAIL          = -1999990,
    SAY_BATTLE_ABOMINATIONS     = -1999991,

    SOUND_CROWD_ROAR            = 22584,

    NPC_FORSAKEN_CROSSBOWMAN    = 38210,
    NPC_FORSAKEN_INFANTRY       = 38192,
    NPC_VILE_ABOMINATION        = 38420,
    NPC_LORNA_BATTLE            = 38426,
    NPC_EMBERSTONE_CANNON       = 38424,
    NPC_FREED_VILLAGER          = 38425,

    SAY_BATTLE_LORNA_CANNONS    = -1999992,
    SAY_BATTLE_LIAM_LORNA       = -1999993,
    SAY_BATTLE_PREVAIL_VOICED   = -1999994,
    SAY_BATTLE_GOREROT          = -1999995,
    SAY_BATTLE_DARIUS_CATAPULT  = -1999996,
    SAY_BATTLE_DARIUS_JOIN      = -1999997,

    NPC_GOREROT                 = 38331,
    NPC_DARIUS_BATTLE           = 38415,

    NPC_GENN_BATTLE             = 38470,
    NPC_SYLVANAS_BATTLE         = 38469,
    NPC_LIAM_DYING              = 38474,
    NPC_BANSHEE_BATTLE          = 38473,
    NPC_WORGEN_WARRIOR_BATTLE   = 38348,
    NPC_BATTLE_CITY_CREDIT      = 38854,
    NPC_GILNEAN_MILITIA         = 38221,

    DISPLAY_GENN_WORGEN         = 31177,   ///< the king charges in worgen form

    SAY_FINALE_GENN_BLOCK       = -2000001,
    SAY_FINALE_GENN_SYLVANAS    = -2000002,
    SAY_FINALE_SYLV_ENOUGH      = -2000003,
    SAY_FINALE_SYLV_LEADER      = -2000004,
    SAY_FINALE_LIAM_FATHER      = -2000005,
    SAY_FINALE_GENN_NO          = -2000006,
    SAY_FINALE_LIAM_DID_IT      = -2000007,
    SAY_FINALE_LIAM_TOOK_BACK   = -2000008,

    SPELL_GENN_VS_SYLVANAS      = 72107,   ///< the king's leap onto the queen
    SPELL_BANSHEE_QUEENS_WAIL   = 72113,   ///< her 25% knockback-stun on everybody
};

// A battle creature that died or fades must respawn at its DB spawn:
// the army carries its in-memory spawn with it (RanksRehome), and a
// respawn from a mid-city battle pose breaks the next run's muster.
static void RestoreDbSpawn(Creature* pCreature)
{
    if (CreatureData const* pData = sObjectMgr.GetCreatureData(pCreature->GetGUIDLow()))
    {
        pCreature->SetSpawn(Geometry::Vector3(pData->posX, pData->posY, pData->posZ),
                            pData->orientation);
    }
}

// Battle target selection, shared by every friendly fighter: of the foes
// in range, take the one with the FEWEST attackers already on it, and
// break ties by distance. Nearest-first dogpiled the closest victim
// while the rest of the wall stood untouched.
static Creature* SelectBattleFoe(Creature* pSeeker, float fRange)
{
    static const uint32 auiFoes[3] = { 38210, 38192, 38420 };

    Creature* pBest = NULL;
    size_t uiBestLoad = 0;
    float fBestDist = 0.0f;

    for (uint8 i = 0; i < 3; ++i)
    {
        std::list<Creature*> lFoes;
        GetCreatureListWithEntryInGrid(lFoes, pSeeker, auiFoes[i], fRange);

        for (std::list<Creature*>::iterator itr = lFoes.begin(); itr != lFoes.end(); ++itr)
        {
            if (!(*itr)->IsAlive())
            {
                continue;
            }

            const size_t uiLoad = (*itr)->getAttackers().size();
            const float fDist = pSeeker->GetDistance(*itr);

            if (!pBest || uiLoad < uiBestLoad ||
                (uiLoad == uiBestLoad && fDist < fBestDist))
            {
                pBest = *itr;
                uiBestLoad = uiLoad;
                fBestDist = fDist;
            }
        }
    }

    return pBest;
}

// The militia's kill-chain: the NEAREST foe, not the least crowded -
// a soldier who drops his man takes the next one standing beside him.
static Creature* SelectNearestBattleFoe(Creature* pSeeker, float fRange)
{
    static const uint32 auiFoes[3] = { 38210, 38192, 38420 };

    Creature* pBest = NULL;
    float fBestDist = 0.0f;

    for (uint8 i = 0; i < 3; ++i)
    {
        Creature* pFoe = GetClosestCreatureWithEntry(pSeeker, auiFoes[i], fRange);

        if (pFoe && pFoe->IsAlive())
        {
            const float fDist = pSeeker->GetDistance(pFoe);

            if (!pBest || fDist < fBestDist)
            {
                pBest = pFoe;
                fBestDist = fDist;
            }
        }
    }

    return pBest;
}

// The advance spine, from the capture: muster -> city gate -> first
// plaza brawl -> the southern square before the abomination wall.
static const float BATTLE_MARCH[][3] =
{
    { -1436.00f, 1363.50f, 35.70f },
    { -1443.70f, 1401.10f, 35.70f },
    { -1514.20f, 1419.00f, 35.40f },
    { -1551.20f, 1413.50f, 35.70f },
    { -1553.30f, 1377.80f, 35.70f },
    { -1559.50f, 1335.50f, 35.60f },
    { -1560.50f, 1325.00f, 35.60f },
};

// The army's advance, distilled from 11,335 captured militia moves:
// per-minute destination centroids draw seven front lines from the city
// gate to the finale square. A soldier who has cleared his block moves
// up to the next front and hunts there.
static const float BATTLE_FRONTS[][3] =
{
    { -1478.0f, 1399.0f, 35.6f },
    { -1561.0f, 1338.0f, 35.6f },
    { -1656.0f, 1312.0f, 20.3f },
    { -1728.0f, 1343.0f, 20.1f },
    { -1777.0f, 1422.0f, 20.3f },
    { -1761.0f, 1592.0f, 20.6f },
    { -1687.0f, 1629.0f, 20.6f },
};

// The militia are riflemen: the capture logs over two thousand casts of
// Shoot (6660) from them across the battle - fire at range, steel up
// close.
struct npc_gilnean_militia : public CreatureScript
{
    npc_gilnean_militia() : CreatureScript("npc_gilnean_militia") {}

    struct npc_gilnean_militiaAI : public ScriptedAI
    {
        npc_gilnean_militiaAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiShootTimer;
        uint32 m_uiSweepTimer;
        uint32 m_uiFront;
        bool m_bBattle;
        uint32 m_uiLureState;    ///< 0 none, 1 running in, 2 dragging, 3 holding
        uint32 m_uiLureTimer;
        ObjectGuid m_lureGuid;   ///< the abomination being baited
        bool m_bFiringLine;      ///< planted at the finale, rifles on the queen

        void Reset() override
        {
            m_uiShootTimer = 1500;
            m_uiSweepTimer = 3000;
            m_uiFront = 0;
            m_bBattle = false;
            m_uiLureState = 0;
            m_uiLureTimer = 0;
            m_lureGuid.Clear();
            m_bFiringLine = false;

            // Routed movement: the old IGNORE_PATHFINDING band-aid (the
            // mastiff-saga churn workaround) walked the ranks through
            // houses. The reachability grace in SelectHostileTarget now
            // absorbs the transient path verdicts that caused the churn.
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/,
                            Unit* /*pInvoker*/, uint32 /*uiMiscValue*/) override
        {
            // The prince releases the ranks at FOR GILNEAS - only then
            // does the citywide sweep begin.
            if (eventType == AI_EVENT_CUSTOM_A)
            {
                m_bBattle = true;
            }
            // The charge order: not a synchronized pivot - this soldier's
            // own sweep takes it up somewhere in the next 2.5 seconds, so
            // the line breaks into the fight as a ragged human wave.
            else if (eventType == AI_EVENT_CUSTOM_C)
            {
                m_bBattle = true;
                m_uiSweepTimer = urand(0, 2500);
            }
            // The finale: plant where you stand and open fire on the
            // Banshee Queen and her ghosts (the capture's 6660 volleys).
            else if (eventType == AI_EVENT_CUSTOM_B)
            {
                m_bFiringLine = true;
                m_uiLureState = 0;
                m_uiShootTimer = urand(400, 4200);          // ragged first volley
                m_creature->CombatStop(true);
                m_creature->DeleteThreatList();
                m_creature->GetMotionMaster()->Clear(false);
                m_creature->GetMotionMaster()->MoveIdle();

                if (Creature* pQueen = GetClosestCreatureWithEntry(m_creature,
                        NPC_SYLVANAS_BATTLE, 100.0f))
                {
                    m_creature->SetFacingToObject(pQueen);
                }
            }
        }

        void EnterEvadeMode() override
        {
            // The default evade fires AFTER KilledUnit and clobbers the
            // rejoin: it wipes the motion master and walks the soldier to
            // his anchor. A battle soldier sheds combat and falls back
            // into the column instead.
            m_creature->RemoveAllAurasOnEvade();
            m_creature->DeleteThreatList();
            m_creature->CombatStop(true);
            m_creature->SetLootRecipient(NULL);

            if (m_bBattle)
            {
                RejoinThePrince();
            }
            else
            {
                m_creature->GetMotionMaster()->MoveTargetedHome();
            }
        }

        void RejoinThePrince()
        {
            if (Creature* pLiam = GetClosestCreatureWithEntry(m_creature,
                    NPC_LIAM_BATTLE, 200.0f))
            {
                if (pLiam->IsAlive())
                {
                    // A stable formation slot (keyed on the guid, so it stays
                    // the same soldier's place in the ranks) that aims where
                    // the prince is GOING - the column marches as ranks
                    // instead of a follow-knot rubber-banding behind him.
                    const uint32 uiSeat = m_creature->GetGUIDLow();
                    m_creature->SetWalk(false);
                    m_creature->GetMotionMaster()->MoveInFormation(pLiam,
                        3.5f + (uiSeat % 9) * 1.3f,
                        M_PI_F + ((int32(uiSeat % 11) - 5) * 0.22f));
                }
            }
        }

        void JustDied(Unit* /*pKiller*/) override
        {
            // The five-minute respawn happens at the MUSTER, not at the
            // street corner where this soldier fell.
            RestoreDbSpawn(m_creature);
        }

        /// "The rest of you, lure the abominations into the open!" - the
        /// open is the street in front of the emplaced guns.
        bool TryStartLure()
        {
            // Every third soldier is a lure; the rest hold the line.
            if (m_creature->GetGUIDLow() % 3)
            {
                return false;
            }

            std::list<Creature*> lAboms;
            GetCreatureListWithEntryInGrid(lAboms, m_creature,
                NPC_VILE_ABOMINATION, 100.0f);

            for (std::list<Creature*>::iterator itr = lAboms.begin();
                 itr != lAboms.end(); ++itr)
            {
                Creature* pAbom = *itr;

                // Still standing on the wall, not already dragged out.
                if (pAbom->IsAlive() &&
                    pAbom->GetDistance2d(-1625.0f, 1308.0f) > 35.0f)
                {
                    m_uiLureState = 1;
                    m_uiLureTimer = 15000;                  // failsafe
                    m_lureGuid = pAbom->GetObjectGuid();
                    m_creature->SetWalk(false);
                    AttackStart(pAbom);
                    pAbom->AddThreat(m_creature, 1000.0f);  // the bait sticks
                    return true;
                }
            }

            return false;
        }

        void LureTick(const uint32 uiDiff)
        {
            Creature* pAbom = m_creature->GetMap()->GetCreature(m_lureGuid);

            if (!pAbom || !pAbom->IsAlive())
            {
                m_uiLureState = 0;
                m_lureGuid.Clear();
                return;
            }

            if (m_uiLureTimer <= uiDiff)
            {
                m_uiLureTimer = 0;
            }
            else
            {
                m_uiLureTimer -= uiDiff;
            }

            if (m_uiLureState == 1)
            {
                // Tagged and close (or out of patience): turn and DRAG -
                // stop swinging but keep the threat, and run for the open.
                if (m_creature->IsWithinDistInMap(pAbom, 10.0f) || !m_uiLureTimer)
                {
                    m_uiLureState = 2;
                    m_uiLureTimer = 12000;                  // failsafe
                    m_creature->AttackStop(true);
                    m_creature->GetMotionMaster()->MovePoint(70,
                        -1625.0f + frand(-6.0f, 6.0f),
                        1308.0f + frand(-5.0f, 5.0f), 19.9f);
                }
            }
            else if (m_uiLureState == 2 && !m_uiLureTimer)
            {
                m_uiLureState = 3;                          // failsafe: fight here
            }

            if (m_uiLureState == 3)
            {
                m_uiLureState = 0;
                m_lureGuid.Clear();
                AttackStart(pAbom);                         // turn and fight in the open
            }
        }

        void MovementInform(uint32 uiMovementType, uint32 uiPointId) override
        {
            // The drag reached the open: turn and fight.
            if (uiMovementType == POINT_MOTION_TYPE && uiPointId == 70 &&
                m_uiLureState == 2)
            {
                m_uiLureState = 3;
            }
        }

        void KilledUnit(Unit* /*pVictim*/) override
        {
            // Anchor on the corpse - and take a BREATH. The instant
            // corpse-to-next-fight chaining read as bots; the sweep picks
            // the next fight a few human seconds from now.
            m_creature->SetSpawn(Geometry::Vector3(m_creature->GetPositionX(),
                m_creature->GetPositionY(), m_creature->GetPositionZ()),
                m_creature->GetOrientation());

            if (m_bBattle)
            {
                // Stand over the kill. Without this the popping chase
                // revealed the formation slot BEHIND the column and the
                // soldier walked backwards until the sweep re-engaged -
                // the aggro/go-back/target twitch loop.
                m_creature->GetMotionMaster()->MoveIdle();
                m_uiSweepTimer = urand(1500, 4500);
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // The firing line: volleys at the ghosts first, the queen when
            // none stand - and dead silence during the freeze-frame.
            if (m_bFiringLine)
            {
                if (m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED))
                {
                    return;
                }

                if (m_uiShootTimer <= uiDiff)
                {
                    Creature* pMark = GetClosestCreatureWithEntry(m_creature,
                        NPC_BANSHEE_BATTLE, 60.0f);

                    if (!pMark || !pMark->IsAlive())
                    {
                        pMark = GetClosestCreatureWithEntry(m_creature,
                            NPC_SYLVANAS_BATTLE, 80.0f);
                    }

                    if (pMark && pMark->IsAlive() &&
                        !m_creature->IsNonMeleeSpellCasted(false))
                    {
                        m_creature->SetFacingToObject(pMark);
                        m_creature->CastSpell(pMark, 6660, false);
                        m_uiShootTimer = urand(3500, 6500);
                    }
                    else
                    {
                        m_uiShootTimer = 1500;
                    }
                }
                else
                {
                    m_uiShootTimer -= uiDiff;
                }

                return;
            }

            // A lure mid-play is committed to it.
            if (m_uiLureState)
            {
                LureTick(uiDiff);

                if (m_uiLureState == 2)
                {
                    return;                                 // dragging: no other business
                }
            }

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                if (!m_bBattle)
                {
                    return;                                 // parade rest at the muster
                }

                // The sweep: between fights a soldier hunts the next
                // Forsaken group on his own - point to point, group to
                // group, until the district is clear. The column march
                // (follow) takes precedence.
                if (m_uiSweepTimer <= uiDiff)
                {
                    m_uiSweepTimer = urand(2500, 5000);    // unhurried attention

                    // Fight what stands NEAREST, within the SOLDIER's own
                    // thirty yards. The old leash measured fights against
                    // the marching prince - every step he took made engaged
                    // foes "illegal" and the line broke off, re-aggroed and
                    // broke off again. A man's fight is what is around HIM;
                    // the formation drags his locality forward anyway.
                    Creature* pFoe = SelectNearestBattleFoe(m_creature, 30.0f);

                    if (pFoe)
                    {
                        m_creature->SetSpawn(Geometry::Vector3(
                            m_creature->GetPositionX(), m_creature->GetPositionY(),
                            m_creature->GetPositionZ()), m_creature->GetOrientation());
                        m_creature->SetWalk(false);
                        AttackStart(pFoe);
                        pFoe->AddThreat(m_creature);
                    }
                    // ...no line work: answer the prince's order and bait an
                    // abomination off the main street...
                    else if (TryStartLure())
                    {
                        m_creature->SetSpawn(Geometry::Vector3(
                            m_creature->GetPositionX(), m_creature->GetPositionY(),
                            m_creature->GetPositionZ()), m_creature->GetOrientation());
                    }
                    // ...otherwise HOLD GROUND. A soldier rejoins the column
                    // only when it has genuinely left him behind - the
                    // automatic snap-back to the slot between every kill was
                    // the circle: rejoin, catch aggro, turn, target already
                    // dead, rejoin... The march beats reform the ranks on
                    // purpose; between them, a man stands his block.
                    else if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() != FORMATION_MOTION_TYPE)
                    {
                        Creature* pLiam = GetClosestCreatureWithEntry(m_creature,
                            NPC_LIAM_BATTLE, 200.0f);

                        if (!pLiam || !m_creature->IsWithinDistInMap(pLiam, 45.0f))
                        {
                            m_creature->SetSpawn(Geometry::Vector3(
                                m_creature->GetPositionX(), m_creature->GetPositionY(),
                                m_creature->GetPositionZ()), m_creature->GetOrientation());
                            RejoinThePrince();
                        }
                    }
                }
                else
                {
                    m_uiSweepTimer -= uiDiff;
                }

                return;
            }

            Unit* pVictim = m_creature->getVictim();

            if (m_uiShootTimer <= uiDiff)
            {
                // A rifle is fired STANDING - never while running - and on
                // a man's rhythm, not a metronome's.
                if (m_creature->IsStopped() &&
                    !m_creature->IsWithinDistInMap(pVictim, 8.0f) &&
                    m_creature->IsWithinDistInMap(pVictim, 30.0f) &&
                    !m_creature->IsNonMeleeSpellCasted(false))
                {
                    m_creature->CastSpell(pVictim, 6660, false);
                    m_uiShootTimer = urand(3000, 5200);
                }
                else
                {
                    m_uiShootTimer = 1200;
                }
            }
            else
            {
                m_uiShootTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_gilnean_militiaAI(pCreature);
    }
};

// The delivery, straight from the capture (12:41:44-12:42:08): at
// Lorna's beat, three guns and sixteen hands spawn at the foot of the
// Emberstone slope, run the guns up onto the plaza, arc around its
// centre, and split - two guns emplace at the top of the stairs and
// one rolls on down to the lower street, muzzle at the abomination
// wall. On arrival each gun sheds NOT_SELECTABLE and becomes a
// working piece, and the crew stays at its gun.
//
// The old version drove ONE gun on wall-clock timers with the crew on
// MoveFollow. The follow generator repaths through the stair navmesh
// every tick and jams (the trace proved the gun finished all five
// legs; the stuck "crew on the stairs" were the followers), and the
// 20-yard despawn sweep at the end never reached the stragglers. Now
// every mover walks the same explicit capture legs with pathfinding
// off - a straight spline always completes - and the chain advances
// on MovementInform, not on a clock.
struct CannonLeg
{
    float m_fX, m_fY, m_fZ;
};

static const CannonLeg CANNON_ROUTE_A[] =                   // down to the lower street
{
    { -1553.00f, 1284.50f, 14.00f },
    { -1564.90f, 1280.00f, 24.20f },
    { -1585.40f, 1279.90f, 35.88f },
    { -1592.90f, 1287.30f, 36.00f },
    { -1582.80f, 1299.00f, 35.99f },
    { -1567.30f, 1310.30f, 35.94f },
    { -1571.10f, 1312.60f, 35.63f },
    { -1589.90f, 1314.60f, 26.72f },
    { -1615.24f, 1309.86f, 19.09f },
};

static const CannonLeg CANNON_ROUTE_B[] =                   // top of the stairs, north gun
{
    { -1551.50f, 1281.50f, 13.00f },
    { -1563.50f, 1278.60f, 23.50f },
    { -1578.50f, 1275.60f, 33.90f },
    { -1584.50f, 1275.90f, 35.88f },
    { -1590.50f, 1291.50f, 36.00f },
    { -1578.60f, 1298.80f, 35.96f },
    { -1570.30f, 1312.00f, 35.63f },
    { -1574.30f, 1314.78f, 35.63f },
};

static const CannonLeg CANNON_ROUTE_C[] =                   // top of the stairs, south gun
{
    { -1552.50f, 1287.50f, 13.50f },
    { -1564.80f, 1284.90f, 23.30f },
    { -1586.20f, 1280.60f, 35.88f },
    { -1590.60f, 1288.30f, 35.99f },
    { -1569.40f, 1301.80f, 35.93f },
    { -1575.13f, 1317.56f, 35.67f },
};

static const CannonLeg* CANNON_ROUTES[3] = { CANNON_ROUTE_A, CANNON_ROUTE_B, CANNON_ROUTE_C };
static const uint8 CANNON_ROUTE_LEN[3] = { 9, 8, 6 };
static const float CANNON_ROUTE_FACING[3] = { 3.10f, 2.97f, 3.19f };

// The shared leg-walker: miscValue = route | (slot << 2); slot 0 is
// the gun itself, slots 1-4 the crew.
struct npc_cannon_walkerAI : public ScriptedAI
{
    npc_cannon_walkerAI(Creature* pCreature, bool bIsGun) : ScriptedAI(pCreature),
        m_bIsGun(bIsGun)
    {
        m_uiRoute = 0;
        m_uiSlot = 0;
        m_uiLeg = 0;
        m_uiStartTimer = 0;
        m_bDone = false;
    }

    bool m_bIsGun;
    uint32 m_uiRoute;
    uint32 m_uiSlot;
    uint32 m_uiLeg;
    uint32 m_uiStartTimer;
    bool m_bDone;
    Movement::PointsArray m_ride;

    void Reset() override
    {
        // An uncrewed gun cannot be boarded while it rolls.
        if (m_bIsGun && !m_bDone)
        {
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }
    }

    void AttackStart(Unit* /*pWho*/) override {}
    void MoveInLineOfSight(Unit* /*pWho*/) override {}
    void AttackedBy(Unit* /*pAttacker*/) override {}

    void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/,
                        Unit* /*pInvoker*/, uint32 uiMiscValue) override
    {
        if (eventType != AI_EVENT_CUSTOM_A || m_uiStartTimer || m_bDone)
        {
            return;
        }

        m_uiRoute = uiMiscValue & 0x3;
        m_uiSlot = uiMiscValue >> 2;
        m_uiLeg = 0;
        m_uiStartTimer = m_uiSlot ? 650 + m_uiSlot * 150 : 400;

        // One pace for the whole knot: the crew's template runs faster
        // than the gun (1.14 vs 1.0) and overtook it mid-slope.
        m_creature->SetSpeedRate(MOVE_RUN, 1.0f);
    }

    /// The whole authored route as one spline; crew points sit in the gun's
    /// wake (behind the carriage, flanking) so four hands read as a push.
    void BuildRide()
    {
        m_ride.clear();

        float fPrevX = m_creature->GetPositionX();
        float fPrevY = m_creature->GetPositionY();

        for (uint32 i = 0; i < CANNON_ROUTE_LEN[m_uiRoute]; ++i)
        {
            const CannonLeg& leg = CANNON_ROUTES[m_uiRoute][i];
            float fX = leg.m_fX;
            float fY = leg.m_fY;

            if (m_uiSlot)
            {
                float fDX = fX - fPrevX;
                float fDY = fY - fPrevY;
                float fLen = sqrt(fDX * fDX + fDY * fDY);

                if (fLen > 0.1f)
                {
                    fDX /= fLen;
                    fDY /= fLen;

                    float fBack = 1.6f + (m_uiSlot - 1) * 1.0f;
                    float fSide = (m_uiSlot & 1) ? 1.3f : -1.3f;

                    fX += -fDX * fBack - fDY * fSide;
                    fY += -fDY * fBack + fDX * fSide;
                }
            }

            m_ride.push_back(Movement::Vector3(fX, fY, leg.m_fZ));
            fPrevX = leg.m_fX;
            fPrevY = leg.m_fY;
        }
    }

    void MovementInform(uint32 uiMovementType, uint32 uiPointId) override
    {
        if (uiMovementType != PATH_MOTION_TYPE || uiPointId != 1 || m_bDone)
        {
            return;
        }

        // Emplaced: the gun opens for business, the crew stands to.
        m_bDone = true;
        m_creature->SetFacingTo(CANNON_ROUTE_FACING[m_uiRoute]);

        if (m_bIsGun)
        {
            m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_uiStartTimer || m_bDone)
        {
            return;
        }

        if (m_uiStartTimer > uiDiff)
        {
            m_uiStartTimer -= uiDiff;
            return;
        }

        m_uiStartTimer = 0;
        m_creature->SetWalk(false);
        BuildRide();
        m_creature->GetMotionMaster()->MovePath(1, m_ride, false);
    }
};

struct npc_emberstone_cannon : public CreatureScript
{
    npc_emberstone_cannon() : CreatureScript("npc_emberstone_cannon") {}

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_cannon_walkerAI(pCreature, true);
    }
};

struct npc_cannon_crew : public CreatureScript
{
    npc_cannon_crew() : CreatureScript("npc_cannon_crew") {}

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_cannon_walkerAI(pCreature, false);
    }
};

/*######
# npc_lorna_battle - Lorna through the battle (38426)
######*/

// The capture's Lorna: she spawns WITH the crew at the foot of the
// slope, runs up to the plaza edge by the stairs and delivers her
// line from there (12:41:53); at the abomination brawl she runs back
// down and leads the lure (charging the wall at 12:42:51), then
// fights along the push all the way to the finale square, where the
// scene freezes her.
static const CannonLeg LORNA_UP[] =
{
    { -1560.12f, 1283.28f, 20.28f },
    { -1575.44f, 1279.97f, 31.47f },
    { -1598.46f, 1289.56f, 36.04f },
};

static const CannonLeg LORNA_DOWN[] =
{
    { -1570.65f, 1316.27f, 35.63f },
    { -1589.55f, 1317.09f, 27.11f },
    { -1622.32f, 1309.68f, 20.06f },
};

enum
{
    LORNA_PHASE_CAMP            = 0,
    LORNA_PHASE_UP              = 1,
    LORNA_PHASE_AT_GUNS         = 2,
    LORNA_PHASE_DOWN            = 3,
    LORNA_PHASE_BATTLE          = 4,

    SPELL_MILITIA_SHOOT         = 6660,
};

struct npc_lorna_battle : public CreatureScript
{
    npc_lorna_battle() : CreatureScript("npc_lorna_battle") {}

    struct npc_lorna_battleAI : public ScriptedAI
    {
        npc_lorna_battleAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_uiPhase = LORNA_PHASE_CAMP;
        }

        uint32 m_uiPhase;
        uint32 m_uiShootTimer;
        uint32 m_uiSweepTimer;

        void Reset() override
        {
            m_uiShootTimer = 1500;
            m_uiSweepTimer = 2000;
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/,
                            Unit* /*pInvoker*/, uint32 /*uiMiscValue*/) override
        {
            if (eventType == AI_EVENT_CUSTOM_A && m_uiPhase == LORNA_PHASE_CAMP)
            {
                m_uiPhase = LORNA_PHASE_UP;

                Movement::PointsArray ride;

                for (uint8 i = 0; i < 3; ++i)
                {
                    ride.push_back(Movement::Vector3(LORNA_UP[i].m_fX,
                        LORNA_UP[i].m_fY, LORNA_UP[i].m_fZ));
                }

                m_creature->SetWalk(false);
                m_creature->GetMotionMaster()->MovePath(21, ride, false);
            }
            else if (eventType == AI_EVENT_CUSTOM_B && m_uiPhase == LORNA_PHASE_AT_GUNS)
            {
                m_uiPhase = LORNA_PHASE_DOWN;

                Movement::PointsArray ride;

                for (uint8 i = 0; i < 3; ++i)
                {
                    ride.push_back(Movement::Vector3(LORNA_DOWN[i].m_fX,
                        LORNA_DOWN[i].m_fY, LORNA_DOWN[i].m_fZ));
                }

                m_creature->SetWalk(false);
                m_creature->GetMotionMaster()->MovePath(22, ride, false);
            }
        }

        void MovementInform(uint32 uiMovementType, uint32 uiPointId) override
        {
            if (uiMovementType != PATH_MOTION_TYPE)
            {
                return;
            }

            if (uiPointId == 21 && m_uiPhase == LORNA_PHASE_UP)
            {
                m_uiPhase = LORNA_PHASE_AT_GUNS;
                m_creature->SetFacingTo(0.95f);             // toward the prince at the bridge
                DoScriptText(SAY_BATTLE_LORNA_CANNONS, m_creature);
            }
            else if (uiPointId == 22 && m_uiPhase == LORNA_PHASE_DOWN)
            {
                m_uiPhase = LORNA_PHASE_BATTLE;
            }
        }

        void RejoinThePrince()
        {
            if (Creature* pLiam = GetClosestCreatureWithEntry(m_creature,
                    NPC_LIAM_BATTLE, 200.0f))
            {
                if (pLiam->IsAlive())
                {
                    m_creature->SetWalk(false);
                    m_creature->GetMotionMaster()->MoveInFormation(pLiam, 5.0f,
                        M_PI_F * 0.9f);
                }
            }
        }

        void EnterEvadeMode() override
        {
            m_creature->RemoveAllAurasOnEvade();
            m_creature->DeleteThreatList();
            m_creature->CombatStop(true);
            m_creature->SetLootRecipient(NULL);

            if (m_uiPhase == LORNA_PHASE_BATTLE)
            {
                RejoinThePrince();                          // never back to the camp
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // The finale freeze-frame.
            if (m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED))
            {
                return;
            }

            if (m_uiPhase != LORNA_PHASE_BATTLE)
            {
                return;
            }

            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                if (m_uiSweepTimer <= uiDiff)
                {
                    m_uiSweepTimer = urand(2500, 5000);

                    Creature* pFoe = SelectBattleFoe(m_creature, 60.0f);

                    if (pFoe)
                    {
                        m_creature->SetWalk(false);
                        AttackStart(pFoe);
                        pFoe->AddThreat(m_creature);
                    }
                    else if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() != FORMATION_MOTION_TYPE)
                    {
                        RejoinThePrince();
                    }
                }
                else
                {
                    m_uiSweepTimer -= uiDiff;
                }

                return;
            }

            Unit* pVictim = m_creature->getVictim();

            if (m_uiShootTimer <= uiDiff)
            {
                if (m_creature->IsStopped() &&
                    !m_creature->IsWithinDistInMap(pVictim, 8.0f) &&
                    m_creature->IsWithinDistInMap(pVictim, 30.0f) &&
                    !m_creature->IsNonMeleeSpellCasted(false))
                {
                    m_creature->CastSpell(pVictim, SPELL_MILITIA_SHOOT, false);
                    m_uiShootTimer = urand(3000, 5200);
                }
                else
                {
                    m_uiShootTimer = 1000;
                }
            }
            else
            {
                m_uiShootTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_lorna_battleAI(pCreature);
    }
};

/*######
# npc_battle_worgen_warrior - the worgen waves (38348)
######*/

// The capture spawns worgen warrior waves through the Gorerot era and
// they fight the Forsaken where they stand (their spawn rows sit on
// the capture spots to the decimal). Imported as wander/waypoint
// statics they paced the square instead; now they hold their posts
// and stage their own fights.
struct npc_battle_worgen_warrior : public CreatureScript
{
    npc_battle_worgen_warrior() : CreatureScript("npc_battle_worgen_warrior") {}

    struct npc_battle_worgen_warriorAI : public ScriptedAI
    {
        npc_battle_worgen_warriorAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiScanTimer;

        void Reset() override
        {
            m_uiScanTimer = urand(1000, 3000);
            m_creature->SetNoCallAssistance(true);
        }

        void EnterEvadeMode() override
        {
            // Shed the fight AND its chase stack, then walk back to the
            // post routed - the default evade left dead chases stacked.
            m_creature->RemoveAllAurasOnEvade();
            m_creature->DeleteThreatList();
            m_creature->CombatStop(true);
            m_creature->SetLootRecipient(NULL);
            m_creature->GetMotionMaster()->Clear(false);
            m_creature->GetMotionMaster()->MoveTargetedHome();
        }

        void DamageDeal(Unit* pDoneTo, uint32& uiDamage) override
        {
            // The district's war is CINEMA: the pack never finishes its
            // Forsaken sparring partners (a won street stands dead and
            // silent - the report), and it never chips Gorerot either;
            // the catapults kill the giant, the army kills the streets.
            if (pDoneTo->GetTypeId() == TYPEID_UNIT &&
                !pDoneTo->IsCharmerOrOwnerPlayerOrPlayerItself())
            {
                uiDamage = 0;
            }
        }

        void DamageTaken(Unit* pDealer, uint32& uiDamage) override
        {
            // And the Forsaken never finish the pack - the pairs brawl for
            // as long as the battle phase stands. Gorerot is the exception:
            // he slaughters the bait waves, and the 15s respawns feed him.
            if (pDealer && pDealer->GetTypeId() == TYPEID_UNIT &&
                pDealer->GetEntry() != NPC_GOREROT &&
                !pDealer->IsCharmerOrOwnerPlayerOrPlayerItself())
            {
                uiDamage = 0;
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                if (m_uiScanTimer <= uiDiff)
                {
                    m_uiScanTimer = urand(2500, 6000);

                    // The capture's Gorerot beat: the moment the giant is up,
                    // the whole pack CHARGES him from the west-street posts
                    // and dies on him in waves while the catapults work -
                    // wave-2 worgen flip to target 38331 within seconds of
                    // his spawn, and the posts refill ~6s after each wipe.
                    if (Creature* pGorerot = GetClosestCreatureWithEntry(m_creature,
                            NPC_GOREROT, 120.0f))
                    {
                        if (pGorerot->IsAlive())
                        {
                            m_creature->SetWalk(false);
                            AttackStart(pGorerot);
                            pGorerot->AddThreat(m_creature);
                            return;
                        }
                    }

                    Creature* pFoe = SelectBattleFoe(m_creature, 40.0f);

                    if (pFoe)
                    {
                        m_creature->SetWalk(false);
                        AttackStart(pFoe);
                        pFoe->AddThreat(m_creature);

                        if (!pFoe->getVictim() && pFoe->AI())
                        {
                            pFoe->AI()->AttackStart(m_creature);
                        }
                    }
                }
                else
                {
                    m_uiScanTimer -= uiDiff;
                }

                return;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_battle_worgen_warriorAI(pCreature);
    }
};

/*######
# npc_soultethered_banshee - Sylvanas' guard (38473)
######*/

// Three banshees stand with Sylvanas at the finale square. Untouchable
// until the king charges; they fight only him, and vanish at Enough!.
struct npc_soultethered_banshee : public CreatureScript
{
    npc_soultethered_banshee() : CreatureScript("npc_soultethered_banshee") {}

    struct npc_soultethered_bansheeAI : public ScriptedAI
    {
        npc_soultethered_bansheeAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        void Reset() override
        {
            m_creature->SetNoCallAssistance(true);
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
        }

        void MoveInLineOfSight(Unit* /*pWho*/) override {}

        void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/,
                            Unit* pInvoker, uint32 /*uiMiscValue*/) override
        {
            if (eventType == AI_EVENT_CUSTOM_A && pInvoker)
            {
                m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
                AttackStart(pInvoker);
            }
        }

        void UpdateAI(const uint32 /*uiDiff*/) override
        {
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_soultethered_bansheeAI(pCreature);
    }
};

/*######
# npc_sylvanas_battle - the finale scene (38469)
######*/

// The capture's finale, beat for beat: the king charges in worgen
// form and duels the Banshee Queen with steel for half a minute
// (12:47:36-12:48:07); at Enough! her guard vanishes, Genn is thrown
// back, and the field freezes; she promises Gilneas a leaderless
// future; the prince races in shouting for his father and takes the
// arrow; she flees east and is gone (12:48:28).
struct npc_sylvanas_battle : public CreatureScript
{
    npc_sylvanas_battle() : CreatureScript("npc_sylvanas_battle") {}

    struct npc_sylvanas_battleAI : public ScriptedAI
    {
        npc_sylvanas_battleAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_uiStep = 0;
        }

        uint32 m_uiStep;
        uint32 m_uiStepTimer;

        void Reset() override
        {
            m_uiStepTimer = 0;
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
        }

        void MoveInLineOfSight(Unit* /*pWho*/) override {}

        void EnterEvadeMode() override
        {
            // The scene owns her: no evade-home, no state reset mid-duel.
            m_creature->RemoveAllAurasOnEvade();
            m_creature->DeleteThreatList();
            m_creature->CombatStop(true);
            m_creature->SetLootRecipient(NULL);
        }

        void DamageTaken(Unit* /*pDealer*/, uint32& uiDamage) override
        {
            // The duel is real steel, but the queen does not die here.
            uint32 uiFloor = m_creature->GetMaxHealth() / 4;

            if (m_creature->GetHealth() < uiDamage + uiFloor)
            {
                uiDamage = m_creature->GetHealth() > uiFloor
                    ? m_creature->GetHealth() - uiFloor : 0;

                // 25%: the Wail. Retail's trigger is her health, not a
                // clock - pull the Enough! beat to now (the 31s timer
                // remains only as the no-damage failsafe).
                if (m_uiStep == 3 && m_uiStepTimer > 1200)
                {
                    m_uiStepTimer = 1200;
                }
            }
        }

        void ReceiveAIEvent(AIEventType eventType, Creature* /*pSender*/,
                            Unit* /*pInvoker*/, uint32 /*uiMiscValue*/) override
        {
            if (eventType == AI_EVENT_CUSTOM_A && !m_uiStep)
            {
                m_uiStep = 1;
                m_uiStepTimer = 300;
            }
        }

        Creature* GetGenn()
        {
            return GetClosestCreatureWithEntry(m_creature, NPC_GENN_BATTLE, 150.0f);
        }

        void FreezeWitness(uint32 uiEntry, bool bFreeze)
        {
            std::list<Creature*> lWitness;
            GetCreatureListWithEntryInGrid(lWitness, m_creature, uiEntry, 250.0f);

            for (std::list<Creature*>::iterator itr = lWitness.begin();
                 itr != lWitness.end(); ++itr)
            {
                if (!(*itr)->IsAlive())
                {
                    continue;
                }

                if (bFreeze)
                {
                    (*itr)->CombatStop(true);
                    (*itr)->GetMotionMaster()->MoveIdle();
                    (*itr)->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
                }
                else
                {
                    (*itr)->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
                }
            }
        }

        void MovementInform(uint32 uiMovementType, uint32 uiPointId) override
        {
            // The flight east as one spline, then gone.
            if (uiMovementType == PATH_MOTION_TYPE && uiPointId == 100)
            {
                m_creature->ForcedDespawn(200);
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiStepTimer)
            {
                if (m_uiStepTimer > uiDiff)
                {
                    m_uiStepTimer -= uiDiff;
                }
                else
                {
                    m_uiStepTimer = 0;

                    switch (m_uiStep)
                    {
                        case 1:                             // the king charges
                            if (Creature* pGenn = GetGenn())
                            {
                                pGenn->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE);
                                pGenn->SetDisplayId(DISPLAY_GENN_WORGEN);
                                pGenn->GetMotionMaster()->MoveJump(
                                    -1678.89f, 1614.87f, 20.61f, 24.0f, 8.0f);
                                pGenn->CastSpell(m_creature,
                                    SPELL_GENN_VS_SYLVANAS, true);
                            }
                            m_uiStepTimer = 1500;
                            break;
                        case 2:                             // the duel
                        {
                            m_creature->RemoveFlag(UNIT_FIELD_FLAGS,
                                UNIT_FLAG_OOC_NOT_ATTACKABLE);

                            if (Creature* pGenn = GetGenn())
                            {
                                AttackStart(pGenn);
                                pGenn->AddThreat(m_creature);

                                if (pGenn->AI())
                                {
                                    pGenn->AI()->AttackStart(m_creature);
                                }

                                std::list<Creature*> lGuard;
                                GetCreatureListWithEntryInGrid(lGuard, m_creature,
                                    NPC_BANSHEE_BATTLE, 80.0f);

                                for (std::list<Creature*>::iterator itr = lGuard.begin();
                                     itr != lGuard.end(); ++itr)
                                {
                                    if ((*itr)->IsAlive() && (*itr)->AI())
                                    {
                                        (*itr)->AI()->ReceiveAIEvent(AI_EVENT_CUSTOM_A,
                                            *itr, pGenn, 0);
                                    }
                                }
                            }

                            m_uiStepTimer = 31000;
                            break;
                        }
                        case 3:                             // Enough! - the Wail
                        {
                            DoScriptText(SAY_FINALE_SYLV_ENOUGH, m_creature);
                            m_creature->CastSpell(m_creature,
                                SPELL_BANSHEE_QUEENS_WAIL, true);
                            m_creature->CombatStop(true);
                            m_creature->GetMotionMaster()->MoveIdle();

                            if (Creature* pGenn = GetGenn())
                            {
                                pGenn->CombatStop(true);
                                pGenn->GetMotionMaster()->MoveJump(
                                    -1680.42f, 1620.50f, 20.61f, 18.0f, 6.0f);
                            }

                            std::list<Creature*> lGuard;
                            GetCreatureListWithEntryInGrid(lGuard, m_creature,
                                NPC_BANSHEE_BATTLE, 100.0f);

                            for (std::list<Creature*>::iterator itr = lGuard.begin();
                                 itr != lGuard.end(); ++itr)
                            {
                                (*itr)->ForcedDespawn();
                            }

                            FreezeWitness(NPC_LORNA_BATTLE, true);
                            FreezeWitness(NPC_DARIUS_BATTLE, true);
                            FreezeWitness(NPC_GILNEAN_MILITIA, true);
                            m_uiStepTimer = 2400;
                            break;
                        }
                        case 4:                             // a leaderless Gilneas
                            DoScriptText(SAY_FINALE_SYLV_LEADER, m_creature);
                            m_creature->SetFacingTo(4.94f); // toward the prince's road
                            m_uiStepTimer = 3700;
                            break;
                        case 5:                             // the prince races in
                            m_creature->SummonCreature(NPC_LIAM_DYING,
                                -1660.79f, 1620.83f, 20.53f, 2.95f,
                                TEMPSPAWN_TIMED_DESPAWN, 900000);
                            m_uiStepTimer = 5300;
                            break;
                        case 6:                             // the arrow lands; she is gone
                            if (Creature* pGenn = GetGenn())
                            {
                                DoScriptText(SAY_FINALE_GENN_NO, pGenn);
                            }

                            m_creature->SetFlag(UNIT_FIELD_FLAGS,
                                UNIT_FLAG_OOC_NOT_ATTACKABLE);
                            m_creature->SetWalk(false);
                            {
                                Movement::PointsArray flight;
                                flight.push_back(Movement::Vector3(-1658.06f, 1620.19f, 20.53f));
                                flight.push_back(Movement::Vector3(-1636.61f, 1620.66f, 20.53f));
                                flight.push_back(Movement::Vector3(-1631.25f, 1644.79f, 20.55f));
                                m_creature->GetMotionMaster()->MovePath(100, flight, false);
                            }
                            m_uiStep = 0;
                            return;
                    }

                    ++m_uiStep;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_sylvanas_battleAI(pCreature);
    }
};

/*######
# npc_liam_dying - the prince's last stand (38474)
######*/

// Summoned at Sylvanas' beat: he sprints in shouting for his father,
// dives between the queen and the king, and falls. The tableau -
// the dead prince, the kneeling king - holds while the field thaws.
struct npc_liam_dying : public CreatureScript
{
    npc_liam_dying() : CreatureScript("npc_liam_dying") {}

    struct npc_liam_dyingAI : public ScriptedAI
    {
        npc_liam_dyingAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_bStarted = false;
        }

        bool m_bStarted;
        uint32 m_uiStep;
        uint32 m_uiStepTimer;

        void Reset() override
        {
            if (m_bStarted)
            {
                return;
            }

            m_bStarted = true;
            m_uiStep = 0;
            m_uiStepTimer = 0;

            m_creature->SetWalk(false);
            DoScriptText(SAY_FINALE_LIAM_FATHER, m_creature);

            Movement::PointsArray dash;
            dash.push_back(Movement::Vector3(-1679.42f, 1620.52f, 20.61f));
            dash.push_back(Movement::Vector3(-1678.52f, 1614.78f, 20.61f));
            m_creature->GetMotionMaster()->MovePath(1, dash, false);
        }

        void AttackStart(Unit* /*pWho*/) override {}
        void MoveInLineOfSight(Unit* /*pWho*/) override {}
        void AttackedBy(Unit* /*pAttacker*/) override {}

        void MovementInform(uint32 uiMovementType, uint32 uiPointId) override
        {
            if (uiMovementType == PATH_MOTION_TYPE && uiPointId == 1)
            {
                m_creature->SetStandState(UNIT_STAND_STATE_DEAD);
                m_uiStep = 1;
                m_uiStepTimer = 1700;
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_uiStepTimer)
            {
                return;
            }

            if (m_uiStepTimer > uiDiff)
            {
                m_uiStepTimer -= uiDiff;
                return;
            }

            switch (m_uiStep)
            {
                case 1:                                     // the king comes to his son
                    if (Creature* pGenn = GetClosestCreatureWithEntry(m_creature,
                            NPC_GENN_BATTLE, 60.0f))
                    {
                        pGenn->SetWalk(true);
                        pGenn->GetMotionMaster()->MovePoint(2,
                            -1678.83f, 1615.73f, 20.61f);
                    }
                    m_uiStepTimer = 2100;
                    break;
                case 2:
                    DoScriptText(SAY_FINALE_LIAM_DID_IT, m_creature);
                    m_uiStepTimer = 700;
                    break;
                case 3:                                     // the king kneels
                    if (Creature* pGenn = GetClosestCreatureWithEntry(m_creature,
                            NPC_GENN_BATTLE, 60.0f))
                    {
                        pGenn->GetMotionMaster()->MoveIdle();
                        pGenn->SetFacingToObject(m_creature);
                        pGenn->SetStandState(UNIT_STAND_STATE_KNEEL);
                    }
                    m_uiStepTimer = 2900;
                    break;
                case 4:
                {
                    DoScriptText(SAY_FINALE_LIAM_TOOK_BACK, m_creature);

                    // The quest's event credit lands HERE, not at Gorerot:
                    // the battle is not won until the scene has played and
                    // the prince has paid for the city.
                    Map::PlayerList const& lCredit = m_creature->GetMap()->GetPlayers();

                    for (Map::PlayerList::const_iterator itr = lCredit.begin();
                         itr != lCredit.end(); ++itr)
                    {
                        Player* pPlayer = itr->getSource();

                        if (pPlayer && pPlayer->IsAlive() &&
                            pPlayer->IsWithinDistInMap(m_creature, 300.0f))
                        {
                            pPlayer->KilledMonsterCredit(NPC_BATTLE_CITY_CREDIT);
                        }
                    }

                    m_uiStepTimer = 13000;
                    break;
                }
                case 5:                                     // the field thaws
                {
                    const uint32 auiWitness[2] = { NPC_LORNA_BATTLE, NPC_DARIUS_BATTLE };

                    for (uint8 i = 0; i < 2; ++i)
                    {
                        std::list<Creature*> lWitness;
                        GetCreatureListWithEntryInGrid(lWitness, m_creature,
                            auiWitness[i], 250.0f);

                        for (std::list<Creature*>::iterator itr = lWitness.begin();
                             itr != lWitness.end(); ++itr)
                        {
                            (*itr)->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
                        }
                    }

                    // The firing line moves out - and re-musters fresh at
                    // the city gate within seconds for the next battle.
                    std::list<Creature*> lLine;
                    GetCreatureListWithEntryInGrid(lLine, m_creature,
                        NPC_GILNEAN_MILITIA, 250.0f);

                    for (std::list<Creature*>::iterator itr = lLine.begin();
                         itr != lLine.end(); ++itr)
                    {
                        (*itr)->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_STUNNED);
                        RestoreDbSpawn(*itr);
                        (*itr)->SetRespawnDelay(15);
                        (*itr)->ForcedDespawn();
                    }

                    m_uiStepTimer = 0;
                    return;
                }
            }

            ++m_uiStep;
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_liam_dyingAI(pCreature);
    }
};

// Damaged goods: the capture shows a Damaged Catapult collapsing to
// zero health fourteen seconds after its shot lands - one throw arms
// the break. The death ejects the gunner; the spawn cycle brings a
// fresh wreck for the next attempt.
struct npc_damaged_catapult : public CreatureScript
{
    npc_damaged_catapult() : CreatureScript("npc_damaged_catapult") {}

    struct npc_damaged_catapultAI : public ScriptedAI
    {
        npc_damaged_catapultAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiBreakTimer;

        void Reset() override
        {
            m_uiBreakTimer = 0;
        }

        void AttackStart(Unit* /*pWho*/) override {}
        void MoveInLineOfSight(Unit* /*pWho*/) override {}

        void SpellHitTarget(Unit* /*pTarget*/, const SpellEntry* pSpell) override
        {
            if ((pSpell->ID == 72051 || pSpell->ID == 72050) && !m_uiBreakTimer)
            {
                m_uiBreakTimer = 14000;                     // the throw was too much
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_uiBreakTimer)
            {
                return;
            }

            if (m_uiBreakTimer <= uiDiff)
            {
                m_uiBreakTimer = 0;
                m_creature->DealDamage(m_creature, m_creature->GetHealth(), NULL,
                    DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
            }
            else
            {
                m_uiBreakTimer -= uiDiff;
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_damaged_catapultAI(pCreature);
    }
};

struct npc_prince_liam_battle : public CreatureScript
{
    npc_prince_liam_battle() : CreatureScript("npc_prince_liam_battle") {}

    struct npc_prince_liam_battleAI : public ScriptedAI
    {
        npc_prince_liam_battleAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiStep;
        uint32 m_uiStepTimer;
        uint32 m_uiProwlTimer;
        uint32 m_uiCooldown;
        uint32 m_uiGorerotWait;
        uint32 m_uiRegroupWait;
        uint32 m_uiAbandonWait;

        void Reset() override
        {
            m_uiStep = 0;
            m_uiStepTimer = 0;
            m_uiProwlTimer = 2000;
            m_uiCooldown = 0;
            m_uiGorerotWait = 0;
            m_uiRegroupWait = 0;
            m_uiAbandonWait = 0;

            // The capture's prince fights - but his SCHEDULE never does.
            // Combat cannot move him (the clock owns his feet); he swings
            // at whoever stands on him and survives on the damage floor.
            m_creature->addUnitState(UNIT_STAT_NO_COMBAT_MOVEMENT);
        }

        void MoveInLineOfSight(Unit* /*pWho*/) override {}

        void AttackedBy(Unit* pAttacker) override
        {
            // Whoever lays hands on the prince gets the rapier - but the
            // fight comes to HIM: NO_COMBAT_MOVEMENT means he never takes
            // a single step for it.
            if (!m_creature->getVictim() && pAttacker)
            {
                AttackStart(pAttacker);
            }
        }

        void DamageTaken(Unit* /*pDealer*/, uint32& uiDamage) override
        {
            // The prince bleeds, but the battle cannot lose its spine.
            uint32 uiFloor = m_creature->GetMaxHealth() * 15 / 100;

            if (m_creature->GetHealth() < uiDamage + uiFloor)
            {
                uiDamage = m_creature->GetHealth() > uiFloor
                    ? m_creature->GetHealth() - uiFloor : 0;
            }
        }

        /// A march leg: shed whatever fight he is in, then move.
        void MarchTo(uint32 uiPointId, float fX, float fY, float fZ)
        {
            m_creature->CombatStop(true);
            m_creature->DeleteThreatList();

            // Drop every leftover generator (finished chases, stale legs)
            // before laying the leg: a stale point beneath a popping chase
            // resumed later and marched the prince BACKWARDS.
            m_creature->GetMotionMaster()->Clear(false);
            m_creature->SetWalk(false);
            m_creature->GetMotionMaster()->MovePoint(uiPointId, fX, fY, fZ);
        }


        void EnterEvadeMode() override
        {
            // The event's spine must NEVER evade-walk home or reset the
            // step machine (the default does both - the "prince marches
            // backwards" report). Shed the fight; the machine drives on.
            m_creature->RemoveAllAurasOnEvade();
            m_creature->DeleteThreatList();
            m_creature->CombatStop(true);
            m_creature->SetLootRecipient(NULL);
        }


        /// The living quest-holder this battle is being fought FOR, or NULL.
        Player* FindQuestPlayer(float fRange) const
        {
            Map::PlayerList const& lPlayers = m_creature->GetMap()->GetPlayers();

            for (Map::PlayerList::const_iterator itr = lPlayers.begin();
                 itr != lPlayers.end(); ++itr)
            {
                Player* pPlayer = itr->getSource();

                if (!pPlayer || !pPlayer->IsAlive() ||
                    !pPlayer->IsWithinDistInMap(m_creature, fRange))
                {
                    continue;
                }

                // Carrying the quest in ANY live state counts: the scene
                // credit flips it COMPLETE long before the turn-in, and
                // judging that player "gone" struck the set in front of
                // them two minutes later.
                QuestStatus status = pPlayer->GetQuestStatus(QUEST_BATTLE_FOR_GILNEAS);

                if (status == QUEST_STATUS_INCOMPLETE ||
                    (status == QUEST_STATUS_COMPLETE &&
                     !pPlayer->GetQuestRewardStatus(QUEST_BATTLE_FOR_GILNEAS)))
                {
                    return pPlayer;
                }
            }

            return NULL;
        }

        /// The player abandoned the battle mid-march: strike the set. The
        /// whole cast re-musters in seconds and the machine goes back to
        /// the prowl, so the next attempt gets the battle from the top.
        void ResetBattle()
        {
            std::list<Creature*> lStrike;
            GetCreatureListWithEntryInGrid(lStrike, m_creature, NPC_GILNEAN_MILITIA, 300.0f);
            GetCreatureListWithEntryInGrid(lStrike, m_creature, NPC_LORNA_BATTLE, 300.0f);
            GetCreatureListWithEntryInGrid(lStrike, m_creature, NPC_DARIUS_BATTLE, 300.0f);

            for (std::list<Creature*>::iterator itr = lStrike.begin();
                 itr != lStrike.end(); ++itr)
            {
                RestoreDbSpawn(*itr);
                (*itr)->SetRespawnDelay(15);
                (*itr)->ForcedDespawn();
            }

            // The summoned cast must not survive into the next run: left
            // standing, the guns and finale actors would duplicate.
            const uint32 auiSummons[4] = { NPC_EMBERSTONE_CANNON, NPC_FREED_VILLAGER,
                                           NPC_GENN_BATTLE, NPC_LIAM_DYING };

            for (uint8 i = 0; i < 4; ++i)
            {
                std::list<Creature*> lProps;
                GetCreatureListWithEntryInGrid(lProps, m_creature, auiSummons[i], 400.0f);

                for (std::list<Creature*>::iterator itr = lProps.begin();
                     itr != lProps.end(); ++itr)
                {
                    (*itr)->ForcedDespawn();
                }
            }

            m_creature->SetRespawnDelay(15);
            m_creature->ForcedDespawn();
        }


        void RanksRehome()
        {
            // The evade leash reads the SPAWN pose: left at the muster it
            // yanked mid-fight soldiers all the way home. The army carries
            // its home with it.
            std::list<Creature*> lMilitia;
            GetCreatureListWithEntryInGrid(lMilitia, m_creature, 38221, 90.0f);

            for (std::list<Creature*>::iterator itr = lMilitia.begin();
                 itr != lMilitia.end(); ++itr)
            {
                if ((*itr)->IsAlive())
                {
                    (*itr)->SetSpawn(Geometry::Vector3((*itr)->GetPositionX(),
                        (*itr)->GetPositionY(), (*itr)->GetPositionZ()),
                        (*itr)->GetOrientation());
                }
            }
        }

        void RanksFollow()
        {
            RanksRehome();

            std::list<Creature*> lMilitia;
            GetCreatureListWithEntryInGrid(lMilitia, m_creature, 38221, 90.0f);

            for (std::list<Creature*>::iterator itr = lMilitia.begin();
                 itr != lMilitia.end(); ++itr)
            {
                if ((*itr)->IsAlive())
                {
                    if ((*itr)->AI())
                    {
                        (*itr)->AI()->ReceiveAIEvent(AI_EVENT_CUSTOM_A,
                            *itr, m_creature, 0);           // released to battle
                    }

                    // A soldier already riding his slot keeps riding it:
                    // re-Mutating everyone in one tick made fifty men stop
                    // and relaunch together - the column-wide lurch.
                    if ((*itr)->GetMotionMaster()->GetCurrentMovementGeneratorType() ==
                        FORMATION_MOTION_TYPE)
                    {
                        continue;
                    }

                    (*itr)->SetWalk(false);
                    // Guid-keyed formation slots: every soldier owns a fixed
                    // place in the ranks and aims where the prince is GOING.
                    // Routed legs; a momentarily unroutable slot waits a
                    // beat instead of walking through a wall.
                    const uint32 uiSeat = (*itr)->GetGUIDLow();
                    (*itr)->GetMotionMaster()->MoveInFormation(m_creature,
                        3.5f + (uiSeat % 9) * 1.3f,
                        M_PI_F + ((int32(uiSeat % 11) - 5) * 0.22f));
                }
            }
        }

        void RanksEngage()
        {
            RanksRehome();

            // The order goes out; every soldier answers on HIS beat - the
            // sweep engages a sane nearby foe within 0-2.5s, so the line
            // breaks into the fight as a wave, not a synchronized pivot.
            std::list<Creature*> lMilitia;
            GetCreatureListWithEntryInGrid(lMilitia, m_creature, 38221, 90.0f);

            for (std::list<Creature*>::iterator itr = lMilitia.begin();
                 itr != lMilitia.end(); ++itr)
            {
                if ((*itr)->IsAlive() && (*itr)->AI())
                {
                    (*itr)->AI()->ReceiveAIEvent(AI_EVENT_CUSTOM_C,
                        *itr, m_creature, 0);
                }
            }
        }

        void CheerTheRanks()
        {
            std::list<Creature*> lMilitia;
            GetCreatureListWithEntryInGrid(lMilitia, m_creature, 38221, 60.0f);

            for (std::list<Creature*>::iterator itr = lMilitia.begin();
                 itr != lMilitia.end(); ++itr)
            {
                if ((*itr)->IsAlive())
                {
                    (*itr)->HandleEmote(EMOTE_ONESHOT_CHEER);
                }
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // Whatever state the machine is in, swing at whoever he is on.
            if (m_creature->getVictim())
            {
                DoMeleeAttackIfReady();
            }

            // The battle is the PLAYER's event: between the first march leg
            // and the finale trigger, hold the current beat while the
            // quest-holder is away - and if they abandon it for two
            // minutes, strike the set for a fresh start.
            if (m_uiStep >= 8 && m_uiStep <= 27 && m_uiStepTimer)
            {
                if (!FindQuestPlayer(150.0f))
                {
                    m_uiAbandonWait += uiDiff;

                    if (m_uiAbandonWait >= 120000)
                    {
                        ResetBattle();
                    }

                    return;
                }

                m_uiAbandonWait = 0;
            }

            if (m_uiCooldown)
            {
                if (m_uiCooldown <= uiDiff)
                {
                    m_uiCooldown = 0;
                }
                else
                {
                    m_uiCooldown -= uiDiff;
                }
            }

            // The prince waits for his audience: a quest holder walking up
            // to the muster starts the speech.
            if (!m_uiStepTimer && !m_uiCooldown)
            {
                if (m_uiProwlTimer <= uiDiff)
                {
                    m_uiProwlTimer = 2000;

                    Map::PlayerList const& lPlayers = m_creature->GetMap()->GetPlayers();

                    for (Map::PlayerList::const_iterator itr = lPlayers.begin();
                         itr != lPlayers.end(); ++itr)
                    {
                        Player* pPlayer = itr->getSource();

                        if (pPlayer && pPlayer->IsAlive() &&
                            pPlayer->GetQuestStatus(QUEST_BATTLE_FOR_GILNEAS) == QUEST_STATUS_INCOMPLETE &&
                            pPlayer->IsWithinDistInMap(m_creature, 45.0f))
                        {
                            m_uiStep = 1;
                            m_uiStepTimer = 1000;
                            break;
                        }
                    }
                }
                else
                {
                    m_uiProwlTimer -= uiDiff;
                }

                return;
            }

            if (!m_uiStepTimer)
            {
                return;
            }

            if (m_uiStepTimer > uiDiff)
            {
                m_uiStepTimer -= uiDiff;
                return;
            }

            switch (m_uiStep)
            {
                case 1: DoScriptText(SAY_BATTLE_SPEECH_1, m_creature); m_uiStepTimer = 8400;  break;
                case 2: DoScriptText(SAY_BATTLE_SPEECH_2, m_creature); m_uiStepTimer = 10900; break;
                case 3: DoScriptText(SAY_BATTLE_SPEECH_3, m_creature); m_uiStepTimer = 10900; break;
                case 4: DoScriptText(SAY_BATTLE_SPEECH_4, m_creature); m_uiStepTimer = 10800; break;
                case 5: DoScriptText(SAY_BATTLE_SPEECH_5, m_creature); m_uiStepTimer = 10900; break;
                case 6:                                     // NEVER SURRENDER - the ranks roar
                    DoScriptText(SAY_BATTLE_SPEECH_6, m_creature);
                    CheerTheRanks();
                    m_uiStepTimer = 3600;
                    break;
                case 7:                                     // FOR GILNEAS - the ride begins
                    DoScriptText(SAY_BATTLE_FOR_GILNEAS, m_creature);
                    m_creature->PlayDirectSound(SOUND_CROWD_ROAR);
                    CheerTheRanks();
                    RanksFollow();
                    m_creature->SetWalk(false);
                    MarchTo(1,
                        BATTLE_MARCH[0][0], BATTLE_MARCH[0][1], BATTLE_MARCH[0][2]);
                    m_uiStepTimer = 29000;
                    break;
                case 8:                                     // "Attack!" mid-march
                    DoScriptText(SAY_BATTLE_ATTACK, m_creature);
                    MarchTo(2,
                        BATTLE_MARCH[1][0], BATTLE_MARCH[1][1], BATTLE_MARCH[1][2]);
                    m_uiStepTimer = 20000;
                    break;
                case 9:
                    RanksRehome();
                    MarchTo(3,
                        BATTLE_MARCH[2][0], BATTLE_MARCH[2][1], BATTLE_MARCH[2][2]);
                    m_uiStepTimer = 25000;
                    break;
                case 10:                                    // first plaza brawl
                    DoScriptText(SAY_BATTLE_TIME_UP, m_creature);
                    RanksEngage();
                    m_uiStepTimer = 20000;
                    break;
                case 11:
                    DoScriptText(SAY_BATTLE_PREVAIL, m_creature);
                    RanksFollow();
                    MarchTo(4,
                        BATTLE_MARCH[3][0], BATTLE_MARCH[3][1], BATTLE_MARCH[3][2]);
                    m_uiStepTimer = 14000;
                    break;
                case 12:
                    RanksRehome();
                    MarchTo(5,
                        BATTLE_MARCH[4][0], BATTLE_MARCH[4][1], BATTLE_MARCH[4][2]);
                    m_uiStepTimer = 12000;
                    break;
                case 13:
                    MarchTo(6,
                        BATTLE_MARCH[5][0], BATTLE_MARCH[5][1], BATTLE_MARCH[5][2]);
                    m_uiStepTimer = 12000;
                    break;
                case 14:
                    RanksRehome();
                    DoScriptText(SAY_BATTLE_PREVAIL, m_creature);
                    MarchTo(7,
                        BATTLE_MARCH[6][0], BATTLE_MARCH[6][1], BATTLE_MARCH[6][2]);
                    m_uiStepTimer = 11000;
                    break;
                case 15:                                    // the wall ahead
                    RanksEngage();
                    m_uiStepTimer = 15000;
                    break;
                case 16:                                    // down the stairs
                    DoScriptText(SAY_BATTLE_ABOMINATIONS, m_creature);
                    RanksRehome();
                    RanksFollow();
                    MarchTo(8, -1596.10f, 1314.50f, 22.80f);
                    m_uiStepTimer = 9000;
                    break;
                case 17:                                    // Lorna brings the guns
                {
                    // The delivery cast, on the capture's spawn spots at the
                    // foot of the Emberstone slope: three guns, twelve hands.
                    static const float aGunSpawns[3][4] =
                    {
                        { -1541.33f, 1287.97f, 5.73f, 3.47f },
                        { -1539.26f, 1284.00f, 5.01f, 3.36f },
                        { -1540.84f, 1291.60f, 4.91f, 3.41f },
                    };

                    for (uint8 r = 0; r < 3; ++r)
                    {
                        if (Creature* pGun = m_creature->SummonCreature(NPC_EMBERSTONE_CANNON,
                                aGunSpawns[r][0], aGunSpawns[r][1], aGunSpawns[r][2],
                                aGunSpawns[r][3], TEMPSPAWN_TIMED_DESPAWN, 900000))
                        {
                            if (pGun->AI())
                            {
                                pGun->AI()->ReceiveAIEvent(AI_EVENT_CUSTOM_A, pGun, pGun, r);
                            }
                        }

                        for (uint8 c = 0; c < 4; ++c)
                        {
                            if (Creature* pHand = m_creature->SummonCreature(NPC_FREED_VILLAGER,
                                    aGunSpawns[r][0] + frand(-1.5f, 1.5f),
                                    aGunSpawns[r][1] + frand(-1.5f, 1.5f),
                                    aGunSpawns[r][2] + 0.5f, aGunSpawns[r][3],
                                    TEMPSPAWN_TIMED_DESPAWN, 900000))
                            {
                                if (pHand->AI())
                                {
                                    pHand->AI()->ReceiveAIEvent(AI_EVENT_CUSTOM_A, pHand,
                                        pHand, r | (uint32(c) + 1) << 2);
                                }
                            }
                        }
                    }

                    // Lorna runs up with the crew; her AI speaks the line
                    // when she crests the slope, as on retail.
                    if (Creature* pLorna = GetClosestCreatureWithEntry(m_creature,
                            NPC_LORNA_BATTLE, 150.0f))
                    {
                        if (pLorna->AI())
                        {
                            pLorna->AI()->ReceiveAIEvent(AI_EVENT_CUSTOM_A, pLorna,
                                pLorna, 0);
                        }
                    }

                    // The capture's spacing: her line lands ~9s in (the run
                    // up the slope) and her VO runs 7.25s - the prince
                    // answers 18.7s after the beat, never over her voice.
                    m_uiStepTimer = 18700;
                    break;
                }
                case 18:                                    // man the cannons, lure the beasts
                    DoScriptText(SAY_BATTLE_LIAM_LORNA, m_creature);
                    MarchTo(9,
                        -1637.70f, 1307.60f, 19.80f);
                    m_uiStepTimer = 12000;
                    break;
                case 19:                                    // the abomination brawl
                    DoScriptText(SAY_BATTLE_PREVAIL_VOICED, m_creature);
                    RanksEngage();

                    // Lorna leaves the guns and leads the lure downhill.
                    if (Creature* pLorna = GetClosestCreatureWithEntry(m_creature,
                            NPC_LORNA_BATTLE, 120.0f))
                    {
                        if (pLorna->AI())
                        {
                            pLorna->AI()->ReceiveAIEvent(AI_EVENT_CUSTOM_B, pLorna,
                                pLorna, 0);
                        }
                    }

                    m_uiStepTimer = 30000;
                    break;
                case 20:                                    // Attack! - on to the square
                    DoScriptText(SAY_BATTLE_ATTACK, m_creature);
                    RanksRehome();
                    RanksFollow();
                    MarchTo(10,
                        -1760.10f, 1341.80f, 20.00f);
                    m_uiStepTimer = 14000;
                    break;
                case 21:                                    // the Gorerot square
                    RanksEngage();

                    if (Creature* pGorerot = GetClosestCreatureWithEntry(m_creature,
                            NPC_GOREROT, 90.0f))
                    {
                        DoScriptText(SAY_BATTLE_GOREROT, pGorerot);
                    }

                    m_uiStepTimer = 11000;
                    break;
                case 22:                                    // the catapult call
                    if (Creature* pDarius = GetClosestCreatureWithEntry(m_creature,
                            NPC_DARIUS_BATTLE, 120.0f))
                    {
                        DoScriptText(SAY_BATTLE_DARIUS_CATAPULT, pDarius);
                    }
                    m_uiStepTimer = 2000;
                    break;
                case 23:                                    // the giant must fall first
                {
                    Creature* pGorerot = GetClosestCreatureWithEntry(m_creature,
                        NPC_GOREROT, 150.0f);

                    if (pGorerot && pGorerot->IsAlive() && m_uiGorerotWait < 180000)
                    {
                        m_uiGorerotWait += 2000;
                        m_uiStepTimer = 2000;               // hold the line
                        return;                     // stand his ground; foes come to him
                    }

                    // The giant is down - but the square is not: retail
                    // lingers ~70 seconds of cleanup before Darius speaks.
                    // A shortened beat of the same shape: fight on.
                    if (m_uiRegroupWait < 25000)
                    {
                        m_uiRegroupWait += 2000;
                        m_uiStepTimer = 2000;

                        return;                     // stand his ground; foes come to him
                    }

                    m_uiRegroupWait = 0;

                    // Gorerot is down: Darius calls the march to the king -
                    // and rides with it, at the prince's flank, for the rest
                    // of the fight across the city.
                    if (Creature* pDarius = GetClosestCreatureWithEntry(m_creature,
                            NPC_DARIUS_BATTLE, 120.0f))
                    {
                        DoScriptText(SAY_BATTLE_DARIUS_JOIN, pDarius);
                        pDarius->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE);
                        pDarius->SetWalk(false);
                        pDarius->GetMotionMaster()->MoveFollow(m_creature,
                            4.0f, M_PI_F * 0.85f);
                    }
                    RanksRehome();
                    RanksFollow();
                    MarchTo(11,
                        -1777.00f, 1422.00f, 20.30f);
                    m_uiStepTimer = 12000;
                    break;
                }
                case 24:                                    // north up the west street
                {
                    RanksRehome();

                    // The capture's spine: retail Liam rides the WEST street
                    // (x -1802..-1808) as one authored spline. The old
                    // waypoint (-1761,1592) sat on an isolated mesh island -
                    // the navprobe's only two partial verdicts on the route.
                    static const float aWestSpine[][3] =
                    {
                        { -1802.62f, 1411.98f, 19.94f },
                        { -1804.16f, 1423.31f, 19.77f },
                        { -1806.62f, 1446.45f, 19.09f },
                        { -1807.79f, 1470.31f, 19.34f },
                        { -1807.33f, 1497.12f, 19.77f },
                        { -1800.12f, 1539.74f, 21.77f },
                        { -1795.73f, 1569.68f, 20.57f },
                        { -1780.15f, 1606.23f, 20.59f },
                        { -1772.31f, 1616.20f, 20.59f },
                    };

                    Movement::PointsArray spine;

                    for (uint8 i = 0; i < 9; ++i)
                    {
                        spine.push_back(Movement::Vector3(aWestSpine[i][0],
                            aWestSpine[i][1], aWestSpine[i][2]));
                    }

                    m_creature->CombatStop(true);
                    m_creature->DeleteThreatList();
                    m_creature->GetMotionMaster()->Clear(false);
                    m_creature->SetWalk(false);
                    m_creature->GetMotionMaster()->MovePath(24, spine, false);
                    m_uiStepTimer = 52000;
                    break;
                }
                case 25:                                    // the final square
                    RanksRehome();
                    m_uiStepTimer = 3000;
                    break;
                case 26:                                    // the last brawl at the square
                    RanksEngage();
                    m_uiStepTimer = 8000;
                    break;
                case 27:                                    // the king enters
                {
                    if (Creature* pGenn = m_creature->SummonCreature(NPC_GENN_BATTLE,
                            -1739.14f, 1661.59f, 20.59f, 5.62f,
                            TEMPSPAWN_TIMED_DESPAWN, 900000))
                    {
                        pGenn->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PASSIVE);
                        pGenn->SetWalk(false);
                        pGenn->GetMotionMaster()->MovePoint(1,
                            -1699.16f, 1630.61f, 20.61f);
                        DoScriptText(SAY_FINALE_GENN_BLOCK, pGenn);
                    }

                    // The prince breaks off to seal the north road.
                    m_creature->CombatStop(true);
                    m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
                    MarchTo(14,
                        -1757.84f, 1676.58f, 22.16f);

                    // Darius holds at the square edge for the scene.
                    if (Creature* pDarius = GetClosestCreatureWithEntry(m_creature,
                            NPC_DARIUS_BATTLE, 120.0f))
                    {
                        pDarius->CombatStop(true);
                        pDarius->GetMotionMaster()->MoveIdle();
                    }

                    m_uiStepTimer = 6800;
                    break;
                }
                case 28:                                    // SYLVANAS!!
                {
                    Creature* pGenn = GetClosestCreatureWithEntry(m_creature,
                        NPC_GENN_BATTLE, 150.0f);

                    if (pGenn)
                    {
                        DoScriptText(SAY_FINALE_GENN_SYLVANAS, pGenn);
                    }

                    if (Creature* pSylvanas = GetClosestCreatureWithEntry(m_creature,
                            NPC_SYLVANAS_BATTLE, 200.0f))
                    {
                        if (pSylvanas->AI())
                        {
                            pSylvanas->AI()->ReceiveAIEvent(AI_EVENT_CUSTOM_A, pSylvanas,
                                pGenn ? (Unit*)pGenn : (Unit*)m_creature, 0);
                        }
                    }

                    m_uiStepTimer = 3200;
                    break;
                }
                case 29:                                    // the ranks become the firing line
                {
                    // The Forsaken remnants leave the stage...
                    const uint32 auiFade[2] = { NPC_FORSAKEN_CROSSBOWMAN,
                                                NPC_FORSAKEN_INFANTRY };

                    for (uint8 i = 0; i < 2; ++i)
                    {
                        std::list<Creature*> lFade;
                        GetCreatureListWithEntryInGrid(lFade, m_creature,
                            auiFade[i], 200.0f);

                        for (std::list<Creature*>::iterator itr = lFade.begin();
                             itr != lFade.end(); ++itr)
                        {
                            RestoreDbSpawn(*itr);
                            (*itr)->ForcedDespawn();
                        }
                    }

                    // ...and the army does NOT vanish (the old cut felt like
                    // a stage collapse): it plants at the square's edge and
                    // rakes the queen and her ghosts with rifle volleys,
                    // exactly as the capture shows all through the duel.
                    std::list<Creature*> lLine;
                    GetCreatureListWithEntryInGrid(lLine, m_creature,
                        NPC_GILNEAN_MILITIA, 200.0f);

                    for (std::list<Creature*>::iterator itr = lLine.begin();
                         itr != lLine.end(); ++itr)
                    {
                        if ((*itr)->IsAlive() && (*itr)->AI())
                        {
                            (*itr)->SetSpawn(Geometry::Vector3(
                                (*itr)->GetPositionX(), (*itr)->GetPositionY(),
                                (*itr)->GetPositionZ()), (*itr)->GetOrientation());
                            (*itr)->AI()->ReceiveAIEvent(AI_EVENT_CUSTOM_B,
                                *itr, *itr, 0);
                        }
                    }

                    m_uiStep = 0;
                    m_uiStepTimer = 0;
                    m_creature->SetRespawnDelay(15);        // back at the muster in seconds
                    m_creature->ForcedDespawn();
                    return;
                }
            }

            ++m_uiStep;
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_prince_liam_battleAI(pCreature);
    }
};

/*######
# npc_gilneas_child
######*/

// `Save the Children!` (14368). Three children hide around Duskhaven; the
// player clicks each one, which casts the child's own rescue spell
// (68596/68597/68598 via `npc_spellclick_spells`, player as caster). The
// spells are empty shells - a SCRIPT and a DUMMY effect - so nothing
// happened here, and rapid clicking only earned the GCD's "not ready yet".
//
// The shape is TrinityCore's: on the hit the child stops being clickable,
// thanks its rescuer after a beat, runs for the nearest cellar - James one
// leg, Ashley two, Cynthia three - and fades once inside. The quest credit
// is the child's own entry, given to the caster of the click. Cynthia, the
// youngest, stands there crying until someone comes for her.
enum
{
    NPC_CHILD_CYNTHIA           = 36287,
    NPC_CHILD_ASHLEY            = 36288,
    NPC_CHILD_JAMES             = 36289,

    SPELL_SAVE_JAMES            = 68596,
    SPELL_SAVE_CYNTHIA          = 68597,
    SPELL_SAVE_ASHLEY           = 68598,

    SAY_CYNTHIA_RESCUED         = -1999958,
    SAY_ASHLEY_RESCUED          = -1999959,
    SAY_JAMES_RESCUED           = -1999960,

    SAY_PLAYER_TO_CYNTHIA       = -1999961,
    SAY_PLAYER_TO_ASHLEY        = -1999962,
    SAY_PLAYER_TO_JAMES         = -1999963,

    CHILD_TALK_DELAY            = 2500,    ///< ms from click to the thank-you
    CHILD_RUN_DELAY_JAMES       = 3600,    ///< ms from the line to the run
    CHILD_RUN_DELAY_OTHERS      = 2300,
    CHILD_CRY_INTERVAL          = 1300,    ///< Cynthia, until rescued
};

static const float JAMES_ESCAPE[3]      = { -1913.021f, 2558.333f, 1.511007f };
static const float ASHLEY_ESCAPE[2][3]  =
{
    { -1923.283f, 2552.308f, 12.73581f },
    { -1920.023f, 2558.055f, 7.076692f }
};
static const float CYNTHIA_ESCAPE[3][3] =
{
    { -1969.230f, 2517.465f, 2.580818f },
    { -1947.472f, 2515.521f, 2.318746f },
    { -1926.536f, 2519.312f, 2.246772f }
};

struct npc_gilneas_child : public CreatureScript
{
    npc_gilneas_child() : CreatureScript("npc_gilneas_child") {}

    struct npc_gilneas_childAI : public ScriptedAI
    {
        npc_gilneas_childAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        ObjectGuid m_rescuerGuid;
        uint32 m_uiTalkTimer;
        uint32 m_uiRunTimer;
        uint32 m_uiCryTimer;
        uint8  m_uiLeg;

        void Reset() override
        {
            m_rescuerGuid.Clear();
            m_uiTalkTimer = 0;
            m_uiRunTimer = 0;
            m_uiLeg = 0;
            m_uiCryTimer =
                m_creature->GetEntry() == NPC_CHILD_CYNTHIA ? 1000 : 0;
        }

        void SpellHit(Unit* pCaster, const SpellEntry* pSpell) override
        {
            if (pSpell->ID != SPELL_SAVE_JAMES &&
                pSpell->ID != SPELL_SAVE_CYNTHIA &&
                pSpell->ID != SPELL_SAVE_ASHLEY)
            {
                return;
            }

            if (m_uiTalkTimer || m_uiRunTimer)                // already saved
            {
                return;
            }

            if (pCaster && pCaster->GetTypeId() == TYPEID_PLAYER)
            {
                m_rescuerGuid = pCaster->GetObjectGuid();
                ((Player*)pCaster)->KilledMonsterCredit(m_creature->GetEntry());

                // The rescuer speaks first - the capture has the player's own
                // line on the wire a tenth of a second after every click.
                switch (m_creature->GetEntry())
                {
                    case NPC_CHILD_CYNTHIA:
                        DoScriptText(SAY_PLAYER_TO_CYNTHIA, pCaster, m_creature);
                        break;
                    case NPC_CHILD_ASHLEY:
                        DoScriptText(SAY_PLAYER_TO_ASHLEY, pCaster, m_creature);
                        break;
                    case NPC_CHILD_JAMES:
                        DoScriptText(SAY_PLAYER_TO_JAMES, pCaster, m_creature);
                        break;
                }
            }

            // Saved: no second click, no more crying.
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            m_uiCryTimer = 0;
            m_uiTalkTimer = CHILD_TALK_DELAY;
        }

        void RunForTheCellar()
        {
            m_creature->SetWalk(false, true);

            switch (m_creature->GetEntry())
            {
                case NPC_CHILD_JAMES:
                    m_creature->GetMotionMaster()->MovePoint(0,
                        JAMES_ESCAPE[0], JAMES_ESCAPE[1], JAMES_ESCAPE[2]);
                    m_creature->ForcedDespawn(5200);
                    break;
                case NPC_CHILD_ASHLEY:
                    m_creature->GetMotionMaster()->MovePoint(1,
                        ASHLEY_ESCAPE[0][0], ASHLEY_ESCAPE[0][1], ASHLEY_ESCAPE[0][2]);
                    m_creature->ForcedDespawn(3800);
                    break;
                case NPC_CHILD_CYNTHIA:
                    m_creature->GetMotionMaster()->MovePoint(1,
                        CYNTHIA_ESCAPE[0][0], CYNTHIA_ESCAPE[0][1], CYNTHIA_ESCAPE[0][2]);
                    m_creature->ForcedDespawn(8500);
                    break;
            }
        }

        void MovementInform(uint32 uiType, uint32 uiPointId) override
        {
            if (uiType != POINT_MOTION_TYPE || !uiPointId)
            {
                return;
            }

            // The girls' escape routes have corners; each arrival sets the
            // next leg on the tick after (never from Finalize itself).
            ++m_uiLeg;
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiCryTimer)
            {
                if (m_uiCryTimer <= uiDiff)
                {
                    m_uiCryTimer = CHILD_CRY_INTERVAL;
                    m_creature->HandleEmote(EMOTE_ONESHOT_CRY);
                }
                else
                {
                    m_uiCryTimer -= uiDiff;
                }
            }

            if (m_uiTalkTimer)
            {
                if (m_uiTalkTimer <= uiDiff)
                {
                    m_uiTalkTimer = 0;

                    Unit* pRescuer =
                        m_creature->GetMap()->GetPlayer(m_rescuerGuid);

                    switch (m_creature->GetEntry())
                    {
                        case NPC_CHILD_CYNTHIA:
                            DoScriptText(SAY_CYNTHIA_RESCUED, m_creature, pRescuer);
                            break;
                        case NPC_CHILD_ASHLEY:
                            DoScriptText(SAY_ASHLEY_RESCUED, m_creature, pRescuer);
                            break;
                        case NPC_CHILD_JAMES:
                            DoScriptText(SAY_JAMES_RESCUED, m_creature, pRescuer);
                            break;
                    }

                    m_uiRunTimer = m_creature->GetEntry() == NPC_CHILD_JAMES
                        ? CHILD_RUN_DELAY_JAMES : CHILD_RUN_DELAY_OTHERS;
                }
                else
                {
                    m_uiTalkTimer -= uiDiff;
                }
            }

            if (m_uiRunTimer)
            {
                if (m_uiRunTimer <= uiDiff)
                {
                    m_uiRunTimer = 0;
                    RunForTheCellar();
                }
                else
                {
                    m_uiRunTimer -= uiDiff;
                }
            }

            // The multi-leg escapes: issue the next corner once the previous
            // arrival has been counted.
            if (m_uiLeg)
            {
                const uint8 uiLeg = m_uiLeg;
                m_uiLeg = 0;

                if (m_creature->GetEntry() == NPC_CHILD_ASHLEY && uiLeg == 1)
                {
                    m_creature->GetMotionMaster()->MovePoint(0,
                        ASHLEY_ESCAPE[1][0], ASHLEY_ESCAPE[1][1], ASHLEY_ESCAPE[1][2]);
                }
                else if (m_creature->GetEntry() == NPC_CHILD_CYNTHIA && uiLeg <= 2)
                {
                    m_creature->GetMotionMaster()->MovePoint(uiLeg + 1,
                        CYNTHIA_ESCAPE[uiLeg][0], CYNTHIA_ESCAPE[uiLeg][1],
                        CYNTHIA_ESCAPE[uiLeg][2]);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_gilneas_childAI(pCreature);
    }
};

/*######
# npc_horrid_abomination
######*/

// `You Can't Take 'Em Alone` (14348). The Horrid Abominations of Duskhaven
// cannot be brought down by force - lethal damage is shrugged off with a
// Restitching - and the quest's Black Gunpowder Keg is the whole point:
//
//     Toss Keg (69094, the item) lands 68555 `Keg Placed` on the target.
//     The abomination panics, stops fighting, and calls to be shot.
//     A Duskhaven Watchman (or Prince Liam) obliges with 68559 `Shoot`.
//     The keg goes up: bones and gore in a circle (42266/42267/42274),
//     the explosion proper (68560), quest credit for whoever threw the keg,
//     and what is left of the abomination folds up.
//
// The shape is TrinityCore's for the same quest; only the caller of the shot
// differs - their `Abomination Kill Me` AOE is answered by a scripted Liam,
// here the abomination itself finds the nearest gunman, which spares a second
// script for the same bang.
enum
{
    SPELL_KEG_PLACED            = 68555,
    SPELL_ABOMINATION_SHOOT     = 68559,
    SPELL_EXPLOSION_POISON      = 42266,
    SPELL_EXPLOSION_BONE_1      = 42267,
    SPELL_EXPLOSION_BONE_2      = 42274,
    SPELL_ABOMINATION_EXPLOSION = 68560,
    SPELL_RESTITCHING           = 68864,

    NPC_ABOMINATION_CREDIT      = 36233,
    NPC_DUSKHAVEN_WATCHMAN      = 36602,
    NPC_PRINCE_LIAM_DUSKHAVEN   = 36140,

    SAY_ABOMINATION_KEG_1       = -1999953,
    SAY_ABOMINATION_KEG_2       = -1999954,
    SAY_ABOMINATION_KEG_3       = -1999955,
    SAY_ABOMINATION_KEG_4       = -1999956,
    SAY_ABOMINATION_KEG_5       = -1999957,

    ABOMINATION_SHOOT_DELAY     = 2000,    ///< ms between the keg landing and the shot
    ABOMINATION_GUNMAN_RANGE    = 60,
};

struct npc_horrid_abomination : public CreatureScript
{
    npc_horrid_abomination() : CreatureScript("npc_horrid_abomination") {}

    struct npc_horrid_abominationAI : public ScriptedAI
    {
        npc_horrid_abominationAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        ObjectGuid m_throwerGuid;
        uint32 m_uiShootTimer;
        bool m_bPrimed;

        void Reset() override
        {
            m_throwerGuid.Clear();
            m_uiShootTimer = 0;
            m_bPrimed = false;
        }

        /// Lethal damage is not how this one goes: it restitches instead.
        void DamageTaken(Unit* /*pDealer*/, uint32& uiDamage) override
        {
            if (uiDamage < m_creature->GetHealth())
            {
                return;
            }

            uiDamage = 0;

            if (!m_bPrimed && !m_creature->HasAura(SPELL_RESTITCHING))
            {
                m_creature->CastSpell(m_creature, SPELL_RESTITCHING, true);
            }
        }

        void SpellHit(Unit* pCaster, const SpellEntry* pSpell) override
        {
            if (pSpell->ID == SPELL_KEG_PLACED && !m_bPrimed)
            {
                m_bPrimed = true;

                if (pCaster)
                {
                    m_throwerGuid = pCaster->GetObjectGuid();
                }

                switch (urand(0, 4))
                {
                    case 0: DoScriptText(SAY_ABOMINATION_KEG_1, m_creature); break;
                    case 1: DoScriptText(SAY_ABOMINATION_KEG_2, m_creature); break;
                    case 2: DoScriptText(SAY_ABOMINATION_KEG_3, m_creature); break;
                    case 3: DoScriptText(SAY_ABOMINATION_KEG_4, m_creature); break;
                    case 4: DoScriptText(SAY_ABOMINATION_KEG_5, m_creature); break;
                }

                // The panic: everything else stops mattering.
                m_creature->AttackStop();
                m_creature->CombatStop();
                m_creature->GetMotionMaster()->Clear(false, true);
                m_creature->GetMotionMaster()->MoveIdle();

                m_uiShootTimer = ABOMINATION_SHOOT_DELAY;
            }
            else if (pSpell->ID == SPELL_ABOMINATION_SHOOT && m_bPrimed &&
                     m_creature->IsAlive() &&
                     !m_creature->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
            {
                if (Player* pThrower = m_creature->GetMap()->GetPlayer(m_throwerGuid))
                {
                    pThrower->KilledMonsterCredit(NPC_ABOMINATION_CREDIT);
                }

                m_creature->RemoveAurasDueToSpell(SPELL_KEG_PLACED);

                // The gore circle, then the bang itself.
                for (uint8 i = 0; i < 11; ++i)
                {
                    m_creature->CastSpell(m_creature, SPELL_EXPLOSION_POISON, true);
                }

                for (uint8 i = 0; i < 6; ++i)
                {
                    m_creature->CastSpell(m_creature, SPELL_EXPLOSION_BONE_1, true);
                }

                for (uint8 i = 0; i < 4; ++i)
                {
                    m_creature->CastSpell(m_creature, SPELL_EXPLOSION_BONE_2, true);
                }

                m_creature->CastSpell(m_creature, SPELL_ABOMINATION_EXPLOSION, true);
                m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                m_creature->ForcedDespawn(5000);
            }
        }

        /// The nearest man with a rifle answers the call.
        void CallTheShot()
        {
            Creature* pGunman = GetClosestCreatureWithEntry(m_creature,
                NPC_DUSKHAVEN_WATCHMAN, float(ABOMINATION_GUNMAN_RANGE));

            if (!pGunman)
            {
                pGunman = GetClosestCreatureWithEntry(m_creature,
                    NPC_PRINCE_LIAM_DUSKHAVEN, float(ABOMINATION_GUNMAN_RANGE));
            }

            if (pGunman)
            {
                pGunman->CastSpell(m_creature, SPELL_ABOMINATION_SHOOT, true);
            }
            else
            {
                // Nobody in rifle range: the keg cooks off on its own rather
                // than leave the quest hanging.
                m_creature->CastSpell(m_creature, SPELL_ABOMINATION_SHOOT, true);
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_uiShootTimer)
            {
                if (m_uiShootTimer <= uiDiff)
                {
                    m_uiShootTimer = 0;
                    CallTheShot();
                }
                else
                {
                    m_uiShootTimer -= uiDiff;
                }

                return;                                     // primed: no more fighting
            }

            ScriptedAI::UpdateAI(uiDiff);
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_horrid_abominationAI(pCreature);
    }
};

/*######
# npc_lord_darius_crowley
######*/

// Turning in Last Stand (14222). The transformation and the completion chain
// used to ride the quest's DB CompleteScript, and the wire showed those two
// queued casts silently never happening while the same turn-in's RewSpellCast
// (the movie) always did. This hook runs in the very call stack RewardQuest
// itself casts from, so it cannot be lost:
//
//   98274 Force Worgen Altered Form - the transformation, forced; the racial
//         TOGGLE this replaced (68996) refuses to cast in combat, and the
//         cathedral battle keeps the player combat-flagged at turn-in.
//   72799 Last Stand Complete - binds the hearthstone to Duskhaven and
//         chains 72788: the teleport and the racials + Darkflight.
//
// Returning true also retires the DB CompleteScript for this quest - 68630
// (spell_area) already carries the teleport, the fade and the wake-up scene.
struct npc_lord_darius_crowley : public CreatureScript
{
    npc_lord_darius_crowley() : CreatureScript("npc_lord_darius_crowley") {}

    bool OnQuestRewarded(Player* pPlayer, Creature* /*pCreature*/, Quest const* pQuest) override
    {
        if (pQuest->GetQuestId() != 14222)
        {
            return false;
        }

        // 98274's CheckCast demands the worgen-form enable (94293) - which
        // the movie cast teaches, and RewardQuest runs this hook BEFORE the
        // movie cast. Teach it here; the movie's own learn is then a no-op.
        if (!pPlayer->HasSpell(94293))
        {
            pPlayer->learnSpell(94293, false);
        }

        pPlayer->CastSpell(pPlayer, 98274, true);
        pPlayer->CastSpell(pPlayer, 72799, true);

        return true;
    }
};

/*######
# npc_krennan_duskhaven
######*/

// The wake-up at Duskhaven, decoded from the capture. Turning in Last Stand
// leaves the player faded to black and stunned under 68630/68631, teleported
// to the stagecoach at Duskhaven, and 69123 summons this Krennan at his side.
// The scene then runs on Krennan's clock:
//
//     +6.5 s   Krennan shakes his head: he is not giving up on you.
//    +11.6 s   Lord Godfrey (36330) appears up the road.
//    +19.8 s   King Genn Greymane (36332) beside him.
//    +21.0 s   Godfrey: put this one down, it's protocol.
//    +28.7 s   Greymane: were those who stayed following protocol?
//    +38.0 s   Greymane: hand me that potion... and double the dosage.
//
// Greymane holds quest 14375; rewarding it swaps the phase to the living town
// (spell_area, 68481) and takes 68630/68631 with it, which stands the player
// up. Retail auto-offers the reward when the transformation movie ends; this
// core loses the auto-accept flag in a known loader truncation, so here the
// player clicks the king - everything else is the capture's own choreography.
//
// The personals are summoned per player, phase 1, and would stack for a
// second onlooker; retail hides each player's copy behind invisibility 9.
// Single set, ten-minute despawn - honest enough for this server.
enum
{
    NPC_LORD_GODFREY_PERSONAL   = 36330,
    NPC_GREYMANE_PERSONAL       = 36332,

    SPELL_NPC_CLOAK             = 68604,   ///< Invisibility 9 - the channel 68631 puts the WATCHER on

    SAY_KRENNAN_NOT_GIVING_UP   = -1999949,
    SAY_GODFREY_PROTOCOL        = -1999950,
    SAY_GREYMANE_PROTOCOL       = -1999951,
    SAY_GREYMANE_DOSAGE         = -1999952,

    KRENNAN_SCENE_DESPAWN       = 600000,  ///< ms; the phase swap hides them long before
    KRENNAN_QUEST_HUMANITY      = 14375,
    KRENNAN_OFFER_DELAY         = 12000,   ///< ms after the last line before the king pushes the reward
};

/// The capture's create positions, to the centimetre. These are the ENTRANCES
/// up the road - everyone then walks to the player: Krennan to a yard off his
/// side, the king to under three yards in front (his say positions in the
/// capture), Godfrey at the king's shoulder.
static const float GODFREY_SPAWN[4]  = { -1843.92f, 2289.66f, 42.49f, 0.240f };
static const float GREYMANE_SPAWN[4] = { -1845.61f, 2288.92f, 42.48f, 0.149f };

static const float KRENNAN_MARK[3]   = { -1819.53f, 2291.25f, 42.33f };
static const float GODFREY_MARK[3]   = { -1822.90f, 2291.10f, 42.23f };
static const float GREYMANE_MARK[3]  = { -1821.09f, 2292.60f, 42.23f };

struct npc_krennan_duskhaven : public CreatureScript
{
    npc_krennan_duskhaven() : CreatureScript("npc_krennan_duskhaven") {}

    struct npc_krennan_duskhavenAI : public ScriptedAI
    {
        npc_krennan_duskhavenAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiStep;
        uint32 m_uiStepTimer;
        uint32 m_uiWalkTimer;
        ObjectGuid m_godfreyGuid;
        ObjectGuid m_greymaneGuid;

        void Reset() override
        {
            m_uiStep = 0;
            m_uiStepTimer = 6500;
            m_uiWalkTimer = 1500;
            m_godfreyGuid.Clear();
            m_greymaneGuid.Clear();

            // The whole scene lives on invisibility channel 9. The player
            // watches from under 68631, and this core's visibility rule makes
            // an invisible viewer BLIND to every unit that does not share his
            // channel - so without the cloak the audience heard the lines and
            // saw nobody. With it, the scene is also private, as on retail:
            // the "Personal" summons are cloaked there too.
            if (m_creature->IsTemporarySummon())
            {
                m_creature->CastSpell(m_creature, SPELL_NPC_CLOAK, true);
            }
        }

        /// The scene ignores the war it was carried out of.
        void AttackStart(Unit* /*pWho*/) override {}
        void MoveInLineOfSight(Unit* /*pWho*/) override {}

        /// Appears at the road entrance, takes the cloak, and walks to his
        /// mark by the player.
        Creature* Summon(uint32 uiEntry, const float* pWhere, const float* pMark)
        {
            Creature* pWho = m_creature->SummonCreature(uiEntry, pWhere[0],
                pWhere[1], pWhere[2], pWhere[3], TEMPSPAWN_TIMED_DESPAWN,
                KRENNAN_SCENE_DESPAWN);

            if (pWho)
            {
                pWho->CastSpell(pWho, SPELL_NPC_CLOAK, true);
                pWho->SetWalk(true, true);
                pWho->GetMotionMaster()->MovePoint(1, pMark[0], pMark[1], pMark[2]);
            }

            return pWho;
        }

        /// A speaker turns to the man on the ground before his line.
        void FacePlayer(Creature* pWho)
        {
            if (!pWho || !m_creature->IsTemporarySummon())
            {
                return;
            }

            TemporarySummon* pSummon = (TemporarySummon*)m_creature;

            if (Player* pPlayer = m_creature->GetMap()->GetPlayer(pSummon->GetSummonerGuid()))
            {
                pWho->SetFacingToObject(pPlayer);
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // The scene runs only on the summoned copy; a stray world spawn
            // of this entry stays a plain bystander.
            if (!m_creature->IsTemporarySummon() || m_uiStep > 6)
            {
                return;
            }

            // The walk to the player's side, a beat after appearing.
            if (m_uiWalkTimer)
            {
                if (m_uiWalkTimer <= uiDiff)
                {
                    m_uiWalkTimer = 0;
                    m_creature->SetWalk(true, true);
                    m_creature->GetMotionMaster()->MovePoint(1, KRENNAN_MARK[0],
                        KRENNAN_MARK[1], KRENNAN_MARK[2]);
                }
                else
                {
                    m_uiWalkTimer -= uiDiff;
                }
            }

            if (m_uiStepTimer > uiDiff)
            {
                m_uiStepTimer -= uiDiff;
                return;
            }

            switch (m_uiStep)
            {
                case 0:
                    FacePlayer(m_creature);
                    DoScriptText(SAY_KRENNAN_NOT_GIVING_UP, m_creature);
                    m_uiStepTimer = 5100;                   // +11.6 s
                    break;
                case 1:
                    if (Creature* pGodfrey = Summon(NPC_LORD_GODFREY_PERSONAL,
                            GODFREY_SPAWN, GODFREY_MARK))
                    {
                        m_godfreyGuid = pGodfrey->GetObjectGuid();
                    }
                    m_uiStepTimer = 8200;                   // +19.8 s
                    break;
                case 2:
                    if (Creature* pGreymane = Summon(NPC_GREYMANE_PERSONAL,
                            GREYMANE_SPAWN, GREYMANE_MARK))
                    {
                        m_greymaneGuid = pGreymane->GetObjectGuid();
                    }
                    m_uiStepTimer = 1200;                   // +21.0 s
                    break;
                case 3:
                    if (Creature* pGodfrey = m_creature->GetMap()->GetCreature(m_godfreyGuid))
                    {
                        FacePlayer(pGodfrey);
                        DoScriptText(SAY_GODFREY_PROTOCOL, pGodfrey);
                    }
                    m_uiStepTimer = 7700;                   // +28.7 s
                    break;
                case 4:
                    if (Creature* pGreymane = m_creature->GetMap()->GetCreature(m_greymaneGuid))
                    {
                        FacePlayer(pGreymane);
                        DoScriptText(SAY_GREYMANE_PROTOCOL, pGreymane);
                    }
                    m_uiStepTimer = 9300;                   // +38.0 s
                    break;
                case 5:
                    if (Creature* pGreymane = m_creature->GetMap()->GetCreature(m_greymaneGuid))
                    {
                        FacePlayer(pGreymane);
                        DoScriptText(SAY_GREYMANE_DOSAGE, pGreymane);
                    }
                    m_uiStepTimer = KRENNAN_OFFER_DELAY;
                    break;
                case 6:
                {
                    // Retail never asks the player to click anyone - he is
                    // stunned, and the client will not let him. The capture
                    // shows the king's reward window server-PUSHED after the
                    // scene; the quest itself was accepted invisibly. Do both:
                    // hand the quest over, complete it (it has no objectives),
                    // and push the offer - the popup works through the stun,
                    // and rewarding it is what lifts 68630/68631 and phases
                    // the living town in.
                    TemporarySummon* pSummon = (TemporarySummon*)m_creature;
                    Player* pPlayer =
                        m_creature->GetMap()->GetPlayer(pSummon->GetSummonerGuid());
                    Creature* pGreymane =
                        m_creature->GetMap()->GetCreature(m_greymaneGuid);

                    if (!pPlayer || !pGreymane)
                    {
                        break;
                    }

                    Quest const* pQuest =
                        sObjectMgr.GetQuestTemplate(KRENNAN_QUEST_HUMANITY);

                    if (!pQuest)
                    {
                        break;
                    }

                    if (pPlayer->GetQuestStatus(KRENNAN_QUEST_HUMANITY) == QUEST_STATUS_NONE &&
                        pPlayer->CanTakeQuest(pQuest, false))
                    {
                        pPlayer->AddQuest(pQuest, pGreymane);
                    }

                    if (pPlayer->CanCompleteQuest(KRENNAN_QUEST_HUMANITY))
                    {
                        pPlayer->CompleteQuest(KRENNAN_QUEST_HUMANITY);
                    }

                    if (pPlayer->GetQuestStatus(KRENNAN_QUEST_HUMANITY) == QUEST_STATUS_COMPLETE &&
                        !pPlayer->GetQuestRewardStatus(KRENNAN_QUEST_HUMANITY))
                    {
                        pPlayer->PlayerTalkClass->SendQuestGiverOfferReward(
                            pQuest, pGreymane->GetObjectGuid(), true);
                    }
                    break;
                }
            }

            ++m_uiStep;
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_krennan_duskhavenAI(pCreature);
    }
};

/*######
# npc_prince_liam_duskhaven
######*/

// The town defense at Duskhaven. The capture has Liam (36140) holding one
// spot through the whole session - twelve movement splines in ninety
// minutes - throwing bottles at the Forsaken pressing the hill: 68552
// `Throw Bottle` one hundred thirty times, 68559 `Shoot` eight, and over
// five hundred melee swings for whoever reached him. Without a script this
// core's default AI chased instead, and the prince brawled downhill
// bare-handed with the gun still on his back.
enum
{
    SPELL_LIAM_THROW_BOTTLE   = 68552,
    SPELL_LIAM_SHOOT          = 68559,

    NPC_FORSAKEN_INVADER      = 34511,

    LIAM_BOTTLE_MIN           = 3000,      ///< ms between bottles at a ranged victim
    LIAM_BOTTLE_MAX           = 5000,
    LIAM_SHOOT_MIN            = 8000,      ///< ms; the rifle works alongside the bottles
    LIAM_SHOOT_MAX            = 14000,

    MARKSMAN_SHOT_MIN         = 2500,      ///< ms between rifle rounds from the line
    MARKSMAN_SHOT_MAX         = 4000,
};

/// Range the hilltop guns cover.
static const float LIAM_COVER_RANGE     = 40.0f;
static const float MARKSMAN_COVER_RANGE = 35.0f;

/// A live invader within fRange, chosen at random so the hilltop's fire
/// spreads across all three brawls instead of ganging on the nearest.
/// Tester's call: the shooters fire INTO the staged fights, Liam included;
/// a forsaken that breaks off to charge a shooter is part of the show.
static Creature* FindChargingInvader(Creature* pShooter, float fRange)
{
    std::list<Creature*> lInvaders;
    GetCreatureListWithEntryInGrid(lInvaders, pShooter, NPC_FORSAKEN_INVADER, fRange);

    std::vector<Creature*> vAlive;

    for (std::list<Creature*>::iterator itr = lInvaders.begin(); itr != lInvaders.end(); ++itr)
    {
        if ((*itr)->IsAlive())
        {
            vAlive.push_back(*itr);
        }
    }

    if (vAlive.empty())
    {
        return NULL;
    }

    return vAlive[urand(0, vAlive.size() - 1)];
}

struct npc_prince_liam_duskhaven : public CreatureScript
{
    npc_prince_liam_duskhaven() : CreatureScript("npc_prince_liam_duskhaven") {}

    struct npc_prince_liam_duskhavenAI : public ScriptedAI
    {
        npc_prince_liam_duskhavenAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            SetCombatMovement(false);
        }

        uint32 m_uiBottleTimer;
        uint32 m_uiShootTimer;

        void Reset() override
        {
            m_uiBottleTimer = urand(LIAM_BOTTLE_MIN, LIAM_BOTTLE_MAX);
            m_uiShootTimer = urand(LIAM_SHOOT_MIN, LIAM_SHOOT_MAX);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // His own attacker comes first; with no one on him he covers
            // the field instead, picking off invaders still charging the
            // hill. Never the ones locked with a watchman - his threat
            // would tear the staged fights apart.
            Unit* pMark = NULL;

            if (m_creature->SelectHostileTarget())
            {
                pMark = m_creature->getVictim();
            }

            if (!pMark)
            {
                pMark = FindChargingInvader(m_creature, LIAM_COVER_RANGE);
            }

            if (!pMark)
            {
                return;
            }

            // The bottle covers what the ground line cannot: anyone he
            // cannot reach from his post gets one lobbed downhill.
            if (m_uiBottleTimer <= uiDiff)
            {
                if (!InMeleeReach(*m_creature, *pMark) &&
                    DoCastSpellIfCan(pMark, SPELL_LIAM_THROW_BOTTLE, CAST_TRIGGERED) == CAST_OK)
                {
                    m_uiBottleTimer = urand(LIAM_BOTTLE_MIN, LIAM_BOTTLE_MAX);
                }
            }
            else
            {
                m_uiBottleTimer -= uiDiff;
            }

            if (m_uiShootTimer <= uiDiff)
            {
                if (!InMeleeReach(*m_creature, *pMark) &&
                    DoCastSpellIfCan(pMark, SPELL_LIAM_SHOOT, CAST_TRIGGERED) == CAST_OK)
                {
                    m_uiShootTimer = urand(LIAM_SHOOT_MIN, LIAM_SHOOT_MAX);
                }
            }
            else
            {
                m_uiShootTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_prince_liam_duskhavenAI(pCreature);
    }
};

/*######
# npc_duskhaven_hill_brawler
######*/

// Pin for the three staged fights on the hill, bound to both sides
// (36211 watchman and 34511 invader). NPC-vs-NPC aggro reaches some
// twenty yards on this core and a wounded fighter calls every ally in
// earshot - with group centers nine yards apart, every brawl locked
// across groups and the whole field collapsed into one pile. The gate
// lets a creature self-initiate only at arm's length, so each knot
// fights its own; being attacked - a rifle round from the line, a
// player - is answered as ever, and players are aggroed at the normal
// radius so the quest plays untouched.
static const float BRAWLER_NPC_AGGRO_RANGE = 3.5f;

struct npc_duskhaven_hill_brawler : public CreatureScript
{
    npc_duskhaven_hill_brawler() : CreatureScript("npc_duskhaven_hill_brawler") {}

    struct npc_duskhaven_hill_brawlerAI : public ScriptedAI
    {
        npc_duskhaven_hill_brawlerAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        void Reset() override
        {
            m_creature->SetNoCallAssistance(true);
        }

        void Aggro(Unit* /*pWho*/) override
        {
            m_creature->SetNoCallAssistance(true);
        }

        void MoveInLineOfSight(Unit* pWho) override
        {
            if (pWho->GetTypeId() != TYPEID_PLAYER &&
                !m_creature->IsWithinDistInMap(pWho, BRAWLER_NPC_AGGRO_RANGE))
            {
                return;
            }

            ScriptedAI::MoveInLineOfSight(pWho);
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_duskhaven_hill_brawlerAI(pCreature);
    }
};

/*######
# npc_duskhaven_watchman_marksman
######*/

// The back line of the hill defense (36653, the same gun every watchman
// carries): planted at their posts, working the rifle at the Forsaken
// still coming, the bayonet for whatever closes in.
struct npc_duskhaven_watchman_marksman : public CreatureScript
{
    npc_duskhaven_watchman_marksman() : CreatureScript("npc_duskhaven_watchman_marksman") {}

    struct npc_duskhaven_watchman_marksmanAI : public ScriptedAI
    {
        npc_duskhaven_watchman_marksmanAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            SetCombatMovement(false);
        }

        uint32 m_uiShotTimer;

        void Reset() override
        {
            m_uiShotTimer = urand(MARKSMAN_SHOT_MIN, MARKSMAN_SHOT_MAX);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            Unit* pMark = NULL;

            if (m_creature->SelectHostileTarget())
            {
                pMark = m_creature->getVictim();
            }

            if (!pMark)
            {
                pMark = FindChargingInvader(m_creature, MARKSMAN_COVER_RANGE);
            }

            if (!pMark)
            {
                return;
            }

            if (m_uiShotTimer <= uiDiff)
            {
                if (!InMeleeReach(*m_creature, *pMark) &&
                    DoCastSpellIfCan(pMark, SPELL_LIAM_SHOOT, CAST_TRIGGERED) == CAST_OK)
                {
                    m_uiShotTimer = urand(MARKSMAN_SHOT_MIN, MARKSMAN_SHOT_MAX);
                }
            }
            else
            {
                m_uiShotTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_duskhaven_watchman_marksmanAI(pCreature);
    }
};

/*######
# npc_koroth_crash_site
######*/

// The ettin-versus-Forsaken theatre at the stagecoach crash site. The
// capture opens it on the 24472 turn-in: Koroth (37808) holds the choke
// and the marching column breaks against him - thirty-eight soldier
// swings on the ettin, an eighteen-for-nineteen exchange with the
// catapult, Captain Asther wading in, Cleave and Hillbreaker AoE coming
// back. None of that can self-ignite from data: the column carries
// UNIT_FLAG_OOC_NOT_ATTACKABLE (the ettin cannot open on them) plus
// no-aggro-on-sight (they will not open on him), and the capture shows
// the same flags WITH the column already combat-flagged at first
// sighting - retail staged the fight server-side. So does this script:
// the ettin drags the nearest marcher into the scrum, shedding the OOC
// flag exactly as its own definition says happens on combat entry, and
// passers-by are pulled in while the brawl runs.
enum
{
    NPC_FORSAKEN_SOLDIER_MARCH  = 37805,
    NPC_CAPTAIN_ASTHER          = 37806,
    NPC_FORSAKEN_CATAPULT_MARCH = 37807,

    SPELL_KOROTH_CLEAVE         = 16044,
    SPELL_KOROTH_HILLBREAKER    = 86112,
    SPELL_KOROTH_DEMO_SHOUT     = 16244,

    KOROTH_ENGAGE_RANGE         = 35,      ///< yd; how far he reaches for the column when idle
    KOROTH_JOIN_RANGE           = 15,      ///< yd; marchers this close are pulled into a running brawl
    KOROTH_MAX_ATTACKERS        = 15,      ///< effectively the whole passing column

    KOROTH_SCAN_INTERVAL        = 2000,    ///< ms between idle scans
    KOROTH_JOIN_INTERVAL        = 3000,    ///< ms between pulling another passer-by in

    NPC_KOROTH_CRASH            = 37808,
    SPELL_CATAPULT_BOULDER      = 72768,   ///< Throw Boulder - the capture's own in-zone hurl

    CATAPULT_FIRE_RANGE         = 55,      ///< yd; the catapult parks and bombards from here
    CATAPULT_SHOT_MIN           = 5000,    ///< ms between boulders
    CATAPULT_SHOT_MAX           = 8000,    ///< ms
    KOROTH_CLEAVE_MIN           = 6000,
    KOROTH_CLEAVE_MAX           = 9000,
    KOROTH_AOE_MIN              = 15000,
    KOROTH_AOE_MAX              = 25000,
    KOROTH_SHOUT_COOLDOWN       = 45000,
};

struct npc_koroth_crash_site : public CreatureScript
{
    npc_koroth_crash_site() : CreatureScript("npc_koroth_crash_site") {}

    struct npc_koroth_crash_siteAI : public ScriptedAI
    {
        npc_koroth_crash_siteAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiScanTimer;
        uint32 m_uiJoinTimer;
        uint32 m_uiCleaveTimer;
        uint32 m_uiAoeTimer;
        uint32 m_uiShoutTimer;

        void Reset() override
        {
            m_uiScanTimer = KOROTH_SCAN_INTERVAL;
            m_uiJoinTimer = KOROTH_JOIN_INTERVAL;
            m_uiCleaveTimer = urand(KOROTH_CLEAVE_MIN, KOROTH_CLEAVE_MAX);
            m_uiAoeTimer = urand(KOROTH_AOE_MIN, KOROTH_AOE_MAX);
            m_uiShoutTimer = 5000;
            m_creature->SetNoCallAssistance(true);
        }

        /// The sparring floor: the column's blows land at zero, so the
        /// brawl is pure theatre and never produces a corpse. A player's
        /// hits stay real.
        void DamageTaken(Unit* pDealer, uint32& uiDamage) override
        {
            if (!pDealer || pDealer->GetTypeId() != TYPEID_PLAYER)
            {
                uiDamage = 0;
            }
        }

        /// Engages EVERY idle column member within fRange at once - a
        /// one-at-a-time pull left the back of the queue strolling through
        /// the brawl untouched. The catapult is not pulled: it parks itself
        /// at range and bombards - see npc_crash_site_marcher.
        void PullAll(float fRange)
        {
            const uint32 auiEntry[2] = { NPC_FORSAKEN_SOLDIER_MARCH,
                                         NPC_CAPTAIN_ASTHER };

            for (uint8 i = 0; i < 2; ++i)
            {
                std::list<Creature*> lFound;
                GetCreatureListWithEntryInGrid(lFound, m_creature, auiEntry[i], fRange);

                for (std::list<Creature*>::iterator itr = lFound.begin(); itr != lFound.end(); ++itr)
                {
                    if (!(*itr)->IsAlive() || (*itr)->getVictim())
                    {
                        continue;
                    }

                    Engage(*itr);
                }
            }
        }

        /// Sheds the marcher's out-of-combat shield - the flag's own
        /// definition has it removed on combat entry - and locks the pair.
        void Engage(Creature* pMarcher)
        {
            pMarcher->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
            pMarcher->SetNoCallAssistance(true);

            if (pMarcher->AI())
            {
                pMarcher->AI()->AttackStart(m_creature);
            }

            if (!m_creature->getVictim())
            {
                AttackStart(pMarcher);
            }
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->getVictim())
            {
                if (m_uiScanTimer <= uiDiff)
                {
                    m_uiScanTimer = KOROTH_SCAN_INTERVAL;
                    PullAll(float(KOROTH_ENGAGE_RANGE));
                }
                else
                {
                    m_uiScanTimer -= uiDiff;
                }

                return;
            }

            // A running brawl swallows the column that marches past it -
            // everyone in reach, every tick, no queue and no cap.
            if (m_uiJoinTimer <= uiDiff)
            {
                m_uiJoinTimer = KOROTH_JOIN_INTERVAL;
                PullAll(float(KOROTH_ENGAGE_RANGE));
            }
            else
            {
                m_uiJoinTimer -= uiDiff;
            }

            if (m_uiCleaveTimer <= uiDiff)
            {
                if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_KOROTH_CLEAVE) == CAST_OK)
                {
                    m_uiCleaveTimer = urand(KOROTH_CLEAVE_MIN, KOROTH_CLEAVE_MAX);
                }
            }
            else
            {
                m_uiCleaveTimer -= uiDiff;
            }

            if (m_uiAoeTimer <= uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_KOROTH_HILLBREAKER) == CAST_OK)
                {
                    m_uiAoeTimer = urand(KOROTH_AOE_MIN, KOROTH_AOE_MAX);
                }
            }
            else
            {
                m_uiAoeTimer -= uiDiff;
            }

            if (m_uiShoutTimer <= uiDiff)
            {
                if (DoCastSpellIfCan(m_creature, SPELL_KOROTH_DEMO_SHOUT) == CAST_OK)
                {
                    m_uiShoutTimer = KOROTH_SHOUT_COOLDOWN;
                }
            }
            else
            {
                m_uiShoutTimer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_koroth_crash_siteAI(pCreature);
    }
};

/*######
# npc_crash_site_marcher
######*/

// The column's own half of the theatre (37805/37806/37807). Whatever AI
// these marchers got by default never answered the ettin - he beat on
// them unopposed - so they carry their own: pulled into the scrum they
// fight back, the ettin's blows land at zero on them as theirs do on
// him, and no one ever falls. They never open a fight themselves - the
// march is the job, the ettin does the pulling - and players remain
// fair game both ways.
struct npc_crash_site_marcher : public CreatureScript
{
    npc_crash_site_marcher() : CreatureScript("npc_crash_site_marcher") {}

    struct npc_crash_site_marcherAI : public ScriptedAI
    {
        npc_crash_site_marcherAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_bIsCatapult = pCreature->GetEntry() == NPC_FORSAKEN_CATAPULT_MARCH;

            if (m_bIsCatapult)
            {
                SetCombatMovement(false);
            }
        }

        bool m_bIsCatapult;
        bool m_bDeployed;
        uint32 m_uiShotTimer;

        void Reset() override
        {
            m_bDeployed = false;
            m_uiShotTimer = 2000;
            m_creature->SetNoCallAssistance(true);
        }

        /// The march ignores everything it passes; combat only ever comes to it.
        void MoveInLineOfSight(Unit* /*pWho*/) override {}

        void DamageTaken(Unit* pDealer, uint32& uiDamage) override
        {
            if (!pDealer || pDealer->GetTypeId() != TYPEID_PLAYER)
            {
                uiDamage = 0;
            }
        }

        Creature* FindKoroth()
        {
            std::list<Creature*> lFound;
            GetCreatureListWithEntryInGrid(lFound, m_creature, NPC_KOROTH_CRASH,
                                           float(CATAPULT_FIRE_RANGE));

            for (std::list<Creature*>::iterator itr = lFound.begin(); itr != lFound.end(); ++itr)
            {
                if ((*itr)->IsAlive())
                {
                    return *itr;
                }
            }

            return NULL;
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // The catapult is artillery, not a ram: the moment the ettin is
            // in range it parks, swings its arm around, and bombards - the
            // sparring floor keeps the boulders theatrical.
            if (m_bIsCatapult)
            {
                if (m_uiShotTimer <= uiDiff)
                {
                    m_uiShotTimer = urand(CATAPULT_SHOT_MIN, CATAPULT_SHOT_MAX);

                    if (Creature* pKoroth = FindKoroth())
                    {
                        if (!m_bDeployed)
                        {
                            m_bDeployed = true;
                            m_creature->StopMoving();
                            m_creature->GetMotionMaster()->MoveIdle();
                        }

                        m_creature->SetFacingToObject(pKoroth);
                        m_creature->CastSpell(pKoroth, SPELL_CATAPULT_BOULDER, true);
                    }
                }
                else
                {
                    m_uiShotTimer -= uiDiff;
                }

                return;
            }

            ScriptedAI::UpdateAI(uiDiff);
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_crash_site_marcherAI(pCreature);
    }
};

/*######
# npc_gilnean_royal_guard
######*/

// The barricade line in the Military District (Safety in Numbers era).
// The capture stages five guard-versus-worgen pairs, each three to four
// yards apart, both sides target-locked and fighting AT their posts -
// the guards never move a step, and dead worgen respawn into the same
// duel. Our spawns sit on the capture spots to the decimal, but two
// statics can never notify each other into combat on this core, so the
// line stood at parade rest. The guard stages his own duel: any worgen
// at arm's length is engaged both ways, in place, with real steel -
// the capture's respawn cycling shows these fights are lossy, not
// sparring.
enum
{
    NPC_BLOODFANG_WORGEN_CITY   = 35118,
    NPC_RAMPAGING_WORGEN_CITY   = 34884,

    GUARD_DUEL_RANGE            = 6,       ///< yd; the paired worgen stands 3-4 away
    GUARD_SCAN_INTERVAL         = 3000,    ///< ms between idle scans
};

struct npc_gilnean_royal_guard : public CreatureScript
{
    npc_gilnean_royal_guard() : CreatureScript("npc_gilnean_royal_guard") {}

    struct npc_gilnean_royal_guardAI : public ScriptedAI
    {
        npc_gilnean_royal_guardAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiScanTimer;

        void Reset() override
        {
            m_uiScanTimer = GUARD_SCAN_INTERVAL;
            m_creature->SetNoCallAssistance(true);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (!m_creature->getVictim())
            {
                if (m_uiScanTimer <= uiDiff)
                {
                    m_uiScanTimer = GUARD_SCAN_INTERVAL;

                    const uint32 auiEntry[2] = { NPC_BLOODFANG_WORGEN_CITY,
                                                 NPC_RAMPAGING_WORGEN_CITY };

                    for (uint8 i = 0; i < 2 && !m_creature->getVictim(); ++i)
                    {
                        std::list<Creature*> lWorgen;
                        GetCreatureListWithEntryInGrid(lWorgen, m_creature,
                                                       auiEntry[i], float(GUARD_DUEL_RANGE));

                        for (std::list<Creature*>::iterator itr = lWorgen.begin(); itr != lWorgen.end(); ++itr)
                        {
                            if (!(*itr)->IsAlive() || (*itr)->getVictim())
                            {
                                continue;
                            }

                            if ((*itr)->AI())
                            {
                                (*itr)->AI()->AttackStart(m_creature);
                            }

                            AttackStart(*itr);
                            break;
                        }
                    }
                }
                else
                {
                    m_uiScanTimer -= uiDiff;
                }

                return;
            }

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_gilnean_royal_guardAI(pCreature);
    }
};

/**
 * Registers Gilneas City script content.
 */
void AddSC_gilneas_city()
{
    Script* s;
    s = new npc_prince_liam_greymane_phase1();
    s->RegisterSelf();
    s = new npc_prince_liam_greymane_phase2();
    s->RegisterSelf();
    s = new rampaging_worgen();
    s->RegisterSelf();
    s = new frightened_citizen_quest();
    s->RegisterSelf();
    s = new go_merchant_square_door();
    s->RegisterSelf();
    s = new npc_gilnean_crow();
    s->RegisterSelf();
    s = new npc_prison_roof_worgen();
    s->RegisterSelf();
    s = new npc_lord_darius_prison();
    s->RegisterSelf();
    s = new npc_josiah_worgen();
    s->RegisterSelf();
    s = new spell_josiah_get_shot();
    s->RegisterSelf();
    s = new npc_lorna_crowley();
    s->RegisterSelf();
    s = new npc_bloodfang_lurker();
    s->RegisterSelf();
    s = new npc_greymanes_horse();
    s->RegisterSelf();
    s = new npc_bloodfang_ripper_cannon();
    s->RegisterSelf();
    s = new npc_crowleys_horse();
    s->RegisterSelf();
    s = new npc_bloodfang_stalker();
    s->RegisterSelf();
    s = new npc_frenzied_stalker();
    s->RegisterSelf();
    s = new npc_northgate_rebel();
    s->RegisterSelf();
    s = new npc_krennan_duskhaven();
    s->RegisterSelf();
    s = new npc_prince_liam_duskhaven();
    s->RegisterSelf();
    s = new npc_duskhaven_watchman_marksman();
    s->RegisterSelf();
    s = new npc_duskhaven_hill_brawler();
    s->RegisterSelf();
    s = new npc_koroth_crash_site();
    s->RegisterSelf();
    s = new npc_crash_site_marcher();
    s->RegisterSelf();
    s = new npc_gilnean_royal_guard();
    s->RegisterSelf();
    s = new npc_lord_darius_crowley();
    s->RegisterSelf();
    s = new npc_horrid_abomination();
    s->RegisterSelf();
    s = new npc_gilneas_child();
    s->RegisterSelf();
    s = new npc_forsaken_catapult();
    s->RegisterSelf();
    s = new spell_call_attack_mastiffs();
    s->RegisterSelf();
    s = new npc_dark_ranger_thyala();
    s->RegisterSelf();
    s = new npc_lord_godfrey_duskhaven();
    s->RegisterSelf();
    s = new npc_drowning_watchman();
    s->RegisterSelf();
    s = new npc_mountain_horse();
    s->RegisterSelf();
    s = new npc_chance_the_cat();
    s->RegisterSelf();
    s = new npc_gwen_armstead();
    s->RegisterSelf();
    s = new npc_swift_mountain_horse();
    s->RegisterSelf();
    s = new npc_king_greymane_observatory();
    s->RegisterSelf();
    s = new npc_exodus_coach();
    s->RegisterSelf();
    s = new npc_exodus_harness();
    s->RegisterSelf();
    s = new npc_dark_scout();
    s->RegisterSelf();
    s = new at_dark_scout_ambush();
    s->RegisterSelf();
    s = new spell_belysra_talisman();
    s->RegisterSelf();
    s = new event_horn_of_taldoren();
    s->RegisterSelf();
    s = new go_ritual_well();
    s->RegisterSelf();
    s = new npc_darius_taldoren();
    s->RegisterSelf();
    s = new npc_godfrey_flank();
    s->RegisterSelf();
    s = new npc_flank_questgiver();
    s->RegisterSelf();
    s = new npc_lord_hewell();
    s->RegisterSelf();
    s = new npc_stout_horse_flank();
    s->RegisterSelf();
    s = new npc_enslaved_villager();
    s->RegisterSelf();
    s = new go_ball_and_chain();
    s->RegisterSelf();
    s = new npc_prince_liam_battle();
    s->RegisterSelf();
    s = new npc_gilnean_militia();
    s->RegisterSelf();
    s = new npc_emberstone_cannon();
    s->RegisterSelf();
    s = new npc_cannon_crew();
    s->RegisterSelf();
    s = new npc_lorna_battle();
    s->RegisterSelf();
    s = new npc_battle_worgen_warrior();
    s->RegisterSelf();
    s = new npc_soultethered_banshee();
    s->RegisterSelf();
    s = new npc_sylvanas_battle();
    s->RegisterSelf();
    s = new npc_liam_dying();
    s->RegisterSelf();
    s = new npc_damaged_catapult();
    s->RegisterSelf();
    s = new npc_rebel_cannon();
    s->RegisterSelf();
}
#else

/*######
# npc_prince_liam_greymane_phase1
######*/

enum {
    SAY_STORYLINE1        = -1654001,
    SAY_STORYLINE2        = -1654002,
    SAY_STORYLINE3        = -1654003
};

struct npc_prince_liam_greymane_phase1 : public CreatureScript
{
    npc_prince_liam_greymane_phase1() : CreatureScript("npc_prince_liam_greymane_phase1") {}

    struct npc_prince_liam_greymane_phase1AI : public ScriptedAI
    {
        npc_prince_liam_greymane_phase1AI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_uiNpcFlags = pCreature->GetUInt32Value(UNIT_NPC_FLAGS);
            Reset();
        }

        ObjectGuid m_casterGuid;
        uint32 m_uiNpcFlags;
        uint32 m_uiSayStoryDelay;
        uint32 m_uiSayStoryTimer;
        int m_uiSayStoryLast;

        bool m_bCanSayStory;

        void Reset() override
        {
            m_casterGuid.Clear();

            m_uiSayStoryDelay = 15000;
            m_uiSayStoryTimer = m_uiSayStoryDelay;
            m_uiSayStoryLast = 0;

            m_bCanSayStory = true;

            m_creature->SetUInt32Value(UNIT_NPC_FLAGS, m_uiNpcFlags);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            if (m_bCanSayStory)
            {
                m_uiSayStoryTimer = m_uiSayStoryDelay;

                // Random switch between 3 texts
                switch (m_uiSayStoryLast)
                {
                    case 0: DoScriptText(SAY_STORYLINE1, m_creature); m_uiSayStoryLast++; break;
                    case 1: DoScriptText(SAY_STORYLINE2, m_creature); m_uiSayStoryLast++; break;
                    case 2: DoScriptText(SAY_STORYLINE3, m_creature); m_uiSayStoryLast = 0; m_uiSayStoryTimer = 60000; break;
                    //case 3: DoScriptText(SAY_HELP4, m_creature); break;
                }

                m_bCanSayStory = false;
            }

            if (m_uiSayStoryTimer < uiDiff)
            {
                m_bCanSayStory = true;
                m_uiSayStoryTimer = m_uiSayStoryDelay;
            }
            else
            {
                m_uiSayStoryTimer -= uiDiff;
            }
        }

    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_prince_liam_greymane_phase1AI(pCreature);
    }
};

/*######
# npc_prince_liam_greymane_phase2
######*/

enum {
    SAY_STORYLINE4        = -1654004,
    SAY_STORYLINE5        = -1654005,
    SAY_STORYLINE6        = -1654006,
    SAY_STORYLINE7        = -1654007,
    SAY_STORYLINE8        = -1654008,

    SAY_STORY_DELAY        = 30000,
    SPELL_SHOOT            = 50092,
    SPELL_CNOCKING        = 67869,

    QUEST_EVAQUATE_THE_MERCHANT_SQUARE = 14098
};

struct npc_prince_liam_greymane_phase2 : public CreatureScript
{
    npc_prince_liam_greymane_phase2() : CreatureScript("npc_prince_liam_greymane_phase2") {}

    struct npc_prince_liam_greymane_phase2AI : public ScriptedAI
    {
        npc_prince_liam_greymane_phase2AI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            Reset();
        }

        uint32 m_uiNpcFlags;
        uint32 m_uiSayStoryTimer;

        Unit* lastVictim;

        int m_uiSayStoryLast;

        bool m_bCanSayStory;
        bool IsSelfRooted;

        void Reset() override
        {
            m_uiSayStoryTimer = SAY_STORY_DELAY;
            m_uiSayStoryLast = 0;
            lastVictim = nullptr;

            m_bCanSayStory = true;
            IsSelfRooted = false;

        }

        void Aggro(Unit* who) override
        {
            if (!InMeleeReach(*m_creature, *who) && !m_creature->CanUseEquippedWeapon(RANGED_ATTACK))
            {
                IsSelfRooted = true;
            }
        }

        void AttackedBy(Unit* pAttacker) override
        {
            // Check if Liam is attacking who attack he and don't jump on multiple attackers
            if (m_creature->getVictim() && (m_creature->getVictim() == pAttacker || lastVictim == pAttacker))
            {
                return;
            }

            if (m_creature->IsFriendlyTo(pAttacker))
            {
                return;
            }

            lastVictim = pAttacker;
            AttackStart(pAttacker);
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // Ready to say emote
            if (m_bCanSayStory)
            {
                m_uiSayStoryTimer = SAY_STORY_DELAY;

                // Sequence switch between 3 texts
                switch (m_uiSayStoryLast)
                {
                    case 0: DoScriptText(SAY_STORYLINE4, m_creature); m_uiSayStoryLast++; break;
                    case 1: DoScriptText(SAY_STORYLINE5, m_creature); m_uiSayStoryLast++; break;
                    case 2: DoScriptText(SAY_STORYLINE6, m_creature); m_uiSayStoryLast++; break;
                    case 3: DoScriptText(SAY_STORYLINE7, m_creature); m_uiSayStoryLast++; break;
                    case 4: DoScriptText(SAY_STORYLINE8, m_creature); m_uiSayStoryLast = 0; break;
                    //case 3: DoScriptText(SAY_HELP4, m_creature); break;
                }

                m_bCanSayStory = false;
            }

            if (m_uiSayStoryTimer < uiDiff)
            {
                m_bCanSayStory = true;
                m_uiSayStoryTimer = SAY_STORY_DELAY;
            }
            else
            {
                m_uiSayStoryTimer -= uiDiff;
            }

            // Return since we have no target
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Equipped ranged weapon usable and not close to victim
            if (CombatDistanceBetween(*m_creature, *m_creature->getVictim(), false) > 0 && m_creature->CanUseEquippedWeapon(RANGED_ATTACK) )
            {
                // Make sure our attack is ready
                if (m_creature->isAttackReady(RANGED_ATTACK))
                {
                    // Use spell instead of normal ranged attack that seem not working. TODO: check ranged attack!
                    DoCastSpellIfCan(m_creature->getVictim(), SPELL_SHOOT);
                    //m_creature->AttackerStateUpdate(m_creature->getVictim(),RANGED_ATTACK);
                    m_creature->resetAttackTimer(RANGED_ATTACK);
                }

            }
            else if (InMeleeReach(*m_creature, *m_creature->getVictim()))
            {
                // If we are within range melee the target
                // Make sure our attack is ready
                if (m_creature->isAttackReady())
                {
                    printf("---> %s melee is ready %s! \n",m_creature->GetName(), m_creature->getVictim()->GetName());
                    m_creature->AttackerStateUpdate(m_creature->getVictim());

                    m_creature->resetAttackTimer();
                }

            }
            else if (IsSelfRooted)
            {
                // Cancel our current spell and then allow movement again
                m_creature->InterruptNonMeleeSpells(false);
                IsSelfRooted = false;
            }

        }

    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_prince_liam_greymane_phase2AI(pCreature);
    }
};

/*######
# rampaging_worgen
######*/

enum {
    SPELL_ENRAGE          = 8599,
    SPELL_ENRAGE_DELAY    = 120000
};

struct rampaging_worgen : public CreatureScript
{
    rampaging_worgen() : CreatureScript("rampaging_worgen") {}

    struct rampaging_worgenAI : public ScriptedAI
    {
        rampaging_worgenAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 spellCooldown;      // This variable acts like the global cooldown that players have (2 minute)
        bool IsSelfRooted;

        void Reset() override
        {
            spellCooldown = 0;
            IsSelfRooted = false;
        }

        void Aggro(Unit* who) override
        {
            if (!InMeleeReach(*m_creature, *who))
            {
                IsSelfRooted = true;
            }
        }

        void UpdateAI(const uint32 diff) override
        {
            // Always decrease our spell cooldown first
            if (spellCooldown > diff)
            {
                spellCooldown -= diff;
            }
            else
            {
                spellCooldown = 0;
            }

            // Enrage timer (only enrage when we are alive and  in combat)
            if (m_creature->IsInCombat() && m_creature->IsAlive())
            {
                // Cast enrage spell if less than 70% hp ONLY and not casted already
                if (m_creature->GetHealthPercent() < 70.0f && !spellCooldown)
                {
                    DoCastSpellIfCan(m_creature, SPELL_ENRAGE);
                    spellCooldown = SPELL_ENRAGE_DELAY;
                }

            }

            // Return since we have no target
            if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            {
                return;
            }

            // Return if we already cast a spell
            if (m_creature->IsNonMeleeSpellCasted(false))
            {
                return;
            }

            // If we are within range melee the target
            if (InMeleeReach(*m_creature, *m_creature->getVictim()))
            {
                // Make sure our attack is ready
                if (m_creature->isAttackReady())
                {
                    m_creature->AttackerStateUpdate(m_creature->getVictim());

                    m_creature->resetAttackTimer();
                }
            }
            else if (IsSelfRooted)
            {
                // Cancel our current spell and then allow movement again
                m_creature->InterruptNonMeleeSpells(false);
                IsSelfRooted = false;
            }

        }

    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new rampaging_worgenAI(pCreature);
    }
};

/** ######
 *  ## creature_frightened_citizen_quest
 *  ######*/
enum {
    SAY_ON_ESCAPE1             = -1654009,
    SAY_ON_ESCAPE2             = -1654010,
    SAY_ON_ESCAPE3             = -1654011,
    SAY_ON_ESCAPE4             = -1654012,
    SAY_ON_ESCAPE5             = -1654013,
    SAY_ON_ESCAPE6             = -1654014,
    SAY_ON_ESCAPE7             = -1654015,
    SAY_ON_ESCAPE_DELAY        = 12000
};

struct frightened_citizen_quest : public CreatureScript
{
    frightened_citizen_quest() : CreatureScript("frightened_citizen_quest") {}

    struct frightened_citizen_questAI : public ScriptedAI
    {
        frightened_citizen_questAI(Creature* pCreature) : ScriptedAI(pCreature) {}

        uint32 m_uiSayOnEscapeTimer;
        bool m_bCanSayOnEscape;

        void Reset() override
        {
            m_uiSayOnEscapeTimer = SAY_ON_ESCAPE_DELAY;
            m_bCanSayOnEscape = true;
        }

        void UpdateAI(const uint32 uiDiff) override
        {
            // Ready to say emote
            if (m_bCanSayOnEscape)
            {
                m_uiSayOnEscapeTimer = SAY_ON_ESCAPE_DELAY;

                // switch between 7 texts randomly
                switch (urand(0, 6))
                {
                    case 0: DoScriptText(SAY_ON_ESCAPE1, m_creature); break;
                    case 1: DoScriptText(SAY_ON_ESCAPE2, m_creature); break;
                    case 2: DoScriptText(SAY_ON_ESCAPE3, m_creature); break;
                    case 3: DoScriptText(SAY_ON_ESCAPE4, m_creature); break;
                    case 4: DoScriptText(SAY_ON_ESCAPE5, m_creature); break;
                    case 5: DoScriptText(SAY_ON_ESCAPE6, m_creature); break;
                    case 6: DoScriptText(SAY_ON_ESCAPE7, m_creature); break;
                }

                m_bCanSayOnEscape = false;
            }

            if (m_uiSayOnEscapeTimer < uiDiff)
            {
                m_bCanSayOnEscape = true;
                m_uiSayOnEscapeTimer = SAY_ON_ESCAPE_DELAY;
            }
            else
            {
                m_uiSayOnEscapeTimer -= uiDiff;
            }

        }

    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new frightened_citizen_questAI(pCreature);
    }
};

/**
 * Registers Gilneas City script content.
 */
void AddSC_gilneas_city()
{
    Script* s;
    s = new npc_prince_liam_greymane_phase1();
    s->RegisterSelf();
    s = new npc_prince_liam_greymane_phase2();
    s->RegisterSelf();
    s = new rampaging_worgen();
    s->RegisterSelf();
    s = new frightened_citizen_quest();
    s->RegisterSelf();
}
#endif
