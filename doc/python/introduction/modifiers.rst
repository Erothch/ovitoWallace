.. _modifiers_overview:

===================================
Data pipelines
===================================

Modifiers are composable function objects that form a `data processing pipeline <https://en.wikipedia.org/wiki/Pipeline_(software)>`__.
They dynamically modify, filter, analyze or extend the data that flows down the pipeline. Here, with *data* we mean
any form of information that OVITO can work with, e.g. particles and their properties, bonds, the simulation cell,
triangles meshes, voxel data, etc. The main purpose of the pipeline concept is to enable a non-destructive and repeatable workflow, i.e.,
a modification pipeline --once set up-- can be re-used repeatedly on multiple input datasets.

A processing pipeline is represented by an instance of the :py:class:`~ovito.pipeline.Pipeline` class in OVITO.
Initially, a pipeline contains no modifiers. That means its output will be identical to its input. The input data
is provided by a separate source object that is attached to the pipeline. 
Typically, this :py:attr:`~ovito.pipeline.Pipeline.source` object is an instance of the :py:class:`~ovito.pipeline.FileSource` class, which reads the input data
from an external data file.

You can insert a modifier into a :py:class:`~ovito.pipeline.Pipeline` by creating a new 
instance of the corresponding modifier class (see the :py:mod:`ovito.modifiers` module for all available modifier types) and then 
adding it to the pipeline's :py:attr:`~ovito.pipeline.Pipeline.modifiers` list::

   from ovito.modifiers import AssignColorModifier

   modifier = AssignColorModifier( color=(0.5, 1.0, 0.0) )
   pipeline.modifiers.append(modifier)
   
The modifiers in the :py:attr:`Pipeline.modifiers <ovito.pipeline.Pipeline.modifiers>` list are executed in a specific order, i.e.,
appending a modifier to the list positions it at the end of the data pipeline and it will be the last one to process
the data that is flowing down the pipeline. In other words, it will see the effect of all preceding modifiers in the sequence.

.. image:: graphics/Pipeline.*
   :width: 50 %
   :align: center

Note that inserting a new modifier into the pipeline --like any change to a pipeline or its modifiers-- does not 
immediately trigger a computation. The modifier's effect will be computed only when the results of the pipeline are requested. 
Evaluation of the pipeline can be triggered either implicitly, e.g. when

  * rendering an image or movie,
  * updating the interactive viewports in OVITO's graphical user interface, 
  * or exporting data using the :py:func:`ovito.io.export_file` function.
  
You can explicitly request an evaluation of a pipeline by calling its :py:meth:`~ovito.pipeline.Pipeline.compute` method.
This method returns a new :py:class:`~ovito.data.DataCollection` object holding the set of data objects that left the pipeline
after all modifiers currently in the pipeline have processed the input data one after another::

    >>> data = pipeline.compute()
    >>> print(data.objects)
    [SimulationCell(), ParticleProperty('Position'), ParticleProperty('Color')]

In this example, the pipeline output contains three data objects: a :py:class:`~ovito.data.SimulationCell`
object and two :py:class:`~ovito.data.ParticleProperty` objects, which store the particle positions and 
particle colors, respectively. We will learn more about the representation of data in OVITO and the :py:class:`~ovito.data.DataCollection` 
class later.

Note that it is possible to change an existing pipeline and the parameters of its modifiers at any time. Such changes do not 
immediately trigger a recomputation of the pipeline results (unlike in the graphical user interface, where changing a modifier's parameters 
lets OVITO immediately recompute the results and update the interactive viewports). In a Python script, we have to 
call the pipeline's :py:meth:`~ovito.pipeline.Pipeline.compute` method again to request a new evaluation of the modifiers
in the pipeline after making a change to the pipeline::

    # Set up a new pipeline containing one modifier:
    pipeline = import_file("simulation.dump")
    pipeline.modifiers.append(AssignColorModifier(color = (0.5, 1.0, 0.0)))
    
    # Evaluate the current pipeline a first time:
    outdata1 = pipeline.compute()

    # Now altering the pipeline by e.g. changing parameters or appending modifiers: 
    pipeline.modifiers[0].color = (0.8, 0.8, 1.0)
    pipeline.modifiers.append(CoordinationAnalysisModifier(cutoff = 5.0))
   
    # Evaluate the pipeline a second time, now getting new results:
    outdata2 = pipeline.compute()

--------------------------------------------------------------
Processing simulation trajectories and time-dependent data 
--------------------------------------------------------------

As mentioned in the :ref:`file_io_overview` section, it is possible to import a simulation trajectory consisting of a sequence of frames into 
OVITO. A pipeline always processes one frame at a time. You can request processing of a specific simulation frame by passing the frame number to the pipeline's :py:meth:`~ovito.pipeline.Pipeline.compute`
method, e.g.::

    pipeline = import_file("trajectory_*.dump")
    data_frame0 = pipeline.compute(0)
    data_frame1 = pipeline.compute(1)
    data_frame2 = pipeline.compute(2)
    ...

The numbering of animation frames starts at 0 in OVITO. Typically, a ``for``-loop of the following form is used to iterate over all frames of a simulation sequence::

    for frame in range(pipeline.source.num_frames):
        data = pipeline.compute(frame)
        ...

The :py:attr:`FileSource.num_frames <ovito.pipeline.FileSource.num_frames>` property tells how many frames the input trajectory contains.

.. note::

    When employing a :py:class:`~ovito.pipeline.Pipeline` in a loop to process a sequence of frames, make sure you 
    do not modify the pipeline inside the loop. Adding new modifiers to the pipeline as part of a for-loop is 
    typically wrong::

        # WRONG APPROACH:
        for frame in range(pipeline.source.num_frames):
            pipeline.modifiers.append(AtomicStrainModifier(cutoff = 3.2))
            data = pipeline.compute(frame)
            ...

    Since the code block gets executed multiple times, this loop would keep appending additional modifiers to the same pipeline, 
    making it longer and longer with every iteration.
    As a result, we would end up with multiple :py:class:`~ovito.modifiers.AtomicStrainModifier` instances in the pipeline, each performing the same 
    computation over and over again when :py:meth:`~ovito.pipeline.Pipeline.compute` is called. 
    Instead, you should populate the pipeline with modifiers just once *before* the loop::

        # Step 1: Setting up the pipeline:
        pipeline.modifiers.append(AtomicStrainModifier(cutoff = 3.2))

        # Step 2: Evaluating the same pipeline for different input frames:
        for frame in range(pipeline.source.num_frames):
            data = pipeline.compute(frame)
            ...
