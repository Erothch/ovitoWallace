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

#ifndef __OVITO_BIN_AND_REDUCE_MODIFIER_H
#define __OVITO_BIN_AND_REDUCE_MODIFIER_H

#include <plugins/particles/Particles.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <plugins/particles/data/ParticlePropertyObject.h>
#include "../../ParticleModifier.h"
#include <qcustomplot.h>

class QCustomPlot;
class QCPItemStraightLine;

namespace Particles {

/*
 * This modifier computes a spatial average (over splices) for a particle
 * property.
 */
class OVITO_PARTICLES_EXPORT BinAndReduceModifier : public ParticleModifier
{
public:
    enum ReductionOperationType { RED_MEAN, RED_SUM, RED_SUM_VOL, RED_MIN, RED_MAX };

	/// Constructor.
	Q_INVOKABLE BinAndReduceModifier(DataSet* dataset);

	/// This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	virtual void initializeModifier(PipelineObject* pipelineObject, ModifierApplication* modApp) override;

	/// Sets the source particle property for which the average should be computed.
	void setSourceProperty(const ParticlePropertyReference& prop) { _sourceProperty = prop; }

	/// Returns the source particle property for which the average is computed.
	const ParticlePropertyReference& sourceProperty() const { return _sourceProperty; }

	/// Returns the reduction operation
	int reductionOperation() const { return _reductionOperation; }

	/// Sets the reduction operation
	void setReductionOperation(int o) { _reductionOperation = o; }

	/// Returns the bin alignment
	int reductionBinAlignment() const { return _binAlignment; }

	/// Sets the bin alignment
	void setBinAlignment(int o) { _binAlignment = o; }

	/// Returns the number of spatial bins of the computed average value.
	int numberOfBins() const { return _numberOfBins; }

	/// Sets the number of spatial bins of the computed average value.
	void setNumberOfBins(int n) { _numberOfBins = n; }

	/// Returns the stored average data.
	const std::vector<FloatType>& binData() const { return _binData; }

	/// Returns the start value of the y-axis.
	FloatType xAxisRangeStart() const { return _xAxisRangeStart; }

	/// Returns the end value of the y-axis.
	FloatType xAxisRangeEnd() const { return _xAxisRangeEnd; }

	/// Set whether the range of the y-axis of the scatter plot should be fixed.
	void setFixYAxisRange(bool fix) { _fixYAxisRange = fix; }

	/// Returns whether the range of the y-axis should be fixed.
	bool fixYAxisRange() const { return _fixYAxisRange; }

	/// Set start and end value of the y-axis.
	void setYAxisRange(FloatType start, FloatType end) { _yAxisRangeStart = start; _yAxisRangeEnd = end; }

	/// Returns the start value of the y-axis.
	FloatType yAxisRangeStart() const { return _yAxisRangeStart; }

	/// Returns the end value of the y-axis.
	FloatType yAxisRangeEnd() const { return _yAxisRangeEnd; }

public:

	Q_PROPERTY(Particles::ParticlePropertyReference sourceProperty READ sourceProperty WRITE setSourceProperty);
	Q_PROPERTY(int reductionOperation READ reductionOperation WRITE setReductionOperation);
	Q_PROPERTY(int numberOfBins READ numberOfBins WRITE setNumberOfBins);

protected:

	/// Modifies the particle object.
	virtual PipelineStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

private:

	/// Stores the start value of the y-axis.
	FloatType _xAxisRangeStart;

	/// Stores the end value of the y-axis.
	FloatType _xAxisRangeEnd;

	/// The particle property that serves as data source to be averaged.
	PropertyField<ParticlePropertyReference> _sourceProperty;

	/// Type of reduction operation
	PropertyField<int> _reductionOperation;

	/// Bin alignment
	PropertyField<int> _binAlignment;

	/// Controls the number of spatial bins.
	PropertyField<int> _numberOfBins;

	/// Controls the whether the range of the y-axis of the avarage values should be fixed.
	PropertyField<bool> _fixYAxisRange;

	/// Controls the start value of the y-axis.
	PropertyField<FloatType> _yAxisRangeStart;

	/// Controls the end value of the y-axis.
	PropertyField<FloatType> _yAxisRangeEnd;

	/// Stores the averaged data.
	std::vector<FloatType> _binData;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Bin and reduce");
	Q_CLASSINFO("ModifierCategory", "Analysis");

	DECLARE_PROPERTY_FIELD(_reductionOperation);
	DECLARE_PROPERTY_FIELD(_binAlignment);
	DECLARE_PROPERTY_FIELD(_numberOfBins);
	DECLARE_PROPERTY_FIELD(_fixYAxisRange);
	DECLARE_PROPERTY_FIELD(_yAxisRangeStart);
	DECLARE_PROPERTY_FIELD(_yAxisRangeEnd);
	DECLARE_PROPERTY_FIELD(_sourceProperty);
};

/******************************************************************************
* A properties editor for the BinAndReduceModifier class.
******************************************************************************/
class BinAndReduceModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE BinAndReduceModifierEditor() : _rangeUpdate(true) {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

protected Q_SLOTS:

	/// Replots the average data computed by the modifier.
	void plotAverages();

	/// Keep y-axis range updated
	void updateYAxisRange(const QCPRange &newRange);

	/// This is called when the user has clicked the "Save Data" button.
	void onSaveData();

private:

	/// The graph widget to display the average data.
	QCustomPlot* _averagesPlot;

	/// Update range when plot ranges change?
	bool _rangeUpdate;

	Q_OBJECT
	OVITO_OBJECT
};

};	// End of namespace

#endif // __OVITO_SPATIAL_AVERAGE_MODIFIER_H