from ovito.modifiers import CalculateDisplacementsModifier
from ovito.pipeline import FileSource

# The modifier that requires a reference config:
mod = CalculateDisplacementsModifier()

# Load the reference config from a separate input file.
mod.reference = FileSource()
mod.reference.load('input/simulation.0.dump')
