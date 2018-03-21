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
#include "NonInteractiveSceneRenderer.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

IMPLEMENT_OVITO_CLASS(NonInteractiveSceneRenderer);

/******************************************************************************
* Determines if this renderer can share geometry data and other resources with 
* the given other renderer.
******************************************************************************/
bool NonInteractiveSceneRenderer::sharesResourcesWith(SceneRenderer* otherRenderer) const
{
	return dynamic_object_cast<NonInteractiveSceneRenderer>(otherRenderer) != nullptr;
}

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void NonInteractiveSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp)
{
	SceneRenderer::beginFrame(time, params, vp);
	_modelTM.setIdentity();
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
