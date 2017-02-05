
# Function parameter output for debugging:
#message(STATUS "${MSVC_BUILD_CONFIG_DIR}")
#message(STATUS "${PHYSX_SOURCE}")
#message(STATUS "${PHYSX_TARGET}")

file(COPY "${PHYSX_SOURCE}/nvToolsExt64_1.dll"
        DESTINATION "${PHYSX_TARGET}/")

if (MSVC_BUILD_CONFIG_DIR STREQUAL "Debug")
  file(COPY "${PHYSX_SOURCE}/PhysX3CharacterKinematicDEBUG_x64.dll"
          DESTINATION "${PHYSX_TARGET}/")
  file(COPY "${PHYSX_SOURCE}/PhysX3CommonDEBUG_x64.dll"
          DESTINATION "${PHYSX_TARGET}/")
  file(COPY "${PHYSX_SOURCE}/PhysX3CookingDEBUG_x64.dll"
          DESTINATION "${PHYSX_TARGET}/")
  file(COPY "${PHYSX_SOURCE}/PhysX3DEBUG_x64.dll"
          DESTINATION "${PHYSX_TARGET}/")
else ()
  file(COPY "${PHYSX_SOURCE}/PhysX3CharacterKinematic_x64.dll"
          DESTINATION "${PHYSX_TARGET}/")
  file(COPY "${PHYSX_SOURCE}/PhysX3Common_x64.dll"
          DESTINATION "${PHYSX_TARGET}/")
  file(COPY "${PHYSX_SOURCE}/PhysX3Cooking_x64.dll"
          DESTINATION "${PHYSX_TARGET}/")
  file(COPY "${PHYSX_SOURCE}/PhysX3_x64.dll"
          DESTINATION "${PHYSX_TARGET}/")
endif ()

