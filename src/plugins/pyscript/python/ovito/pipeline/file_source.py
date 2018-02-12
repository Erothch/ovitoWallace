# Load the native modules and other dependencies.
from ..plugins.PyScript import FileImporter, FileSource, PipelineStatus
from ..data import DataCollection

# Make FileSource a DataCollection.
DataCollection.registerDataCollectionType(FileSource)
assert(issubclass(FileSource, DataCollection))

# Implementation of FileSource.load() method:
def _FileSource_load(self, location, **params):
    """ Associates this data source with a new input file, where it will retrieve its data from.
    
        The function accepts additional keyword arguments, which are forwarded to the format-specific file importer.
        For further information, please see the documentation of the :py:func:`~ovito.io.import_file` function,
        which has the same function interface as this method.

        :param str location: The local file or remote *sftp://* URL to load.
    """

    # Determine the file's format.
    importer = FileImporter.autodetect_format(self.dataset, location)
    if not importer:
        raise RuntimeError("Could not detect the file format. The format might not be supported.")

    # Re-use existing importer if compatible.
    if self.importer and type(self.importer) == type(importer):
        importer = self.importer
        
    # Forward user parameters to the importer.
    for key in params:
        if not hasattr(importer, key):
            raise RuntimeError("Importer object %s does not have an attribute '%s'." % (importer, key))
        importer.__setattr__(key, params[key])

    # Load new data file.
    if not self.set_source(location, importer, False):
        raise RuntimeError("Operation has been canceled by the user.")
    
    # Block execution until data has been loaded. 
    if not self.wait_until_ready(self.dataset.anim.time):
        raise RuntimeError("Operation has been canceled by the user.")
    
    # Raise Python error if loading failed.
    if self.status.type == PipelineStatus.Type.Error:
        raise RuntimeError(self.status.text)

    # Block until list of animation frames has been loaded
    if not self.wait_for_frames_list():
        raise RuntimeError("Operation has been canceled by the user.")
    
FileSource.load = _FileSource_load

# Implementation of FileSource.source_path property, which returns or sets the currently loaded file path.
def _get_FileSource_source_path(self, _originalGetterMethod = FileSource.source_path):
    """ The path or URL of the file (or file sequence) where this data source retrieves its input data from. """
    return _originalGetterMethod.__get__(self)
def _set_FileSource_source_path(self, url):
    """ Sets the URL of the file referenced by this FileSource. """
    self.setSource(url, self.importer, False) 
FileSource.source_path = property(_get_FileSource_source_path, _set_FileSource_source_path)

def _FileSource_compute(self, frame = None):
    """ Retrieves the data from this data source, which will load it from the external file if needed.

        The optional *frame* parameter determines the frame to retrieve, which must be in the range 0 through (:py:attr:`.num_frames`-1).
        If no frame is specified, the current animation position is used (as given by the global setting 
        :py:attr:`AnimationSettings.current_frame <ovito.anim.AnimationSettings.current_frame>`). This is frame 0 by default.

        The :py:class:`!FileSource` uses a caching mechanism to keep one or more frames in memory. Thus, invoking :py:meth:`!compute`
        repeatedly to retrieve the same frame will typically be very fast.

        Note: The returned :py:class:`~ovito.data.PipelineFlowState` data collection contains data objects that are owned by the :py:class:`!FileSource`.
        That means it is not safe to modify these objects as this would lead to unexpected side effects. 
        You can however use the :py:meth:`DataCollection.copy_if_needed() <ovito.data.DataCollection.copy_if_needed>` method
        to make a copy of data objects that you want to modify in place. See the :py:class:`~ovito.data.PipelineFlowState` class
        for more information.

        :param int frame: The animation frame number at which the source should be evaluated. 
        :returns: A :py:class:`~ovito.data.PipelineFlowState` containing the loaded data.
    """
    if frame is not None:
        time = self.source_frame_to_anim_time(frame)
    else:
        time = self.dataset.anim.time

    state = self._evaluate(time)
    if state.status.type == PipelineStatus.Type.Error:
        raise RuntimeError("Data source evaluation failed: %s" % state.status.text)
    assert(state.status.type != PipelineStatus.Type.Pending)

    # Wait for worker threads to finish.
    # This is to avoid warning messages 'QThreadStorage: Thread exited after QThreadStorage destroyed'
    # during Python program exit.
    import PyQt5.QtCore
    PyQt5.QtCore.QThreadPool.globalInstance().waitForDone(0)

    return state

FileSource.compute = _FileSource_compute