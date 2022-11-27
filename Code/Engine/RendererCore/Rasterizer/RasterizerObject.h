#pragma once

#include <Foundation/Math/Mat4.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#include <RendererCore/RendererCoreDLL.h>

class ezGeometry;

class EZ_RENDERERCORE_DLL ezRasterizerObject : public ezRefCounted
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezRasterizerObject);

public:
  ezRasterizerObject();
  ~ezRasterizerObject();

  /// \brief If an object with the given name has been created before, it is returned, otherwise nullptr is returned.
  ///
  /// Use this to quickly query for an existing object. Call CreateMesh() in case the object doesn't exist yet.
  static ezSharedPtr<const ezRasterizerObject> GetObject(ezStringView sUniqueName);

  /// \brief Creates a box object with the specified dimensions. If such a box was created before, the same pointer is returned.
  static ezSharedPtr<const ezRasterizerObject> CreateBox(const ezVec3& vFullExtents);

  /// \brief Creates an object with the given geometry. If an object with the same name was created before, that pointer is returned instead.
  ///
  /// It is assumed that the same name will only be used for identical geometry.
  static ezSharedPtr<const ezRasterizerObject> CreateMesh(ezStringView sUniqueName, const ezGeometry& geometry);

private:
  void CreateMesh(const ezGeometry& geometry);

  friend class ezRasterizerView;
  Occluder m_Occluder;

  static ezMutex s_Mutex;
  static ezMap<ezString, ezSharedPtr<ezRasterizerObject>> s_Objects;
};
