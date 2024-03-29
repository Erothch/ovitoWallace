"""
This example script demonstrates the use of the Voronoi analysis modifier.
It calculates the distribution of Voronoi polyhedra in a bulk metallic glass.

A Voronoi polyhedron is expressed in terms of the Schlaefli notation,
which is a vector of indices (n_1, n_2, n_3, n_4, n_5, n_6, ...),
where n_i is the number of polyhedron faces with i edges/vertices.

The script computes the distribution of these Voronoi index vectors
and lists the 10 most frequent polyhedron types in the simulation. In the case
of a 64%-Cu, 36%-Zr glass, the most frequent polyhedron is the icosahedron.
It has 12 faces with five edges each. Thus, the corresponding Voronoi index 
vector is:

   (0 0 0 0 12 0 ...)
 
"""

# Import OVITO modules.
from ovito.io import *
from ovito.modifiers import *

# Import NumPy module.
import numpy

# Load a simulation snapshot of a Cu-Zr metallic glass.
pipeline = import_file("../data/CuZr_metallic_glass.dump.gz")

# Set atomic radii (required for polydisperse Voronoi tessellation).
atom_types = pipeline.source.data.particles['Particle Type'].types
atom_types[0].radius = 1.35   # Cu atomic radius (atom type 1 in input file)
atom_types[1].radius = 1.55   # Zr atomic radius (atom type 2 in input file)

# Set up the Voronoi analysis modifier.
voro = VoronoiAnalysisModifier(
    compute_indices = True,
    use_radii = True,
    edge_threshold = 0.1
)
pipeline.modifiers.append(voro)
                      
# Let OVITO compute the results.
data = pipeline.compute()

# Access computed Voronoi indices.
# This is an (N) x (M) array, where M is the maximum face order.
voro_indices = data.particles['Voronoi Index']

# This helper function takes a two-dimensional array and computes a frequency 
# histogram of the data rows using some NumPy magic. 
# It returns two arrays (of equal length): 
#    1. The list of unique data rows from the input array
#    2. The number of occurences of each unique row
# Both arrays are sorted in descending order such that the most frequent rows 
# are listed first.
def row_histogram(a):
    ca = numpy.ascontiguousarray(a).view([('', a.dtype)] * a.shape[1])
    unique, indices, inverse = numpy.unique(ca, return_index=True, return_inverse=True)
    counts = numpy.bincount(inverse)
    sort_indices = numpy.argsort(counts)[::-1]
    return (a[indices[sort_indices]], counts[sort_indices])

# Compute frequency histogram.
unique_indices, counts = row_histogram(voro_indices)

# Print the ten most frequent histogram entries.
for i in range(10):
    print("%s\t%i\t(%.1f %%)" % (tuple(unique_indices[i]), 
                                 counts[i], 
                                 100.0*float(counts[i])/len(voro_indices)))
