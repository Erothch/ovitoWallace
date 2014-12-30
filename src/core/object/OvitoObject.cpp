///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <core/Core.h>
#include <core/utilities/io/ObjectLoadStream.h>
#include "OvitoObject.h"
#include "OvitoObjectType.h"

namespace Ovito { inline namespace ObjectSystem {

const Ovito::ObjectSystem::Internal::NativeOvitoObjectType OvitoObject::OOType("OvitoObject", "Core", nullptr, &OvitoObject::staticMetaObject, true);

/******************************************************************************
* Returns true if this object is currently being loaded from an ObjectLoadStream.
******************************************************************************/
bool OvitoObject::isBeingLoaded() const
{
	return (qobject_cast<ObjectLoadStream*>(parent()) != nullptr);
}

}}	// End of namespace
