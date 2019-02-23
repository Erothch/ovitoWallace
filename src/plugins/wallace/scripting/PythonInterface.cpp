#include <plugins/wallace/Wallace.h>
#include <plugins/wallace/modifier/atomic_strain_mod_burgers/AtomicStrainModBurgers.h>
#include <plugins/wallace/modifier/elastic_displacements/CalculateElasticDisplacementsModifier.h>
#include <plugins/wallace/modifier/elastic_stability/CalculateElasticStabilityModifier.h>
#include <plugins/wallace/modifier/elastic_stability/ElasticConstants.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <plugins/particles/scripting/PythonBinding.h>
#include <core/app/PluginManager.h>

namespace Ovito { namespace Plugins { namespace Wallace {

using namespace PyScript;

PYBIND11_MODULE(Wallace, m)
{
    // Register the classes of this plugin with the global PluginManager.
    PluginManager::instance().registerLoadedPluginClasses();

    py::options options;
    options.disable_function_signatures();


    ovito_class<AtomicStrainModBurgers, ReferenceConfigurationModifier>(m,
            ":Base class: :py:class:`ovito.pipeline.ReferenceConfigurationModifier`"
            "\n\n"
            "Computes the atomic-level deformation with respect to a reference configuration. "
            "See also the corresponding `user manual page <../../particles.modifiers.atomic_strain.html>`__ for this modifier. "
            "\n\n"
            "This modifier class inherits from :py:class:`~ovito.pipeline.ReferenceConfigurationModifier`, which provides "
            "various properties that control how the reference configuration is specified and also how particle displacements "
            "are calculated. "
            "By default, frame 0 of the current simulation sequence is used as reference configuration. "
            "\n\n"
            "**Modifier outputs:**"
            "\n\n"
            " * ``Shear Strain`` (:py:class:`~ovito.data.ParticleProperty`):\n"
            "   The *von Mises* shear strain invariant of the atomic Green-Lagrangian strain tensor.\n"
            " * ``Volumetric Strain`` (:py:class:`~ovito.data.ParticleProperty`):\n"
            "   One third of the trace of the atomic Green-Lagrangian strain tensor.\n"
            " * ``Strain Tensor`` (:py:class:`~ovito.data.ParticleProperty`):\n"
            "   The six components of the symmetric Green-Lagrangian strain tensor.\n"
            "   Output of this property must be explicitly enabled with the :py:attr:`.output_strain_tensors` flag.\n"
            " * ``Deformation Gradient`` (:py:class:`~ovito.data.ParticleProperty`):\n"
            "   The nine components of the atomic deformation gradient tensor.\n"
            "   Output of this property must be explicitly enabled with the :py:attr:`.output_deformation_gradients` flag.\n"
            " * ``Stretch Tensor`` (:py:class:`~ovito.data.ParticleProperty`):\n"
            "   The six components of the symmetric right stretch tensor U in the polar decomposition F=RU.\n"
            "   Output of this property must be explicitly enabled with the :py:attr:`.output_stretch_tensors` flag.\n"
            " * ``Rotation`` (:py:class:`~ovito.data.ParticleProperty`):\n"
            "   The atomic microrotation obtained from the polar decomposition F=RU as a quaternion.\n"
            "   Output of this property must be explicitly enabled with the :py:attr:`.output_rotations` flag.\n"
            " * ``Nonaffine Squared Displacement`` (:py:class:`~ovito.data.ParticleProperty`):\n"
            "   The D\\ :sup:`2`\\ :sub:`min` measure of Falk & Langer, which describes the non-affine part of the local deformation.\n"
            "   Output of this property must be explicitly enabled with the :py:attr:`.output_nonaffine_squared_displacements` flag.\n"
            " * ``Selection`` (:py:class:`~ovito.data.ParticleProperty`):\n"
            "   The modifier can select those particles for which a local deformation could not be computed because there were not\n"
            "   enough neighbors within the :py:attr:`.cutoff` range. Those particles with invalid deformation values can subsequently be removed using the\n"
            "   :py:class:`DeleteSelectedParticlesModifier`, for example. Selection of invalid particles is controlled by the :py:attr:`.select_invalid_particles` flag.\n"
            " * ``AtomicStrain.invalid_particle_count`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
            "   The number of particles for which the local strain calculation failed because they had not enough neighbors within the :py:attr:`.cutoff` range.\n"
            )
        .def_property("cutoff", &AtomicStrainModBurgers::cutoff, &AtomicStrainModBurgers::setCutoff,
                "Sets the distance up to which neighbor atoms are taken into account in the local strain calculation."
                "\n\n"
                ":Default: 3.0\n")
        .def_property("output_deformation_gradients", &AtomicStrainModBurgers::calculateDeformationGradients, &AtomicStrainModBurgers::setCalculateDeformationGradients,
                "Controls the output of the per-particle deformation gradient tensors. If ``False``, the computed tensors are not output as a particle property to save memory."
                "\n\n"
                ":Default: ``False``\n")
        .def_property("output_strain_tensors", &AtomicStrainModBurgers::calculateStrainTensors, &AtomicStrainModBurgers::setCalculateStrainTensors,
                "Controls the output of the per-particle strain tensors. If ``False``, the computed strain tensors are not output as a particle property to save memory."
                "\n\n"
                ":Default: ``False``\n")
        .def_property("output_stretch_tensors", &AtomicStrainModBurgers::calculateStretchTensors, &AtomicStrainModBurgers::setCalculateStretchTensors,
                "Flag that controls the calculation of the per-particle stretch tensors."
                "\n\n"
                ":Default: ``False``\n")
        .def_property("output_rotations", &AtomicStrainModBurgers::calculateRotations, &AtomicStrainModBurgers::setCalculateRotations,
                "Flag that controls the calculation of the per-particle rotations."
                "\n\n"
                ":Default: ``False``\n")
        .def_property("output_nonaffine_squared_displacements", &AtomicStrainModBurgers::calculateNonaffineSquaredDisplacements, &AtomicStrainModBurgers::setCalculateNonaffineSquaredDisplacements,
                "Enables the computation of the squared magnitude of the non-affine part of the atomic displacements. The computed values are output in the ``\"Nonaffine Squared Displacement\"`` particle property."
                "\n\n"
                ":Default: ``False``\n")
        .def_property("select_invalid_particles", &AtomicStrainModBurgers::selectInvalidParticles, &AtomicStrainModBurgers::setSelectInvalidParticles,
                "If ``True``, the modifier selects the particle for which the local strain tensor could not be computed (because of an insufficient number of neighbors within the cutoff)."
                "\n\n"
                ":Default: ``True``\n")
        .def_property("burgersContent", &AtomicStrainModBurgers::burgersContent, &AtomicStrainModBurgers::setBurgersContent,
                "Set the burgers vector to be disincluded from the atomic strain calculation."
                "\n\n"
                ":Default: `{0, 0, 0}`\n")
    ;



    ovito_class<CalculateElasticDisplacementsModifier, ReferenceConfigurationModifier>(m,
            ":Base class: :py:class:`ovito.pipeline.ReferenceConfigurationModifier`"
            "\n\n"
            "Computes the displacement vectors of particles with respect to a reference configuration. "
            "See also the corresponding `user manual page <../../particles.modifiers.displacement_vectors.html>`__ for this modifier. "
            "\n\n"
            "This modifier class inherits from :py:class:`~ovito.pipeline.ReferenceConfigurationModifier`, which provides "
            "various properties that control how the reference configuration is specified and also how displacement "
            "vectors are calculated. "
            "By default, frame 0 of the current simulation sequence is used as reference configuration. "
            "\n\n"
            "**Modifier outputs:**"
            "\n\n"
            " * ``Displacement`` (:py:class:`~ovito.data.ParticleProperty`):\n"
            "   The computed displacement vectors\n"
            " * ``Displacement Magnitude`` (:py:class:`~ovito.data.ParticleProperty`):\n"
            "   The length of the computed displacement vectors\n"
            "\n\n")
        .def_property("vis", &CalculateElasticDisplacementsModifier::vectorVis, &CalculateElasticDisplacementsModifier::setVectorVis,
                "A :py:class:`~ovito.vis.VectorVis` element controlling the visual representation of the computed "
                "displacement vectors. "
                "Note that the computed displacement vectors are hidden by default. You can enable "
                "the visualization of arrows as follows: "
                "\n\n"
                ".. literalinclude:: ../example_snippets/calculate_displacements.py\n"
                "   :lines: 4-\n")
        .def_property("burgersContent", &CalculateElasticDisplacementsModifier::burgersContent, &CalculateElasticDisplacementsModifier::setBurgersContent,
                "Set the burgers vector to be disincluded from the atomic strain calculation."
                "\n\n"
                ":Default: `{0, 0, 0}`\n")
    ;


    auto CalculateElasticStabilityModifier_py = ovito_class<CalculateElasticStabilityModifier, AsynchronousModifier>(m,
        ":Base class: :py:class:`ovito.pipeline.AsynchronousModifier`"
        "\n\n"
        "Computes an elastic stability parameter based on the minimum eigenvalues of the symmetric wallace tensor at finite deformation"
        "\n\n"
        "**Modifier outputs**"
        "\n\n",
        "CalculateElasticStabilityModifier")
       .def_property("set_structure", &CalculateElasticStabilityModifier::structure, &CalculateElasticStabilityModifier::setStructure,
                     "Choose the structure type by Laue group. "
                     "Currently must be one of the following: \n"
                     "Hexagonal_High \n"
                     "Cubic_High \n")
       .def_property("set_soec", &CalculateElasticStabilityModifier::soec, &CalculateElasticStabilityModifier::setSoec,
                    "Sets the second order elastic constants. "
                     "Must be in standard reference framen \n"
                     "Will add details on reference frame later")
       .def_property("set_toec", &CalculateElasticStabilityModifier::toec, &CalculateElasticStabilityModifier::setToec,
                    "Sets the second order elastic constants. "
                    "Must be in standard reference frame. \n"
                    "Will add details on reference frame later")
       .def_property("set_transformation_matrix", &CalculateElasticStabilityModifier::CTransformation, &CalculateElasticStabilityModifier::setCTransformation,
                     "Set a transformation matrix "
                     "to go from standard to current frame. "
                     "This just rotates the elastic constants.")

    ;



    py::enum_<ElasticConstants::LaueGroups>(CalculateElasticStabilityModifier_py, "Lattice")
        .value("Hexagonal_High", ElasticConstants::HEXAGONAL_HIGH)
        .value("Cubic_High", ElasticConstants::CUBIC_HIGH)
    ;


}



OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(Wallace);

}

}

}
