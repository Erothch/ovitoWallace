///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2017) Alexander Stukowski
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

#pragma once


#include <gui/GUI.h>
#include <gui/properties/PropertiesEditor.h>
#include <core/oo/RefTarget.h>

namespace Ovito { namespace OSPRay { OVITO_BEGIN_INLINE_NAMESPACE(Internal)
	
/*
 * \brief The UI component for the OSPRayPathTracerBackend class.
 */
class OSPRayPathTracerBackendEditor : public PropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(OSPRayPathTracerBackendEditor)
	
public:

	/// Default constructor.
	Q_INVOKABLE OSPRayPathTracerBackendEditor() = default;

protected:
	
	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


