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


#include <plugins/stdmod/gui/StdModGui.h>
#include <gui/properties/ModifierPropertiesEditor.h>
#include <plugins/stdobj/gui/widgets/DataSeriesPlotWidget.h>
#include <core/utilities/DeferredMethodInvocation.h>

class QwtPlotZoneItem;

namespace Ovito { namespace StdMod {

/**
 * A properties editor for the ScatterPlotModifier class.
 */
class ScatterPlotModifierEditor : public ModifierPropertiesEditor
{
	Q_OBJECT
	OVITO_CLASS(ScatterPlotModifierEditor)
	
public:

	/// Default constructor.
	Q_INVOKABLE ScatterPlotModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Replots the scatter plot.
	void plotScatterPlot();

private:

	/// The graph widget to display the plot.
	DataSeriesPlotWidget* _plotWidget;

	/// Marks the range of selected points in the X direction.
	QwtPlotZoneItem* _selectionRangeIndicatorX;

	/// Marks the range of selected points in the Y direction.
	QwtPlotZoneItem* _selectionRangeIndicatorY;

	/// For deferred invocation of the plot repaint function.
	DeferredMethodInvocation<ScatterPlotModifierEditor, &ScatterPlotModifierEditor::plotScatterPlot> plotLater;
};

}	// End of namespace
}	// End of namespace
