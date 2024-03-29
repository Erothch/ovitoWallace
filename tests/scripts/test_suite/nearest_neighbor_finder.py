from ovito.io import *
from ovito.data import *

pipeline = import_file("../../files/LAMMPS/animation.dump.gz")

nneigh = 10
finder = NearestNeighborFinder(nneigh, pipeline.compute())
num_particles = pipeline.compute().particles.count

counter = 0
for n in finder.find(0):
    assert(n.index >= 0 and n.index < num_particles)
    assert(n.distance > 0.0)
    assert(n.distance_squared > 0.0)
    assert(len(n.delta) == 3)
    counter += 1
assert(counter == nneigh)

coords = (0.1, 0.2, 0.3)
for n in finder.find_at(coords):
    assert(n.index >= 0 and n.index < num_particles)
    assert(n.distance > 0.0)
    assert(n.distance_squared > 0.0)
    assert(len(n.delta) == 3)

pos = pipeline.compute().particles['Position']
assert(next(finder.find_at(pos[0])).index == 0)
assert(next(finder.find_at(pos[1])).index == 1)

iter = finder.find(0)
n1 = next(iter)
print(n1.delta)
n2 = next(iter)
print(n2.delta)
print(n1.delta)