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
        return true;
    }
};

void AddSC_kezan()
{
    Script* s;

    s = new npc_sassy_hardwrench_kezan();
    s->RegisterSelf();
}
