#FIND_PACKAGE(Taco REQUIRED)
FIND_PACKAGE (Eigen3 3.3 REQUIRED NO_MODULE)

SET(SourceFiles
        modifier/elastic_displacements/CalculateElasticDisplacementsModifier.cpp
        modifier/atomic_strain_mod_burgers/AtomicStrainModBurgers.cpp
        modifier/elastic_stability/CalculateElasticStabilityModifier.cpp
        modifier/elastic_stability/ElasticConstants.cpp
)

IF(OVITO_BUILD_PLUGIN_PYSCRIPT)
        LIST(APPEND SourceFiles
                scripting/PythonInterface.cpp
        )
ENDIF()


OVITO_STANDARD_PLUGIN(Wallace
	SOURCES Wallace.cpp ${SourceFiles}
        PLUGIN_DEPENDENCIES Particles CrystalAnalysis
        PRIVATE_LIB_DEPENDENCIES Eigen3::Eigen
        OPTIONAL_PLUGIN_DEPENDENCIES PyScript StdMod
        PYTHON_WRAPPERS "${CMAKE_CURRENT_SOURCE_DIR}/resources/python/"
)

TARGET_INCLUDE_DIRECTORIES(Wallace PRIVATE "${EIGEN3_INCLUDE_DIRS}")
TARGET_INCLUDE_DIRECTORIES(Wallace PRIVATE "${EIGEN3_INCLUDE_DIRS}/unsupported")


# Build corresponding GUI plugin.
IF(OVITO_BUILD_GUI)
        ADD_SUBDIRECTORY(gui)
ENDIF()

# No current windows handling
# Deploy shared taco lib
#OVITO_INSTALL_SHARED_LIB("${taco_LIBRARIES}" "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
