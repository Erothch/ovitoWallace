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
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <plugins/particles/objects/BondsVis.h>
#include <plugins/particles/modifier/ParticleInputHelper.h>
#include <plugins/particles/modifier/ParticleOutputHelper.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include <core/utilities/units/UnitsManager.h>
#include "CreateBondsModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_OVITO_CLASS(CreateBondsModifier);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, cutoffMode);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, uniformCutoff);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, pairCutoffs);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, minimumCutoff);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, onlyIntraMoleculeBonds);
DEFINE_REFERENCE_FIELD(CreateBondsModifier, bondsVis);
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, cutoffMode, "Cutoff mode");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, uniformCutoff, "Cutoff radius");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, pairCutoffs, "Pair-wise cutoffs");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, minimumCutoff, "Lower cutoff");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, onlyIntraMoleculeBonds, "Suppress inter-molecular bonds");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CreateBondsModifier, uniformCutoff, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CreateBondsModifier, minimumCutoff, WorldParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CreateBondsModifier::CreateBondsModifier(DataSet* dataset) : AsynchronousModifier(dataset),
	_cutoffMode(UniformCutoff), 
	_uniformCutoff(3.2), 
	_onlyIntraMoleculeBonds(false), 
	_minimumCutoff(0)
{
	// Create the vis element for rendering the bonds generated by the modifier.
	setBondsVis(new BondsVis(dataset));
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool CreateBondsModifier::OOMetaClass::isApplicableTo(const PipelineFlowState& input) const
{
	return input.findObject<ParticleProperty>() != nullptr;
}

/******************************************************************************
* Sets the cutoff radius for a pair of particle types.
******************************************************************************/
void CreateBondsModifier::setPairCutoff(const QString& typeA, const QString& typeB, FloatType cutoff)
{
	PairCutoffsList newList = pairCutoffs();
	if(cutoff > 0) {
		newList[qMakePair(typeA, typeB)] = cutoff;
		newList[qMakePair(typeB, typeA)] = cutoff;
	}
	else {
		newList.remove(qMakePair(typeA, typeB));
		newList.remove(qMakePair(typeB, typeA));
	}
	setPairCutoffs(newList);
}

/******************************************************************************
* Returns the pair-wise cutoff radius for a pair of particle types.
******************************************************************************/
FloatType CreateBondsModifier::getPairCutoff(const QString& typeA, const QString& typeB) const
{
	auto iter = pairCutoffs().find(qMakePair(typeA, typeB));
	if(iter != pairCutoffs().end()) return iter.value();
	iter = pairCutoffs().find(qMakePair(typeB, typeA));
	if(iter != pairCutoffs().end()) return iter.value();
	return 0;
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void CreateBondsModifier::initializeModifier(ModifierApplication* modApp)
{
	AsynchronousModifier::initializeModifier(modApp);

	// Adopt the upstream BondsVis object if there already is one.
	PipelineFlowState input = modApp->evaluateInputPreliminary();
	if(BondProperty* topologyProperty = BondProperty::findInState(input, BondProperty::TopologyProperty)) {
		for(DataVis* vis : topologyProperty->visElements()) {
			if(BondsVis* bondsVis = dynamic_object_cast<BondsVis>(vis)) {
				setBondsVis(bondsVis);
				break;
			}
		}
	}
}


/******************************************************************************
* Creates and initializes a computation engine that will compute the 
* modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> CreateBondsModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	// Get modifier input.
	ParticleInputHelper ph(dataset(), input);
	ParticleProperty* posProperty = ph.expectStandardProperty<ParticleProperty>(ParticleProperty::PositionProperty);
	SimulationCellObject* simCell = ph.expectSimulationCell();

	// The neighbor list cutoff.
	FloatType maxCutoff = uniformCutoff();

	// Build table of pair-wise cutoff radii.
	ParticleProperty* typeProperty = nullptr;
	std::vector<std::vector<FloatType>> pairCutoffSquaredTable;
	if(cutoffMode() == PairCutoff) {
		typeProperty = ph.expectStandardProperty<ParticleProperty>(ParticleProperty::TypeProperty);
		if(typeProperty) {
			maxCutoff = 0;
			for(PairCutoffsList::const_iterator entry = pairCutoffs().begin(); entry != pairCutoffs().end(); ++entry) {
				FloatType cutoff = entry.value();
				if(cutoff > 0) {
					ElementType* ptype1 = typeProperty->elementType(entry.key().first);
					ElementType* ptype2 = typeProperty->elementType(entry.key().second);
					if(ptype1 && ptype2 && ptype1->id() >= 0 && ptype2->id() >= 0) {
						if((int)pairCutoffSquaredTable.size() <= std::max(ptype1->id(), ptype2->id())) pairCutoffSquaredTable.resize(std::max(ptype1->id(), ptype2->id()) + 1);
						if((int)pairCutoffSquaredTable[ptype1->id()].size() <= ptype2->id()) pairCutoffSquaredTable[ptype1->id()].resize(ptype2->id() + 1, FloatType(0));
						if((int)pairCutoffSquaredTable[ptype2->id()].size() <= ptype1->id()) pairCutoffSquaredTable[ptype2->id()].resize(ptype1->id() + 1, FloatType(0));
						pairCutoffSquaredTable[ptype1->id()][ptype2->id()] = cutoff * cutoff;
						pairCutoffSquaredTable[ptype2->id()][ptype1->id()] = cutoff * cutoff;
						if(cutoff > maxCutoff) maxCutoff = cutoff;
					}
				}
			}
			if(maxCutoff <= 0)
				throwException(tr("At least one positive bond cutoff must be set for a valid pair of particle types."));
		}
	}

	// Get molecule IDs.
	ParticleProperty* moleculeProperty = onlyIntraMoleculeBonds() ? ph.inputStandardProperty<ParticleProperty>(ParticleProperty::MoleculeProperty) : nullptr;

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<BondsEngine>(input, posProperty->storage(),
			typeProperty ? typeProperty->storage() : nullptr, simCell->data(), cutoffMode(),
			maxCutoff, minimumCutoff(), std::move(pairCutoffSquaredTable), moleculeProperty ? moleculeProperty->storage() : nullptr);
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void CreateBondsModifier::BondsEngine::perform()
{
	task()->setProgressText(tr("Generating bonds"));

	// Prepare the neighbor list.
	CutoffNeighborFinder neighborFinder;
	if(!neighborFinder.prepare(_maxCutoff, *_positions, _simCell, nullptr, task().get()))
		return;

	FloatType minCutoffSquared = _minCutoff * _minCutoff;

	// Generate bonds.
	size_t particleCount = _positions->size();
	task()->setProgressMaximum(particleCount);
	if(!_particleTypes) {
		for(size_t particleIndex = 0; particleIndex < particleCount; particleIndex++) {
			for(CutoffNeighborFinder::Query neighborQuery(neighborFinder, particleIndex); !neighborQuery.atEnd(); neighborQuery.next()) {
				if(neighborQuery.distanceSquared() < minCutoffSquared)
					continue;
				if(_moleculeIDs && _moleculeIDs->getInt64(particleIndex) != _moleculeIDs->getInt64(neighborQuery.current()))
					continue;

				Bond bond = { particleIndex, neighborQuery.current(), neighborQuery.unwrappedPbcShift() };

				// Skip every other bond to create only one bond per particle pair.
				if(!bond.isOdd())
					bonds().push_back(bond);
			}
			// Update progress indicator.
			if(!task()->setProgressValueIntermittent(particleIndex))
				return;
		}
	}
	else {
		for(size_t particleIndex = 0; particleIndex < particleCount; particleIndex++) {
			for(CutoffNeighborFinder::Query neighborQuery(neighborFinder, particleIndex); !neighborQuery.atEnd(); neighborQuery.next()) {
				if(neighborQuery.distanceSquared() < minCutoffSquared)
					continue;
				if(_moleculeIDs && _moleculeIDs->getInt64(particleIndex) != _moleculeIDs->getInt64(neighborQuery.current()))
					continue;
				int type1 = _particleTypes->getInt(particleIndex);
				int type2 = _particleTypes->getInt(neighborQuery.current());
				if(type1 >= 0 && type1 < (int)_pairCutoffsSquared.size() && type2 >= 0 && type2 < (int)_pairCutoffsSquared[type1].size()) {
					if(neighborQuery.distanceSquared() <= _pairCutoffsSquared[type1][type2]) {
						Bond bond = { particleIndex, neighborQuery.current(), neighborQuery.unwrappedPbcShift() };
						// Skip every other bond to create only one bond per particle pair.
						if(!bond.isOdd())				
							bonds().push_back(bond);
					}
				}
			}
			// Update progress indicator.
			if(!task()->setProgressValueIntermittent(particleIndex))
				return;
		}
	}
	task()->setProgressValue(particleCount);
}

/******************************************************************************
* Injects the computed results of the engine into the data pipeline.
******************************************************************************/
PipelineFlowState CreateBondsModifier::BondsEngine::emitResults(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
	if(_inputFingerprint.hasChanged(input))
		modApp->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));

	CreateBondsModifier* modifier = static_object_cast<CreateBondsModifier>(modApp->modifier());
	OVITO_ASSERT(modifier);	

	// Add our bonds to the system.
	PipelineFlowState output = input;
	ParticleOutputHelper poh(modApp->dataset(), output);

	poh.addBonds(bonds(), modifier->bondsVis());

	size_t bondsCount = bonds().size();
	poh.outputAttribute(QStringLiteral("CreateBonds.num_bonds"), QVariant::fromValue(bondsCount));

	// If the number of bonds is unusually high, we better turn off bonds display to prevent the program from freezing.
	if(bondsCount > 1000000 && modifier->bondsVis()) {
		modifier->bondsVis()->setEnabled(false);		
		output.setStatus(PipelineStatus(PipelineStatus::Warning, tr("Created %1 bonds, which is a lot. As a precaution, the display of bonds has been disabled. You can manually enable it again if needed.").arg(bondsCount)));
	}
	else {
		output.setStatus(PipelineStatus(PipelineStatus::Success, tr("Created %1 bonds.").arg(bondsCount)));
	}

	return output;	
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
