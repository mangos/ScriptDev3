/**
 * ScriptDev3 is an extension for mangos providing enhanced features for
 * area triggers, creatures, game objects, instances, items, and spells beyond
 * the default database scripting in mangos.
 *
 * Copyright (C) 2014-2026 MaNGOS <https://www.getmangos.eu>
 * Copyright (C) 2006-2013 ScriptDev2 <http://www.scriptdev2.com/>
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

#ifndef SC_PRECOMPILED_H
#define SC_PRECOMPILED_H

// The single prelude for every script. pch.h precompiles this file and nothing
// else; both used to carry their own copy of the list under this same guard, so
// whichever the compiler saw first suppressed the other. The lists had drifted,
// which meant a PCH build and a non-PCH build compiled the scripts against
// different sets of headers -- the three below came only from pch.h, and
// ScriptMgr.h and GameObjectAI.h only from here. This is the union.
#include "system/ScriptDevMgr.h"
#include "ScriptMgr.h"
#include "Object.h"
#include "ObjectGuid.h"
#include "Unit.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "sc_creature.h"
#include "sc_gossip.h"
#include "sc_grid_searchers.h"
#include "sc_instance.h"
#include "SpellAuras.h"
#include "World.h"

// Common.h is being retired across the cores; these are the pieces the scripts
// used to receive through it. Named here once rather than in 490 script files.
#include "Common/TimeConstants.h"
#include "Utilities/MathDefines.h"
#include "Utilities/Util.h"

#include <algorithm>
#include <cmath>
#include <list>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#endif
