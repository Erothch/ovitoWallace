# Load system libraries
import numpy

# Load dependencies
import ovito
import ovito.modifiers

# Load the native code module
from ovito.plugins.Particles import CoordinationAnalysisModifier

# Implement the 'rdf' attribute of the CoordinationAnalysisModifier class.
# This is for backward compatibility with OVITO 2.9.0:
def _CoordinationAnalysisModifier_rdf(self):
    """
    Returns a NumPy array containing the radial distribution function (RDF) computed by the modifier.    
    The returned array is two-dimensional and consists of the [*r*, *g(r)*, ...] data points of the tabulated *g(r)* RDF function(s).
    
    Note that accessing this array is only possible after the modifier has computed its results. 
    Thus, you have to call :py:meth:`Pipeline.compute() <ovito.pipeline.Pipeline.compute>` first to ensure that this information is up to date, see the example above.
    """
    return numpy.transpose(numpy.hstack((self.rdf_x[:,numpy.newaxis], self.rdf_y)))
CoordinationAnalysisModifier.rdf = property(_CoordinationAnalysisModifier_rdf)