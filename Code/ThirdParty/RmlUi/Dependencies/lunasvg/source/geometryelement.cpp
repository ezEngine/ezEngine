#include "geometryelement.h"
#include "parser.h"
#include "layoutcontext.h"

namespace lunasvg {

GeometryElement::GeometryElement(ElementID id)
    : GraphicsElement(id)
{
}

void GeometryElement::layout(LayoutContext* context, LayoutContainer* current)
{
    if(isDisplayNone())
        return;

    auto path = this->path();
    if(path.empty())
        return;
    auto shape = makeUnique<LayoutShape>(this);
    shape->path = std::move(path);
    shape->transform = transform();
    shape->fillData = context->fillData(this);
    shape->strokeData = context->strokeData(this);
    shape->markerData = context->markerData(this, shape->path);
    shape->visibility = visibility();
    shape->clipRule = clip_rule();
    shape->opacity = opacity();
    shape->masker = context->getMasker(mask());
    shape->clipper = context->getClipper(clip_path());
    current->addChild(std::move(shape));
}

PathElement::PathElement()
    : GeometryElement(ElementID::Path)
{
}

Path PathElement::d() const
{
    auto& value = get(PropertyID::D);
    return Parser::parsePath(value);
}

Path PathElement::path() const
{
    return d();
}

PolyElement::PolyElement(ElementID id)
    : GeometryElement(id)
{
}

PointList PolyElement::points() const
{
    auto& value = get(PropertyID::Points);
    return Parser::parsePointList(value);
}

PolygonElement::PolygonElement()
    : PolyElement(ElementID::Polygon)
{
}

Path PolygonElement::path() const
{
    auto points = this->points();
    if(points.empty())
        return Path{};

    Path path;
    path.moveTo(points[0].x, points[0].y);
    for(std::size_t i = 1; i < points.size(); i++)
        path.lineTo(points[i].x, points[i].y);

    path.close();
    return path;
}

PolylineElement::PolylineElement()
    : PolyElement(ElementID::Polyline)
{
}

Path PolylineElement::path() const
{
    auto points = this->points();
    if(points.empty())
        return Path{};

    Path path;
    path.moveTo(points[0].x, points[0].y);
    for(std::size_t i = 1; i < points.size(); i++)
        path.lineTo(points[i].x, points[i].y);

    return path;
}

CircleElement::CircleElement()
    : GeometryElement(ElementID::Circle)
{
}

Length CircleElement::cx() const
{
    auto& value = get(PropertyID::Cx);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length CircleElement::cy() const
{
    auto& value = get(PropertyID::Cy);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length CircleElement::r() const
{
    auto& value = get(PropertyID::R);
    return Parser::parseLength(value, ForbidNegativeLengths, Length::Zero);
}

Path CircleElement::path() const
{
    auto r = this->r();
    if(r.isZero())
        return Path{};

    LengthContext lengthContext(this);
    auto _cx = lengthContext.valueForLength(cx(), LengthMode::Width);
    auto _cy = lengthContext.valueForLength(cy(), LengthMode::Height);
    auto _r = lengthContext.valueForLength(r, LengthMode::Both);

    Path path;
    path.ellipse(_cx, _cy, _r, _r);
    return path;
}

EllipseElement::EllipseElement()
    : GeometryElement(ElementID::Ellipse)
{
}

Length EllipseElement::cx() const
{
    auto& value = get(PropertyID::Cx);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length EllipseElement::cy() const
{
    auto& value = get(PropertyID::Cy);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length EllipseElement::rx() const
{
    auto& value = get(PropertyID::Rx);
    return Parser::parseLength(value, ForbidNegativeLengths, Length::Zero);
}

Length EllipseElement::ry() const
{
    auto& value = get(PropertyID::Ry);
    return Parser::parseLength(value, ForbidNegativeLengths, Length::Zero);
}

Path EllipseElement::path() const
{
    auto rx = this->rx();
    auto ry = this->ry();
    if(rx.isZero() || ry.isZero())
        return Path{};

    LengthContext lengthContext(this);
    auto _cx = lengthContext.valueForLength(cx(), LengthMode::Width);
    auto _cy = lengthContext.valueForLength(cy(), LengthMode::Height);
    auto _rx = lengthContext.valueForLength(rx, LengthMode::Width);
    auto _ry = lengthContext.valueForLength(ry, LengthMode::Height);

    Path path;
    path.ellipse(_cx, _cy, _rx, _ry);
    return path;
}

LineElement::LineElement()
    : GeometryElement(ElementID::Line)
{
}

Length LineElement::x1() const
{
    auto& value = get(PropertyID::X1);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length LineElement::y1() const
{
    auto& value = get(PropertyID::Y1);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length LineElement::x2() const
{
    auto& value = get(PropertyID::X2);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length LineElement::y2() const
{
    auto& value = get(PropertyID::Y2);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Path LineElement::path() const
{
    LengthContext lengthContext(this);
    auto _x1 = lengthContext.valueForLength(x1(), LengthMode::Width);
    auto _y1 = lengthContext.valueForLength(y1(), LengthMode::Height);
    auto _x2 = lengthContext.valueForLength(x2(), LengthMode::Width);
    auto _y2 = lengthContext.valueForLength(y2(), LengthMode::Height);

    Path path;
    path.moveTo(_x1, _y1);
    path.lineTo(_x2, _y2);
    return path;
}

RectElement::RectElement()
    : GeometryElement(ElementID::Rect)
{
}

Length RectElement::x() const
{
    auto& value = get(PropertyID::X);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length RectElement::y() const
{
    auto& value = get(PropertyID::Y);
    return Parser::parseLength(value, AllowNegativeLengths, Length::Zero);
}

Length RectElement::rx() const
{
    auto& value = get(PropertyID::Rx);
    return Parser::parseLength(value, ForbidNegativeLengths, Length::Unknown);
}

Length RectElement::ry() const
{
    auto& value = get(PropertyID::Ry);
    return Parser::parseLength(value, ForbidNegativeLengths, Length::Unknown);
}

Length RectElement::width() const
{
    auto& value = get(PropertyID::Width);
    return Parser::parseLength(value, ForbidNegativeLengths, Length::Zero);
}

Length RectElement::height() const
{
    auto& value = get(PropertyID::Height);
    return Parser::parseLength(value, ForbidNegativeLengths, Length::Zero);
}

Path RectElement::path() const
{
    auto w = this->width();
    auto h = this->height();
    if(w.isZero() || h.isZero())
        return Path{};

    LengthContext lengthContext(this);
    auto _x = lengthContext.valueForLength(x(), LengthMode::Width);
    auto _y = lengthContext.valueForLength(y(), LengthMode::Height);
    auto _w = lengthContext.valueForLength(w, LengthMode::Width);
    auto _h = lengthContext.valueForLength(h, LengthMode::Height);

    auto rx = this->rx();
    auto ry = this->ry();

    auto _rx = lengthContext.valueForLength(rx, LengthMode::Width);
    auto _ry = lengthContext.valueForLength(ry, LengthMode::Height);

    if(!rx.isValid()) _rx = _ry;
    if(!ry.isValid()) _ry = _rx;

    Path path;
    path.rect(_x, _y, _w, _h, _rx, _ry);
    return path;
}

} // namespace lunasvg
