#include "element.h"
#include "clippathelement.h"
#include "defselement.h"
#include "gelement.h"
#include "geometryelement.h"
#include "markerelement.h"
#include "maskelement.h"
#include "paintelement.h"
#include "stopelement.h"
#include "svgelement.h"
#include "symbolelement.h"
#include "useelement.h"
#include "styleelement.h"
#include "parser.h"

namespace lunasvg {

std::unique_ptr<Node> TextNode::clone() const
{
    auto node = makeUnique<TextNode>();
    node->setText(m_text);
    return std::move(node);
}

Element::Element(ElementID id)
    : m_id(id)
{
}

std::unique_ptr<Element> Element::create(ElementID id)
{
    switch(id) {
    case ElementID::Svg:
        return makeUnique<SVGElement>();
    case ElementID::Path:
        return makeUnique<PathElement>();
    case ElementID::G:
        return makeUnique<GElement>();
    case ElementID::Rect:
        return makeUnique<RectElement>();
    case ElementID::Circle:
        return makeUnique<CircleElement>();
    case ElementID::Ellipse:
        return makeUnique<EllipseElement>();
    case ElementID::Line:
        return makeUnique<LineElement>();
    case ElementID::Defs:
        return makeUnique<DefsElement>();
    case ElementID::Polygon:
        return makeUnique<PolygonElement>();
    case ElementID::Polyline:
        return makeUnique<PolylineElement>();
    case ElementID::Stop:
        return makeUnique<StopElement>();
    case ElementID::LinearGradient:
        return makeUnique<LinearGradientElement>();
    case ElementID::RadialGradient:
        return makeUnique<RadialGradientElement>();
    case ElementID::Symbol:
        return makeUnique<SymbolElement>();
    case ElementID::Use:
        return makeUnique<UseElement>();
    case ElementID::Pattern:
        return makeUnique<PatternElement>();
    case ElementID::Mask:
        return makeUnique<MaskElement>();
    case ElementID::ClipPath:
        return makeUnique<ClipPathElement>();
    case ElementID::SolidColor:
        return makeUnique<SolidColorElement>();
    case ElementID::Marker:
        return makeUnique<MarkerElement>();
    case ElementID::Style:
        return makeUnique<StyleElement>();
    default:
        break;
    }

    return nullptr;
}

void Element::set(PropertyID id, const std::string& value, int specificity)
{
    for(auto& property : m_properties) {
        if(property.id == id) {
            if(specificity >= property.specificity) {
                property.specificity = specificity;
                property.value = value;
            }

            return;
        }
    }

    m_properties.push_back({specificity, id, value});
}

static const std::string EmptyString;

const std::string& Element::get(PropertyID id) const
{
    for(auto& property : m_properties) {
        if(property.id == id) {
            return property.value;
        }
    }

    return EmptyString;
}

static const std::string InheritString{"inherit"};

const std::string& Element::find(PropertyID id) const
{
    auto element = this;
    do {
        auto& value = element->get(id);
        if(!value.empty() && value != InheritString)
            return value;
        element = element->parent();
    } while(element);
    return EmptyString;
}

bool Element::has(PropertyID id) const
{
    for(auto& property : m_properties) {
        if(property.id == id) {
            return true;
        }
    }

    return false;
}

Element* Element::previousElement() const
{
    if(parent() == nullptr)
        return nullptr;
    Element* element = nullptr;
    const auto& children = parent()->children();
    auto it = children.begin();
    auto end = children.end();
    for(; it != end; ++it) {
        auto node = it->get();
        if(node->isText())
            continue;
        if(node == this)
            return element;
        element = static_cast<Element*>(node);
    }

    return nullptr;
}

Element* Element::nextElement() const
{
    if(parent() == nullptr)
        return nullptr;
    Element* element = nullptr;
    const auto& children = parent()->children();
    auto it = children.begin();
    auto end = children.end();
    for(; it != end; ++it) {
        auto node = it->get();
        if(node->isText())
            continue;
        if(node == this)
            return element;
        element = static_cast<Element*>(node);
    }

    return nullptr;
}

Node* Element::addChild(std::unique_ptr<Node> child)
{
    child->setParent(this);
    m_children.push_back(std::move(child));
    return &*m_children.back();
}

void Element::layoutChildren(LayoutContext* context, LayoutContainer* current)
{
    for(auto& child : m_children) {
        child->layout(context, current);
    }
}

Rect Element::currentViewport() const
{
    if(parent() == nullptr) {
        auto element = static_cast<const SVGElement*>(this);
        if(element->has(PropertyID::ViewBox))
            return element->viewBox(); 
        return Rect{0, 0, 300, 150};
    }

    if(parent()->id() == ElementID::Svg) {
        auto element = static_cast<SVGElement*>(parent());
        if(element->has(PropertyID::ViewBox))
            return element->viewBox();
        LengthContext lengthContext(element);
        auto _x = lengthContext.valueForLength(element->x(), LengthMode::Width);
        auto _y = lengthContext.valueForLength(element->y(), LengthMode::Height);
        auto _w = lengthContext.valueForLength(element->width(), LengthMode::Width);
        auto _h = lengthContext.valueForLength(element->height(), LengthMode::Height);
        return Rect{_x, _y, _w, _h};
    }

    return parent()->currentViewport();
}

void Element::build(const Document* document)
{
    for(auto& child : m_children) {
        if(child->isText())
            continue;
        auto element = static_cast<Element*>(child.get());
        element->build(document);
    }
}

std::unique_ptr<Node> Element::clone() const
{
    auto element = Element::create(m_id);
    element->setPropertyList(m_properties);
    for(auto& child : m_children)
        element->addChild(child->clone());
    return element;
}

} // namespace lunasvg
