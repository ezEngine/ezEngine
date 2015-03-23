#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactoryBase.h>
#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

/// \brief Type traverser for ezRttiMappedObjectFactory.
struct ezRttiTraverser
{
  static bool IsValid(const ezRTTI* pType);
  static const ezRTTI* GetParentType(const ezRTTI* pType);
};

/// \brief ezRtti pointer based mapped object factory. \see ezRttiMappedObjectFactoryBase
template <typename Object>
struct ezRttiMappedObjectFactory : public ezRttiMappedObjectFactoryBase<const ezRTTI*, Object, ezRttiTraverser>
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezRttiMappedObjectFactory);
public:
  ezRttiMappedObjectFactory();
};

/// \brief Type traverser for ezReflectedTypeMappedObjectFactory.
struct ezReflectedTypeHandleTraverser
{
  static bool IsValid(ezReflectedTypeHandle hType);
  static ezReflectedTypeHandle GetParentType(ezReflectedTypeHandle hType);
};

/// \brief ezReflectedTypeHandle based mapped object factory. \see ezRttiMappedObjectFactoryBase
template <typename Object>
struct ezReflectedTypeMappedObjectFactory : public ezRttiMappedObjectFactoryBase<ezReflectedTypeHandle, Object, ezReflectedTypeHandleTraverser>
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezReflectedTypeMappedObjectFactory);
public:
  ezReflectedTypeMappedObjectFactory();
};

#include <ToolsFoundation/Factory/Implementation/RttiMappedObjectFactory_inl.h>
