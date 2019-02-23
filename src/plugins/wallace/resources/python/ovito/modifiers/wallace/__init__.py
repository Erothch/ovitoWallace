# Load dependencies
import ovito.modifiers
import ovito.modifiers.particles

# Load the native code modules.
import ovito.plugins.Particles
from ovito.plugins.Wallace import AtomicStrainModBurgers, CalculateElasticDisplacementsModifier, CalculateElasticStabilityModifier

# Inject modifier classes into parent module.
ovito.modifiers.AtomicStrainModBurgers = ovito.plugins.Wallace.AtomicStrainModBurgers
ovito.modifiers.CalculateElasticDisplacementsModifier = ovito.plugins.Wallace.CalculateElasticDisplacementsModifier
ovito.modifiers.CalculateElasticStabilityModifier = ovito.plugins.Wallace.CalculateElasticStabilityModifier

ovito.modifiers.__all__ += ['AtomicStrainModBurgers', 'CalculateElasticDisplacementsModifier',
            'CalculateElasticStabilityModifier']

# Copy enum list.
