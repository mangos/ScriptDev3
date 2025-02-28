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

#ifndef SC_SCRIPTLOADER_H
#define SC_SCRIPTLOADER_H

/**
 * @brief Adds all scripts to the script manager.
 */
void AddScripts();

/**
 * @brief Adds scripts for the Eastern Kingdoms.
 */
void AddEasternKingdomsScripts();

/**
 * @brief Adds scripts for Kalimdor.
 */
void AddKalimdorScripts();

#if defined (TBC) || defined (WOTLK) || defined (CATA) || defined(MISTS)
/**
 * @brief Adds scripts for Outlands.
 */
void AddOutlandsScripts();
#endif

#if defined (WOTLK) || defined (CATA) || defined(MISTS)
/**
 * @brief Adds scripts for Northrend.
 */
void AddNorthrendScripts();
#endif

/**
 * @brief Adds world scripts.
 */
void AddWorldScripts();

/**
 * @brief Adds battleground scripts.
 */
void AddBattlegroundScripts();

#endif
