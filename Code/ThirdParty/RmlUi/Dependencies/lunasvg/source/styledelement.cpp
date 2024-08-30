#include "styledelement.h"
#include "parser.h"

namespace lunasvg {

StyledElement::StyledElement(ElementID id)
    : Element(id)
{
}

Paint StyledElement::fill() const
{
    auto& value = find(PropertyID::Fill);
    return Parser::parsePaint(value, this, Color::Black);
}

Paint StyledElement::stroke() const
{
    auto& value = find(PropertyID::Stroke);
    return Parser::parsePaint(value, this, Color::Transparent);
}

Color StyledElement::color() const
{
    auto& value = find(PropertyID::Color);
    return Parser::parseColor(value, this, Color::Black);
}

Color StyledElement::stop_color() const
{
    auto& value = find(PropertyID::Stop_Color);
    return Parser::parseColor(value, this, Color::Black);
}

Color StyledElement::solid_color() const
{
    auto& value = find(PropertyID::Solid_Color);
    return Parser::parseColor(value, this, Color::Black);
}

double StyledElement::opacity() const
{
    auto& value = get(PropertyID::Opacity);
    return Parser::parseNumberPercentage(value, 1.0);
}

double StyledElement::fill_opacity() const
{
    auto& value = find(PropertyID::Fill_Opacity);
    return Parser::parseNumberPercentage(value, 1.0);
}

double StyledElement::stroke_opacity() const
{
    auto& value = find(PropertyID::Stroke_Opacity);
    return Parser::parseNumberPercentage(value, 1.0);
}

double StyledElement::stop_opacity() const
{
    auto& value = find(PropertyID::Stop_Opacity);
    return Parser::parseNumberPercentage(value, 1.0);
}

double StyledElement::solid_opacity() const
{
    auto& value = find(PropertyID::Solid_Opacity);
    return Parser::parseNumberPercentage(value, 1.0);
}

double StyledElement::stroke_miterlimit() const
{
    auto& value = find(PropertyID::Stroke_Miterlimit);
    return Parser::parseNumber(value, 4.0);
}

Length StyledElement::stroke_width() const
{
    auto& value = find(PropertyID::Stroke_Width);
    return Parser::parseLength(value, ForbidNegativeLengths, Length::One);
}

Length StyledElement::stroke_dashoffset() const
{
    auto& value = find(PropertyID::Stroke_Dashoffset);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

LengthList StyledElement::stroke_dasharray() const
{
    auto& value = find(PropertyID::Stroke_Dasharray);
    return Parser::parseLengthList(value, ForbidNegativeLengths);
}

WindRule StyledElement::fill_rule() const
{
    auto& value = find(PropertyID::Fill_Rule);
    return Parser::parseWindRule(value);
}

WindRule StyledElement::clip_rule() const
{
    auto& value = find(PropertyID::Clip_Rule);
    return Parser::parseWindRule(value);
}

LineCap StyledElement::stroke_linecap() const
{
    auto& value = find(PropertyID::Stroke_Linecap);
    return Parser::parseLineCap(value);
}

LineJoin StyledElement::stroke_linejoin() const
{
    auto& value = find(PropertyID::Stroke_Linejoin);
    return Parser::parseLineJoin(value);
}

Display StyledElement::display() const
{
    auto& value = get(PropertyID::Display);
    return Parser::parseDisplay(value);
}

Visibility StyledElement::visibility() const
{
    auto& value = find(PropertyID::Visibility);
    return Parser::parseVisibility(value);
}

Overflow StyledElement::overflow() const
{
    auto& value = get(PropertyID::Overflow);
    return Parser::parseOverflow(value, parent() == nullptr ? Overflow::Visible : Overflow::Hidden);
}

std::string StyledElement::clip_path() const
{
    auto& value = get(PropertyID::Clip_Path);
    return Parser::parseUrl(value);
}

std::string StyledElement::mask() const
{
    auto& value = get(PropertyID::Mask);
    return Parser::parseUrl(value);
}

std::string StyledElement::marker_start() const
{
    auto& value = find(PropertyID::Marker_Start);
    return Parser::parseUrl(value);
}

std::string StyledElement::marker_mid() const
{
    auto& value = find(PropertyID::Marker_Mid);
    return Parser::parseUrl(value);
}

std::string StyledElement::marker_end() const
{
    auto& value = find(PropertyID::Marker_End);
    return Parser::parseUrl(value);
}

bool StyledElement::isDisplayNone() const
{
    return display() == Display::None;
}

bool StyledElement::isOverflowHidden() const
{
    return overflow() == Overflow::Hidden;
}

} // namespace lunasvg
