#include "markerelement.h"
#include "parser.h"
#include "layoutcontext.h"

namespace lunasvg {

MarkerElement::MarkerElement()
    : StyledElement(ElementID::Marker)
{
}

Length MarkerElement::refX() const
{
    auto& value = get(PropertyID::RefX);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length MarkerElement::refY() const
{
    auto& value = get(PropertyID::RefY);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length MarkerElement::markerWidth() const
{
    auto& value = get(PropertyID::MarkerWidth);
    return Parser::parseLength(value, ForbidNegativeLengths, Length::Three);
}

Length MarkerElement::markerHeight() const
{
    auto& value = get(PropertyID::MarkerHeight);
    return Parser::parseLength(value, ForbidNegativeLengths, Length::Three);
}

Angle MarkerElement::orient() const
{
    auto& value = get(PropertyID::Orient);
    return Parser::parseAngle(value);
}

MarkerUnits MarkerElement::markerUnits() const
{
    auto& value = get(PropertyID::MarkerUnits);
    return Parser::parseMarkerUnits(value);
}

Rect MarkerElement::viewBox() const
{
    auto& value = get(PropertyID::ViewBox);
    return Parser::parseViewBox(value);
}

PreserveAspectRatio MarkerElement::preserveAspectRatio() const
{
    auto& value = get(PropertyID::PreserveAspectRatio);
    return Parser::parsePreserveAspectRatio(value);
}

std::unique_ptr<LayoutMarker> MarkerElement::getMarker(LayoutContext* context)
{
    auto markerWidth = this->markerWidth();
    auto markerHeight = this->markerHeight();
    if(markerWidth.isZero() || markerHeight.isZero() || context->hasReference(this))
        return nullptr;

    LengthContext lengthContext(this);
    auto _refX = lengthContext.valueForLength(refX(), LengthMode::Width);
    auto _refY = lengthContext.valueForLength(refY(), LengthMode::Height);
    auto _markerWidth = lengthContext.valueForLength(markerWidth, LengthMode::Width);
    auto _markerHeight = lengthContext.valueForLength(markerHeight, LengthMode::Height);

    auto viewBox = this->viewBox();
    auto preserveAspectRatio = this->preserveAspectRatio();
    auto viewTransform = preserveAspectRatio.getMatrix(_markerWidth, _markerHeight, viewBox);
    viewTransform.map(_refX, _refY, &_refX, &_refY);

    LayoutBreaker layoutBreaker(context, this);
    auto marker = makeUnique<LayoutMarker>(this);
    marker->refX = _refX;
    marker->refY = _refY;
    marker->transform = viewTransform;
    marker->orient = orient();
    marker->units = markerUnits();
    marker->clip = isOverflowHidden() ? preserveAspectRatio.getClip(_markerWidth, _markerHeight, viewBox) : Rect::Invalid;
    marker->opacity = opacity();
    marker->masker = context->getMasker(mask());
    marker->clipper = context->getClipper(clip_path());
    layoutChildren(context, marker.get());
    return marker;
}

} // namespace lunasvg
