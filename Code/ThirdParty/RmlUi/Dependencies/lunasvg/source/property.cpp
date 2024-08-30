#include "property.h"
#include "element.h"
#include "lunasvg.h"

#include <algorithm>
#include <cmath>

namespace lunasvg {

const Color Color::Black(0xFF000000);
const Color Color::White(0xFFFFFFFF);
const Color Color::Transparent(0x00000000);

Color& Color::combine(double opacity)
{
    *this = combined(opacity);
    return *this;
}

Color Color::combined(double opacity) const
{
    auto rgb = m_value & 0x00FFFFFF;
    auto a = static_cast<int>(clamp(opacity * alpha(), 0.0, 255.0));
    return Color(rgb | a << 24);
}

Paint::Paint(const Color& color)
    : m_color(color)
{
}

Paint::Paint(const std::string& ref, const Color& color)
    : m_ref(ref), m_color(color)
{
}

Point::Point(double x, double y)
    : x(x), y(y)
{
}

const Rect Rect::Empty{0, 0, 0, 0};
const Rect Rect::Invalid{0, 0, -1, -1};

Rect::Rect(double x, double y, double w, double h)
    : x(x), y(y), w(w), h(h)
{
}

Rect::Rect(const Box& box)
    : x(box.x), y(box.y), w(box.w), h(box.h)
{
}

Rect Rect::operator&(const Rect& rect) const
{
    if(!rect.valid())
        return *this;

    if(!valid())
        return rect;

    auto l = std::max(x, rect.x);
    auto t = std::max(y, rect.y);
    auto r = std::min(x + w, rect.x + rect.w);
    auto b = std::min(y + h, rect.y + rect.h);

    return Rect{l, t, r-l, b-t};
}

Rect Rect::operator|(const Rect& rect) const
{
    if(!rect.valid())
        return *this;

    if(!valid())
        return rect;

    auto l = std::min(x, rect.x);
    auto t = std::min(y, rect.y);
    auto r = std::max(x + w, rect.x + rect.w);
    auto b = std::max(y + h, rect.y + rect.h);

    return Rect{l, t, r-l, b-t};
}

Rect& Rect::intersect(const Rect& rect)
{
    *this = *this & rect;
    return *this;
}

Rect& Rect::unite(const Rect& rect)
{
    *this = *this | rect;
    return *this;
}

const Transform Transform::Identity(1, 0, 0, 1, 0, 0);

Transform::Transform(double m00, double m10, double m01, double m11, double m02, double m12)
    : m00(m00), m10(m10), m01(m01), m11(m11), m02(m02), m12(m12)
{
}

Transform::Transform(const Matrix& matrix)
    : m00(matrix.a), m10(matrix.b), m01(matrix.c), m11(matrix.d), m02(matrix.e), m12(matrix.f)
{
}

Transform Transform::inverted() const
{
    double det = (m00 * m11 - m10 * m01);
    if(det == 0.0)
        return Transform{};

    double inv_det = 1.0 / det;
    double _m00 = m00 * inv_det;
    double _m10 = m10 * inv_det;
    double _m01 = m01 * inv_det;
    double _m11 = m11 * inv_det;
    double _m02 = (m01 * m12 - m11 * m02) * inv_det;
    double _m12 = (m10 * m02 - m00 * m12) * inv_det;

    return Transform{_m11, -_m10, -_m01, _m00, _m02, _m12};
}

Transform Transform::operator*(const Transform& transform) const
{
    double _m00 = m00 * transform.m00 + m10 * transform.m01;
    double _m10 = m00 * transform.m10 + m10 * transform.m11;
    double _m01 = m01 * transform.m00 + m11 * transform.m01;
    double _m11 = m01 * transform.m10 + m11 * transform.m11;
    double _m02 = m02 * transform.m00 + m12 * transform.m01 + transform.m02;
    double _m12 = m02 * transform.m10 + m12 * transform.m11 + transform.m12;

    return Transform{_m00, _m10, _m01, _m11, _m02, _m12};
}

Transform& Transform::operator*=(const Transform& transform)
{
    *this = *this * transform;
    return *this;
}

Transform& Transform::premultiply(const Transform& transform)
{
    *this = transform * *this;
    return *this;
}

Transform& Transform::postmultiply(const Transform& transform)
{
    *this = *this * transform;
    return *this;
}

Transform& Transform::rotate(double angle)
{
    *this = rotated(angle) * *this;
    return *this;
}

Transform& Transform::rotate(double angle, double cx, double cy)
{
    *this = rotated(angle, cx, cy) * *this;
    return *this;
}

Transform& Transform::scale(double sx, double sy)
{
    *this = scaled(sx, sy) * *this;
    return *this;
}

Transform& Transform::shear(double shx, double shy)
{
    *this = sheared(shx, shy) * *this;
    return *this;
}

Transform& Transform::translate(double tx, double ty)
{
    *this = translated(tx, ty) * *this;
    return *this;
}

Transform& Transform::transform(double _m00, double _m10, double _m01, double _m11, double _m02, double _m12)
{
    *this = Transform{_m00, _m10, _m01, _m11, _m02, _m12} * *this;
    return *this;
}

Transform& Transform::identity()
{
    *this = Transform{1, 0, 0, 1, 0, 0};
    return *this;
}

Transform& Transform::invert()
{
    *this = inverted();
    return *this;
}

void Transform::map(double x, double y, double* _x, double* _y) const
{
    *_x = x * m00 + y * m01 + m02;
    *_y = x * m10 + y * m11 + m12;
}

Point Transform::map(double x, double y) const
{
    map(x, y, &x, &y);
    return Point{x, y};
}

Point Transform::map(const Point& point) const
{
    return map(point.x, point.y);
}

Rect Transform::map(const Rect& rect) const
{
    if(!rect.valid())
        return Rect::Invalid;

    auto x1 = rect.x;
    auto y1 = rect.y;
    auto x2 = rect.x + rect.w;
    auto y2 = rect.y + rect.h;

    const Point p[] = {
        map(x1, y1), map(x2, y1),
        map(x2, y2), map(x1, y2)
    };

    auto l = p[0].x;
    auto t = p[0].y;
    auto r = p[0].x;
    auto b = p[0].y;

    for(int i = 1; i < 4; i++) {
        if(p[i].x < l) l = p[i].x;
        if(p[i].x > r) r = p[i].x;
        if(p[i].y < t) t = p[i].y;
        if(p[i].y > b) b = p[i].y;
    }

    return Rect{l, t, r-l, b-t};
}

static const double pi = 3.14159265358979323846;

Transform Transform::rotated(double angle)
{
    auto c = std::cos(angle * pi / 180.0);
    auto s = std::sin(angle * pi / 180.0);

    return Transform{c, s, -s, c, 0, 0};
}

Transform Transform::rotated(double angle, double cx, double cy)
{
    auto c = std::cos(angle * pi / 180.0);
    auto s = std::sin(angle * pi / 180.0);

    auto x = cx * (1 - c) + cy * s;
    auto y = cy * (1 - c) - cx * s;

    return Transform{c, s, -s, c, x, y};
}

Transform Transform::scaled(double sx, double sy)
{
    return Transform{sx, 0, 0, sy, 0, 0};
}

Transform Transform::sheared(double shx, double shy)
{
    auto x = std::tan(shx * pi / 180.0);
    auto y = std::tan(shy * pi / 180.0);

    return Transform{1, y, x, 1, 0, 0};
}

Transform Transform::translated(double tx, double ty)
{
    return Transform{1, 0, 0, 1, tx, ty};
}

void Path::moveTo(double x, double y)
{
    m_commands.push_back(PathCommand::MoveTo);
    m_points.emplace_back(x, y);
}

void Path::lineTo(double x, double y)
{
    m_commands.push_back(PathCommand::LineTo);
    m_points.emplace_back(x, y);
}

void Path::cubicTo(double x1, double y1, double x2, double y2, double x3, double y3)
{
    m_commands.push_back(PathCommand::CubicTo);
    m_points.emplace_back(x1, y1);
    m_points.emplace_back(x2, y2);
    m_points.emplace_back(x3, y3);
}

void Path::close()
{
    if(m_commands.empty())
        return;

    if(m_commands.back() == PathCommand::Close)
        return;

    m_commands.push_back(PathCommand::Close);
}

void Path::reset()
{
    m_commands.clear();
    m_points.clear();
}

bool Path::empty() const
{
    return m_commands.empty();
}

void Path::quadTo(double cx, double cy, double x1, double y1, double x2, double y2)
{
    auto cx1 = 2.0 / 3.0 * x1 + 1.0 / 3.0 * cx;
    auto cy1 = 2.0 / 3.0 * y1 + 1.0 / 3.0 * cy;
    auto cx2 = 2.0 / 3.0 * x1 + 1.0 / 3.0 * x2;
    auto cy2 = 2.0 / 3.0 * y1 + 1.0 / 3.0 * y2;
    cubicTo(cx1, cy1, cx2, cy2, x2, y2);
}

void Path::arcTo(double cx, double cy, double rx, double ry, double xAxisRotation, bool largeArcFlag, bool sweepFlag, double x, double y)
{
    rx = std::fabs(rx);
    ry = std::fabs(ry);

    auto sin_th = std::sin(xAxisRotation * pi / 180.0);
    auto cos_th = std::cos(xAxisRotation * pi / 180.0);

    auto dx = (cx - x) / 2.0;
    auto dy = (cy - y) / 2.0;
    auto dx1 =  cos_th * dx + sin_th * dy;
    auto dy1 = -sin_th * dx + cos_th * dy;
    auto Pr1 = rx * rx;
    auto Pr2 = ry * ry;
    auto Px = dx1 * dx1;
    auto Py = dy1 * dy1;
    auto check = Px / Pr1 + Py / Pr2;
    if(check > 1) {
        rx = rx * std::sqrt(check);
        ry = ry * std::sqrt(check);
    }

    auto a00 =  cos_th / rx;
    auto a01 =  sin_th / rx;
    auto a10 = -sin_th / ry;
    auto a11 =  cos_th / ry;
    auto x0 = a00 * cx + a01 * cy;
    auto y0 = a10 * cx + a11 * cy;
    auto x1 = a00 * x + a01 * y;
    auto y1 = a10 * x + a11 * y;
    auto d = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
    auto sfactor_sq = 1.0 / d - 0.25;
    if(sfactor_sq < 0) sfactor_sq = 0;
    auto sfactor = std::sqrt(sfactor_sq);
    if(sweepFlag == largeArcFlag) sfactor = -sfactor;
    auto xc = 0.5 * (x0 + x1) - sfactor * (y1 - y0);
    auto yc = 0.5 * (y0 + y1) + sfactor * (x1 - x0);

    auto th0 = std::atan2(y0 - yc, x0 - xc);
    auto th1 = std::atan2(y1 - yc, x1 - xc);

    auto th_arc = th1 - th0;
    if(th_arc < 0.0 && sweepFlag)
        th_arc += 2.0 * pi;
    else if(th_arc > 0.0 && !sweepFlag)
        th_arc -= 2.0 * pi;

    auto n_segs = static_cast<int>(std::ceil(std::fabs(th_arc / (pi * 0.5 + 0.001))));
    for(int i = 0; i < n_segs; i++) {
        auto th2 = th0 + i * th_arc / n_segs;
        auto th3 = th0 + (i + 1) * th_arc / n_segs;

        auto _a00 =  cos_th * rx;
        auto _a01 = -sin_th * ry;
        auto _a10 =  sin_th * rx;
        auto _a11 =  cos_th * ry;

        auto thHalf = 0.5 * (th3 - th2);
        auto t = (8.0 / 3.0) * std::sin(thHalf * 0.5) * std::sin(thHalf * 0.5) / std::sin(thHalf);
        auto _x1 = xc + std::cos(th2) - t * std::sin(th2);
        auto _y1 = yc + std::sin(th2) + t * std::cos(th2);
        auto _x3 = xc + std::cos(th3);
        auto _y3 = yc + std::sin(th3);
        auto _x2 = _x3 + t * std::sin(th3);
        auto _y2 = _y3 - t * std::cos(th3);

        auto cx1 = _a00 * _x1 + _a01 * _y1;
        auto cy1 = _a10 * _x1 + _a11 * _y1;
        auto cx2 = _a00 * _x2 + _a01 * _y2;
        auto cy2 = _a10 * _x2 + _a11 * _y2;
        auto cx3 = _a00 * _x3 + _a01 * _y3;
        auto cy3 = _a10 * _x3 + _a11 * _y3;
        cubicTo(cx1, cy1, cx2, cy2, cx3, cy3);
    }
}

static const double kappa = 0.55228474983079339840;

void Path::ellipse(double cx, double cy, double rx, double ry)
{
    auto left = cx - rx;
    auto top = cy - ry;
    auto right = cx + rx;
    auto bottom = cy + ry;

    auto cpx = rx * kappa;
    auto cpy = ry * kappa;

    moveTo(cx, top);
    cubicTo(cx+cpx, top, right, cy-cpy, right, cy);
    cubicTo(right, cy+cpy, cx+cpx, bottom, cx, bottom);
    cubicTo(cx-cpx, bottom, left, cy+cpy, left, cy);
    cubicTo(left, cy-cpy, cx-cpx, top, cx, top);
    close();
}

void Path::rect(double x, double y, double w, double h, double rx, double ry)
{
    rx = std::min(rx, w * 0.5);
    ry = std::min(ry, h * 0.5);

    auto right = x + w;
    auto bottom = y + h;

    if(rx == 0.0 && ry == 0.0) {
        moveTo(x, y);
        lineTo(right, y);
        lineTo(right, bottom);
        lineTo(x, bottom);
        lineTo(x, y);
        close();
    } else {
        double cpx = rx * kappa;
        double cpy = ry * kappa;
        moveTo(x, y+ry);
        cubicTo(x, y+ry-cpy, x+rx-cpx, y, x+rx, y);
        lineTo(right-rx, y);
        cubicTo(right-rx+cpx, y, right, y+ry-cpy, right, y+ry);
        lineTo(right, bottom-ry);
        cubicTo(right, bottom-ry+cpy, right-rx+cpx, bottom, right-rx, bottom);
        lineTo(x+rx, bottom);
        cubicTo(x+rx-cpx, bottom, x, bottom-ry+cpy, x, bottom-ry);
        lineTo(x, y+ry);
        close();
    }
}

Rect Path::box() const
{
    if(m_points.empty())
        return Rect{};

    auto l = m_points[0].x;
    auto t = m_points[0].y;
    auto r = m_points[0].x;
    auto b = m_points[0].y;

    for(std::size_t i = 1; i < m_points.size(); i++) {
        if(m_points[i].x < l) l = m_points[i].x;
        if(m_points[i].x > r) r = m_points[i].x;
        if(m_points[i].y < t) t = m_points[i].y;
        if(m_points[i].y > b) b = m_points[i].y;
    }

    return Rect{l, t, r-l, b-t};
}

PathIterator::PathIterator(const Path& path)
    : m_commands(path.commands()),
      m_points(path.points().data())
{
}

PathCommand PathIterator::currentSegment(std::array<Point, 3>& points) const
{
    auto command = m_commands[m_index];
    switch(command) {
    case PathCommand::MoveTo:
        points[0] = m_points[0];
        m_startPoint = points[0];
        break;
    case PathCommand::LineTo:
        points[0] = m_points[0];
        break;
    case PathCommand::CubicTo:
        points[0] = m_points[0];
        points[1] = m_points[1];
        points[2] = m_points[2];
        break;
    case PathCommand::Close:
        points[0] = m_startPoint;
        break;
    }

    return command;
}

bool PathIterator::isDone() const
{
    return (m_index >= m_commands.size());
}

void PathIterator::next()
{
    switch(m_commands[m_index]) {
    case PathCommand::MoveTo:
    case PathCommand::LineTo:
        m_points += 1;
        break;
    case PathCommand::CubicTo:
        m_points += 3;
        break;
    default:
        break;
    }

    m_index += 1;
}

const Length Length::Unknown{0, LengthUnits::Unknown};
const Length Length::Zero{0, LengthUnits::Number};
const Length Length::One{1, LengthUnits::Number};
const Length Length::Three{3, LengthUnits::Number};
const Length Length::HundredPercent{100, LengthUnits::Percent};
const Length Length::FiftyPercent{50, LengthUnits::Percent};
const Length Length::OneTwentyPercent{120, LengthUnits::Percent};
const Length Length::MinusTenPercent{-10, LengthUnits::Percent};

Length::Length(double value)
    : m_value(value)
{
}

Length::Length(double value, LengthUnits units)
    : m_value(value), m_units(units)
{
}

static const double dpi = 96.0;

double Length::value(double max) const
{
    switch(m_units) {
    case LengthUnits::Number:
    case LengthUnits::Px:
        return m_value;
    case LengthUnits::In:
        return m_value * dpi;
    case LengthUnits::Cm:
        return m_value * dpi / 2.54;
    case LengthUnits::Mm:
        return m_value * dpi / 25.4;
    case LengthUnits::Pt:
        return m_value * dpi / 72.0;
    case LengthUnits::Pc:
        return m_value * dpi / 6.0;
    case LengthUnits::Percent:
        return m_value * max / 100.0;
    default:
        break;
    }

    return 0.0;
}

static const double sqrt2 = 1.41421356237309504880;

double Length::value(const Element* element, LengthMode mode) const
{
    if(m_units == LengthUnits::Percent) {
        auto viewport = element->currentViewport();
        auto w = viewport.w;
        auto h = viewport.h;
        auto max = (mode == LengthMode::Width) ? w : (mode == LengthMode::Height) ? h : std::sqrt(w*w+h*h) / sqrt2;
        return m_value * max / 100.0;
    }

    return value(1.0);
}

LengthContext::LengthContext(const Element* element)
    : m_element(element)
{
}

LengthContext::LengthContext(const Element* element, Units units)
    : m_element(element), m_units(units)
{
}

double LengthContext::valueForLength(const Length& length, LengthMode mode) const
{
    if(m_units == Units::ObjectBoundingBox)
        return length.value(1.0);
    return length.value(m_element, mode);
}

PreserveAspectRatio::PreserveAspectRatio(Align align, MeetOrSlice scale)
    : m_align(align), m_scale(scale)
{
}

Transform PreserveAspectRatio::getMatrix(double width, double height, const Rect& viewBox) const
{
    if(viewBox.empty())
        return Transform{};

    auto xscale = width / viewBox.w;
    auto yscale = height / viewBox.h;
    if(m_align == Align::None) {
        auto xoffset = -viewBox.x * xscale;
        auto yoffset = -viewBox.y * yscale;
        return Transform{xscale, 0, 0, yscale, xoffset, yoffset};
    }

    auto scale = (m_scale == MeetOrSlice::Meet) ? std::min(xscale, yscale) : std::max(xscale, yscale);
    auto viewWidth = viewBox.w * scale;
    auto viewHeight = viewBox.h * scale;

    auto xoffset = -viewBox.x * scale;
    auto yoffset = -viewBox.y * scale;

    switch(m_align) {
    case Align::xMidYMin:
    case Align::xMidYMid:
    case Align::xMidYMax:
        xoffset += (width - viewWidth) * 0.5;
        break;
    case Align::xMaxYMin:
    case Align::xMaxYMid:
    case Align::xMaxYMax:
        xoffset += (width - viewWidth);
        break;
    default:
        break;
    }

    switch(m_align) {
    case Align::xMinYMid:
    case Align::xMidYMid:
    case Align::xMaxYMid:
        yoffset += (height - viewHeight) * 0.5;
        break;
    case Align::xMinYMax:
    case Align::xMidYMax:
    case Align::xMaxYMax:
        yoffset += (height - viewHeight);
        break;
    default:
        break;
    }

    return Transform{scale, 0, 0, scale, xoffset, yoffset};
}

Rect PreserveAspectRatio::getClip(double width, double height, const Rect& viewBox) const
{
    if(viewBox.empty())
        return Rect{0, 0, width, height};
    if(m_scale == MeetOrSlice::Meet)
        return viewBox;
    auto scale = std::max(width / viewBox.w, height / viewBox.h);
    auto xOffset = -viewBox.x * scale;
    auto yOffset = -viewBox.y * scale;
    auto viewWidth = viewBox.w * scale;
    auto viewHeight = viewBox.h * scale;
    switch(m_align) {
    case Align::xMidYMin:
    case Align::xMidYMid:
    case Align::xMidYMax:
        xOffset += (width - viewWidth) * 0.5f;
        break;
    case Align::xMaxYMin:
    case Align::xMaxYMid:
    case Align::xMaxYMax:
        xOffset += (width - viewWidth);
        break;
    default:
        break;
    }

    switch(m_align) {
    case Align::xMinYMid:
    case Align::xMidYMid:
    case Align::xMaxYMid:
        yOffset += (height - viewHeight) * 0.5f;
        break;
    case Align::xMinYMax:
    case Align::xMidYMax:
    case Align::xMaxYMax:
        yOffset += (height - viewHeight);
        break;
    default:
        break;
    }

    return Rect(-xOffset / scale, -yOffset / scale, width / scale, height / scale);
}

Angle::Angle(MarkerOrient type)
    : m_type(type)
{
}

Angle::Angle(double value, MarkerOrient type)
    : m_value(value), m_type(type)
{
}

} // namespace lunasvg
