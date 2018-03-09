# Load the native modules and other dependencies.
from ..plugins.PyScript import FileImporter, FileSource, ImportMode, PipelineStatus
from ..data import DataCollection
from ..pipeline import Pipeline
import ovito
import sys
try:
    # Python 3.x
    import collections.abc as collections
except ImportError:
    # Python 2.x
    import collections

def import_file(location, **params):
    """ Imports data from an external file. 
    
        This Python function corresponds to the *Load File* menu command in OVITO's
        user interface. The format of the imported file is automatically detected (see `list of supported formats <../../usage.import.html#usage.import.formats>`__).
        But depending on the file's format, additional keyword parameters may need to be supplied 
        to specify how the data should be interpreted. These keyword parameters are documented below.
        
        :param location: The path to import. This can be a local file or a remote *sftp://* URL, see below.
        :returns: The :py:class:`~ovito.pipeline.Pipeline` that has been created for the imported data.

        The function creates and returns a new :py:class:`~ovito.pipeline.Pipeline` wired to a
        :py:class:`~ovito.pipeline.FileSource`, which is responsible for loading the selected input data from disk. The :py:func:`!import_file`
        function requests a first evaluation of the new pipeline in order to fill the pipeline's input cache
        with data from the file.
        
        Note that the newly created :py:class:`~ovito.pipeline.Pipeline` is not inserted into the three-dimensional scene. 
        That means the loaded dataset won't automatically appear in rendered images nor the interactive viewports of the graphical OVITO program.
        For that, you need to explicitly insert the pipeline into the scene by calling :py:meth:`~ovito.pipeline.Pipeline.add_to_scene` if desired.
        
        Furthermore, note that the returned :py:class:`~ovito.pipeline.Pipeline` may be re-used to load a different 
        input file later on. Instead of calling :py:func:`!import_file` again to load another file,
        you can use the :py:meth:`pipeline.source.load(...) <ovito.pipeline.FileSource.load>` method to replace the input file 
        of the already existing pipeline.
        
        **File columns**
        
        When importing XYZ files or *binary* LAMMPS dump files, the mapping of file columns 
        to OVITO's particle properties must be specified using the ``columns`` keyword parameter::
        
            pipeline = import_file("file.xyz", columns = 
              ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z"])
        
        The number of list entries must match the number of data columns in the input file. 
        See the list of :ref:`standard particle properties <particle-types-list>` for possible target properties. If you specify a 
        non-standard property name for a column, a user-defined :py:class:`~ovito.data.ParticleProperty` object is created from the values in that file column.
        For vector properties, the component name must be appended to the property's base name as demonstrated for the ``Position`` property in the example above. 
        To completely ignore a file column during import, specify ``None`` at the corresponding position in the ``columns`` list.
        
        For *text-based* LAMMPS dump files, OVITO automatically determines a reasonable column-to-property mapping, but you may override it using the
        ``columns`` keyword. This can make sense, for example, if the file columns containing the particle coordinates
        do not follow the standard name scheme ``x``, ``y``, and ``z`` (e.g. when reading time-averaged atomic positions computed by LAMMPS). 
        
        **Frame sequences**
        
        OVITO automatically detects if the imported file contains multiple data frames (timesteps). 
        Alternatively (and additionally), it is possible to load a sequence of files in the same directory by using the ``*`` wildcard character 
        in the filename. Note that ``*`` may appear only once, only in the filename component of the path, and only in place of numeric digits. 
        Finally, it is possible to pass an explicit list of file paths to the :py:func:`!import_file` function, which will be loaded
        as an animatable sequence. All modes can be combined, for example to load two file sets from different directories as one
        consecutive sequence::

           import_file('sim.xyz')     # Loads all frames from the given file 
           import_file('sim.*.xyz')   # Loads 'sim.0.xyz', 'sim.100.xyz', 'sim.200.xyz', etc.
           import_file(['sim_a.xyz', 'sim_b.xyz'])  # Loads an explicit list of files
           import_file([
                'dir_a/sim.*.xyz', 
                'dir_b/sim.*.xyz']) # Loads several file sequences from different directories
        
        The number of frames found in the input file(s) is reported by the :py:attr:`~ovito.pipeline.FileSource.num_frames` attribute of the pipeline's :py:class:`~ovito.pipeline.FileSource`
        You can step through the frames with a ``for``-loop as follows:
        
        .. literalinclude:: ../example_snippets/import_access_animation_frames.py
         
        **LAMMPS atom style**
        
        When loading a LAMMPS data file that is based on an atom style other than "atomic", the style must be explicitly
        specified using the ``atom_style`` keyword parameter. Only the following LAMMPS atom styles are currently supported by
        OVITO: ``angle``, ``atomic``, ``body``, ``bond``, ``charge``, ``dipole``, ``full``, ``molecular``.
        LAMMPS data files may contain a hint indicating the atom style (in particular files generated by LAMMPS's own ``write_data`` command contain that hint).
        Such files can be loaded without specifying the ``atom_style`` keyword.
            
        **Topology and trajectory files**

        Some simulation codes write a *topology* file and separate *trajectory* file. The former contains only static information like the bonding
        between atoms, the atom types, etc., which do not change during an MD simulation run, while the latter stores the varying data (first and foremost
        the atomic trajectories). To load such a topology-trajectory pair of files in OVITO, first load the topology file with
        the :py:func:`!import_file` function, then insert a :py:class:`~ovito.modifiers.LoadTrajectoryModifier` into the returned :py:class:`~ovito.pipeline.Pipeline`
        to also load the trajectory data.
    """

    # Process input parameter
    if isinstance(location, str if sys.version_info[0] >= 3 else basestring):
        location_list = [location]
    elif isinstance(location, collections.Sequence):
        location_list = list(location)
    else:
        raise TypeError("Invalid input file location. Expected string or sequence of strings.")
    first_location = location_list[0]
    
    # Determine the file's format.
    importer = FileImporter.autodetect_format(ovito.dataset, first_location)
    if not importer:
        raise RuntimeError("Could not detect the file format. The format might not be supported.")
    
    # Forward user parameters to the file importer object.
    for key in params:
        if not hasattr(importer, key):
            raise RuntimeError("Invalid keyword parameter. Object %s doesn't have a parameter named '%s'." % (repr(importer), key))
        importer.__setattr__(key, params[key])

    # Import data.
    if not importer.import_file(location_list, ImportMode.AddToScene, False):
        raise RuntimeError("Operation has been canceled by the user.")

    # Get the newly created pipeline.
    pipeline = ovito.dataset.selected_pipeline
    if not isinstance(pipeline, Pipeline):
        raise RuntimeError("File import failed. Nothing was imported.")
    
    try:
        # Block execution until file is loaded.
        state = pipeline.evaluate_pipeline(pipeline.dataset.anim.time)
        
        # Raise exception if error occurs during loading.
        if state.status.type == PipelineStatus.Type.Error:
            raise RuntimeError(state.status.text)

        # Block until full list of animation frames has been obtained.
        if isinstance(pipeline.source, FileSource) and not pipeline.source.wait_for_frames_list():
            raise RuntimeError("Operation has been canceled by the user.")
    except:
        # Delete newly created pipeline in case import fails.
        pipeline.delete()
        raise
    
    # The importer will insert the pipeline into the scene in the GUI by default.
    # But in the scripting enviroment we immediately undo it.
    pipeline.remove_from_scene()
    
    return pipeline    