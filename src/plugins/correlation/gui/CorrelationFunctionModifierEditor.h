///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
//  Copyright (2017) Lars Pastewka
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

#ifndef __OVITO_CORRELATION_FUNCTION_MODIFIER_EDITOR_H
#define __OVITO_CORRELATION_FUNCTION_MODIFIER_EDITOR_H

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/gui/modifier/ParticleModifierEditor.h>
#include <core/utilities/DeferredMethodInvocation.h>

class QwtPlot;
class QwtPlotCurve;

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * A properties editor for the CorrelationFunctionModifier class.
 */
class CorrelationFunctionModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE CorrelationFunctionModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

	/// Replots one of the correlation function computed by the modifier.
	void plotData(const QVector<FloatType> &xData, const QVector<FloatType> &yData,
				  QwtPlot *plot, QwtPlotCurve *&curve,
				  FloatType offset=0.0, FloatType fac=1.0);

protected Q_SLOTS:

	/// Replots the correlation function computed by the modifier.
	void plotAllData();

    /// This is called when the user has clicked the "Save Data" button.
    void onSaveData();

private:

	/// The plotting widget for displaying the computed real-space correlation function.
	QwtPlot* _realSpacePlot;

	/// The plotting widget for displaying the computed reciprocal-space correlation function.
	QwtPlot* _reciprocalSpacePlot;

	/// The plot item for the real-space correlation function.
    QwtPlotCurve* _realSpaceCurve = nullptr;

	/// The plot item for the short-ranged part of the real-space correlation function.
    QwtPlotCurve* _neighCurve = nullptr;

	/// The plot item for the reciprocal-space correlation function.
    QwtPlotCurve* _reciprocalSpaceCurve = nullptr;

	/// For deferred invocation of the plot repaint function.
	DeferredMethodInvocation<CorrelationFunctionModifierEditor, &CorrelationFunctionModifierEditor::plotAllData> plotAllDataLater;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

#endif // __OVITO_CORRELATION_FUNCTION_MODIFIER_EDITOR_H
