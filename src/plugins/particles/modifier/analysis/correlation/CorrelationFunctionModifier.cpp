///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
//  Copyright (2016) Lars Pastewka
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

#include <plugins/particles/Particles.h>
#include <core/scene/objects/DataObject.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/app/Application.h>
#include <core/animation/AnimationSettings.h>
#include "CorrelationFunctionModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, CorrelationFunctionModifier, AsynchronousParticleModifier);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, _sourceProperty1, "SourceProperty1");
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, _sourceProperty2, "SourceProperty2");
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, _cutoff, "Cutoff", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, _numberOfBins, "NumberOfBins", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, _sourceProperty1, "First property");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, _sourceProperty2, "Second property");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, _cutoff, "Cutoff radius");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, _numberOfBins, "Number of histogram bins");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CorrelationFunctionModifier, _cutoff, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(CorrelationFunctionModifier, _numberOfBins, IntegerParameterUnit, 4, 100000);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CorrelationFunctionModifier::CorrelationFunctionModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_cutoff(3.2), _numberOfBins(200)
{
	INIT_PROPERTY_FIELD(CorrelationFunctionModifier::_sourceProperty1);
	INIT_PROPERTY_FIELD(CorrelationFunctionModifier::_sourceProperty2);
	INIT_PROPERTY_FIELD(CorrelationFunctionModifier::_cutoff);
	INIT_PROPERTY_FIELD(CorrelationFunctionModifier::_numberOfBins);
}


/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void CorrelationFunctionModifier::initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp)
{
	ParticleModifier::initializeModifier(pipeline, modApp);

	// Use the first available particle property from the input state as data source when the modifier is newly created.
	if(sourceProperty1().isNull() || sourceProperty2().isNull()) {
		PipelineFlowState input = pipeline->evaluatePipeline(dataset()->animationSettings()->time(), modApp, false);
		ParticlePropertyReference bestProperty;
		for(DataObject* o : input.objects()) {
			ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(o);
			if(property && (property->dataType() == qMetaTypeId<int>() || property->dataType() == qMetaTypeId<FloatType>())) {
				bestProperty = ParticlePropertyReference(property, (property->componentCount() > 1) ? 0 : -1);
			}
		}
		if(!bestProperty.isNull()) {
			if (sourceProperty1().isNull())
				setSourceProperty1(bestProperty);
			if (sourceProperty2().isNull())
				setSourceProperty2(bestProperty);
		}
	}
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> CorrelationFunctionModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	qDebug() << "CorrelationFunctionModifier::createEngine";

	// Get the source
	if(sourceProperty1().isNull())
		throwException(tr("Select a first particle property first."));
	if(sourceProperty2().isNull())
		throwException(tr("Select a second particle property first."));

	// Get the current positions.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);

	// Get the current selected properties.
	ParticlePropertyObject* property1 = sourceProperty1().findInState(input());
	ParticlePropertyObject* property2 = sourceProperty2().findInState(input());

	// Get simulation cell.
	SimulationCellObject* inputCell = expectSimulationCell();

	qDebug() << posProperty << property1 << property2;

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<CorrelationAnalysisEngine>(validityInterval,
													   posProperty->storage(),
													   property1->storage(),
													   property2->storage(),
													   inputCell->data(),
													   cutoff());
}

/******************************************************************************
* Map property onto grid.
******************************************************************************/
void CorrelationFunctionModifier::CorrelationAnalysisEngine::mapToSpatialGrid(ParticleProperty *property,
																			  size_t propertyVectorComponent,
																			  int nX, int nY, int nZ,
																			  QVector<double> &gridData)
{
	size_t vecComponent = std::max(size_t(0), propertyVectorComponent);
	size_t vecComponentCount = property->componentCount();

	int numberOfGridPoints = nX*nY*nZ;

	// Resize real space grid.
	gridData.resize(numberOfGridPoints);

	// Get periodic boundary flag.
	std::array<bool, 3> pbc = cell().pbcFlags();

	// Get reciprocal cell.
    AffineTransformation reciprocalCell = cell().inverseMatrix();

	if(property->size() > 0) {
		const Point3* pos = positions()->constDataPoint3();
		const Point3* pos_end = pos + positions()->size();

		if(property->dataType() == qMetaTypeId<FloatType>()) {
			const FloatType* v = property->constDataFloat() + vecComponent;
			const FloatType* v_end = v + (property->size() * vecComponentCount);
            for(; v != v_end; v += vecComponentCount, ++pos) {
                if(!std::isnan(*v)) {
                	Point3 fractionalPos = reciprocalCell*(*pos);
                    int binIndexX = int( fractionalPos.x() * nX );
                    int binIndexY = int( fractionalPos.y() * nY );
                    int binIndexZ = int( fractionalPos.z() * nZ );
                    if(pbc[0]) binIndexX = SimulationCell::modulo(binIndexX, nX);
                    if(pbc[1]) binIndexY = SimulationCell::modulo(binIndexY, nY);
                    if(pbc[2]) binIndexZ = SimulationCell::modulo(binIndexZ, nZ);
                    if(binIndexX >= 0 && binIndexX < nX && binIndexY >= 0 && binIndexY < nY && binIndexZ >= 0 && binIndexZ < nZ) {
                        size_t binIndex = (binIndexX+binIndexY*nX)*nZ+binIndexZ;
                        gridData[binIndex] += *v;
                    }
                }
            }
		}
	}
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void CorrelationFunctionModifier::CorrelationAnalysisEngine::perform()
{
	qDebug() << "CorrelationFunctionModifier::CorrelationAnalysisEngine::perform";

	setProgressText(tr("Computing correlation function"));

/*
	QVector<double> gridProperty1, gridProperty2;
	mapToSpatialGrid(sourceProperty1(),
				     0, // FIXME! Selected vector component should be passed to engine.
				     20, 20, 20,
				     gridProperty1);
	mapToSpatialGrid(sourceProperty2(),
				     0, // FIXME! Selected vector component should be passed to engine.
				     20, 20, 20,
				     gridProperty2);
*/

	int n = 20;
	_realSpaceCorrelationFunction.resize(n);
	_realSpaceCorrelationFunctionX.resize(n);
	for (int i = 0; i < n; i++) {
		_realSpaceCorrelationFunction[i] = sin(i);
		_realSpaceCorrelationFunctionX[i] = i;
	}
	qDebug() << "r" << _realSpaceCorrelationFunctionX;
	qDebug() << "q" << _realSpaceCorrelationFunction;
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void CorrelationFunctionModifier::transferComputationResults(ComputeEngine* engine)
{	
	qDebug() << "CorrelationFunctionModifier::transferComputationResults";

	CorrelationAnalysisEngine* eng = static_cast<CorrelationAnalysisEngine*>(engine);
	qDebug() << "rr" << eng->realSpaceCorrelationFunction();
	_realSpaceCorrelationFunction = eng->realSpaceCorrelationFunction();
	_realSpaceCorrelationFunctionX = eng->realSpaceCorrelationFunctionX();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus CorrelationFunctionModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	qDebug() << "CorrelationFunctionModifier::applyComputationResults";
	return PipelineStatus::Success;
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void CorrelationFunctionModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	qDebug() << "CorrelationFunctionModifier::propertyChanged";

	AsynchronousParticleModifier::propertyChanged(field);

	// Recompute modifier results when the parameters have been changed.
	if(field == PROPERTY_FIELD(CorrelationFunctionModifier::_cutoff) ||
			field == PROPERTY_FIELD(CorrelationFunctionModifier::_numberOfBins))
		invalidateCachedResults();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
