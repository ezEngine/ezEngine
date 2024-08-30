#include "plutovg-private.h"

void plutovg_color_init_rgb(plutovg_color_t* color, double r, double g, double b)
{
    plutovg_color_init_rgba(color, r, g,  b, 1.0);
}

void plutovg_color_init_rgba(plutovg_color_t* color, double r, double g, double b, double a)
{
    color->r = plutovg_clamp(r, 0.0, 1.0);
    color->g = plutovg_clamp(g, 0.0, 1.0);
    color->b = plutovg_clamp(b, 0.0, 1.0);
    color->a = plutovg_clamp(a, 0.0, 1.0);
}

void plutovg_gradient_init_linear(plutovg_gradient_t* gradient, double x1, double y1, double x2, double y2)
{
    gradient->type = plutovg_gradient_type_linear;
    gradient->spread = plutovg_spread_method_pad;
    gradient->opacity = 1.0;
    plutovg_array_clear(gradient->stops);
    plutovg_matrix_init_identity(&gradient->matrix);
    plutovg_gradient_set_values_linear(gradient, x1, y1, x2, y2);
}

void plutovg_gradient_init_radial(plutovg_gradient_t* gradient, double cx, double cy, double cr, double fx, double fy, double fr)
{
    gradient->type = plutovg_gradient_type_radial;
    gradient->spread = plutovg_spread_method_pad;
    gradient->opacity = 1.0;
    plutovg_array_clear(gradient->stops);
    plutovg_matrix_init_identity(&gradient->matrix);
    plutovg_gradient_set_values_radial(gradient, cx, cy, cr, fx, fy, fr);
}

void plutovg_gradient_set_spread(plutovg_gradient_t* gradient, plutovg_spread_method_t spread)
{
    gradient->spread = spread;
}

plutovg_spread_method_t plutovg_gradient_get_spread(const plutovg_gradient_t* gradient)
{
    return gradient->spread;
}

void plutovg_gradient_set_matrix(plutovg_gradient_t* gradient, const plutovg_matrix_t* matrix)
{
    gradient->matrix = *matrix;
}

void plutovg_gradient_get_matrix(const plutovg_gradient_t* gradient, plutovg_matrix_t *matrix)
{
    *matrix = gradient->matrix;
}

void plutovg_gradient_add_stop_rgb(plutovg_gradient_t* gradient, double offset, double r, double g, double b)
{
    plutovg_gradient_add_stop_rgba(gradient, offset, r, g, b, 1.0);
}

void plutovg_gradient_add_stop_rgba(plutovg_gradient_t* gradient, double offset, double r, double g, double b, double a)
{
    if(offset < 0.0) offset = 0.0;
    if(offset > 1.0) offset = 1.0;

    plutovg_array_ensure(gradient->stops, 1);
    plutovg_gradient_stop_t* stops = gradient->stops.data;
    int nstops = gradient->stops.size;
    int i = 0;
    for(; i < nstops; i++) {
        if(offset < stops[i].offset) {
            memmove(&stops[i+1], &stops[i], (size_t)(nstops - i) * sizeof(plutovg_gradient_stop_t));
            break;
        }
    }

    plutovg_gradient_stop_t* stop = &stops[i];
    stop->offset = offset;
    plutovg_color_init_rgba(&stop->color, r, g, b, a);
    gradient->stops.size += 1;
}

void plutovg_gradient_add_stop_color(plutovg_gradient_t* gradient, double offset, const plutovg_color_t* color)
{
    plutovg_gradient_add_stop_rgba(gradient, offset, color->r, color->g, color->b, color->a);
}

void plutovg_gradient_add_stop(plutovg_gradient_t* gradient, const plutovg_gradient_stop_t* stop)
{
    plutovg_gradient_add_stop_rgba(gradient, stop->offset, stop->color.r, stop->color.g, stop->color.b, stop->color.a);
}

void plutovg_gradient_clear_stops(plutovg_gradient_t* gradient)
{
    gradient->stops.size = 0;
}

int plutovg_gradient_get_stop_count(const plutovg_gradient_t* gradient)
{
    return gradient->stops.size;
}

plutovg_gradient_stop_t* plutovg_gradient_get_stops(const plutovg_gradient_t* gradient)
{
    return gradient->stops.data;
}

plutovg_gradient_type_t plutovg_gradient_get_type(const plutovg_gradient_t* gradient)
{
    return gradient->type;
}

void plutovg_gradient_get_values_linear(const plutovg_gradient_t* gradient, double* x1, double* y1, double* x2, double* y2)
{
    if(x1) *x1 = gradient->values[0];
    if(y1) *y1 = gradient->values[1];
    if(x2) *x2 = gradient->values[2];
    if(y2) *y2 = gradient->values[3];
}

void plutovg_gradient_get_values_radial(const plutovg_gradient_t* gradient, double* cx, double* cy, double* cr, double* fx, double* fy, double* fr)
{
    if(cx) *cx = gradient->values[0];
    if(cy) *cy = gradient->values[1];
    if(cr) *cr = gradient->values[2];
    if(fx) *fx = gradient->values[3];
    if(fy) *fy = gradient->values[4];
    if(fr) *fr = gradient->values[5];
}

void plutovg_gradient_set_values_linear(plutovg_gradient_t* gradient, double x1, double y1, double x2, double y2)
{
    gradient->values[0] = x1;
    gradient->values[1] = y1;
    gradient->values[2] = x2;
    gradient->values[3] = y2;
}

void plutovg_gradient_set_values_radial(plutovg_gradient_t* gradient, double cx, double cy, double cr, double fx, double fy, double fr)
{
    gradient->values[0] = cx;
    gradient->values[1] = cy;
    gradient->values[2] = cr;
    gradient->values[3] = fx;
    gradient->values[4] = fy;
    gradient->values[5] = fr;
}

void plutovg_gradient_set_opacity(plutovg_gradient_t* gradient, double opacity)
{
    gradient->opacity = plutovg_clamp(opacity, 0.0, 1.0);
}

double plutovg_gradient_get_opacity(const plutovg_gradient_t* gradient)
{
    return gradient->opacity;
}

void plutovg_gradient_copy(plutovg_gradient_t* gradient, const plutovg_gradient_t* source)
{
    gradient->type = source->type;
    gradient->spread = source->spread;
    gradient->matrix = source->matrix;
    gradient->opacity = source->opacity;
    plutovg_array_ensure(gradient->stops, source->stops.size);
    memcpy(gradient->values, source->values, sizeof(source->values));
    memcpy(gradient->stops.data, source->stops.data, source->stops.size * sizeof(plutovg_gradient_stop_t));
}

void plutovg_gradient_destroy(plutovg_gradient_t* gradient)
{
    plutovg_array_destroy(gradient->stops);
}

void plutovg_texture_init(plutovg_texture_t* texture, plutovg_surface_t* surface, plutovg_texture_type_t type)
{
    surface = plutovg_surface_reference(surface);
    plutovg_surface_destroy(texture->surface);
    texture->type = type;
    texture->surface = surface;
    texture->opacity = 1.0;
    plutovg_matrix_init_identity(&texture->matrix);
}

void plutovg_texture_set_type(plutovg_texture_t* texture, plutovg_texture_type_t type)
{
    texture->type = type;
}

plutovg_texture_type_t plutovg_texture_get_type(const plutovg_texture_t* texture)
{
    return texture->type;
}

void plutovg_texture_set_matrix(plutovg_texture_t* texture, const plutovg_matrix_t* matrix)
{
    texture->matrix = *matrix;
}

void plutovg_texture_get_matrix(const plutovg_texture_t* texture, plutovg_matrix_t* matrix)
{
    *matrix = texture->matrix;
}

void plutovg_texture_set_surface(plutovg_texture_t* texture, plutovg_surface_t* surface)
{
    surface = plutovg_surface_reference(surface);
    plutovg_surface_destroy(texture->surface);
    texture->surface = surface;
}

plutovg_surface_t* plutovg_texture_get_surface(const plutovg_texture_t* texture)
{
    return texture->surface;
}

void plutovg_texture_set_opacity(plutovg_texture_t* texture, double opacity)
{
    texture->opacity = plutovg_clamp(opacity, 0.0, 1.0);
}

double plutovg_texture_get_opacity(const plutovg_texture_t* texture)
{
    return texture->opacity;
}

void plutovg_texture_copy(plutovg_texture_t* texture, const plutovg_texture_t* source)
{
    plutovg_surface_t* surface = plutovg_surface_reference(source->surface);
    plutovg_surface_destroy(texture->surface);
    texture->type = source->type;
    texture->surface = surface;
    texture->opacity = source->opacity;
    texture->matrix = source->matrix;
}

void plutovg_texture_destroy(plutovg_texture_t* texture)
{
    plutovg_surface_destroy(texture->surface);
}

void plutovg_paint_init(plutovg_paint_t* paint)
{
    paint->type = plutovg_paint_type_color;
    paint->texture.surface = NULL;
    plutovg_array_init(paint->gradient.stops);
    plutovg_color_init_rgb(&paint->color, 0, 0, 0);
}

void plutovg_paint_destroy(plutovg_paint_t* paint)
{
    plutovg_texture_destroy(&paint->texture);
    plutovg_gradient_destroy(&paint->gradient);
}

void plutovg_paint_copy(plutovg_paint_t* paint, const plutovg_paint_t* source)
{
    paint->type = source->type;
    if(source->type == plutovg_paint_type_color)
        paint->color = source->color;
    else if(source->type == plutovg_paint_type_color)
        plutovg_gradient_copy(&paint->gradient, &paint->gradient);
    else
        plutovg_texture_copy(&paint->texture, &paint->texture);
}
