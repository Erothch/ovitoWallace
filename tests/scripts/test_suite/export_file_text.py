import ovito
from ovito.io import import_file, export_file
import os
import os.path

test_data_dir = "../../files/"

node = import_file(test_data_dir + "LAMMPS/animation1.dump")
export_file(node, "_export_file_text.data", "txt/attr", columns = ["Timestep", "SourceFrame"], multiple_frames = True)
os.remove("_export_file_text.data")