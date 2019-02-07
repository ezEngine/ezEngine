#pragma once

#include <KrautPlugin/KrautPluginDLL.h>

struct ezResourceEvent;

class ezKrautLodInfo;
class ezKrautRenderData;

class ezKrautTreeComponent;
class ezKrautTreeComponentManager;

enum class ezKrautLodType : ezUInt8
{
  None = 0xFF,
  Mesh = 0,
  StaticImpostor = 1,
  BillboardImpostor = 2,
};

enum class ezKrautMaterialType : ezUInt8
{
  None = 0xFF,
  Branch = 0,
  Frond = 1,
  Leaf = 2,
  StaticImpostor = 3,
  BillboardImpostor = 4,
};
