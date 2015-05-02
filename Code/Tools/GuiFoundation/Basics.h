#pragma once

#include <Foundation/Basics.h>
#include <ToolsFoundation/Basics.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
  #ifdef BUILDSYSTEM_BUILDING_GUIFOUNDATION_LIB
    #define EZ_GUIFOUNDATION_DLL __declspec(dllexport)
  #else
    #define EZ_GUIFOUNDATION_DLL __declspec(dllimport)
  #endif
#else
  #define EZ_GUIFOUNDATION_DLL
#endif

class QWidget;
class QObject;

class EZ_GUIFOUNDATION_DLL QtScopedUpdatesDisabled
{
public:
  QtScopedUpdatesDisabled(QWidget* pWidget1, QWidget* pWidget2 = nullptr, QWidget* pWidget3 = nullptr, QWidget* pWidget4 = nullptr, QWidget* pWidget5 = nullptr, QWidget* pWidget6 = nullptr);
  ~QtScopedUpdatesDisabled();

private:
  QWidget* m_pWidgets[6]; 
};


class EZ_GUIFOUNDATION_DLL QtScopedBlockSignals
{
public:
  QtScopedBlockSignals(QObject* pObject1, QObject* pObject2 = nullptr, QObject* pObject3 = nullptr, QObject* pObject4 = nullptr, QObject* pObject5 = nullptr, QObject* pObject6 = nullptr);
  ~QtScopedBlockSignals();

private:
  QObject* m_pObjects[6];
};