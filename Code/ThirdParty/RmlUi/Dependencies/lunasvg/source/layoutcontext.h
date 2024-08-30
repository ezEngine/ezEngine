#ifndef LAYOUTCONTEXT_H
#define LAYOUTCONTEXT_H

#include "property.h"
#include "canvas.h"

#include <list>
#include <map>
#include <set>

namespace lunasvg {

enum class LayoutId {
    Symbol,
    Group,
    Shape,
    Mask,
    ClipPath,
    Marker,
    LinearGradient,
    RadialGradient,
    Pattern,
    SolidColor
};

class RenderState;
class Node;

class LayoutObject {
public:
    LayoutObject(Node* node, LayoutId id);
    virtual ~LayoutObject() = default;
    virtual void render(RenderState&) const {}
    virtual void apply(RenderState&) const {}

    Rect map(const Rect& rect) const { return localTransform().map(rect); }

    virtual const Transform& localTransform() const { return Transform::Identity; }
    virtual const Rect& fillBoundingBox() const { return Rect::Invalid; }
    virtual const Rect& strokeBoundingBox() const { return Rect::Invalid; }

    bool isPaint() const { return m_id == LayoutId::LinearGradient || m_id == LayoutId::RadialGradient || m_id == LayoutId::Pattern || m_id == LayoutId::SolidColor; }
    bool isHidden() const { return isPaint() || m_id == LayoutId::ClipPath || m_id == LayoutId::Mask || m_id == LayoutId::Marker; }

    Node* node() const { return m_node; }
    LayoutId id() const { return m_id; }

private:
    Node* m_node;
    LayoutId m_id;
};

using LayoutList = std::list<std::unique_ptr<LayoutObject>>;

class LayoutContainer : public LayoutObject {
public:
    LayoutContainer(Node* node, LayoutId id);

    const Rect& fillBoundingBox() const;
    const Rect& strokeBoundingBox() const;
    const LayoutList& children() const { return m_children; }

    LayoutObject* addChild(std::unique_ptr<LayoutObject> child);
    LayoutObject* addChildIfNotEmpty(std::unique_ptr<LayoutContainer> child);
    void renderChildren(RenderState& state) const;

protected:
    LayoutList m_children;
    mutable Rect m_fillBoundingBox{Rect::Invalid};
    mutable Rect m_strokeBoundingBox{Rect::Invalid};
};

class LayoutClipPath : public LayoutContainer {
public:
    LayoutClipPath(Node* node);

    void apply(RenderState& state) const;

public:
    Units units;
    Transform transform;
    const LayoutClipPath* clipper;
};

class LayoutMask : public LayoutContainer {
public:
    LayoutMask(Node* node);

    void apply(RenderState& state) const;

public:
    double x;
    double y;
    double width;
    double height;
    Units units;
    Units contentUnits;
    double opacity;
    const LayoutMask* masker;
    const LayoutClipPath* clipper;
};

class LayoutSymbol final : public LayoutContainer {
public:
    LayoutSymbol(Node* node);

    void render(RenderState& state) const final;

    const Transform& localTransform() const final { return transform; }

public:
    double width;
    double height;
    Transform transform;
    Rect clip;
    double opacity;
    const LayoutMask* masker;
    const LayoutClipPath* clipper;
};

class LayoutGroup final : public LayoutContainer {
public:
    LayoutGroup(Node* node);

    void render(RenderState& state) const final;
    const Transform& localTransform() const final { return transform; }

public:
    Transform transform;
    double opacity;
    const LayoutMask* masker;
    const LayoutClipPath* clipper;
};

class LayoutMarker final : public LayoutContainer {
public:
    LayoutMarker(Node* node);

    Transform markerTransform(const Point& origin, double angle, double strokeWidth) const;
    Rect markerBoundingBox(const Point& origin, double angle, double strokeWidth) const;
    void renderMarker(RenderState& state, const Point& origin, double angle, double strokeWidth) const;

public:
    double refX;
    double refY;
    Transform transform;
    Angle orient;
    MarkerUnits units;
    Rect clip;
    double opacity;
    const LayoutMask* masker;
    const LayoutClipPath* clipper;
};

class LayoutPattern final : public LayoutContainer {
public:
    LayoutPattern(Node* node);

    void apply(RenderState& state) const final;

public:
    double x;
    double y;
    double width;
    double height;
    Transform transform;
    Units units;
    Units contentUnits;
    Rect viewBox;
    PreserveAspectRatio preserveAspectRatio;
};

class LayoutGradient : public LayoutObject {
public:
    LayoutGradient(Node* node, LayoutId id);

public:
    Transform transform;
    SpreadMethod spreadMethod;
    Units units;
    GradientStops stops;
};

class LayoutLinearGradient final : public LayoutGradient {
public:
    LayoutLinearGradient(Node* node);

    void apply(RenderState& state) const final;

public:
    double x1;
    double y1;
    double x2;
    double y2;
};

class LayoutRadialGradient final : public LayoutGradient {
public:
    LayoutRadialGradient(Node* node);

    void apply(RenderState& state) const final;

public:
    double cx;
    double cy;
    double r;
    double fx;
    double fy;
};

class LayoutSolidColor final : public LayoutObject {
public:
    LayoutSolidColor(Node* node);

    void apply(RenderState& state) const final;

public:
    Color color;
};

class FillData {
public:
    FillData() = default;

    void fill(RenderState& state, const Path& path) const;

public:
    const LayoutObject* painter{nullptr};
    Color color{Color::Transparent};
    double opacity{0};
    WindRule fillRule{WindRule::NonZero};
};

class StrokeData {
public:
    StrokeData() = default;

    void stroke(RenderState& state, const Path& path) const;
    void inflate(Rect& box) const;

public:
    const LayoutObject* painter{nullptr};
    Color color{Color::Transparent};
    double opacity{0};
    double width{1};
    double miterlimit{4};
    LineCap cap{LineCap::Butt};
    LineJoin join{LineJoin::Miter};
    DashData dash;
};

class MarkerPosition {
public:
    MarkerPosition(const LayoutMarker* marker, const Point& origin, double angle);

public:
    const LayoutMarker* marker;
    Point origin;
    double angle;
};

using MarkerPositionList = std::vector<MarkerPosition>;

class MarkerData {
public:
    MarkerData() = default;

    void add(const LayoutMarker* marker, const Point& origin, double angle);
    void render(RenderState& state) const;
    void inflate(Rect& box) const;

public:
    MarkerPositionList positions;
    double strokeWidth{1};
};

class LayoutShape final : public LayoutObject {
public:
    LayoutShape(Node* node);

    void render(RenderState& state) const;
    const Transform& localTransform() const final { return transform; }
    const Rect& fillBoundingBox() const;
    const Rect& strokeBoundingBox() const;

public:
    Path path;
    Transform transform;
    FillData fillData;
    StrokeData strokeData;
    MarkerData markerData;
    Visibility visibility;
    WindRule clipRule;
    double opacity;
    const LayoutMask* masker;
    const LayoutClipPath* clipper;

private:
    mutable Rect m_fillBoundingBox{Rect::Invalid};
    mutable Rect m_strokeBoundingBox{Rect::Invalid};
};

enum class RenderMode {
    Display,
    Clipping
};

struct BlendInfo {
    const LayoutClipPath* clipper;
    const LayoutMask* masker;
    double opacity;
    Rect clip;
};

class RenderState {
public:
    RenderState(const LayoutObject* object, RenderMode mode);

    void beginGroup(RenderState& state, const BlendInfo& info);
    void endGroup(RenderState& state, const BlendInfo& info);

    const LayoutObject* object() const { return m_object;}
    RenderMode mode() const { return m_mode; }
    const Rect& objectBoundingBox() const { return m_object->fillBoundingBox(); }

public:
    std::shared_ptr<Canvas> canvas;
    Transform transform;

private:
    const LayoutObject* m_object;
    RenderMode m_mode;
};

class Document;
class StyledElement;
class GeometryElement;

class LayoutContext {
public:
    LayoutContext(const Document* document, LayoutSymbol* root);

    Element* getElementById(const std::string& id) const;
    LayoutObject* getResourcesById(const std::string& id) const;
    LayoutObject* addToResourcesCache(const std::string& id, std::unique_ptr<LayoutObject> resources);
    LayoutMask* getMasker(const std::string& id);
    LayoutClipPath* getClipper(const std::string& id);
    LayoutMarker* getMarker(const std::string& id);
    LayoutObject* getPainter(const std::string& id);

    FillData fillData(const StyledElement* element);
    DashData dashData(const StyledElement* element);
    StrokeData strokeData(const StyledElement* element);
    MarkerData markerData(const GeometryElement* element, const Path& path);

    void addReference(const Element* element);
    void removeReference(const Element* element);
    bool hasReference(const Element* element) const;

private:
    const Document* m_document;
    LayoutSymbol* m_root;
    std::map<std::string, LayoutObject*> m_resourcesCache;
    std::set<const Element*> m_references;
};

class LayoutBreaker {
public:
    LayoutBreaker(LayoutContext* context, const Element* element);
    ~LayoutBreaker();

private:
    LayoutContext* m_context;
    const Element* m_element;
};

} // namespace lunasvg

#endif // LAYOUTCONTEXT_H
