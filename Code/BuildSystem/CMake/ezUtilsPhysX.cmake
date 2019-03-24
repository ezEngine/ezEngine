######################################
### PhysX support
######################################

set (EZ_BUILD_PHYSX OFF CACHE BOOL "Whether support for nVidia PhysX should be added")
set (EZ_QT_DIR $ENV{QTDIR} CACHE PATH "Directory of qt installation")

######################################
### ez_requires_physx()
######################################

macro(ez_requires_physx)

	ez_requires(${EZ_BUILD_PHYSX})

endmacro()
