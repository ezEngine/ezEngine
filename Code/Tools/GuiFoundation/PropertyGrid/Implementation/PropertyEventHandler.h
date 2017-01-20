#pragma once

#include <GuiFoundation/Basics.h>
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

class EZ_GUIFOUNDATION_DLL ezPropertyEventHandler
{
public:
  ezPropertyEventHandler();

  void Init(ezQtPropertyGridWidget* pGrid);
  void PrepareToDie();

  void PropertyChangedHandler(const ezPropertyEvent& ed);

private:
  bool m_bUndead;
  ezQtPropertyGridWidget* m_pGrid;
};
