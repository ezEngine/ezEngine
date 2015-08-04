#include <PCH.h>
#include <EditorPluginScene/Objects/TestObjects.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneObjectEditorProperties, ezReflectedClass, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Name", GetName, SetName),
    EZ_MEMBER_PROPERTY("Pivot", m_vPivot),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezSceneObjectEditorProperties::ezSceneObjectEditorProperties()
{
  static int bla = 0;
  bla++;

  ezStringBuilder s;
  s.Format("Object %i", bla);

  SetName(s.GetData());

  m_vPivot.SetZero();
}

