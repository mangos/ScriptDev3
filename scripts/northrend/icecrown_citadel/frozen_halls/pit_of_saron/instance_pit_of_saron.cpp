/**
 * ScriptDev3 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2006-2013 ScriptDev2 <http://www.scriptdev2.com/>
 * Copyright (C) 2014-2025 MaNGOS <https://www.getmangos.eu>
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

/* ScriptData
SDName: instance_pit_of_saron
SD%Complete: 80%
SDComment:
SDCategory: Pit of Saron
EndScriptData */

#include "precompiled.h"
#include "pit_of_saron.h"

enum
{
    // Intro
    SAY_TYRANNUS_INTRO_1            = -1658001,
    SAY_JAINA_INTRO_1               = -1658002,
    SAY_SYLVANAS_INTRO_1            = -1658003,
    SAY_TYRANNUS_INTRO_2            = -1658004,
    SAY_TYRANNUS_INTRO_3            = -1658005,
    SAY_JAINA_INTRO_2               = -1658006,
    SAY_SYLVANAS_INTRO_2            = -1658007,
    SAY_TYRANNUS_INTRO_4            = -1658008,
    SAY_JAINA_INTRO_3               = -1658009,
    SAY_JAINA_INTRO_4               = -1658010,
    SAY_SYLVANAS_INTRO_3            = -1658011,
    SAY_JAINA_INTRO_5               = -1658012,
    SAY_SYLVANAS_INTRO_4            = -1658013,

    // Intro spells
    SPELL_NECROMATIC_POWER          = 69347,
    SPELL_FEIGN_DEATH               = 28728,
    SPELL_RAISE_DEAD                = 69350,

    // Garfrost outro
    SAY_TYRANNUS_GARFROST           = -1658020,
    SAY_GENERAL_GARFROST            = -1658021,

    // Ick and Krick outro
    SAY_JAINA_KRICK_1               = -1658036,
    SAY_SYLVANAS_KRICK_1            = -1658037,
    SAY_OUTRO_2                     = -1658038,
    SAY_JAINA_KRICK_2               = -1658039,
    SAY_SYLVANAS_KRICK_2            = -1658040,
    SAY_OUTRO_3                     = -1658041,
    SAY_TYRANNUS_KRICK_1            = -1658042,
    SAY_OUTRO_4                     = -1658043,
    SAY_TYRANNUS_KRICK_2            = -1658044,
    SAY_JAINA_KRICK_3               = -1658045,
    SAY_SYLVANAS_KRICK_3            = -1658046,

    // Ick and Krick outro spells
    SPELL_STRANGULATING             = 69413,
    SPELL_KRICK_KILL_CREDIT         = 71308,
    SPELL_SUICIDE                   = 7,

    // Ambush and Gauntlet
    SAY_TYRANNUS_AMBUSH_1           = -1658047,
    SAY_TYRANNUS_AMBUSH_2           = -1658048,
    SAY_GAUNTLET                    = -1658049,

    // Gauntlet spells
    SPELL_ICICLE_SUMMON             = 69424,
    SPELL_ACHIEVEMENT_CHECK         = 72845,

    // Tyrannus intro
    SAY_PREFIGHT_1                  = -1658050,
    SAY_VICTUS_TRASH                = -1658051,
    SAY_IRONSKULL_TRASH             = -1658068,
    SAY_PREFIGHT_2                  = -1658052,

    SPELL_EJECT_ALL_PASSENGERS      = 50630,
    // SPELL_CSA_DUMMY_EFFECT_1     = 56685,                // What is this?

    // Sindragosa outro
    SAY_VICTUS_OUTRO_1              = -1658061,
    SAY_IRONSKULL_OUTRO_2           = -1658069,
    SAY_GENERAL_OUTRO_2             = -1658062,
    SAY_JAINA_OUTRO_1               = -1658063,
    SAY_SYLVANAS_OUTRO_1            = -1658064,
    SAY_JAINA_OUTRO_2               = -1658065,
    SAY_JAINA_OUTRO_3               = -1658066,
    SAY_SYLVANAS_OUTRO_2            = -1658067,

    SPELL_FROST_BOMB                = 70521,
    SPELL_FROZEN_AFTERMATH          = 70518,
    SPELL_ARCANE_FORM               = 70573,
    SPELL_CALL_OF_SYLVANAS_1        = 70636,                // triggers 70639
    SPELL_CALL_OF_SYLVANAS_2        = 70638,
    // SPELL_CALL_OF_SYLVANAS_3     = 70642,
    SPELL_JAINAS_CALL_1             = 70527,                // triggers 70525
    SPELL_JAINAS_CALL_2             = 70623,
};

static const DialogueEntryTwoSide aPoSDialogues[] =
{
    // Instance intro
    {NPC_TYRANNUS_INTRO,   0,                  0,                    0,                  4000},
    {SAY_TYRANNUS_INTRO_1, NPC_TYRANNUS_INTRO, 0,                    0,                  6000},
    {SAY_TYRANNUS_INTRO_2, NPC_TYRANNUS_INTRO, 0,                    0,                  12000},
    {SAY_JAINA_INTRO_1,    NPC_JAINA_PART1,    SAY_SYLVANAS_INTRO_1, NPC_SYLVANAS_PART1, 5000},         // ToDo: move the soldiers to attack position
    {SAY_TYRANNUS_INTRO_3, NPC_TYRANNUS_INTRO, 0,                    0,                  5000},
    {SPELL_NECROMATIC_POWER, 0,                0,                    0,                  3000},
    {SAY_JAINA_INTRO_2,    NPC_JAINA_PART1,    SAY_SYLVANAS_INTRO_2, NPC_SYLVANAS_PART1, 4000},
    {SAY_TYRANNUS_INTRO_4, NPC_TYRANNUS_INTRO, 0,                    0,                  4000},         // ToDo: send the solderis back to fight as zombies
    {SAY_JAINA_INTRO_3,    NPC_JAINA_PART1,    0,                    0,                  6000},
    {SAY_JAINA_INTRO_4,    NPC_JAINA_PART1,    SAY_SYLVANAS_INTRO_3, NPC_SYLVANAS_PART1, 5000},
    {SAY_JAINA_INTRO_5,    NPC_JAINA_PART1,    SAY_SYLVANAS_INTRO_4, NPC_SYLVANAS_PART1, 0},

    // Garfrost outro
    {NPC_GARFROST,         0,                  0,                    0,                  4000},         // ToDo: move the freed slaves to position
    {SAY_GENERAL_GARFROST, NPC_VICTUS_PART1,   SAY_GENERAL_GARFROST, NPC_IRONSKULL_PART1, 2000},
    {SAY_TYRANNUS_GARFROST, NPC_TYRANNUS_INTRO, 0,                   0,                  0},

    // Ick and Krick outro
    {SAY_JAINA_KRICK_1,    NPC_JAINA_PART1,    SAY_SYLVANAS_KRICK_1, NPC_SYLVANAS_PART1, 6000},
    {SAY_OUTRO_2,          NPC_KRICK,          0,                    0,                  16000},
    {SAY_JAINA_KRICK_2,    NPC_JAINA_PART1,    SAY_SYLVANAS_KRICK_2, NPC_SYLVANAS_PART1, 7000},
    {SAY_OUTRO_3,          NPC_KRICK,          0,                    0,                  7000},
    {SAY_TYRANNUS_KRICK_1, NPC_TYRANNUS_INTRO, 0,                    0,                  3000},
    {SPELL_STRANGULATING,  0,                  0,                    0,                  3000},
    {SAY_OUTRO_4,          NPC_KRICK,          0,                    0,                  3000},
    {SAY_TYRANNUS_KRICK_2, NPC_TYRANNUS_INTRO, 0,                    0,                  11000},
    {SAY_JAINA_KRICK_3,    NPC_JAINA_PART1,    SAY_SYLVANAS_KRICK_3, NPC_SYLVANAS_PART1, 0},

    // Tyrannus intro
    {NPC_TYRANNUS,         0,                  0,                    0,                  10000},        // ToDo: move the freed slaves to position
    {SAY_PREFIGHT_1,       NPC_TYRANNUS,       0,                    0,                  13000},
    {SAY_VICTUS_TRASH,     NPC_VICTUS_PART2,   SAY_IRONSKULL_TRASH,  NPC_IRONSKULL_PART2, 9000},
    {SAY_PREFIGHT_2,       NPC_TYRANNUS,       0,                    0,                  10000},
    {NPC_RIMEFANG,         0,                  0,                    0,                  0},

    // Tyrannus outro
    {NPC_SINDRAGOSA,       0,                  0,                    0,                  30000},
    {SAY_VICTUS_OUTRO_1,   NPC_VICTUS_PART2,   SAY_IRONSKULL_OUTRO_2, NPC_IRONSKULL_PART2, 17000},
    {SAY_GENERAL_OUTRO_2,  NPC_VICTUS_PART2,   SAY_GENERAL_OUTRO_2,  NPC_IRONSKULL_PART2, 14000},
    {SAY_JAINA_OUTRO_1,    NPC_JAINA_PART2,    SAY_SYLVANAS_OUTRO_1, NPC_SYLVANAS_PART2,  1000},
    {SPELL_FROST_BOMB,     0,                  0,                    0,                   7000},
    {NPC_JAINA_PART2,      0,                  0,                    0,                   8000},
    {SAY_JAINA_OUTRO_2,    NPC_JAINA_PART2,    SAY_SYLVANAS_OUTRO_2, NPC_SYLVANAS_PART2,  15000},
    {SAY_JAINA_OUTRO_3,    NPC_JAINA_PART2,    0,                    0,                   0},
    {0, 0, 0, 0, 0},
};

static const float afTyrannusMovePos[4][3] =
{
    { 922.6365f, 145.877f, 643.2216f },                       // Hide position
    { 835.5887f, 139.4345f, 530.9526f },                      // Ick position
    { 906.9048f, -49.03813f, 618.8016f },                     // Tunnel position
    { 966.3345f, 159.2058f, 665.0453f },                      // Rimefang position
};

struct EventNpcLocations
{
    uint32 uiEntryHorde, uiEntryAlliance;
    float fX, fY, fZ, fO;
    float fMoveX, fMoveY, fMoveZ;
};

const EventNpcLocations aEventBeginLocations[3] =
{
    { NPC_SYLVANAS_PART1, NPC_JAINA_PART1, 430.3012f, 212.204f, 530.1146f, 0.042f, 440.7882f, 213.7587f, 528.7103f },
    { NPC_KILARA, NPC_ELANDRA, 429.7142f, 212.3021f, 530.2822f, 0.14f, 438.9462f, 215.4271f, 528.7087f },
    { NPC_LORALEN, NPC_KORELN, 429.5675f, 211.7748f, 530.3246f, 5.972f, 438.5052f, 211.5399f, 528.7085f },
    // ToDo: add the soldiers here when proper waypoint movement is supported
};

const EventNpcLocations aEventFirstAmbushLocations[2] =
{
    { NPC_YMIRJAR_DEATHBRINGER, 0, 951.6696f, 53.06405f, 567.5153f, 1.51f, 914.7256f, 76.66406f, 553.8029f },
    { NPC_YMIRJAR_DEATHBRINGER, 0, 950.9911f, 60.26712f, 566.7658f, 1.79f, 883.1805f, 52.69792f, 527.6385f },
};

const EventNpcLocations aEventSecondAmbushLocations[] =
{
    { NPC_FALLEN_WARRIOR, 0, 916.658f, -55.94097f, 591.6827f, 1.85f, 950.5694f, 31.85649f, 572.2693f },
    { NPC_FALLEN_WARRIOR, 0, 923.8055f, -55.63195f, 591.8663f, 1.85f, 941.3954f, 35.83769f, 571.4308f },
    { NPC_FALLEN_WARRIOR, 0, 936.0625f, -53.52778f, 592.0226f, 1.85f, 934.8011f, 8.024931f, 577.3419f },
    { NPC_FALLEN_WARRIOR, 0, 919.7518f, -68.39236f, 592.2916f, 1.85f, 932.5734f, -22.54153f, 587.403f },
    { NPC_FALLEN_WARRIOR, 0, 926.8993f, -68.08334f, 592.0798f, 1.85f, 922.6043f, -22.07627f, 585.6684f },
    { NPC_FALLEN_WARRIOR, 0, 939.1563f, -65.97916f, 592.2205f, 1.85f, 927.0928f, -32.97949f, 589.3028f },
    { NPC_COLDWRAITH, 0, 924.0261f, -62.3316f, 592.0191f, 2.01f, 929.4673f, 9.722589f, 577.4904f },
    { NPC_COLDWRAITH, 0, 936.4531f, -60.45486f, 592.1215f, 1.63f, 936.1395f, -4.003471f, 581.3139f },
    { NPC_COLDWRAITH, 0, 935.8055f, -72.76736f, 592.077f, 1.66f, 933.8441f, -47.83234f, 591.7538f },
    { NPC_COLDWRAITH, 0, 923.3785f, -74.6441f, 592.368f, 2.37f, 920.726f, -42.32272f, 589.9808f }
};

const EventNpcLocations aEventTunnelEndLocations[] =
{
    { NPC_IRONSKULL_PART2, NPC_VICTUS_PART2, 1071.45f, 48.23907f, 630.4871f, 1.68f, 1046.361f, 124.7031f, 628.2811f },
    // ToDo: add the freed slaves here when proper waypoint movement is supported
};
const EventNpcLocations aEventOutroLocations[] =
{
    { NPC_SINDRAGOSA, 0, 842.8611f, 194.5556f, 531.6536f, 6.108f, 900.106f, 181.677f, 659.374f },
    { NPC_SYLVANAS_PART2, NPC_JAINA_PART2, 1062.85f, 100.075f, 631.0021f, 1.77f, 1062.85f, 100.075f, 631.0021f },
};

struct is_pit_of_saron : public InstanceScript
{
    is_pit_of_saron() : InstanceScript("instance_pit_of_saron") {}

    class instance_pit_of_saron : public ScriptedInstance, private DialogueHelper
    {
    public:
        instance_pit_of_saron(Map* pMap) : ScriptedInstance(pMap), DialogueHelper(aPoSDialogues),
            m_uiAmbushAggroCount(0),
            m_uiTeam(TEAM_NONE),
            m_uiIciclesTimer(0)
        {
            Initialize();
        }

        ~instance_pit_of_saron() {}

        void Initialize() override
        {
            memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));
            InitializeDialogueHelper(this);

            for (uint8 i = 0; i < MAX_SPECIAL_ACHIEV_CRITS; ++i)
            {
                m_abAchievCriteria[i] = false;
            }
        }

        void OnCreatureCreate(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_TYRANNUS_INTRO:
            case NPC_JAINA_PART1:
            case NPC_SYLVANAS_PART1:
            case NPC_GARFROST:
            case NPC_KRICK:
            case NPC_ICK:
            case NPC_TYRANNUS:
            case NPC_RIMEFANG:
            case NPC_IRONSKULL_PART1:
            case NPC_VICTUS_PART1:
            case NPC_IRONSKULL_PART2:
            case NPC_VICTUS_PART2:
            case NPC_JAINA_PART2:
            case NPC_SYLVANAS_PART2:
            case NPC_SINDRAGOSA:
                m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetObjectGuid();
                break;
            case NPC_STALKER:
                m_lTunnelStalkersGuidList.push_back(pCreature->GetObjectGuid());
                break;
            case NPC_YMIRJAR_DEATHBRINGER:
            case NPC_YMIRJAR_WRATHBRINGER:
            case NPC_YMIRJAR_FLAMEBEARER:
            case NPC_FALLEN_WARRIOR:
            case NPC_COLDWRAITH:
                // Sort only the temporary summons
                if (pCreature->IsTemporarySummon())
                {
                    m_lAmbushNpcsGuidList.push_back(pCreature->GetObjectGuid());
                }
                break;
            case NPC_GENERAL_BUNNY:
                if (pCreature->GetPositionY() < 130.0f)
                {
                    if (pCreature->GetOrientation() != 0)
                    {
                        m_lArcaneShieldBunniesGuidList.push_back(pCreature->GetObjectGuid());
                    }
                    else
                    {
                        m_lFrozenAftermathBunniesGuidList.push_back(pCreature->GetObjectGuid());
                    }
                }
                break;
            }
        }

        void OnObjectCreate(GameObject* pGo) override
        {
            switch (pGo->GetEntry())
            {
            case GO_ICEWALL:
                if (m_auiEncounter[TYPE_GARFROST] == DONE && m_auiEncounter[TYPE_KRICK] == DONE)
                {
                    pGo->SetGoState(GO_STATE_ACTIVE);
                }
                break;
            case GO_HALLS_OF_REFLECT_PORT:
                break;

            default:
                return;
            }
            m_mGoEntryGuidStore[pGo->GetEntry()] = pGo->GetObjectGuid();
        }

        void OnPlayerEnter(Player* pPlayer) override
        {
            if (!m_uiTeam)                                          // very first player to enter
            {
                m_uiTeam = pPlayer->GetTeam();
                SetDialogueSide(m_uiTeam == ALLIANCE);
                ProcessIntroEventNpcs(pPlayer);
            }
        }

        void OnCreatureEnterCombat(Creature* pCreature) override
        {
            if (pCreature->GetEntry() == NPC_YMIRJAR_DEATHBRINGER)
            {
                ++m_uiAmbushAggroCount;

                // Summon the rest of the mobs at the 2nd ambush
                if (m_uiAmbushAggroCount == 2)
                {
                    Creature* pTyrannus = GetSingleCreatureFromStorage(NPC_TYRANNUS_INTRO);
                    if (!pTyrannus)
                    {
                        return;
                    }

                    DoScriptText(SAY_TYRANNUS_AMBUSH_2, pTyrannus);
                    pTyrannus->SetWalk(false);
                    pTyrannus->GetMotionMaster()->MovePoint(0, afTyrannusMovePos[2][0], afTyrannusMovePos[2][1], afTyrannusMovePos[2][2]);

                    // Spawn Mobs
                    for (uint8 i = 0; i < countof(aEventSecondAmbushLocations); ++i)
                    {
                        if (Creature* pSummon = pTyrannus->SummonCreature(aEventSecondAmbushLocations[i].uiEntryHorde, aEventSecondAmbushLocations[i].fX, aEventSecondAmbushLocations[i].fY,
                            aEventSecondAmbushLocations[i].fZ, aEventSecondAmbushLocations[i].fO, TEMPSPAWN_DEAD_DESPAWN, 0))
                        {
                            pSummon->SetWalk(false);
                            pSummon->GetMotionMaster()->MovePoint(1, aEventSecondAmbushLocations[i].fMoveX, aEventSecondAmbushLocations[i].fMoveY, aEventSecondAmbushLocations[i].fMoveZ);
                        }
                    }
                }
            }
        }

        void OnCreatureDeath(Creature* pCreature) override
        {
            switch (pCreature->GetEntry())
            {
            case NPC_YMIRJAR_DEATHBRINGER:
            case NPC_YMIRJAR_WRATHBRINGER:
            case NPC_YMIRJAR_FLAMEBEARER:
            case NPC_FALLEN_WARRIOR:
            case NPC_COLDWRAITH:
                // Check for tunnel event end - these mobs are not summoned
                if (pCreature->IsTemporarySummon())
                {
                    m_lAmbushNpcsGuidList.remove(pCreature->GetObjectGuid());

                    // If empty start tunnel event
                    if (m_lAmbushNpcsGuidList.empty())
                    {
                        Creature* pTyrannus = GetSingleCreatureFromStorage(NPC_TYRANNUS_INTRO);
                        if (!pTyrannus)
                        {
                            return;
                        }

                        DoScriptText(SAY_GAUNTLET, pTyrannus);
                        pTyrannus->SetWalk(false);
                        pTyrannus->GetMotionMaster()->MovePoint(0, afTyrannusMovePos[0][0], afTyrannusMovePos[0][1], afTyrannusMovePos[0][2]);
                        pTyrannus->ForcedDespawn(20000);

                        m_uiIciclesTimer = urand(3000, 5000);
                        SetSpecialAchievementCriteria(TYPE_ACHIEV_DONT_LOOK_UP, true);
                    }
                }
                break;
            }
        }

        void SetData(uint32 uiType, uint32 uiData) override
        {
            switch (uiType)
            {
            case TYPE_GARFROST:
                if (uiData == DONE && m_auiEncounter[TYPE_KRICK] == DONE)
                {
                    DoUseDoorOrButton(GO_ICEWALL);
                }
                if (uiData == IN_PROGRESS)
                {
                    SetSpecialAchievementCriteria(TYPE_ACHIEV_DOESNT_GO_ELEVEN, true);
                }
                else if (uiData == DONE)
                {
                    StartNextDialogueText(NPC_GARFROST);
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_KRICK:
                if (uiData == DONE && m_auiEncounter[TYPE_GARFROST] == DONE)
                {
                    DoUseDoorOrButton(GO_ICEWALL);
                }
                if (uiData == SPECIAL)
                {
                    // Used just to start the epilogue
                    StartNextDialogueText(SAY_JAINA_KRICK_1);
                    return;
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_TYRANNUS:
                if (uiData == DONE)
                {
                    StartNextDialogueText(NPC_SINDRAGOSA);
                }
                else if (uiData == SPECIAL)
                {
                    // Used just to start the intro
                    StartNextDialogueText(NPC_TYRANNUS);
                    return;
                }
                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_AMBUSH:
                if (uiData == DONE)
                {
                    // Complete tunnel achievement
                    if (Creature* pTyrannus = GetSingleCreatureFromStorage(NPC_TYRANNUS))
                    {
                        pTyrannus->CastSpell(pTyrannus, SPELL_ACHIEVEMENT_CHECK, true);
                    }

                    m_uiIciclesTimer = 0;
                }
                else if (uiData == IN_PROGRESS)
                {
                    DoStartAmbushEvent();
                }

                m_auiEncounter[uiType] = uiData;
                break;
            case TYPE_ACHIEV_DOESNT_GO_ELEVEN:
            case TYPE_ACHIEV_DONT_LOOK_UP:
                SetSpecialAchievementCriteria(uiType - TYPE_ACHIEV_DOESNT_GO_ELEVEN, bool(uiData));
                return;
            default:
                return;
            }

            if (uiData == DONE)
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " " << m_auiEncounter[2] << " " << m_auiEncounter[3];

                m_strInstData = saveStream.str();

                SaveToDB();
                OUT_SAVE_INST_DATA_COMPLETE;
            }
        }

        uint32 GetData(uint32 uiType) const override
        {
            if (uiType < MAX_ENCOUNTER)
            {
                return m_auiEncounter[uiType];
            }

            if (uiType == TYPE_DATA_PLAYER_TEAM)
            {
                return m_uiTeam;
            }

            return 0;
        }

        bool CheckAchievementCriteriaMeet(uint32 uiCriteriaId, Player const* pSource, Unit const* pTarget, uint32 uiMiscValue1 /* = 0*/) const override
        {
            switch (uiCriteriaId)
            {
            case ACHIEV_CRIT_DOESNT_GO_ELEVEN:
                return m_abAchievCriteria[0];
            case ACHIEV_CRIT_DONT_LOOK_UP:
                return m_abAchievCriteria[1];

            default:
                return false;
            }
        }

        const char* Save() const override { return m_strInstData.c_str(); }
        void Load(const char* chrIn) override
        {
            if (!chrIn)
            {
                OUT_LOAD_INST_DATA_FAIL;
                return;
            }

            OUT_LOAD_INST_DATA(chrIn);

            std::istringstream loadStream(chrIn);
            loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2] >> m_auiEncounter[3];

            for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            {
                if (m_auiEncounter[i] == IN_PROGRESS)
                {
                    m_auiEncounter[i] = NOT_STARTED;
                }
            }

            OUT_LOAD_INST_DATA_COMPLETE;
        }

        void Update(uint32 uiDiff)
        {
            DialogueUpdate(uiDiff);

            if (m_uiIciclesTimer)
            {
                if (m_uiIciclesTimer <= uiDiff)
                {
                    for (GuidList::const_iterator itr = m_lTunnelStalkersGuidList.begin(); itr != m_lTunnelStalkersGuidList.end(); ++itr)
                    {
                        // Only 5% of the stalkers will actually spawn an icicle
                        if (roll_chance_i(95))
                        {
                            continue;
                        }

                        if (Creature* pStalker = instance->GetCreature(*itr))
                        {
                            pStalker->CastSpell(pStalker, SPELL_ICICLE_SUMMON, true);
                        }
                    }
                    m_uiIciclesTimer = urand(3000, 5000);
                }
                else
                {
                    m_uiIciclesTimer -= uiDiff;
                }
            }
        }

    protected:
        void DoStartAmbushEvent()
        {
            Creature* pTyrannus = GetSingleCreatureFromStorage(NPC_TYRANNUS_INTRO);
            if (!pTyrannus)
            {
                return;
            }

            DoScriptText(SAY_TYRANNUS_AMBUSH_1, pTyrannus);

            // Spawn Mobs
            for (uint8 i = 0; i < countof(aEventFirstAmbushLocations); ++i)
            {
                if (Creature* pSummon = pTyrannus->SummonCreature(aEventFirstAmbushLocations[i].uiEntryHorde, aEventFirstAmbushLocations[i].fX, aEventFirstAmbushLocations[i].fY,
                    aEventFirstAmbushLocations[i].fZ, aEventFirstAmbushLocations[i].fO, TEMPSPAWN_DEAD_DESPAWN, 0))
                {
                    pSummon->SetWalk(false);
                    pSummon->GetMotionMaster()->MovePoint(1, aEventFirstAmbushLocations[i].fMoveX, aEventFirstAmbushLocations[i].fMoveY, aEventFirstAmbushLocations[i].fMoveZ);
                }
            }
        }

        void SetSpecialAchievementCriteria(uint32 uiType, bool bIsMet)
        {
            if (uiType < MAX_SPECIAL_ACHIEV_CRITS)
            {
                m_abAchievCriteria[uiType] = bIsMet;
            }
        }

        void JustDidDialogueStep(int32 iEntry) override
        {
            switch (iEntry)
            {
            case SPELL_NECROMATIC_POWER:
                // Transfor all soldiers into undead
                if (Creature* pTyrannus = GetSingleCreatureFromStorage(NPC_TYRANNUS_INTRO))
                {
                    pTyrannus->CastSpell(pTyrannus, SPELL_NECROMATIC_POWER, true);
                }
                break;
            case SAY_OUTRO_3:
                // Move Tyrannus into position
                if (Creature* pTyrannus = GetSingleCreatureFromStorage(NPC_TYRANNUS_INTRO))
                {
                    pTyrannus->SetWalk(false);
                    pTyrannus->GetMotionMaster()->MovePoint(0, afTyrannusMovePos[1][0], afTyrannusMovePos[1][1], afTyrannusMovePos[1][2]);
                }
                break;
            case SPELL_STRANGULATING:
                // Strangulate Krick
                if (Creature* pKrick = GetSingleCreatureFromStorage(NPC_KRICK))
                {
                    pKrick->CastSpell(pKrick, SPELL_STRANGULATING, true);
                    pKrick->SetLevitate(true);
                    pKrick->GetMotionMaster()->MovePoint(0, pKrick->GetPositionX(), pKrick->GetPositionY(), pKrick->GetPositionZ() + 5.0f);
                }
                break;
            case SAY_TYRANNUS_KRICK_2:
                // Kill Krick
                if (Creature* pKrick = GetSingleCreatureFromStorage(NPC_KRICK))
                {
                    pKrick->CastSpell(pKrick, SPELL_KRICK_KILL_CREDIT, true);
                    pKrick->CastSpell(pKrick, SPELL_SUICIDE, true);
                }
                break;
            case SAY_JAINA_INTRO_3:
            case SAY_JAINA_KRICK_3:
                // Move Tyrannus to a safe position
                if (Creature* pTyrannus = GetSingleCreatureFromStorage(NPC_TYRANNUS_INTRO))
                {
                    pTyrannus->GetMotionMaster()->MovePoint(0, afTyrannusMovePos[0][0], afTyrannusMovePos[0][1], afTyrannusMovePos[0][2]);
                }
                break;
            case NPC_TYRANNUS:
            {
                                 Creature* pTyrannus = GetSingleCreatureFromStorage(NPC_TYRANNUS);
                                 if (!pTyrannus)
                                 {
                                     return;
                                 }

                                 // Spawn tunnel end event mobs
                                 for (uint8 i = 0; i < countof(aEventTunnelEndLocations); ++i)
                                 {
                                     if (Creature* pSummon = pTyrannus->SummonCreature(m_uiTeam == HORDE ? aEventTunnelEndLocations[i].uiEntryHorde : aEventTunnelEndLocations[i].uiEntryAlliance,
                                         aEventTunnelEndLocations[i].fX, aEventTunnelEndLocations[i].fY, aEventTunnelEndLocations[i].fZ, aEventTunnelEndLocations[i].fO, TEMPSPAWN_DEAD_DESPAWN, 0))
                                     {
                                         pSummon->SetWalk(false);
                                         pSummon->GetMotionMaster()->MovePoint(0, aEventTunnelEndLocations[i].fMoveX, aEventTunnelEndLocations[i].fMoveY, aEventTunnelEndLocations[i].fMoveZ);
                                     }
                                 }
                                 break;
            }
            case NPC_RIMEFANG:
                // Eject Tyrannus and prepare for combat
                if (Creature* pRimefang = GetSingleCreatureFromStorage(NPC_RIMEFANG))
                {
                    pRimefang->CastSpell(pRimefang, SPELL_EJECT_ALL_PASSENGERS, true);
                    pRimefang->SetWalk(false);
                    pRimefang->GetMotionMaster()->MovePoint(0, afTyrannusMovePos[3][0], afTyrannusMovePos[3][1], afTyrannusMovePos[3][2]);
                }
                if (Creature* pTyrannus = GetSingleCreatureFromStorage(NPC_TYRANNUS))
                {
                    pTyrannus->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                }
                break;
            case SAY_VICTUS_OUTRO_1:
            {
                                       Player* pPlayer = GetPlayerInMap();
                                       if (!pPlayer)
                                       {
                                           return;
                                       }

                                       // Spawn Sindragosa
                                       if (Creature* pSummon = pPlayer->SummonCreature(aEventOutroLocations[0].uiEntryHorde, aEventOutroLocations[0].fX, aEventOutroLocations[0].fY,
                                           aEventOutroLocations[0].fZ, aEventOutroLocations[0].fO, TEMPSPAWN_TIMED_DESPAWN, 2 * MINUTE * IN_MILLISECONDS))
                                       {
                                           pSummon->SetWalk(false);
                                           pSummon->GetMotionMaster()->MovePoint(0, aEventOutroLocations[0].fMoveX, aEventOutroLocations[0].fMoveY, aEventOutroLocations[0].fMoveZ);
                                       }
                                       // Spawn Jaina or Sylvanas
                                       if (Creature* pSummon = pPlayer->SummonCreature(m_uiTeam == HORDE ? aEventOutroLocations[1].uiEntryHorde : aEventOutroLocations[1].uiEntryAlliance,
                                           aEventOutroLocations[1].fX, aEventOutroLocations[1].fY, aEventOutroLocations[1].fZ, aEventOutroLocations[1].fO, TEMPSPAWN_TIMED_DESPAWN, 24 * HOUR * IN_MILLISECONDS))
                                       {
                                           pSummon->SetWalk(false);
                                           pSummon->GetMotionMaster()->MovePoint(0, aEventOutroLocations[1].fMoveX, aEventOutroLocations[1].fMoveY, aEventOutroLocations[1].fMoveZ);
                                       }
                                       break;
            }
            case SAY_JAINA_OUTRO_1:
                // Visual effect
                for (GuidList::const_iterator itr = m_lArcaneShieldBunniesGuidList.begin(); itr != m_lArcaneShieldBunniesGuidList.end(); ++itr)
                {
                    if (Creature* pBunny = instance->GetCreature(*itr))
                    {
                        pBunny->CastSpell(pBunny, SPELL_ARCANE_FORM, true);
                    }
                }
                // Teleport players
                if (Creature* pTemp = GetSingleCreatureFromStorage(m_uiTeam == HORDE ? NPC_SYLVANAS_PART2 : NPC_JAINA_PART2))
                {
                    pTemp->CastSpell(pTemp, m_uiTeam == HORDE ? SPELL_CALL_OF_SYLVANAS_2 : SPELL_JAINAS_CALL_2, true);
                    pTemp->CastSpell(pTemp, m_uiTeam == HORDE ? SPELL_CALL_OF_SYLVANAS_2 : SPELL_JAINAS_CALL_2, true);
                }
                break;
            case SPELL_FROST_BOMB:
                // Frost bomb on the platform
                if (Creature* pSindragosa = GetSingleCreatureFromStorage(NPC_SINDRAGOSA))
                {
                    pSindragosa->CastSpell(pSindragosa, SPELL_FROST_BOMB, true);
                }
                // Visual effect
                for (GuidList::const_iterator itr = m_lFrozenAftermathBunniesGuidList.begin(); itr != m_lFrozenAftermathBunniesGuidList.end(); ++itr)
                {
                    if (Creature* pBunny = instance->GetCreature(*itr))
                    {
                        pBunny->CastSpell(pBunny, SPELL_FROZEN_AFTERMATH, true);
                    }
                }
                break;
            case NPC_JAINA_PART2:
                // Visual effect remove
                for (GuidList::const_iterator itr = m_lArcaneShieldBunniesGuidList.begin(); itr != m_lArcaneShieldBunniesGuidList.end(); ++itr)
                {
                    if (Creature* pBunny = instance->GetCreature(*itr))
                    {
                        pBunny->RemoveAurasDueToSpell(SPELL_ARCANE_FORM);
                    }
                }
                // Sindragosa exit
                if (Creature* pSindragosa = GetSingleCreatureFromStorage(NPC_SINDRAGOSA))
                {
                    pSindragosa->GetMotionMaster()->MovePoint(0, 759.148f, 199.955f, 720.857f);
                }
                // Jaina / Sylvanas starts moving (should use wp)
                if (Creature* pTemp = GetSingleCreatureFromStorage(m_uiTeam == HORDE ? NPC_SYLVANAS_PART2 : NPC_JAINA_PART2))
                {
                    pTemp->SetWalk(true);
                    pTemp->GetMotionMaster()->MovePoint(0, 1057.76f, 111.927f, 628.4123f);
                }
                break;
            case SAY_JAINA_OUTRO_2:
                if (Creature* pTemp = GetSingleCreatureFromStorage(m_uiTeam == HORDE ? NPC_SYLVANAS_PART2 : NPC_JAINA_PART2))
                {
                    pTemp->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
                }

                // ToDo: Jaina / Sylvanas should have some waypoint movement here and the door should be opened only when they get in front of it.
                DoUseDoorOrButton(GO_HALLS_OF_REFLECT_PORT);
                break;
            }
        }

        void ProcessIntroEventNpcs(Player* pPlayer)
        {
            if (!pPlayer)
            {
                return;
            }

            // Not if the bosses are already killed
            if (GetData(TYPE_GARFROST) == DONE || GetData(TYPE_KRICK) == DONE)
            {
                return;
            }

            StartNextDialogueText(NPC_TYRANNUS_INTRO);

            // Spawn Begin Mobs
            for (uint8 i = 0; i < countof(aEventBeginLocations); ++i)
            {
                // ToDo: maybe despawn the intro npcs when the other events occur
                if (Creature* pSummon = pPlayer->SummonCreature(m_uiTeam == HORDE ? aEventBeginLocations[i].uiEntryHorde : aEventBeginLocations[i].uiEntryAlliance,
                    aEventBeginLocations[i].fX, aEventBeginLocations[i].fY, aEventBeginLocations[i].fZ, aEventBeginLocations[i].fO, TEMPSPAWN_TIMED_DESPAWN, 24 * HOUR * IN_MILLISECONDS))
                {
                    pSummon->SetWalk(false);
                    pSummon->GetMotionMaster()->MovePoint(0, aEventBeginLocations[i].fMoveX, aEventBeginLocations[i].fMoveY, aEventBeginLocations[i].fMoveZ);
                }
            }
        }

        uint32 m_auiEncounter[MAX_ENCOUNTER];
        std::string m_strInstData;

        bool m_abAchievCriteria[MAX_SPECIAL_ACHIEV_CRITS];

        uint8 m_uiAmbushAggroCount;
        uint32 m_uiTeam;                                    // Team of first entered player, used to set if Jaina or Silvana to spawn
        uint32 m_uiIciclesTimer;

        GuidList m_lTunnelStalkersGuidList;
        GuidList m_lAmbushNpcsGuidList;
        GuidList m_lArcaneShieldBunniesGuidList;
        GuidList m_lFrozenAftermathBunniesGuidList;
    };

    InstanceData* GetInstanceData(Map* pMap) override
    {
        return new instance_pit_of_saron(pMap);
    }
};

void AddSC_instance_pit_of_saron()
{
    Script* s;

    s = new is_pit_of_saron();
    s->RegisterSelf();

    //pNewScript = new Script;
    //pNewScript->Name = "instance_pit_of_saron";
    //pNewScript->GetInstanceData = &GetInstanceData_instance_pit_of_saron;
    //pNewScript->RegisterSelf();
}
