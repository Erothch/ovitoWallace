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

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * A properties editor for the CoordinationAnalysisModifier class.
 */
class CoordinationAnalysisModifierEditor : public ModifierPropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(CoordinationAnalysisModifierEditor)

public:

	/// Default constructor.
	Q_INVOKABLE CoordinationAnalysisModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Replots the RDF computed by the modifier.
	void plotRDF();

private:

	/// The plotting widget for displaying the computed RDFs.
	DataSeriesPlotWidget* _rdfPlot;

	/// For deferred invocation of the plot repaint function.
	DeferredMethodInvocation<CoordinationAnalysisModifierEditor, &CoordinationAnalysisModifierEditor::plotRDF> plotRDFLater;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
