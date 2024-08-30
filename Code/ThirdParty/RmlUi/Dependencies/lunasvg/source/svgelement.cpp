#include "svgelement.h"
#include "parser.h"
#include "layoutcontext.h"

namespace lunasvg {

SVGElement::SVGElement()
    : GraphicsElement(ElementID::Svg)
{
}

Length SVGElement::x() const
{
    auto& value = get(PropertyID::X);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length SVGElement::y() const
{
    auto& value = get(PropertyID::Y);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length SVGElement::width() const
{
    auto& value = get(PropertyID::Width);
    return Parser::parseLength(value, ForbidNegativeLengths, Length::HundredPercent);
}

Length SVGElement::height() const
{
    auto& value = get(PropertyID::Height);
    return Parser::parseLength(value, ForbidNegativeLengths, Length::HundredPercent);
}

Rect SVGElement::viewBox() const
{
    auto& value = get(PropertyID::ViewBox);
    return Parser::parseViewBox(value);
}

PreserveAspectRatio SVGElement::preserveAspectRatio() const
{
    auto& value = get(PropertyID::PreserveAspectRatio);
    return Parser::parsePreserveAspectRatio(value);
}

std::unique_ptr<LayoutSymbol> SVGElement::layoutTree(const Document* document)
{
    if(isDisplayNone())
        return nullptr;
    auto w = this->width();
    auto h = this->height();
    if(w.isZero() || h.isZero())
        return nullptr;
    LengthContext lengthContext(this);
    auto _x = lengthContext.valueForLength(x(), LengthMode::Width);
    auto _y = lengthContext.valueForLength(y(), LengthMode::Height);
    auto _w = lengthContext.valueForLength(w, LengthMode::Width);
    auto _h = lengthContext.valueForLength(h, LengthMode::Height);

    Point transformOrigin(_w * 0.5, _h * 0.5); // transform-origin: 50% 50%
    auto transform = Transform::translated(transformOrigin.x, transformOrigin.y);
    transform.premultiply(this->transform());
    transform.translate(-transformOrigin.x, -transformOrigin.y);

    auto viewBox = this->viewBox();
    auto preserveAspectRatio = this->preserveAspectRatio();
    auto viewTranslation = Transform::translated(_x, _y);
    auto viewTransform = preserveAspectRatio.getMatrix(_w, _h, viewBox);

    auto root = makeUnique<LayoutSymbol>(this);
    root->width = _w;
    root->height = _h;
    root->transform = (viewTransform * viewTranslation) * transform;
    root->clip = isOverflowHidden() ? preserveAspectRatio.getClip(_w, _h, viewBox) : Rect::Invalid;
    root->opacity = opacity();

    LayoutContext context(document, root.get());
    root->masker = context.getMasker(mask());
    root->clipper = context.getClipper(clip_path());
    layoutChildren(&context, root.get());
    if((w.isRelative() || h.isRelative()) && !has(PropertyID::ViewBox)) {
        auto box = root->map(root->strokeBoundingBox());
        root->width = w.value(box.x + box.w);
        root->height = h.value(box.y + box.h);
    }

    return root;
}

void SVGElement::layout(LayoutContext* context, LayoutContainer* current)
{
    if(isDisplayNone())
        return;
    auto w = this->width();
    auto h = this->height();
    if(w.isZero() || h.isZero())
        return;

    LengthContext lengthContext(this);
    auto _x = lengthContext.valueForLength(x(), LengthMode::Width);
    auto _y = lengthContext.valueForLength(y(), LengthMode::Height);
    auto _w = lengthContext.valueForLength(w, LengthMode::Width);
    auto _h = lengthContext.valueForLength(h, LengthMode::Height);

    auto viewBox = this->viewBox();
    auto preserveAspectRatio = this->preserveAspectRatio();
    auto viewTranslation = Transform::translated(_x, _y);
    auto viewTransform = preserveAspectRatio.getMatrix(_w, _h, viewBox);

    auto symbol = makeUnique<LayoutSymbol>(this);
    symbol->width = _w;
    symbol->height = _h;
    symbol->transform = (viewTransform * viewTranslation) * transform();
    symbol->clip = isOverflowHidden() ? preserveAspectRatio.getClip(_w, _h, viewBox) : Rect::Invalid;
    symbol->opacity = opacity();
    symbol->masker = context->getMasker(mask());
    symbol->clipper = context->getClipper(clip_path());
    layoutChildren(context, symbol.get());
    current->addChildIfNotEmpty(std::move(symbol));
}

} // namespace lunasvg
