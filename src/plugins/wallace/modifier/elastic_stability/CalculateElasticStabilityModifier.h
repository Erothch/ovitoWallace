///////////////////////////////////////////////////////////////////////////////
// Written 2018 by Eric for use in Ovito Wallace Plugin
//
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <plugins/wallace/Wallace.h>
#include <plugins/particles/objects/VectorVis.h>
#include <plugins/particles/objects/ParticlesObject.h>
#include <plugins/particles/util/ParticleOrderingFingerprint.h>
#include <core/utilities/linalg/AffineTransformation.h>
#include <plugins/stdobj/simcell/SimulationCell.h>
#include <core/dataset/pipeline/AsynchronousModifier.h>
#include "ElasticConstants.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
* \brief Calculate the Wallace tensor and elastic stability parameter based on the deformation gradient
*/
class OVITO_PARTICLES_EXPORT CalculateElasticStabilityModifier : public AsynchronousModifier
{
    Q_OBJECT
    OVITO_CLASS(CalculateElasticStabilityModifier)

    Q_CLASSINFO("DisplayName", "Elastic Stability");
    Q_CLASSINFO("ModifierCategory", "Analysis");

public:
    ///Constructor
    Q_INVOKABLE CalculateElasticStabilityModifier(DataSet* dataset);

protected:

    /// Creates a computation engine that will compute the modifier's results.
    virtual Future<ComputeEnginePtr> createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input) override;

private:

    ///computes the Modifier's results
    class ElasticStabilityEngine : public ComputeEngine
    {
        public:
            ElasticStabilityEngine(ParticleOrderingFingerprint fingerprint, ConstPropertyPtr positions,
                ConstPropertyPtr deformationGradient, ConstPropertyPtr strainTensors, const SimulationCell& simCell, const int structure,
                std::vector<float> soec, std::vector<float> toec, AffineTransformation(CTransformation)) :
                _positions(std::move(positions)),
                _strainTensors(std::move(strainTensors)),
                _deformationGradient(std::move(deformationGradient)),
                _wallaceTensor(std::make_shared<PropertyStorage>(fingerprint.particleCount(), PropertyStorage::Float, 21, 21 * sizeof(FloatType) , tr("Wallace Tensor"), false)),
                _stabilityParameter(std::make_shared<PropertyStorage>(fingerprint.particleCount(), PropertyStorage::Float, 1, 0, tr("Stability Parameter"), false)),
                _simCell(simCell),
                _inputFingerprint(std::move(fingerprint)),
                _structure(structure),
                _soec(soec),
                _toec(toec),
                _refStabPar(0),
                _stressTensor(std::make_shared<PropertyStorage>(fingerprint.particleCount(), PropertyStorage::Float, 6, 6 * sizeof(FloatType) , tr("Stress from elastic deformation"), false)),
                _soecDeformed(std::make_shared<PropertyStorage>(fingerprint.particleCount(), PropertyStorage::Float, 21, 21 * sizeof(FloatType) , tr("SOEC at finite deformation"), false)),
                _CTransformation(CTransformation)
            {}

            virtual void cleanup() override {
                _positions.reset();
                ComputeEngine::cleanup();
            }

            /// Computes the modifier's results.
            virtual void perform() override;

            /// Injects the computed results into the data pipeline.
            virtual void emitResults(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

            /// Returns the property storage that contains the computed per-particle wallace tensor.
            const PropertyPtr& wallaceTensor() const { return _wallaceTensor; }

            /// Returns the property storage that contains the computed per-particle stability parameter.
            const PropertyPtr& stabilityParameter() const { return _stabilityParameter; }

            /// Returns the property storage that contains the input particle positions.
            const ConstPropertyPtr& positions() const { return _positions; }

            /// Returns the property storage that contains the input particle positions.
            const ConstPropertyPtr& strainTensors() const { return _strainTensors; }

            /// Returns the property storage that contains the input particle positions.
            const ConstPropertyPtr& deformationGradient() const { return _deformationGradient; }

            /// Returns the simulation cell data.
            const SimulationCell& cell() const { return _simCell; }

            ///Returns the SOEC at finite deformation.
            const PropertyPtr& soecDeformed() const { return _soecDeformed; }

            ///Returns the stress calculated with second and third order elastic constants.
            const PropertyPtr& stressTensor() const { return _stressTensor; }

    private:    

        void computeElasticStability(size_t particleIndex);
        void computeDeformedElasticConstants(size_t particleIndex);
        void computeStress(size_t particleIndex);

        const SimulationCell _simCell;
        ConstPropertyPtr _positions;
        const PropertyPtr _wallaceTensor;
        const PropertyPtr _stabilityParameter;
        PropertyPtr _soecDeformed;
        PropertyPtr _stressTensor;
        ConstPropertyPtr _deformationGradient;
        ConstPropertyPtr _strainTensors;
        ParticleOrderingFingerprint _inputFingerprint;
        const std::vector<float> _soec;
        const std::vector<float> _toec;
        Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6>> _vC2;
        Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6, 6>> _vC3;
        //Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6>> _D1;
        //Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6, 6>> _D2;
        Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3>> _I4;
        const AffineTransformation _CTransformation;
        Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3>> _ETransformation;
        float _refStabPar;
        const int _structure; //This is the symmetry type of the input
        /* This uses the Laue group
         * 1 -> N (Triclinic)
         * 2 -> M (Monoclinic)
         * 3 -> O (Orthorhombic) etc.
         */
    };

   DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(std::vector<float>, soec, setSoec, PROPERTY_FIELD_MEMORIZE);
   DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(std::vector<float>, toec, setToec, PROPERTY_FIELD_MEMORIZE);
   DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(int, structure, setStructure, PROPERTY_FIELD_MEMORIZE);
   DECLARE_MODIFIABLE_PROPERTY_FIELD_FLAGS(AffineTransformation, CTransformation, setCTransformation, PROPERTY_FIELD_MEMORIZE);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE

}
}
