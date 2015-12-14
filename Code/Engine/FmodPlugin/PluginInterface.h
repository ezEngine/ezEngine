#pragma once

class ezFmodInterface
{
public:

  virtual void SetNumListeners(ezUInt8 uiNumListeners) = 0;
  virtual ezUInt8 GetNumListeners() = 0;

};

