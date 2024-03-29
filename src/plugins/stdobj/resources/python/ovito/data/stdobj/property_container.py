try:
    # Python 3.x
    import collections.abc as collections
except ImportError:
    # Python 2.x
    import collections
import numpy

# Load dependencies.
import ovito

# Load the native code module.
from ovito.plugins.StdObj import PropertyContainer, Property

# Give the PropertyContainer class a dict-like interface for accessing properties by name:
collections.Mapping.register(PropertyContainer)

# len() function returns the number of properties when called on a PropertyContainer.
PropertyContainer.__len__ = lambda self: len(self.properties)

# Returns a Python iterator yielding the names of all properties stored in the PropertyContainer.
def _PropertyContainer__iter__(self):
    for p in self.properties:
        yield p.name
PropertyContainer.__iter__ = _PropertyContainer__iter__

# Returns the property with the given name.
def _PropertyContainer__getitem__(self, key):
    if key.endswith('_'):
        if not self.is_safe_to_modify:
            raise ValueError("Requesting a mutable version of a property is not possible for a container that itself is not mutable. "
                            "Make sure you are working with a mutable version of the {} object.".format(self.__class__.__name__))
        request_mutable = True
        key = key[:-1]
    else:
        request_mutable = False
    for p in self.properties:
        if p.name == key:
            if request_mutable:
                return self.make_mutable(p)
            else:
                return p
    raise KeyError("{} data object does not contain the property '{}'.".format(self.__class__.__name__, str(key)))
PropertyContainer.__getitem__ = _PropertyContainer__getitem__

# Returns the property with the given name or the given default value if no such property exists in the container.
def _PropertyContainer_get(self, key, default=None):
    try: return self[key]
    except KeyError: return default
PropertyContainer.get = _PropertyContainer_get

# Returns the list of names of all properties in the PropertyContainer.
def _PropertyContainer_keys(self):
    return collections.KeysView(self)
PropertyContainer.keys = _PropertyContainer_keys

# Returns the list of name-property pairs in the PropertyContainer.
def _PropertyContainer_items(self):
    return collections.ItemsView(self)
PropertyContainer.items = _PropertyContainer_items

# Returns the list Property objects in the PropertyContainer.
def _PropertyContainer_values(self):
    return self.properties
PropertyContainer.values = _PropertyContainer_values

# Helper function for registering standard property accessor fields for a PropertyContainer subclass.
def create_property_accessor(property_name, doc = None):
    def getter(self):
        return self[property_name]
    def setter(self, val):
        self.create_property(property_name, data=val)
    return property(getter, setter, doc=doc)
PropertyContainer._create_property_accessor = staticmethod(create_property_accessor)

# Implementation of the PropertyContainer.create_property() method.
def _PropertyContainer_create_property(self, name, dtype=None, components=None, data=None):
    """
    Adds a new property to the container and optionally initializes it with 
    the per-element data provided by the *data* parameter. The method returns the new :py:class:`Property` 
    instance.

    The method allows to create *standard* as well as *user-defined* properties. 
    To create a *standard* property, one of the :ref:`standard property names <particle-types-list>` must be provided as *name* argument:
    
    .. literalinclude:: ../example_snippets/property_container.py
        :lines: 16-17
    
    The length of the provided *data* array must match the number of existing elements in the container, which is given by the :py:attr:`.count` attribute.
    You can alternatively assign the per-element values to the property after its construction: 

    .. literalinclude:: ../example_snippets/property_container.py
        :lines: 23-25

    To create a *user-defined* property, use a non-standard property name:
    
    .. literalinclude:: ../example_snippets/property_container.py
        :lines: 29-30
    
    In this case the data type and the number of vector components of the new property are inferred from
    the provided *data* Numpy array. Providing a one-dimensional array creates a scalar property while
    a two-dimensional array creates a vectorial property.
    Alternatively, the *dtype* and *components* parameters can be specified explicitly
    if you are going to assign the property values after property creation:

    .. literalinclude:: ../example_snippets/property_container.py
        :lines: 34-36

    If the property to be created already exists in the container, it is replaced with a new one.
    The existing per-element data from the old property is however retained if *data* is ``None``.

    Note: If the container contains no properties yet, then the number of elements (e.g. particles or bonds) is still undefined.
    In this case the :py:meth:`!create_property` method lets you *define* the number of elements when inserting the very first property
    by specifying a *data* array of the desired length. For example, to create a new :py:class:`Particles` container from scratch
    with 10 particles, a Numpy array of length 10 is used to initialize the ``Position`` particle property:

    .. literalinclude:: ../example_snippets/property_container.py
        :lines: 40-45
    
    After the initial ``Positions`` property has been created, the number of particles in the container is now determined and any 
    subsequently added properties must have the exact same length.

    :param name: Either a :ref:`standard property type constant <particle-types-list>` or a name string.
    :param data: An optional data array with per-element values for initializing the new property.
                    The size of the array must match the element :py:attr:`.count` of the container
                    and the shape must be consistent with the number of components of the property to be created.
    :param dtype: The element data type when creating a user-defined property. Must be either ``int`` or ``float``.
    :param int components: The number of vector components when creating a user-defined property.
    :returns: The newly created :py:class:`Property` object.
    """

    # Input parameter validation and inferrence:
    if isinstance(name, int):
        if name <= 0:
            raise TypeError("Invalid standard property type.")
        property_type = name
    else:
        property_name = name
        property_type = self.standard_property_type_id(property_name)

    if property_type != 0:
        if components is not None:
            raise ValueError("Specifying a vector component count for a standard property is not allowed.")
        if dtype is not None:
            raise ValueError("Specifying a data type for a standard property is not allowed.")
    else:
        if components is None or dtype is None:
            if data is None: raise ValueError("Must provide a 'data' array if data type or component count are not specified.")
        if data is not None:
            data = numpy.asanyarray(data)
            if data.ndim < 1 or data.ndim > 2:
                raise ValueError("Provided data array must be either 1 or 2-dimensional.")
        if components is None:
            components = data.shape[1] if data.ndim==2 else 1
        if dtype is None:
            dtype = data.dtype
        if components < 1:
            raise ValueError("Invalid number of vector components specified for a user-defined property: {}".format(components))
        if not property_name or '.' in property_name:
            raise ValueError("Invalid name for a property: '{}'. The name contains illegal characters.".format(property_name))

        # Translate data type from Python to Qt metatype id.
        if dtype == int or dtype == numpy.int_:
            dtype = Property.DataType.Int
        elif dtype == numpy.longlong or dtype == numpy.int64:
            dtype = Property.DataType.Int64
        elif dtype == float:
            dtype = Property.DataType.Float
        else:
            raise TypeError("Invalid property data type. Only 'int', 'int64' or 'float' are allowed.")
    
    # Check if property already exists in the container.
    existing_prop = None
    for prop in self.properties:
        if property_type != 0 and prop.type == property_type:
            existing_prop = prop
        elif property_type == 0 and prop.name == property_name:
            existing_prop = prop

    if len(self.properties) == 0:
        if data is None:
            raise RuntimeError("Cannot create property, because the container contains no elements yet and no initial property data has been specified.")
        else:
            num_elements = len(data)
    else:
        num_elements = self.count

    # Check data array dimensions.
    if data is not None:
        if len(data) != num_elements:
            raise ValueError("Property array size mismatch. Length of data array is {}, but number of elements in container is {}.".format(len(data), num_elements))
                
    if existing_prop is None:
        # If property does not exist yet in the container, create and add a new Property instance.
        if property_type != 0:
            prop = self.create_standard_property(property_type, data is None, len(data) if not data is None else 0)
        else:
            prop = self.create_user_property(property_name, dtype, components, 0, data is None, len(data) if not data is None else 0)
    else:
        # Make sure the data layout of the existing property is compatible with the requested layout.
        if components is not None and existing_prop.components != components:
            raise ValueError("Existing property '{}' has {} vector component(s), but {} component(s) have been requested for the new property.".format(existing_prop.name, existing_prop.components, components))
        if dtype is not None and existing_prop.data_type != dtype:
            raise ValueError("Existing property '{}' has data type '{}', but data type '{}' has been requested for the new property.".format(
                existing_prop.name, PyQt5.QtCore.QMetaType.typeName(existing_prop.data_type), PyQt5.QtCore.QMetaType.typeName(dtype)))
        
        # Make a copy of the existing property so that we can safely modify it.
        prop = self.make_mutable(existing_prop)

    # Initialize property with per-element data if provided.
    if data is not None:
        with prop:
            prop[...] = data
    return prop
PropertyContainer.create_property = _PropertyContainer_create_property

# Here only for backward compatibility with OVITO 2.9.0:
def _PropertyContainer_create(self, name, dtype=None, components=None, data=None):
    return self.create_property(name, dtype=dtype, components=components, data=data)
PropertyContainer.create = _PropertyContainer_create