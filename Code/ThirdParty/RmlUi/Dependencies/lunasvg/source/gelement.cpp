#include "gelement.h"
#include "layoutcontext.h"

namespace lunasvg {

GElement::GElement()
    : GraphicsElement(ElementID::G)
{
}

void GElement::layout(LayoutContext* context, LayoutContainer* current)
{
    if(isDisplayNone())
        return;
    auto group = makeUnique<LayoutGroup>(this);
    group->transform = transform();
    group->opacity = opacity();
    group->masker = context->getMasker(mask());
    group->clipper = context->getClipper(clip_path());
    layoutChildren(context, group.get());
    current->addChildIfNotEmpty(std::move(group));
}

} // namespace lunasvg
