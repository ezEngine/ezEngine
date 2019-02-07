#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/Declarations.h>

class ezQtPropertyGridWidget;
class ezAbstractProperty;

struct ezPropertyEvent
{
  enum class Type
  {
    SingleValueChanged,
    BeginTemporary,
    EndTemporary,
    CancelTemporary,
  };

  Type m_Type;
  const ezAbstractProperty* m_pProperty;
  const ezHybridArray<ezPropertySelection, 8>* m_pItems;
  ezVariant m_Value;
};
