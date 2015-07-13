#include <PCH.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshAssetProperties, ezReflectedClass, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Mesh File", m_sMeshFile),
    EZ_MEMBER_PROPERTY("Mesh Scaling", m_fMeshScaling),
    EZ_MEMBER_PROPERTY("Material Slot 0", m_sSlot0),
    EZ_MEMBER_PROPERTY("Material Slot 1", m_sSlot1),
    EZ_MEMBER_PROPERTY("Material Slot 2", m_sSlot2),
    EZ_MEMBER_PROPERTY("Material Slot 3", m_sSlot3),
    EZ_MEMBER_PROPERTY("Material Slot 4", m_sSlot4),
    EZ_MEMBER_PROPERTY("Material Slot 5", m_sSlot5),
    EZ_MEMBER_PROPERTY("Material Slot 6", m_sSlot6),
    EZ_MEMBER_PROPERTY("Material Slot 7", m_sSlot7),
    EZ_MEMBER_PROPERTY("Material Slot 8", m_sSlot8),
    EZ_MEMBER_PROPERTY("Material Slot 9", m_sSlot9),
    EZ_MEMBER_PROPERTY("Material Slot 10", m_sSlot10),
    EZ_MEMBER_PROPERTY("Material Slot 11", m_sSlot11),
    EZ_MEMBER_PROPERTY("Material Slot 12", m_sSlot12),
    EZ_MEMBER_PROPERTY("Material Slot 13", m_sSlot13),
    EZ_MEMBER_PROPERTY("Material Slot 14", m_sSlot14),
    EZ_MEMBER_PROPERTY("Material Slot 15", m_sSlot15),
    EZ_MEMBER_PROPERTY("Material Slot 16", m_sSlot16),
    EZ_MEMBER_PROPERTY("Material Slot 17", m_sSlot17),
    EZ_MEMBER_PROPERTY("Material Slot 18", m_sSlot18),
    EZ_MEMBER_PROPERTY("Material Slot 19", m_sSlot19),
    EZ_MEMBER_PROPERTY("Material Slot 20", m_sSlot20),
    EZ_MEMBER_PROPERTY("Material Slot 21", m_sSlot21),
    EZ_MEMBER_PROPERTY("Material Slot 22", m_sSlot22),
    EZ_MEMBER_PROPERTY("Material Slot 23", m_sSlot23),
    EZ_MEMBER_PROPERTY("Material Slot 24", m_sSlot24),
    EZ_MEMBER_PROPERTY("Material Slot 25", m_sSlot25),
    EZ_MEMBER_PROPERTY("Material Slot 26", m_sSlot26),
    EZ_MEMBER_PROPERTY("Material Slot 27", m_sSlot27),
    EZ_MEMBER_PROPERTY("Material Slot 28", m_sSlot28),
    EZ_MEMBER_PROPERTY("Material Slot 29", m_sSlot29),
    EZ_MEMBER_PROPERTY("Material Slot 30", m_sSlot30),
    EZ_MEMBER_PROPERTY("Material Slot 31", m_sSlot31),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezMeshAssetProperties::ezMeshAssetProperties()
{
  m_uiVertices = 0;
  m_uiTriangles = 0;
  m_fMeshScaling = 1.0f;
}

const ezString& ezMeshAssetProperties::GetResourceSlotProperty(ezUInt32 uiSlot) const
{
  uiSlot %= 32;

  switch (uiSlot)
  {
  case 0:   return m_sSlot0;
  case 1:   return m_sSlot1;
  case 2:   return m_sSlot2;
  case 3:   return m_sSlot3;
  case 4:   return m_sSlot4;
  case 5:   return m_sSlot5;
  case 6:   return m_sSlot6;
  case 7:   return m_sSlot7;
  case 8:   return m_sSlot8;
  case 9:   return m_sSlot9;
  case 10:  return m_sSlot10;
  case 11:  return m_sSlot11;
  case 12:  return m_sSlot12;
  case 13:  return m_sSlot13;
  case 14:  return m_sSlot14;
  case 15:  return m_sSlot15;
  case 16:  return m_sSlot16;
  case 17:  return m_sSlot17;
  case 18:  return m_sSlot18;
  case 19:  return m_sSlot19;
  case 20:  return m_sSlot20;
  case 21:  return m_sSlot21;
  case 22:  return m_sSlot22;
  case 23:  return m_sSlot23;
  case 24:  return m_sSlot24;
  case 25:  return m_sSlot25;
  case 26:  return m_sSlot26;
  case 27:  return m_sSlot27;
  case 28:  return m_sSlot28;
  case 29:  return m_sSlot29;
  case 30:  return m_sSlot30;
  case 31:  return m_sSlot31;
  }

  return m_sSlot0;
}