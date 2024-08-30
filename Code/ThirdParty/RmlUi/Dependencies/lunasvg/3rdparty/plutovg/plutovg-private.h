#ifndef PLUTOVG_PRIVATE_H
#define PLUTOVG_PRIVATE_H

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "plutovg.h"

struct plutovg_surface {
    int ref;
    unsigned char* data;
    int owndata;
    int width;
    int height;
    int stride;
};

struct plutovg_path {
    int ref;
    int contours;
    plutovg_point_t start;
    struct {
        plutovg_path_element_t* data;
        int size;
        int capacity;
    } elements;
    struct {
        plutovg_point_t* data;
        int size;
        int capacity;
    } points;
};

struct plutovg_gradient {
    int ref;
    plutovg_gradient_type_t type;
    plutovg_spread_method_t spread;
    plutovg_matrix_t matrix;
    double values[6];
    double opacity;
    struct {
        plutovg_gradient_stop_t* data;
        int size;
        int capacity;
    } stops;
};

struct plutovg_texture {
    int ref;
    plutovg_texture_type_t type;
    plutovg_surface_t* surface;
    plutovg_matrix_t matrix;
    double opacity;
};

typedef int plutovg_paint_type_t;

enum {
    plutovg_paint_type_color,
    plutovg_paint_type_gradient,
    plutovg_paint_type_texture
};

typedef struct {
    plutovg_paint_type_t type;
    plutovg_color_t color;
    plutovg_gradient_t gradient;
    plutovg_texture_t texture;
} plutovg_paint_t;

typedef struct {
    int x;
    int len;
    int y;
    unsigned char coverage;
} plutovg_span_t;

typedef struct {
    struct {
        plutovg_span_t* data;
        int size;
        int capacity;
    } spans;

    int x;
    int y;
    int w;
    int h;
} plutovg_rle_t;

typedef struct {
    double offset;
    double* data;
    int size;
} plutovg_dash_t;

typedef struct {
    double width;
    double miterlimit;
    plutovg_line_cap_t cap;
    plutovg_line_join_t join;
    plutovg_dash_t* dash;
} plutovg_stroke_data_t;

typedef struct plutovg_state {
    plutovg_rle_t* clippath;
    plutovg_paint_t paint;
    plutovg_matrix_t matrix;
    plutovg_fill_rule_t winding;
    plutovg_stroke_data_t stroke;
    plutovg_operator_t op;
    double opacity;
    struct plutovg_state* next;
} plutovg_state_t;

struct plutovg {
    int ref;
    plutovg_surface_t* surface;
    plutovg_state_t* state;
    plutovg_path_t* path;
    plutovg_rle_t* rle;
    plutovg_rle_t* clippath;
    plutovg_rect_t clip;
    void* outline_data;
    size_t outline_size;
};

void plutovg_paint_init(plutovg_paint_t* paint);
void plutovg_paint_destroy(plutovg_paint_t* paint);
void plutovg_paint_copy(plutovg_paint_t* paint, const plutovg_paint_t* source);

void plutovg_gradient_copy(plutovg_gradient_t* gradient, const plutovg_gradient_t* source);
void plutovg_gradient_destroy(plutovg_gradient_t* gradient);

void plutovg_texture_copy(plutovg_texture_t* texture, const plutovg_texture_t* source);
void plutovg_texture_destroy(plutovg_texture_t* texture);

plutovg_rle_t* plutovg_rle_create(void);
void plutovg_rle_destroy(plutovg_rle_t* rle);
void plutovg_rle_rasterize(plutovg_t* pluto, plutovg_rle_t* rle, const plutovg_path_t* path, const plutovg_matrix_t* matrix, const plutovg_rect_t* clip, const plutovg_stroke_data_t* stroke, plutovg_fill_rule_t winding);
plutovg_rle_t* plutovg_rle_intersection(const plutovg_rle_t* a, const plutovg_rle_t* b);
void plutovg_rle_clip_path(plutovg_rle_t* rle, const plutovg_rle_t* clip);
plutovg_rle_t* plutovg_rle_clone(const plutovg_rle_t* rle);
void plutovg_rle_clear(plutovg_rle_t* rle);

plutovg_dash_t* plutovg_dash_create(double offset, const double* data, int size);
plutovg_dash_t* plutovg_dash_clone(const plutovg_dash_t* dash);
void plutovg_dash_destroy(plutovg_dash_t* dash);
plutovg_path_t* plutovg_dash_path(const plutovg_dash_t* dash, const plutovg_path_t* path);

plutovg_state_t* plutovg_state_create(void);
plutovg_state_t* plutovg_state_clone(const plutovg_state_t* state);
void plutovg_state_destroy(plutovg_state_t* state);

void plutovg_blend(plutovg_t* pluto, const plutovg_rle_t* rle);
void plutovg_blend_color(plutovg_t* pluto, const plutovg_rle_t* rle, const plutovg_color_t* color);
void plutovg_blend_gradient(plutovg_t* pluto, const plutovg_rle_t* rle, const plutovg_gradient_t* gradient);
void plutovg_blend_texture(plutovg_t* pluto, const plutovg_rle_t* rle, const plutovg_texture_t* texture);

#define plutovg_sqrt2 1.41421356237309504880
#define plutovg_pi 3.14159265358979323846
#define plutovg_two_pi 6.28318530717958647693
#define plutovg_half_pi 1.57079632679489661923
#define plutovg_kappa 0.55228474983079339840

#define plutovg_min(a, b) ((a) < (b) ? (a) : (b))
#define plutovg_max(a, b) ((a) > (b) ? (a) : (b))
#define plutovg_clamp(v, lo, hi) ((v) < (lo) ? (lo) : (hi) < (v) ? (hi) : (v))
#define plutovg_div255(x) (((x) + ((x) >> 8) + 0x80) >> 8)

#define plutovg_alpha(c) ((c) >> 24)
#define plutovg_red(c) (((c) >> 16) & 0xff)
#define plutovg_green(c) (((c) >> 8) & 0xff)
#define plutovg_blue(c) (((c) >> 0) & 0xff)

#define plutovg_array_init(array) \
do { \
        array.data = NULL; \
        array.size = 0; \
        array.capacity = 0; \
} while(0)

#define plutovg_array_ensure(array, count) \
    do { \
        if(array.size + count > array.capacity) { \
            int capacity = array.size + count; \
            int newcapacity = array.capacity == 0 ? 8 : array.capacity; \
            while(newcapacity < capacity) { newcapacity *= 2; } \
            array.data = realloc(array.data, newcapacity * sizeof(array.data[0])); \
            array.capacity = newcapacity; \
    } \
} while(0)

#define plutovg_array_clear(array) (array.size = 0)
#define plutovg_array_destroy(array) free(array.data)

#endif // PLUTOVG_PRIVATE_H
