/**
 * ScriptDev3 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013 ScriptDev2 <http://www.scriptdev2.com/>
 * Copyright (C) 2014-2026 MaNGOS <https://www.getmangos.eu>
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Kezan
SD%Complete: 0
SDComment: Placeholder
SDCategory: Kezan
EndScriptData */

/* ContentData
npc_sassy_hardwrench_kezan
npc_defiant_troll_q14069
EndContentData */

#include "precompiled.h"

/*######
#
######*/

enum
{
    QUEST_MEGS_IN_MARKETING = 28349,
    SAY_SASSY_PROMOTION_MALE   = -1999942,
    SAY_SASSY_PROMOTION_FEMALE = -1999943,
};

struct npc_sassy_hardwrench_kezan : public CreatureScript
{
    npc_sassy_hardwrench_kezan() : CreatureScript("npc_sassy_hardwrench_kezan") {}

    bool OnQuestAccept(Player* pPlayer, Creature* pCreature, Quest const* pQuest) override
    {
        if (!pPlayer || !pCreature || !pQuest || pQuest->GetQuestId() != QUEST_MEGS_IN_MARKETING)
        {
            return false;
        }

        DoScriptText(pPlayer->getGender() == GENDER_FEMALE ? SAY_SASSY_PROMOTION_FEMALE : SAY_SASSY_PROMOTION_MALE, pCreature, pPlayer);
    QUEST_GOOD_HELP_IS_HARD_TO_FIND         = 14069,
    NPC_DEFIANT_TROLL                       = 34830,
    SPELL_GOBLIN_ALL_IN_1_DER_BELT_SHOCKER = 66306,
    MAX_TROLLS_TO_ADJUST                    = 8,
    DEFIANT_TROLL_DESPAWN_DELAY             = 2000,
};

static const char* const aDefiantTrollTexts[] =
{
    "Don't tase me, mon!",
    "I report you to HR!",
    "I'm going. I'm going!",
    "Oops, break's over.",
    "Ouch! Dat hurt!",
    "Sorry, mon. It won't happen again.",
    "What I doin' wrong? Don't I get a lunch and two breaks a day, mon?",
    "Work was bettah in da Undermine!",
};

struct npc_defiant_troll_q14069 : public CreatureScript
{
    npc_defiant_troll_q14069() : CreatureScript("npc_defiant_troll_q14069") {}

    struct npc_defiant_troll_q14069AI : public ScriptedAI
    {
        npc_defiant_troll_q14069AI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            Reset();
        }

        void Reset() override
        {
            m_bAdjusted = false;
        }

        bool m_bAdjusted;
    };

    CreatureAI* GetAI(Creature* pCreature) override
    {
        return new npc_defiant_troll_q14069AI(pCreature);
    }

    bool OnSpellClick(Player* pPlayer, Creature* pCreature, uint32 uiSpellId) override
    {
        if (!pPlayer || !pCreature || pCreature->GetEntry() != NPC_DEFIANT_TROLL ||
            uiSpellId != SPELL_GOBLIN_ALL_IN_1_DER_BELT_SHOCKER)
        {
            return true;
        }

        QuestStatus status = pPlayer->GetQuestStatus(QUEST_GOOD_HELP_IS_HARD_TO_FIND);
        if (status != QUEST_STATUS_INCOMPLETE && status != QUEST_STATUS_COMPLETE)
        {
            return true;
        }

        if (status == QUEST_STATUS_COMPLETE ||
            pPlayer->GetReqKillOrCastCurrentCount(QUEST_GOOD_HELP_IS_HARD_TO_FIND, NPC_DEFIANT_TROLL) >= MAX_TROLLS_TO_ADJUST)
        {
            pPlayer->GetSession()->SendNotification("The goblin All-In-1-Der Belt's battery is depleted.");
            return true;
        }

        npc_defiant_troll_q14069AI* pAI = dynamic_cast<npc_defiant_troll_q14069AI*>(pCreature->AI());
        if (!pAI || pAI->m_bAdjusted)
        {
            return true;
        }

        pAI->m_bAdjusted = true;
        pCreature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);

        pPlayer->CastSpell(pCreature, SPELL_GOBLIN_ALL_IN_1_DER_BELT_SHOCKER, true);
        pCreature->MonsterSay(aDefiantTrollTexts[urand(0, sizeof(aDefiantTrollTexts) / sizeof(aDefiantTrollTexts[0]) - 1)], LANG_UNIVERSAL, pPlayer);
        pCreature->ForcedDespawn(DEFIANT_TROLL_DESPAWN_DELAY);

        return true;
    }
};

void AddSC_kezan()
{
    Script* s;

    s = new npc_sassy_hardwrench_kezan();
    s->RegisterSelf();

    s = new npc_defiant_troll_q14069();
    s->RegisterSelf();
}
