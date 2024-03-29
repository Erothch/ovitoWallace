# Load dependencies
import ovito.io.particles

# Load the native code modules
import ovito.plugins.CrystalAnalysis

# Register export formats.
ovito.io.export_file._formatTable["ca"] = ovito.plugins.CrystalAnalysis.CAExporter
ovito.io.export_file._formatTable["vtk/disloc"] = ovito.plugins.CrystalAnalysis.VTKDislocationsExporter
