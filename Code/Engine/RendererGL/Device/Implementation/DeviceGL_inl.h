
const ezGALFormatLookupTableGL& ezGALDeviceGL::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}

template<typename ResourceType, typename DescriptionType> ResourceType* ezGALDeviceGL::DefaultCreate(const DescriptionType& Description)
{
  ResourceType* pResource = EZ_DEFAULT_NEW(ResourceType)(Description);

  if (!pResource->InitPlatform(this).Succeeded())
  {
    EZ_DEFAULT_DELETE(pResource);
    return nullptr;
  }

  return pResource;
}

template<typename ResourceType, typename DescriptionType, typename DataPtr> ResourceType* ezGALDeviceGL::DefaultCreate(const DescriptionType& Description, DataPtr pInitialData)
{
  ResourceType* pResource = EZ_DEFAULT_NEW(ResourceType)(Description);

  if (!pResource->InitPlatform(this, pInitialData).Succeeded())
  {
    EZ_DEFAULT_DELETE(pResource);
    return nullptr;
  }

  return pResource;
}

template<typename ResourceType, typename ResourceTypeBaseClass> void ezGALDeviceGL::DefaultDestroy(ResourceTypeBaseClass* pResource)
{
  ResourceType* pResourceGL = static_cast<ResourceType*>(pResource);
  pResourceGL->DeInitPlatform(this);
  EZ_DEFAULT_DELETE(pResourceGL);
}