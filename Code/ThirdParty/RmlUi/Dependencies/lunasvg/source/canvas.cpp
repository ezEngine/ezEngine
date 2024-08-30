#include "canvas.h"

#include <cmath>

namespace lunasvg {

static plutovg_matrix_t to_plutovg_matrix(const Transform& transform);
static plutovg_fill_rule_t to_plutovg_fill_rule(WindRule winding);
static plutovg_operator_t to_plutovg_operator(BlendMode mode);
static plutovg_line_cap_t to_plutovg_line_cap(LineCap cap);
static plutovg_line_join_t to_plutovg_line_join(LineJoin join);
static plutovg_spread_method_t to_plutovg_spread_method(SpreadMethod spread);
static plutovg_texture_type_t to_plutovg_texture_type(TextureType type);
static void to_plutovg_stops(plutovg_gradient_t* gradient, const GradientStops& stops);
static void to_plutovg_path(plutovg_t* pluto, const Path& path);

std::shared_ptr<Canvas> Canvas::create(unsigned char* data, unsigned int width, unsigned int height, unsigned int stride)
{
    return std::shared_ptr<Canvas>(new Canvas(data, static_cast<int>(width), static_cast<int>(height), static_cast<int>(stride)));
}

std::shared_ptr<Canvas> Canvas::create(double x, double y, double width, double height)
{
    if(width <= 0.0 || height <= 0.0)
        return std::shared_ptr<Canvas>(new Canvas(0, 0, 1, 1));

    auto l = static_cast<int>(floor(x));
    auto t = static_cast<int>(floor(y));
    auto r = static_cast<int>(ceil(x + width));
    auto b = static_cast<int>(ceil(y + height));
    return std::shared_ptr<Canvas>(new Canvas(l, t, r - l, b - t));
}

std::shared_ptr<Canvas> Canvas::create(const Rect& box)
{
    return create(box.x, box.y, box.w, box.h);
}

Canvas::Canvas(unsigned char* data, int width, int height, int stride)
{
    m_surface = plutovg_surface_create_for_data(data, width, height, stride);
    m_pluto = plutovg_create(m_surface);
    plutovg_matrix_init_identity(&m_translation);
    plutovg_rect_init(&m_rect, 0, 0, width, height);
}

Canvas::Canvas(int x, int y, int width, int height)
{
    m_surface = plutovg_surface_create(width, height);
    m_pluto = plutovg_create(m_surface);
    plutovg_matrix_init_translate(&m_translation, -x, -y);
    plutovg_rect_init(&m_rect, x, y, width, height);
}

Canvas::~Canvas()
{
    plutovg_surface_destroy(m_surface);
    plutovg_destroy(m_pluto);
}

void Canvas::setColor(const Color& color)
{
    plutovg_set_rgba(m_pluto, color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0, color.alpha() / 255.0);
}

void Canvas::setLinearGradient(double x1, double y1, double x2, double y2, const GradientStops& stops, SpreadMethod spread, const Transform& transform)
{
    auto gradient = plutovg_set_linear_gradient(m_pluto, x1, y1, x2, y2);
    auto matrix = to_plutovg_matrix(transform);
    to_plutovg_stops(gradient, stops);
    plutovg_gradient_set_spread(gradient, to_plutovg_spread_method(spread));
    plutovg_gradient_set_matrix(gradient, &matrix);
}

void Canvas::setRadialGradient(double cx, double cy, double r, double fx, double fy, const GradientStops& stops, SpreadMethod spread, const Transform& transform)
{
    auto gradient = plutovg_set_radial_gradient(m_pluto, cx, cy, r, fx, fy, 0);
    auto matrix = to_plutovg_matrix(transform);
    to_plutovg_stops(gradient, stops);
    plutovg_gradient_set_spread(gradient, to_plutovg_spread_method(spread));
    plutovg_gradient_set_matrix(gradient, &matrix);
}

void Canvas::setTexture(const Canvas* source, TextureType type, const Transform& transform)
{
    auto texture = plutovg_set_texture(m_pluto, source->surface(), to_plutovg_texture_type(type));
    auto matrix = to_plutovg_matrix(transform);
    plutovg_texture_set_matrix(texture, &matrix);
}

void Canvas::fill(const Path& path, const Transform& transform, WindRule winding, BlendMode mode, double opacity)
{
    auto matrix = to_plutovg_matrix(transform);
    plutovg_matrix_multiply(&matrix, &matrix, &m_translation);
    to_plutovg_path(m_pluto, path);
    plutovg_set_matrix(m_pluto, &matrix);
    plutovg_set_fill_rule(m_pluto, to_plutovg_fill_rule(winding));
    plutovg_set_opacity(m_pluto, opacity);
    plutovg_set_operator(m_pluto, to_plutovg_operator(mode));
    plutovg_fill(m_pluto);
}

void Canvas::stroke(const Path& path, const Transform& transform, double width, LineCap cap, LineJoin join, double miterlimit, const DashData& dash, BlendMode mode, double opacity)
{
    auto matrix = to_plutovg_matrix(transform);
    plutovg_matrix_multiply(&matrix, &matrix, &m_translation);
    to_plutovg_path(m_pluto, path);
    plutovg_set_matrix(m_pluto, &matrix);
    plutovg_set_line_width(m_pluto, width);
    plutovg_set_line_cap(m_pluto, to_plutovg_line_cap(cap));
    plutovg_set_line_join(m_pluto, to_plutovg_line_join(join));
    plutovg_set_miter_limit(m_pluto, miterlimit);
    plutovg_set_dash(m_pluto, dash.offset, dash.array.data(), static_cast<int>(dash.array.size()));
    plutovg_set_operator(m_pluto, to_plutovg_operator(mode));
    plutovg_set_opacity(m_pluto, opacity);
    plutovg_stroke(m_pluto);
}

void Canvas::blend(const Canvas* source, BlendMode mode, double opacity)
{
    plutovg_set_texture_surface(m_pluto, source->surface(), source->x(), source->y());
    plutovg_set_operator(m_pluto, to_plutovg_operator(mode));
    plutovg_set_opacity(m_pluto, opacity);
    plutovg_set_matrix(m_pluto, &m_translation);
    plutovg_paint(m_pluto);
}

void Canvas::mask(const Rect& clip, const Transform& transform)
{
    auto matrix = to_plutovg_matrix(transform);
    auto path = plutovg_path_create();
    plutovg_path_add_rect(path, clip.x, clip.y, clip.w, clip.h);
    plutovg_path_transform(path, &matrix);
    plutovg_rect(m_pluto, m_rect.x, m_rect.y, m_rect.w, m_rect.h);
    plutovg_add_path(m_pluto, path);
    plutovg_path_destroy(path);

    plutovg_set_rgba(m_pluto, 0, 0, 0, 0);
    plutovg_set_fill_rule(m_pluto, plutovg_fill_rule_even_odd);
    plutovg_set_operator(m_pluto, plutovg_operator_src);
    plutovg_set_opacity(m_pluto, 0.0);
    plutovg_set_matrix(m_pluto, &m_translation);
    plutovg_fill(m_pluto);
}

void Canvas::luminance()
{
    auto width = plutovg_surface_get_width(m_surface);
    auto height = plutovg_surface_get_height(m_surface);
    auto stride = plutovg_surface_get_stride(m_surface);
    auto data = plutovg_surface_get_data(m_surface);
    for(int y = 0; y < height; y++) {
        auto pixels = reinterpret_cast<uint32_t*>(data + stride * y);
        for(int x = 0; x < width; x++) {
            auto pixel = pixels[x];
            auto r = (pixel >> 16) & 0xFF;
            auto g = (pixel >> 8) & 0xFF;
            auto b = (pixel >> 0) & 0xFF;
            auto l = (2*r + 3*g + b) / 6;

            pixels[x] = l << 24;
        }
    }
}

unsigned int Canvas::width() const
{
    return plutovg_surface_get_width(m_surface);
}

unsigned int Canvas::height() const
{
    return plutovg_surface_get_height(m_surface);
}

unsigned int Canvas::stride() const
{
    return plutovg_surface_get_stride(m_surface);
}

unsigned char* Canvas::data() const
{
    return plutovg_surface_get_data(m_surface);
}

plutovg_matrix_t to_plutovg_matrix(const Transform& transform)
{
    plutovg_matrix_t matrix;
    plutovg_matrix_init(&matrix, transform.m00, transform.m10, transform.m01, transform.m11, transform.m02, transform.m12);
    return matrix;
}

plutovg_fill_rule_t to_plutovg_fill_rule(WindRule winding)
{
    return winding == WindRule::EvenOdd ? plutovg_fill_rule_even_odd : plutovg_fill_rule_non_zero;
}

plutovg_operator_t to_plutovg_operator(BlendMode mode)
{
    return mode == BlendMode::Src ? plutovg_operator_src : mode == BlendMode::Src_Over ? plutovg_operator_src_over : mode == BlendMode::Dst_In ? plutovg_operator_dst_in : plutovg_operator_dst_out;
}

plutovg_line_cap_t to_plutovg_line_cap(LineCap cap)
{
    return cap == LineCap::Butt ? plutovg_line_cap_butt : cap == LineCap::Round ? plutovg_line_cap_round : plutovg_line_cap_square;
}

plutovg_line_join_t to_plutovg_line_join(LineJoin join)
{
    return join == LineJoin::Miter ? plutovg_line_join_miter : join == LineJoin::Round ? plutovg_line_join_round : plutovg_line_join_bevel;
}

static plutovg_spread_method_t to_plutovg_spread_method(SpreadMethod spread)
{
    return spread == SpreadMethod::Pad ? plutovg_spread_method_pad : spread == SpreadMethod::Reflect ? plutovg_spread_method_reflect : plutovg_spread_method_repeat;
}

static plutovg_texture_type_t to_plutovg_texture_type(TextureType type)
{
    return type == TextureType::Plain ? plutovg_texture_type_plain : plutovg_texture_type_tiled;
}

static void to_plutovg_stops(plutovg_gradient_t* gradient, const GradientStops& stops)
{
    for(const auto& stop : stops) {
        auto offset = std::get<0>(stop);
        auto& color = std::get<1>(stop);
        plutovg_gradient_add_stop_rgba(gradient, offset, color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0, color.alpha() / 255.0);
    }
}

void to_plutovg_path(plutovg_t* pluto, const Path& path)
{
    PathIterator it(path);
    std::array<Point, 3> p;
    while(!it.isDone()) {
        switch(it.currentSegment(p)) {
        case PathCommand::MoveTo:
            plutovg_move_to(pluto, p[0].x, p[0].y);
            break;
        case PathCommand::LineTo:
            plutovg_line_to(pluto, p[0].x, p[0].y);
            break;
        case PathCommand::CubicTo:
            plutovg_cubic_to(pluto, p[0].x, p[0].y, p[1].x, p[1].y, p[2].x, p[2].y);
            break;
        case PathCommand::Close:
            plutovg_close_path(pluto);
            break;
        }

        it.next();
    }
}

} // namespace lunasvg
