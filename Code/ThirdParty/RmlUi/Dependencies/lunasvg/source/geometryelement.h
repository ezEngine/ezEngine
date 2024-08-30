#ifndef GEOMETRYELEMENT_H
#define GEOMETRYELEMENT_H

#include "graphicselement.h"

namespace lunasvg {

class LayoutShape;

class GeometryElement : public GraphicsElement {
public:
    GeometryElement(ElementID id);

    bool isGeometry() const final { return true; }
    void layout(LayoutContext* context, LayoutContainer* current) final;
    virtual Path path() const = 0;
};

class PathElement final : public GeometryElement {
public:
    PathElement();

    Path d() const;
    Path path() const final;
};

class PolyElement : public GeometryElement {
public:
    PolyElement(ElementID id);

    PointList points() const;
};

class PolygonElement final : public PolyElement {
public:
    PolygonElement();

    Path path() const final;
};

class PolylineElement final : public PolyElement {
public:
    PolylineElement();

    Path path() const final;
};

class CircleElement final : public GeometryElement {
public:
    CircleElement();

    Length cx() const;
    Length cy() const;
    Length r() const;

    Path path() const final;
};

class EllipseElement final : public GeometryElement {
public:
    EllipseElement();

    Length cx() const;
    Length cy() const;
    Length rx() const;
    Length ry() const;

    Path path() const final;
};

class LineElement final : public GeometryElement {
public:
    LineElement();

    Length x1() const;
    Length y1() const;
    Length x2() const;
    Length y2() const;

    Path path() const final;
};

class RectElement final : public GeometryElement {
public:
    RectElement();

    Length x() const;
    Length y() const;
    Length rx() const;
    Length ry() const;
    Length width() const;
    Length height() const;

    Path path() const final;
};

} // namespace lunasvg

#endif // GEOMETRYELEMENT_H
