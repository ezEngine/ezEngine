#include <GameFoundation/PCH.h>
#include <Animation/Resources/PropertyAnimResource.h>
#include <Animation/Components/PropertyAnimComponent.h>


void bla()
{
  /// \todo HACK @@@ :-P static reference to force the linker to look at this stuff, we might need an Ensurepluginloaded/linked macro
  ezPropertyAnimResource res;
  ezPropertyAnimComponent comp;
}


EZ_STATICLINK_LIBRARY(GameFoundation)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(GameFoundation_DefaultResources);
  EZ_STATICLINK_REFERENCE(GameFoundation_GameApplication);
  EZ_STATICLINK_REFERENCE(GameFoundation_GameState);
}

