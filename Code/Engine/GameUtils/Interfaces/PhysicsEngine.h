#pragma once

#include <GameUtils/Basics.h>
#include <Foundation/Reflection/Implementation/DynamicRTTI.h>

/// \brief Base class for a physics engine integration
///
/// The interface provides access to some common physics functionality that is needed in game play code.
/// Most physics engine specific functionality must be provided by engine specific components.
class EZ_GAMEUTILS_DLL ezPhysicsEngineInterface : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPhysicsEngineInterface, ezReflectedClass);

public:
  /// \brief Automatically sets the singleton pointer
  ezPhysicsEngineInterface();

  /// \brief Automatically clears the singleton pointer
  ~ezPhysicsEngineInterface();

  /// \brief Returns the currently active singleton instance
  static EZ_FORCE_INLINE  ezPhysicsEngineInterface* GetInstance() { return s_pInstance; }

public:




private:
  static ezPhysicsEngineInterface* s_pInstance;
};