SET(SourceFiles
        modifier/elastic_displacements/CalculateElasticDisplacementsModifierEditor.cpp
        modifier/atomic_strain_mod_burgers/AtomicStrainModBurgersEditor.cpp
        modifier/elastic_stability/CalculateElasticStabilityModifierEditor.cpp
)

OVITO_STANDARD_PLUGIN(WallaceGui
        SOURCES ${SourceFiles}
        PLUGIN_DEPENDENCIES Wallace ParticlesGui
        GUI_PLUGIN
)

TARGET_INCLUDE_DIRECTORIES(WallaceGui PRIVATE "${EIGEN3_INCLUDE_DIRS}")
TARGET_INCLUDE_DIRECTORIES(WallaceGui PRIVATE "${EIGEN3_INCLUDE_DIRS}/unsupported")
