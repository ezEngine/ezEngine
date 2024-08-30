#ifndef ELEMENT_H
#define ELEMENT_H

#include <memory>
#include <list>

#include "property.h"
#include "lunasvg.h"

namespace lunasvg {

enum class ElementID {
    Unknown = 0,
    Star,
    Circle,
    ClipPath,
    Defs,
    Ellipse,
    G,
    Image,
    Line,
    LinearGradient,
    Marker,
    Mask,
    Path,
    Pattern,
    Polygon,
    Polyline,
    RadialGradient,
    Rect,
    SolidColor,
    Stop,
    Style,
    Svg,
    Symbol,
    Text,
    TSpan,
    Use
};

enum class PropertyID {
    Unknown = 0,
    Class,
    Clip_Path,
    Clip_Rule,
    ClipPathUnits,
    Color,
    Cx,
    Cy,
    D,
    Display,
    Fill,
    Fill_Opacity,
    Fill_Rule,
    Fx,
    Fy,
    GradientTransform,
    GradientUnits,
    Height,
    Href,
    Id,
    Marker_End,
    Marker_Mid,
    Marker_Start,
    MarkerHeight,
    MarkerUnits,
    MarkerWidth,
    Mask,
    MaskContentUnits,
    MaskUnits,
    Offset,
    Opacity,
    Orient,
    Overflow,
    PatternContentUnits,
    PatternTransform,
    PatternUnits,
    Points,
    PreserveAspectRatio,
    R,
    RefX,
    RefY,
    Rx,
    Ry,
    Solid_Color,
    Solid_Opacity,
    SpreadMethod,
    Stop_Color,
    Stop_Opacity,
    Stroke,
    Stroke_Dasharray,
    Stroke_Dashoffset,
    Stroke_Linecap,
    Stroke_Linejoin,
    Stroke_Miterlimit,
    Stroke_Opacity,
    Stroke_Width,
    Style,
    Transform,
    ViewBox,
    Visibility,
    Width,
    X,
    X1,
    X2,
    Y,
    Y1,
    Y2
};

ElementID elementid(const std::string& name);
PropertyID csspropertyid(const std::string& name);
PropertyID propertyid(const std::string& name);

struct Property {
    int specificity;
    PropertyID id;
    std::string value;
};

using PropertyList = std::vector<Property>;

template<typename T, typename... Args>
inline std::unique_ptr<T> makeUnique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

class LayoutObject;
class LayoutContainer;
class LayoutContext;
class Element;

class Node {
public:
    Node() = default;
    virtual ~Node() = default;

    virtual bool isText() const { return false; }
    virtual bool isPaint() const { return false; }
    virtual bool isGeometry() const { return false; }
    virtual void layout(LayoutContext*, LayoutContainer*) {}
    virtual std::unique_ptr<Node> clone() const = 0;

    void setParent(Element* parent) { m_parent = parent; }
    Element* parent() const { return m_parent; }

    LayoutObject* box() const { return m_box; }
    void setBox(LayoutObject* box) { m_box = box; }

private:
    Element* m_parent = nullptr;
    LayoutObject* m_box = nullptr;
};

class TextNode final : public Node {
public:
    TextNode() = default;

    bool isText() const final { return true; }
    std::unique_ptr<Node> clone() const final;

    void setText(std::string text) { m_text = std::move(text); }
    const std::string& text() const { return m_text; }

private:
    std::string m_text;
};

using NodeList = std::list<std::unique_ptr<Node>>;

class Element : public Node {
public:
    static std::unique_ptr<Element> create(ElementID id);

    void set(PropertyID id, const std::string& value, int specificity);
    const std::string& get(PropertyID id) const;
    const std::string& find(PropertyID id) const;
    bool has(PropertyID id) const;

    const PropertyList& properties() const { return m_properties; }
    void setPropertyList(PropertyList properties) { m_properties = std::move(properties); }

    Element* previousElement() const;
    Element* nextElement() const;
    Node* addChild(std::unique_ptr<Node> child);
    void layoutChildren(LayoutContext* context, LayoutContainer* current);
    Rect currentViewport() const;

    ElementID id() const { return m_id; }
    const NodeList& children() const { return m_children; }

    virtual void build(const Document* document);

    template<typename T>
    void transverse(T callback) {
        if(!callback(this))
            return;
        for(auto& child : m_children) {
            if(child->isText()) {
                if(!callback(child.get()))
                    return;
                continue;
            }

            auto element = static_cast<Element*>(child.get());
            element->transverse(callback);
        }
    }

    std::unique_ptr<Node> clone() const final;

protected:
    Element(ElementID id);
    ElementID m_id;
    NodeList m_children;
    PropertyList m_properties;
};

} // namespace lunasvg

#endif // ELEMENT_H
