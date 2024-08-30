#include "paintelement.h"
#include "stopelement.h"
#include "parser.h"
#include "layoutcontext.h"

#include <set>

namespace lunasvg {

PaintElement::PaintElement(ElementID id)
    : StyledElement(id)
{
}

GradientElement::GradientElement(ElementID id)
    : PaintElement(id)
{
}

Transform GradientElement::gradientTransform() const
{
    auto& value = get(PropertyID::GradientTransform);
    return Parser::parseTransform(value);
}

SpreadMethod GradientElement::spreadMethod() const
{
    auto& value = get(PropertyID::SpreadMethod);
    return Parser::parseSpreadMethod(value);
}

Units GradientElement::gradientUnits() const
{
    auto& value = get(PropertyID::GradientUnits);
    return Parser::parseUnits(value, Units::ObjectBoundingBox);
}

std::string GradientElement::href() const
{
    auto& value = get(PropertyID::Href);
    return Parser::parseHref(value);
}

GradientStops GradientElement::buildGradientStops() const
{
    GradientStops gradientStops;
    double prevOffset = 0.0;
    for(auto& child : children()) {
        if(child->isText())
            continue;
        auto element = static_cast<Element*>(child.get());
        if(element->id() != ElementID::Stop)
            continue;
        auto stop = static_cast<StopElement*>(element);
        auto offset = std::max(prevOffset, stop->offset());
        prevOffset = offset;
        gradientStops.emplace_back(offset, stop->stopColorWithOpacity());
    }

    return gradientStops;
}

LinearGradientElement::LinearGradientElement()
    : GradientElement(ElementID::LinearGradient)
{
}

Length LinearGradientElement::x1() const
{
    auto& value = get(PropertyID::X1);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length LinearGradientElement::y1() const
{
    auto& value = get(PropertyID::Y1);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length LinearGradientElement::x2() const
{
    auto& value = get(PropertyID::X2);
    return Parser::parseLength(value, AllowNegativeLengths, Length::HundredPercent);
}

Length LinearGradientElement::y2() const
{
    auto& value = get(PropertyID::Y2);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

std::unique_ptr<LayoutObject> LinearGradientElement::getPainter(LayoutContext* context)
{
    LinearGradientAttributes attributes;
    std::set<const GradientElement*> processedGradients;
    const GradientElement* current = this;

    while(true) {
        if(!attributes.hasGradientTransform() && current->has(PropertyID::GradientTransform))
            attributes.setGradientTransform(current->gradientTransform());
        if(!attributes.hasSpreadMethod() && current->has(PropertyID::SpreadMethod))
            attributes.setSpreadMethod(current->spreadMethod());
        if(!attributes.hasGradientUnits() && current->has(PropertyID::GradientUnits))
            attributes.setGradientUnits(current->gradientUnits());
        if(!attributes.hasGradientStops())
            attributes.setGradientStops(current->buildGradientStops());

        if(current->id() == ElementID::LinearGradient) {
            auto element = static_cast<const LinearGradientElement*>(current);
            if(!attributes.hasX1() && element->has(PropertyID::X1))
                attributes.setX1(element->x1());
            if(!attributes.hasY1() && element->has(PropertyID::Y1))
                attributes.setY1(element->y1());
            if(!attributes.hasX2() && element->has(PropertyID::X2))
                attributes.setX2(element->x2());
            if(!attributes.hasY2() && element->has(PropertyID::Y2))
                attributes.setY2(element->y2());
        }

        auto ref = context->getElementById(current->href());
        if(!ref || !(ref->id() == ElementID::LinearGradient || ref->id() == ElementID::RadialGradient))
            break;
        processedGradients.insert(current);
        current = static_cast<const GradientElement*>(ref);
        if(processedGradients.find(current) != processedGradients.end()) {
            break;
        }
    }

    auto& stops = attributes.gradientStops();
    if(stops.empty())
        return nullptr;

    LengthContext lengthContext(this, attributes.gradientUnits());
    auto x1 = lengthContext.valueForLength(attributes.x1(), LengthMode::Width);
    auto y1 = lengthContext.valueForLength(attributes.y1(), LengthMode::Height);
    auto x2 = lengthContext.valueForLength(attributes.x2(), LengthMode::Width);
    auto y2 = lengthContext.valueForLength(attributes.y2(), LengthMode::Height);
    if((x1 == x2 && y1 == y2) || stops.size() == 1) {
        auto solid = makeUnique<LayoutSolidColor>(this);
        solid->color = std::get<1>(stops.back());
        return std::move(solid);
    }

    auto gradient = makeUnique<LayoutLinearGradient>(this);
    gradient->transform = attributes.gradientTransform();
    gradient->spreadMethod = attributes.spreadMethod();
    gradient->units = attributes.gradientUnits();
    gradient->stops = attributes.gradientStops();
    gradient->x1 = x1;
    gradient->y1 = y1;
    gradient->x2 = x2;
    gradient->y2 = y2;
    return std::move(gradient);
}

RadialGradientElement::RadialGradientElement()
    : GradientElement(ElementID::RadialGradient)
{
}

Length RadialGradientElement::cx() const
{
    auto& value = get(PropertyID::Cx);
    return Parser::parseLength(value, AllowNegativeLengths, Length::FiftyPercent);
}

Length RadialGradientElement::cy() const
{
    auto& value = get(PropertyID::Cy);
    return Parser::parseLength(value, AllowNegativeLengths, Length::FiftyPercent);
}

Length RadialGradientElement::r() const
{
    auto& value = get(PropertyID::R);
    return Parser::parseLength(value, ForbidNegativeLengths, Length::FiftyPercent);
}

Length RadialGradientElement::fx() const
{
    auto& value = get(PropertyID::Fx);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length RadialGradientElement::fy() const
{
    auto& value = get(PropertyID::Fy);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

std::unique_ptr<LayoutObject> RadialGradientElement::getPainter(LayoutContext* context)
{
    RadialGradientAttributes attributes;
    std::set<const GradientElement*> processedGradients;
    const GradientElement* current = this;

    while(true) {
        if(!attributes.hasGradientTransform() && current->has(PropertyID::GradientTransform))
            attributes.setGradientTransform(current->gradientTransform());
        if(!attributes.hasSpreadMethod() && current->has(PropertyID::SpreadMethod))
            attributes.setSpreadMethod(current->spreadMethod());
        if(!attributes.hasGradientUnits() && current->has(PropertyID::GradientUnits))
            attributes.setGradientUnits(current->gradientUnits());
        if(!attributes.hasGradientStops())
            attributes.setGradientStops(current->buildGradientStops());

        if(current->id() == ElementID::RadialGradient) {
            auto element = static_cast<const RadialGradientElement*>(current);
            if(!attributes.hasCx() && element->has(PropertyID::Cx))
                attributes.setCx(element->cx());
            if(!attributes.hasCy() && element->has(PropertyID::Cy))
                attributes.setCy(element->cy());
            if(!attributes.hasR() && element->has(PropertyID::R))
                attributes.setR(element->r());
            if(!attributes.hasFx() && element->has(PropertyID::Fx))
                attributes.setFx(element->fx());
            if(!attributes.hasFy() && element->has(PropertyID::Fy))
                attributes.setFy(element->fy());
        }

        auto ref = context->getElementById(current->href());
        if(!ref || !(ref->id() == ElementID::LinearGradient || ref->id() == ElementID::RadialGradient))
            break;
        processedGradients.insert(current);
        current = static_cast<const GradientElement*>(ref);
        if(processedGradients.find(current) != processedGradients.end()) {
            break;
        }
    }

    if(!attributes.hasFx())
        attributes.setFx(attributes.cx());
    if(!attributes.hasFy())
        attributes.setFy(attributes.cy());

    auto& stops = attributes.gradientStops();
    if(stops.empty())
        return nullptr;

    auto& r = attributes.r();
    if(r.isZero() || stops.size() == 1) {
        auto solid = makeUnique<LayoutSolidColor>(this);
        solid->color = std::get<1>(stops.back());
        return std::move(solid);
    }

    auto gradient = makeUnique<LayoutRadialGradient>(this);
    gradient->transform = attributes.gradientTransform();
    gradient->spreadMethod = attributes.spreadMethod();
    gradient->units = attributes.gradientUnits();
    gradient->stops = attributes.gradientStops();

    LengthContext lengthContext(this, attributes.gradientUnits());
    gradient->cx = lengthContext.valueForLength(attributes.cx(), LengthMode::Width);
    gradient->cy = lengthContext.valueForLength(attributes.cy(), LengthMode::Height);
    gradient->r = lengthContext.valueForLength(attributes.r(), LengthMode::Both);
    gradient->fx = lengthContext.valueForLength(attributes.fx(), LengthMode::Width);
    gradient->fy = lengthContext.valueForLength(attributes.fy(), LengthMode::Height);
    return std::move(gradient);
}

PatternElement::PatternElement()
    : PaintElement(ElementID::Pattern)
{
}

Length PatternElement::x() const
{
    auto& value = get(PropertyID::X);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length PatternElement::y() const
{
    auto& value = get(PropertyID::Y);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length PatternElement::width() const
{
    auto& value = get(PropertyID::Width);
    return Parser::parseLength(value, ForbidNegativeLengths, Length::Zero);
}

Length PatternElement::height() const
{
    auto& value = get(PropertyID::Height);
    return Parser::parseLength(value, ForbidNegativeLengths, Length::Zero);
}

Transform PatternElement::patternTransform() const
{
    auto& value = get(PropertyID::PatternTransform);
    return Parser::parseTransform(value);
}

Units PatternElement::patternUnits() const
{
    auto& value = get(PropertyID::PatternUnits);
    return Parser::parseUnits(value, Units::ObjectBoundingBox);
}

Units PatternElement::patternContentUnits() const
{
    auto& value = get(PropertyID::PatternContentUnits);
    return Parser::parseUnits(value, Units::UserSpaceOnUse);
}

Rect PatternElement::viewBox() const
{
    auto& value = get(PropertyID::ViewBox);
    return Parser::parseViewBox(value);
}

PreserveAspectRatio PatternElement::preserveAspectRatio() const
{
    auto& value = get(PropertyID::PreserveAspectRatio);
    return Parser::parsePreserveAspectRatio(value);
}

std::string PatternElement::href() const
{
    auto& value = get(PropertyID::Href);
    return Parser::parseHref(value);
}

std::unique_ptr<LayoutObject> PatternElement::getPainter(LayoutContext* context)
{
    if(context->hasReference(this))
        return nullptr;

    PatternAttributes attributes;
    std::set<const PatternElement*> processedPatterns;
    PatternElement* current = this;

    while(true) {
        if(!attributes.hasX() && current->has(PropertyID::X))
            attributes.setX(current->x());
        if(!attributes.hasY() && current->has(PropertyID::Y))
            attributes.setY(current->y());
        if(!attributes.hasWidth() && current->has(PropertyID::Width))
            attributes.setWidth(current->width());
        if(!attributes.hasHeight() && current->has(PropertyID::Height))
            attributes.setHeight(current->height());
        if(!attributes.hasPatternTransform() && current->has(PropertyID::PatternTransform))
            attributes.setPatternTransform(current->patternTransform());
        if(!attributes.hasPatternUnits() && current->has(PropertyID::PatternUnits))
            attributes.setPatternUnits(current->patternUnits());
        if(!attributes.hasPatternContentUnits() && current->has(PropertyID::PatternContentUnits))
            attributes.setPatternContentUnits(current->patternContentUnits());
        if(!attributes.hasViewBox() && current->has(PropertyID::ViewBox))
            attributes.setViewBox(current->viewBox());
        if(!attributes.hasPreserveAspectRatio() && current->has(PropertyID::PreserveAspectRatio))
            attributes.setPreserveAspectRatio(current->preserveAspectRatio());
        if(!attributes.hasPatternContentElement() && !current->children().empty())
            attributes.setPatternContentElement(current);

        auto ref = context->getElementById(current->href());
        if(!ref || ref->id() != ElementID::Pattern)
            break;
        processedPatterns.insert(current);
        current = static_cast<PatternElement*>(ref);
        if(processedPatterns.find(current) != processedPatterns.end()) {
            break;
        }
    }

    auto& width = attributes.width();
    auto& height = attributes.height();
    auto element = attributes.patternContentElement();
    if(element == nullptr || width.isZero() || height.isZero())
        return nullptr;

    LayoutBreaker layoutBreaker(context, this);
    auto pattern = makeUnique<LayoutPattern>(this);
    pattern->transform = attributes.patternTransform();
    pattern->units = attributes.patternUnits();
    pattern->contentUnits = attributes.patternContentUnits();
    pattern->viewBox = attributes.viewBox();
    pattern->preserveAspectRatio = attributes.preserveAspectRatio();

    LengthContext lengthContext(this, attributes.patternUnits());
    pattern->x = lengthContext.valueForLength(attributes.x(), LengthMode::Width);
    pattern->y = lengthContext.valueForLength(attributes.y(), LengthMode::Height);
    pattern->width = lengthContext.valueForLength(attributes.width(), LengthMode::Width);
    pattern->height = lengthContext.valueForLength(attributes.height(), LengthMode::Height);
    element->layoutChildren(context, pattern.get());
    return std::move(pattern);
}

SolidColorElement::SolidColorElement()
    : PaintElement(ElementID::SolidColor)
{
}

std::unique_ptr<LayoutObject> SolidColorElement::getPainter(LayoutContext*)
{
    auto solid = makeUnique<LayoutSolidColor>(this);
    solid->color = solid_color();
    solid->color.combine(solid_opacity());
    return std::move(solid);
}

} // namespace lunasvg
