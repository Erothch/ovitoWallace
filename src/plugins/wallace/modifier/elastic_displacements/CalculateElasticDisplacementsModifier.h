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
#include <plugins/particles/objects/VectorVis.h>
#include <plugins/particles/objects/ParticlesObject.h>
#include <plugins/particles/modifier/analysis/ReferenceConfigurationModifier.h>
#include <plugins/particles/util/ParticleOrderingFingerprint.h>
#include <plugins/stdobj/simcell/SimulationCell.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief Calculates the per-particle displacement vectors based on a reference configuration.
 */
class OVITO_PARTICLES_EXPORT CalculateElasticDisplacementsModifier : public ReferenceConfigurationModifier
{
	Q_OBJECT
	OVITO_CLASS(CalculateElasticDisplacementsModifier)

	Q_CLASSINFO("DisplayName", "Elastic Displacement vectors");
	Q_CLASSINFO("ModifierCategory", "Analysis");

public:

	/// Constructor.
	Q_INVOKABLE CalculateElasticDisplacementsModifier(DataSet* dataset);

protected:

	/// Creates a computation engine that will compute the modifier's results.
	virtual Future<ComputeEnginePtr> createEngineWithReference(TimePoint time, ModifierApplication* modApp, PipelineFlowState input, const PipelineFlowState& referenceState, TimeInterval validityInterval) override;

private:

	/// Computes the modifier's results.
	class ElasticDisplacementEngine : public RefConfigEngineBase
	{
	public:


		/// Constructor.
		ElasticDisplacementEngine(const TimeInterval& validityInterval, ConstPropertyPtr positions, const SimulationCell& simCell,
				ParticleOrderingFingerprint fingerprint,
				ConstPropertyPtr refPositions, const SimulationCell& simCellRef,
				ConstPropertyPtr identifiers, ConstPropertyPtr refIdentifiers,
                AffineMappingType affineMapping, bool useMinimumImageConvention, Vector3 burgersContent) : // Eric addition vector3
			RefConfigEngineBase(validityInterval, positions, simCell, std::move(refPositions), simCellRef,
				std::move(identifiers), std::move(refIdentifiers), affineMapping, useMinimumImageConvention),
			_elasticDisplacements(ParticlesObject::OOClass().createStandardStorage(fingerprint.particleCount(), ParticlesObject::DisplacementProperty, false)),
			_elasticDisplacementMagnitudes(ParticlesObject::OOClass().createStandardStorage(fingerprint.particleCount(), ParticlesObject::ElasticDisplacementMagnitudeProperty, false)),
            _inputFingerprint(std::move(fingerprint)),
            _burgersContent(burgersContent) {}

		/// Computes the modifier's results.
		virtual void perform() override;

		/// Injects the computed results into the data pipeline.
		virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

		/// Returns the property storage that contains the computed displacement vectors.
		const PropertyPtr& elasticDisplacements() const { return _elasticDisplacements; }
		
		/// Returns the property storage that contains the computed displacement vector magnitudes.
		const PropertyPtr& elasticDisplacementMagnitudes() const { return _elasticDisplacementMagnitudes; }
		
	private:

		const PropertyPtr _elasticDisplacements;
		const PropertyPtr _elasticDisplacementMagnitudes;
		ParticleOrderingFingerprint _inputFingerprint;
        const Vector3 _burgersContent;
	};

	/// The vis element for rendering the displacement vectors.
	DECLARE_MODIFIABLE_REFERENCE_FIELD_FLAGS(VectorVis, vectorVis, setVectorVis, PROPERTY_FIELD_DONT_PROPAGATE_MESSAGES | PROPERTY_FIELD_MEMORIZE);
    DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(Vector3, burgersContent, setBurgersContent, PROPERTY_FIELD_MEMORIZE);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
