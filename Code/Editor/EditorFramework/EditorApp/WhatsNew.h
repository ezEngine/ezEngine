#pragma once

#include <Foundation/Strings/String.h>

class ezWhatsNewText
{
public:
  ezWhatsNewText();
  ~ezWhatsNewText();

  void Load(const char* szFile);
  void StoreLastRead();

  bool HasChanged() const { return m_bHasChanged; }

  const ezString& GetText() const { return m_sText; }

private:
  bool m_bHasChanged = false;
  ezString m_sText;
};
