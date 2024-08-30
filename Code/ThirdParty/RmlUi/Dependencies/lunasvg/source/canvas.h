#ifndef CANVAS_H
#define CANVAS_H

#include "property.h"
#include "plutovg.h"

#include <memory>

namespace lunasvg {

using GradientStop = std::pair<double, Color>;
using GradientStops = std::vector<GradientStop>;

using DashArray = std::vector<double>;

struct DashData {
    DashArray array;
    double offset{0.0};
};

enum class TextureType {
    Plain,
    Tiled
};

enum class BlendMode {
    Src,
    Src_Over,
    Dst_In,
    Dst_Out
};

class CanvasImpl;

class Canvas {
public:
    static std::shared_ptr<Canvas> create(unsigned char* data, unsigned int width, unsigned int height, unsigned int stride);
    static std::shared_ptr<Canvas> create(double x, double y, double width, double height);
    static std::shared_ptr<Canvas> create(const Rect& box);

    void setColor(const Color& color);
    void setLinearGradient(double x1, double y1, double x2, double y2, const GradientStops& stops, SpreadMethod spread, const Transform& transform);
    void setRadialGradient(double cx, double cy, double r, double fx, double fy, const GradientStops& stops, SpreadMethod spread, const Transform& transform);
    void setTexture(const Canvas* source, TextureType type, const Transform& transform);

    void fill(const Path& path, const Transform& transform, WindRule winding, BlendMode mode, double opacity);
    void stroke(const Path& path, const Transform& transform, double width, LineCap cap, LineJoin join, double miterlimit, const DashData& dash, BlendMode mode, double opacity);
    void blend(const Canvas* source, BlendMode mode, double opacity);
    void mask(const Rect& clip, const Transform& transform);

    void luminance();

    unsigned int width() const;
    unsigned int height() const;
    unsigned int stride() const;
    unsigned char* data() const;

    double x() const { return m_rect.x; }
    double y() const { return m_rect.y; }
    Rect rect() const { return Rect(m_rect.x, m_rect.y, m_rect.w, m_rect.h); }
    plutovg_surface_t* surface() const { return m_surface; }

    ~Canvas();

private:
    Canvas(unsigned char* data, int width, int height, int stride);
    Canvas(int x, int y, int width, int height);

    plutovg_surface_t* m_surface;
    plutovg_t* m_pluto;
    plutovg_matrix_t m_translation;
    plutovg_rect_t m_rect;
};

} // namespace lunasvg

#endif // CANVAS_H
