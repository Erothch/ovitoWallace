///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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


#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/stdobj/gui/widgets/DataSeriesPlotWidget.h>
#include <gui/properties/ModifierPropertiesEditor.h>
#include <core/utilities/DeferredMethodInvocation.h>

class QwtPlotZoneItem;

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief A properties editor for the PolyhedralTemplateMatchingModifier class.
 */
class PolyhedralTemplateMatchingModifierEditor : public ModifierPropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(PolyhedralTemplateMatchingModifierEditor)

public:

	/// Default constructor.
	Q_INVOKABLE PolyhedralTemplateMatchingModifierEditor() {}

protected Q_SLOTS:

	/// Replots the histogram computed by the modifier.
	void plotHistogram();

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, const ReferenceEvent& event) override;

private:

	/// The graph widget to display the RMSD histogram.
	DataSeriesPlotWidget* _rmsdPlotWidget;

	/// Marks the RMSD cutoff in the histogram plot.
	QwtPlotZoneItem* _rmsdRangeIndicator;

	/// For deferred invocation of the plot repaint function.
	DeferredMethodInvocation<PolyhedralTemplateMatchingModifierEditor, &PolyhedralTemplateMatchingModifierEditor::plotHistogram> plotHistogramLater;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
