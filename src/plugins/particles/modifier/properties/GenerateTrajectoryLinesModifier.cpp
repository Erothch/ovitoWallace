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

#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/ParticlesObject.h>
#include <plugins/particles/objects/TrajectoryObject.h>
#include <core/dataset/animation/AnimationSettings.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/app/Application.h>
#include <core/dataset/io/FileSource.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/utilities/concurrent/Promise.h>
#include <core/utilities/concurrent/TaskManager.h>
#include "GenerateTrajectoryLinesModifier.h"

namespace Ovito { namespace Particles {

IMPLEMENT_OVITO_CLASS(GenerateTrajectoryLinesModifier);	
DEFINE_PROPERTY_FIELD(GenerateTrajectoryLinesModifier, onlySelectedParticles);
DEFINE_PROPERTY_FIELD(GenerateTrajectoryLinesModifier, useCustomInterval);
DEFINE_PROPERTY_FIELD(GenerateTrajectoryLinesModifier, customIntervalStart);
DEFINE_PROPERTY_FIELD(GenerateTrajectoryLinesModifier, customIntervalEnd);
DEFINE_PROPERTY_FIELD(GenerateTrajectoryLinesModifier, everyNthFrame);
DEFINE_PROPERTY_FIELD(GenerateTrajectoryLinesModifier, unwrapTrajectories);
DEFINE_REFERENCE_FIELD(GenerateTrajectoryLinesModifier, trajectoryVis);
SET_PROPERTY_FIELD_LABEL(GenerateTrajectoryLinesModifier, onlySelectedParticles, "Only selected particles");
SET_PROPERTY_FIELD_LABEL(GenerateTrajectoryLinesModifier, useCustomInterval, "Custom time interval");
SET_PROPERTY_FIELD_LABEL(GenerateTrajectoryLinesModifier, customIntervalStart, "Custom interval start");
SET_PROPERTY_FIELD_LABEL(GenerateTrajectoryLinesModifier, customIntervalEnd, "Custom interval end");
SET_PROPERTY_FIELD_LABEL(GenerateTrajectoryLinesModifier, everyNthFrame, "Every Nth frame");
SET_PROPERTY_FIELD_LABEL(GenerateTrajectoryLinesModifier, unwrapTrajectories, "Unwrap trajectories");
SET_PROPERTY_FIELD_UNITS(GenerateTrajectoryLinesModifier, customIntervalStart, TimeParameterUnit);
SET_PROPERTY_FIELD_UNITS(GenerateTrajectoryLinesModifier, customIntervalEnd, TimeParameterUnit);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GenerateTrajectoryLinesModifier, everyNthFrame, IntegerParameterUnit, 1);

IMPLEMENT_OVITO_CLASS(GenerateTrajectoryLinesModifierApplication);
DEFINE_REFERENCE_FIELD(GenerateTrajectoryLinesModifierApplication, trajectoryData);
SET_MODIFIER_APPLICATION_TYPE(GenerateTrajectoryLinesModifier, GenerateTrajectoryLinesModifierApplication);

/******************************************************************************
* Constructor.
******************************************************************************/
GenerateTrajectoryLinesModifier::GenerateTrajectoryLinesModifier(DataSet* dataset) : Modifier(dataset),
	_onlySelectedParticles(true), 
	_useCustomInterval(false),
	_customIntervalStart(dataset->animationSettings()->animationInterval().start()),
	_customIntervalEnd(dataset->animationSettings()->animationInterval().end()),
	_everyNthFrame(1), 
	_unwrapTrajectories(true)
{
	// Create the vis element for rendering the trajectories created by the modifier.
	setTrajectoryVis(new TrajectoryVis(dataset));
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool GenerateTrajectoryLinesModifier::OOMetaClass::isApplicableTo(const DataCollection& input) const
{
	return input.containsObject<ParticlesObject>();
}

/******************************************************************************
* Modifies the input data in an immediate, preliminary way.
******************************************************************************/
void GenerateTrajectoryLinesModifier::evaluatePreliminary(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	// Inject the precomputed trajectory lines, which are stored in the modifier application, into the pipeline.
	if(GenerateTrajectoryLinesModifierApplication* myModApp = dynamic_object_cast<GenerateTrajectoryLinesModifierApplication>(modApp)) {
		if(myModApp->trajectoryData()) {
			state.addObject(myModApp->trajectoryData());	
		}
	}
}

/******************************************************************************
* Updates the stored trajectories from the source particle object.
******************************************************************************/
bool GenerateTrajectoryLinesModifier::generateTrajectories(AsyncOperation&& operation)
{
	for(ModifierApplication* modApp : modifierApplications()) {
		GenerateTrajectoryLinesModifierApplication* myModApp = dynamic_object_cast<GenerateTrajectoryLinesModifierApplication>(modApp);
		if(!myModApp) continue;

		TimePoint currentTime = dataset()->animationSettings()->time();

		// Get input particles.
		SharedFuture<PipelineFlowState> stateFuture = myModApp->evaluateInput(currentTime);
		if(!operation.waitForFuture(stateFuture))
			return false;

		const PipelineFlowState& state = stateFuture.result();
		const ParticlesObject* particles = state.getObject<ParticlesObject>();
		if(!particles)
			throwException(tr("Cannot generate trajectory lines. The pipeline input contains no particles."));
		const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
		const PropertyObject* selectionProperty = particles->getProperty(ParticlesObject::SelectionProperty);
		const PropertyObject* identifierProperty = particles->getProperty(ParticlesObject::IdentifierProperty);

		// Determine set of input particles.
		std::vector<size_t> selectedIndices;
		std::set<qlonglong> selectedIdentifiers;
		size_t particleCount = 0;
		if(onlySelectedParticles()) {
			if(selectionProperty) {
				if(identifierProperty && identifierProperty->size() == selectionProperty->size()) {
					const int* s = selectionProperty->constDataInt();
					for(auto id : identifierProperty->constInt64Range())
						if(*s++) selectedIdentifiers.insert(id);
					particleCount = selectedIdentifiers.size();
				}
				else {
					const int* s = selectionProperty->constDataInt();
					for(size_t index = 0; index < selectionProperty->size(); index++)
						if(*s++) selectedIndices.push_back(index);
					particleCount = selectedIndices.size();
				}
			}
		}
		else {
			if(identifierProperty) {
				for(auto id : identifierProperty->constInt64Range())
					selectedIdentifiers.insert(id);
				particleCount = selectedIdentifiers.size();
			}
			else {
				selectedIndices.resize(posProperty->size());
				std::iota(selectedIndices.begin(), selectedIndices.end(), size_t(0));
				particleCount = selectedIndices.size();
			}
		}

		// Determine time interval over which trajectories should be generated.
		TimeInterval interval;
		if(useCustomInterval())
			interval = customInterval();
		else if(FileSource* fs = dynamic_object_cast<FileSource>(myModApp->pipelineSource()))
			interval = TimeInterval(0, myModApp->sourceFrameToAnimationTime(fs->numberOfFrames() - 1));
		else 
			interval = dataset()->animationSettings()->animationInterval();

		if(interval.duration() <= 0)
			throwException(tr("Loaded simulation sequence consists only of a single frame. No trajectory lines were created."));

		// Generate list of simulation frames at which particle positions should be sampled.
		QVector<TimePoint> sampleTimes;
		for(TimePoint time = interval.start(); time <= interval.end(); time += everyNthFrame() * dataset()->animationSettings()->ticksPerFrame()) {
			sampleTimes.push_back(time);
		}
		operation.setProgressMaximum(sampleTimes.size());

		// Sample particle positions to generate trajectory points.
		QVector<Point3> points;
		points.reserve(particleCount * sampleTimes.size());
		for(TimePoint time : sampleTimes) {
			operation.setProgressText(tr("Generating trajectory (frame %1 of %2)").arg(operation.progressValue()+1).arg(operation.progressMaximum()));

			SharedFuture<PipelineFlowState> stateFuture = myModApp->evaluateInput(time);
			if(!operation.waitForFuture(stateFuture))
				return false;
		
			const PipelineFlowState& state = stateFuture.result();
			const ParticlesObject* particles = state.getObject<ParticlesObject>();
			if(!particles)
				throwException(tr("Input particle set is empty at frame %1.").arg(dataset()->animationSettings()->timeToFrame(time)));
			const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);

			if(!onlySelectedParticles() && posProperty->size() != particleCount)
				throwException(tr("The current program version cannot create trajectory lines when the number of particles changes over time."));

			if(!selectedIdentifiers.empty()) {
				const PropertyObject* identifierProperty = particles->getProperty(ParticlesObject::IdentifierProperty);
				if(!identifierProperty || identifierProperty->size() != posProperty->size())
					throwException(tr("Input particles do not possess identifiers at frame %1.").arg(dataset()->animationSettings()->timeToFrame(time)));

				// Create a mapping from IDs to indices.
				std::map<qlonglong,size_t> idmap;
				size_t index = 0;
				for(auto id : identifierProperty->constInt64Range())
					idmap.insert(std::make_pair(id, index++));

				for(auto id : selectedIdentifiers) {
					auto entry = idmap.find(id);
					if(entry == idmap.end())
						throwException(tr("Input particle with ID=%1 does not exist at frame %2. This program version cannot create trajectory lines when the number of particles changes over time.").arg(id).arg(dataset()->animationSettings()->timeToFrame(time)));
					points.push_back(posProperty->getPoint3(entry->second));
				}
			}
			else {
				for(auto index : selectedIndices) {
					if(index >= posProperty->size())
						throwException(tr("Input particle at index %1 does not exist at frame %2. This program version cannot create trajectory lines when the number of particles changes over time.").arg(index+1).arg(dataset()->animationSettings()->timeToFrame(time)));
					points.push_back(posProperty->getPoint3(index));
				}
			}

			// Unwrap trajectory points at periodic boundaries of the simulation cell.
			if(unwrapTrajectories() && points.size() > particleCount) {
				if(const SimulationCellObject* simCellObj = state.getObject<SimulationCellObject>()) {
					const SimulationCell& cell = simCellObj->data();
					if(cell.pbcFlags() != std::array<bool,3>{false, false, false}) {
						auto previousPos = points.cbegin() + (points.size() - 2 * particleCount);
						auto currentPos = points.begin() + (points.size() - particleCount);
						for(size_t i = 0; i < particleCount; i++, ++previousPos, ++currentPos) {
							Vector3 delta = cell.wrapVector(*currentPos - *previousPos);
							*currentPos = *previousPos + delta;
						}
						OVITO_ASSERT(currentPos == points.end());
					}
				}
			}

			operation.incrementProgressValue(1);
			if(operation.isCanceled()) 
				return false;
		}

		// Store generated trajectory lines in the modifier application.
		OORef<TrajectoryObject> trajObj = new TrajectoryObject(dataset());
		trajObj->setTrajectories(particleCount, points, sampleTimes);
		trajObj->setVisElement(trajectoryVis());
		myModApp->setTrajectoryData(trajObj);
	}
	return true;
}

}	// End of namespace
}	// End of namespace
