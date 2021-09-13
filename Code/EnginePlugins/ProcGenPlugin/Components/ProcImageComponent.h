#pragma once

#include <Core/World/World.h>
#include <ProcGenPlugin/Declarations.h>

struct ezMsgTransformChanged;
struct ezMsgUpdateLocalBounds;
struct ezMsgExtractProcImages;

using ezImageDataResourceHandle = ezTypedResourceHandle<class ezImageDataResource>;
using ezProcImageComponentManager = ezComponentManager<class ezProcImageComponent, ezBlockStorageType::Compact>;

class EZ_PROCGENPLUGIN_DLL ezProcImageComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezProcImageComponent, ezComponent, ezProcImageComponentManager);

public:
  ezProcImageComponent();
  ~ezProcImageComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  const ezVec3& GetExtents() const { return m_vExtents; }
  void SetExtents(const ezVec3& extents);

  void SetValue(float fValue);
  float GetValue() const { return m_fValue; }

  void SetSortOrder(float fOrder);
  float GetSortOrder() const { return m_fSortOrder; }

  void SetImageFile(const char* szFile); // [ property ]
  const char* GetImageFile() const;      // [ property ]

  void SetImage(const ezImageDataResourceHandle& hResource);
  ezImageDataResourceHandle GetImage() const { return m_Image; }

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void OnTransformChanged(ezMsgTransformChanged& msg);

  static const ezEvent<const ezProcGenInternal::InvalidatedArea&>& GetAreaInvalidatedEvent() { return s_AreaInvalidatedEvent; }

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;
  void OnExtractProcImages(ezMsgExtractProcImages& msg) const;

protected:
  ezVec3 m_vExtents = ezVec3(10.0f);
  float m_fValue = 1.0f;
  float m_fSortOrder = 0.0f;
  ezImageDataResourceHandle m_Image;

  void InvalidateArea();
  void InvalidateArea(const ezBoundingBox& area);

  static ezEvent<const ezProcGenInternal::InvalidatedArea&> s_AreaInvalidatedEvent;
};
