///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2018) Alexander Stukowski
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


#include <core/Core.h>
#include <core/oo/RefTarget.h>
#include <core/dataset/pipeline/PipelineStatus.h>
#include <core/dataset/animation/TimeInterval.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View)

/**
 * \brief Abstract base class for all viewport overlays.
 */
class OVITO_CORE_EXPORT ViewportOverlay : public RefTarget
{
	Q_OBJECT
	OVITO_CLASS(ViewportOverlay)
	
protected:

	/// \brief Constructor.
	ViewportOverlay(DataSet* dataset);

public:

	/// \brief This method asks the overlay to paint its contents over the rendered image.
	virtual void render(const Viewport* viewport, TimePoint time, FrameBuffer* frameBuffer, 
						const ViewProjectionParameters& projParams, const RenderSettings* renderSettings, AsyncOperation& operation) = 0;

	/// \brief This method asks the overlay to paint its contents over the given interactive viewport.
	virtual void renderInteractive(const Viewport* viewport, TimePoint time, QPainter& painter, 
						const ViewProjectionParameters& projParams, const RenderSettings* renderSettings) = 0;

	/// \brief Returns the status of the object, which may indicate an error condition.
	///
	/// The default implementation of this method returns an empty status object.
	/// The object should generate a ReferenceEvent::ObjectStatusChanged event when its status changes.
	virtual PipelineStatus status() const { return PipelineStatus(); }

	/// \brief Moves the position of the overlay in the viewport by the given amount,
	///        which is specified as a fraction of the viewport render size.
	///
	/// Overlay implementations should override this method if they support positioning.
	/// The default method implementation does nothing.
	virtual void moveOverlayInViewport(const Vector2& delta) {}

private:

	/// Option for rendering the overlay contents behind the three-dimensional content.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, renderBehindScene, setRenderBehindScene);
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
