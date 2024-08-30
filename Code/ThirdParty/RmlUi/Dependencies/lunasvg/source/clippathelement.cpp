#include "clippathelement.h"
#include "parser.h"
#include "layoutcontext.h"

namespace lunasvg {

ClipPathElement::ClipPathElement()
    : GraphicsElement(ElementID::ClipPath)
{
}

Units ClipPathElement::clipPathUnits() const
{
    auto& value = get(PropertyID::ClipPathUnits);
    return Parser::parseUnits(value, Units::UserSpaceOnUse);
}

std::unique_ptr<LayoutClipPath> ClipPathElement::getClipper(LayoutContext* context)
{
    if(context->hasReference(this))
        return nullptr;
    LayoutBreaker layoutBreaker(context, this);
    auto clipper = makeUnique<LayoutClipPath>(this);
    clipper->units = clipPathUnits();
    clipper->transform = transform();
    clipper->clipper = context->getClipper(clip_path());
    layoutChildren(context, clipper.get());
    return clipper;
}

} // namespace lunasvg
