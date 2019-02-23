///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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


#include <plugins/wallace/Wallace.h>
#include <plugins/particles/modifier/analysis/ReferenceConfigurationModifier.h>
#include <plugins/particles/objects/ParticlesObject.h>
#include <plugins/particles/util/ParticleOrderingFingerprint.h>
#include <plugins/stdobj/simcell/SimulationCell.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief Calculates the per-particle strain tensors based on a reference configuration.
 */
class OVITO_PARTICLES_EXPORT AtomicStrainModBurgers : public ReferenceConfigurationModifier
{
	Q_OBJECT
    OVITO_CLASS(AtomicStrainModBurgers)

    Q_CLASSINFO("DisplayName", "Atomic strain mod burgers");
	Q_CLASSINFO("ModifierCategory", "Analysis");

public:

	/// Constructor.
    Q_INVOKABLE AtomicStrainModBurgers(DataSet* dataset);

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngineWithReference(TimePoint time, ModifierApplication* modApp, PipelineFlowState input, const PipelineFlowState& referenceState, TimeInterval validityInterval) override;
	
private:

	/// Computes the modifier's results.
	class AtomicStrainEngine : public RefConfigEngineBase
	{
	public:

		/// Constructor.
		//Add bool useElasticDeformation
		AtomicStrainEngine(const TimeInterval& validityInterval, ParticleOrderingFingerprint fingerprint, ConstPropertyPtr positions, const SimulationCell& simCell,
				ConstPropertyPtr refPositions, const SimulationCell& simCellRef,
				ConstPropertyPtr identifiers, ConstPropertyPtr refIdentifiers,
				FloatType cutoff, AffineMappingType affineMapping, bool useMinimumImageConvention,
				bool calculateDeformationGradients, bool calculateStrainTensors,
				bool calculateNonaffineSquaredDisplacements, bool calculateRotations, bool calculateStretchTensors,
                bool selectInvalidParticles, Vector3 burgersContent) :
			RefConfigEngineBase(validityInterval, positions, simCell, refPositions, simCellRef,
                std::move(identifiers), std::move(refIdentifiers), affineMapping, useMinimumImageConvention),
            _cutoff(cutoff),
			_displacements(ParticlesObject::OOClass().createStandardStorage(refPositions->size(), ParticlesObject::DisplacementProperty, false)),
			_shearStrains(std::make_shared<PropertyStorage>(fingerprint.particleCount(), PropertyStorage::Float, 1, 0, tr("Shear Strain"), false)),
			_volumetricStrains(std::make_shared<PropertyStorage>(fingerprint.particleCount(), PropertyStorage::Float, 1, 0, tr("Volumetric Strain"), false)),
			_strainTensors(calculateStrainTensors ? ParticlesObject::OOClass().createStandardStorage(fingerprint.particleCount(), ParticlesObject::StrainTensorProperty, false) : nullptr),
			_deformationGradients(calculateDeformationGradients ? ParticlesObject::OOClass().createStandardStorage(fingerprint.particleCount(), ParticlesObject::DeformationGradientProperty, false) : nullptr),
			_nonaffineSquaredDisplacements(calculateNonaffineSquaredDisplacements ? std::make_shared<PropertyStorage>(fingerprint.particleCount(), PropertyStorage::Float, 1, 0, tr("Nonaffine Squared Displacement"), false) : nullptr),
			_invalidParticles(selectInvalidParticles ? ParticlesObject::OOClass().createStandardStorage(fingerprint.particleCount(), ParticlesObject::SelectionProperty, false) : nullptr),
			_rotations(calculateRotations ? ParticlesObject::OOClass().createStandardStorage(fingerprint.particleCount(), ParticlesObject::RotationProperty, false) : nullptr),
			_stretchTensors(calculateStretchTensors ? ParticlesObject::OOClass().createStandardStorage(fingerprint.particleCount(), ParticlesObject::StretchTensorProperty, false) : nullptr),
            _inputFingerprint(std::move(fingerprint)),
            _burgersContent(burgersContent) {}

		/// This method is called by the system after the computation was successfully completed.
		virtual void cleanup() override {
			_displacements.reset();
			RefConfigEngineBase::cleanup();
		}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

		/// Returns the property storage that contains the computed per-particle shear strain values.
		const PropertyPtr& shearStrains() const { return _shearStrains; }

		/// Returns the property storage that contains the computed per-particle volumetric strain values.
		const PropertyPtr& volumetricStrains() const { return _volumetricStrains; }

		/// Returns the property storage that contains the computed per-particle strain tensors.
		const PropertyPtr& strainTensors() const { return _strainTensors; }

		/// Returns the property storage that contains the computed per-particle deformation gradient tensors.
		const PropertyPtr& deformationGradients() const { return _deformationGradients; }

		/// Returns the property storage that contains the computed per-particle deformation gradient tensors.
		const PropertyPtr& nonaffineSquaredDisplacements() const { return _nonaffineSquaredDisplacements; }

		/// Returns the property storage that contains the selection of invalid particles.
		const PropertyPtr& invalidParticles() const { return _invalidParticles; }

		/// Returns the property storage that contains the computed rotations.
		const PropertyPtr& rotations() const { return _rotations; }

		/// Returns the property storage that contains the computed stretch tensors.
		const PropertyPtr& stretchTensors() const { return _stretchTensors; }

		/// Returns the number of invalid particles for which the strain tensor could not be computed.
		size_t numInvalidParticles() const { return _numInvalidParticles.load(); }

		/// Increments the invalid particle counter by one.
		void addInvalidParticle() { _numInvalidParticles.fetchAndAddRelaxed(1); }

		/// Returns the property storage that contains the computed displacement vectors.
		const PropertyPtr& displacements() const { return _displacements; }
		
	private:

		/// Computes the strain tensor of a single particle.
		void computeStrain(size_t particleIndex, CutoffNeighborFinder& neighborListBuilder);

		const FloatType _cutoff;
		PropertyPtr _displacements;
		QAtomicInt _numInvalidParticles;
		const PropertyPtr _shearStrains;
		const PropertyPtr _volumetricStrains;
		const PropertyPtr _strainTensors;
		const PropertyPtr _deformationGradients;
		const PropertyPtr _nonaffineSquaredDisplacements;
		const PropertyPtr _invalidParticles;
		const PropertyPtr _rotations;
		const PropertyPtr _stretchTensors;		
		ParticleOrderingFingerprint _inputFingerprint;
        const Vector3 _burgersContent;
	};

	/// Controls the cutoff radius for the neighbor lists.
	DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(FloatType, cutoff, setCutoff, PROPERTY_FIELD_MEMORIZE);

	/// Controls the whether atomic deformation gradient tensors should be computed and stored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, calculateDeformationGradients, setCalculateDeformationGradients);

	/// Controls the whether atomic strain tensors should be computed and stored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, calculateStrainTensors, setCalculateStrainTensors);

	/// Controls the whether non-affine displacements should be computed and stored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, calculateNonaffineSquaredDisplacements, setCalculateNonaffineSquaredDisplacements);

	/// Controls the whether local rotations should be computed and stored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, calculateRotations, setCalculateRotations);

	/// Controls the whether atomic stretch tensors should be computed and stored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, calculateStretchTensors, setCalculateStretchTensors);

	/// Controls the whether particles, for which the strain tensor could not be computed, are selected.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, selectInvalidParticles, setSelectInvalidParticles);

    /// Allows an input Burgers Vector to remove from displacement
    DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(Vector3, burgersContent, setBurgersContent, PROPERTY_FIELD_MEMORIZE);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


