#include "useelement.h"
#include "parser.h"
#include "layoutcontext.h"

#include "gelement.h"
#include "svgelement.h"

namespace lunasvg {

UseElement::UseElement()
    : GraphicsElement(ElementID::Use)
{
}

Length UseElement::x() const
{
    auto& value = get(PropertyID::X);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length UseElement::y() const
{
    auto& value = get(PropertyID::Y);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length UseElement::width() const
{
    auto& value = get(PropertyID::Width);
    return Parser::parseLength(value, ForbidNegativeLengths, Length::HundredPercent);
}

Length UseElement::height() const
{
    auto& value = get(PropertyID::Height);
    return Parser::parseLength(value, ForbidNegativeLengths, Length::HundredPercent);
}

std::string UseElement::href() const
{
    auto& value = get(PropertyID::Href);
    return Parser::parseHref(value);
}

void UseElement::layout(LayoutContext* context, LayoutContainer* current)
{
    if(isDisplayNone())
        return;
    LengthContext lengthContext(this);
    auto _x = lengthContext.valueForLength(x(), LengthMode::Width);
    auto _y = lengthContext.valueForLength(y(), LengthMode::Height);

    auto group = makeUnique<LayoutGroup>(this);
    group->transform = Transform::translated(_x, _y) * transform();
    group->opacity = opacity();
    group->masker = context->getMasker(mask());
    group->clipper = context->getClipper(clip_path());
    layoutChildren(context, group.get());
    current->addChildIfNotEmpty(std::move(group));
}

std::unique_ptr<Element> UseElement::cloneTargetElement(const Element* targetElement) const
{
    if(targetElement == this)
        return nullptr;
    switch(targetElement->id()) {
    case ElementID::Circle:
    case ElementID::Ellipse:
    case ElementID::G:
    case ElementID::Image:
    case ElementID::Line:
    case ElementID::Path:
    case ElementID::Polygon:
    case ElementID::Polyline:
    case ElementID::Rect:
    case ElementID::Svg:
    case ElementID::Symbol:
    case ElementID::Text:
    case ElementID::TSpan:
    case ElementID::Use:
        break;
    default:
        return nullptr;
    }

    const auto& idAttr = targetElement->get(PropertyID::Id);
    for(const auto* element = parent(); element; element = element->parent()) {
        if(!idAttr.empty() && idAttr == element->get(PropertyID::Id)) {
            return nullptr;
        }
    }

    auto tagId = targetElement->id();
    if(tagId == ElementID::Symbol) {
        tagId = ElementID::Svg;
    }

    auto newElement = Element::create(tagId);
    newElement->setPropertyList(targetElement->properties());
    if(newElement->id() == ElementID::Svg) {
        for(const auto& property : properties()) {
            if(property.id == PropertyID::Width || property.id == PropertyID::Height) {
                newElement->set(property.id, property.value, 0x0);
            }
        }
    }

    if(newElement->id() == ElementID::Use)
        return newElement;
    for(auto& child : targetElement->children())
        newElement->addChild(child->clone());
    return newElement;
}

void UseElement::build(const Document* document)
{
    auto targetElement = document->getElementById(href());
    if(!targetElement.isNull()) {
        if(auto newElement = cloneTargetElement(targetElement.get())) {
            addChild(std::move(newElement));
        }
    }

    Element::build(document);
}

} // namespace lunasvg
