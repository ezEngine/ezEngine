#include "plutovg-private.h"

#include "plutovg-ft-raster.h"
#include "plutovg-ft-stroker.h"

#include <math.h>
#include <limits.h>

#define ALIGN_SIZE(size) (((size) + 7ul) & ~7ul)
static void ft_outline_init(PVG_FT_Outline* outline, plutovg_t* pluto, int points, int contours)
{
    size_t size_a = ALIGN_SIZE((points + contours) * sizeof(PVG_FT_Vector));
    size_t size_b = ALIGN_SIZE((points + contours) * sizeof(char));
    size_t size_c = ALIGN_SIZE(contours * sizeof(int));
    size_t size_d = ALIGN_SIZE(contours * sizeof(char));
    size_t size_n = size_a + size_b + size_c + size_d;
    if(size_n > pluto->outline_size) {
        pluto->outline_data = realloc(pluto->outline_data, size_n);
        pluto->outline_size = size_n;
    }

    PVG_FT_Byte* data = pluto->outline_data;
    outline->points = (PVG_FT_Vector*)(data);
    outline->tags = outline->contours_flag = NULL;
    outline->contours = NULL;
    if(data){
        outline->tags = (char*)(data + size_a);
        outline->contours = (int*)(data + size_a + size_b);
        outline->contours_flag = (char*)(data + size_a + size_b + size_c);
    }
    outline->n_points = 0;
    outline->n_contours = 0;
    outline->flags = 0x0;
}

#define FT_COORD(x) (PVG_FT_Pos)((x) * 64)
static void ft_outline_move_to(PVG_FT_Outline* ft, double x, double y)
{
    ft->points[ft->n_points].x = FT_COORD(x);
    ft->points[ft->n_points].y = FT_COORD(y);
    ft->tags[ft->n_points] = PVG_FT_CURVE_TAG_ON;
    if(ft->n_points) {
        ft->contours[ft->n_contours] = ft->n_points - 1;
        ft->n_contours++;
    }

    ft->contours_flag[ft->n_contours] = 1;
    ft->n_points++;
}

static void ft_outline_line_to(PVG_FT_Outline* ft, double x, double y)
{
    ft->points[ft->n_points].x = FT_COORD(x);
    ft->points[ft->n_points].y = FT_COORD(y);
    ft->tags[ft->n_points] = PVG_FT_CURVE_TAG_ON;
    ft->n_points++;
}

static void ft_outline_cubic_to(PVG_FT_Outline* ft, double x1, double y1, double x2, double y2, double x3, double y3)
{
    ft->points[ft->n_points].x = FT_COORD(x1);
    ft->points[ft->n_points].y = FT_COORD(y1);
    ft->tags[ft->n_points] = PVG_FT_CURVE_TAG_CUBIC;
    ft->n_points++;

    ft->points[ft->n_points].x = FT_COORD(x2);
    ft->points[ft->n_points].y = FT_COORD(y2);
    ft->tags[ft->n_points] = PVG_FT_CURVE_TAG_CUBIC;
    ft->n_points++;

    ft->points[ft->n_points].x = FT_COORD(x3);
    ft->points[ft->n_points].y = FT_COORD(y3);
    ft->tags[ft->n_points] = PVG_FT_CURVE_TAG_ON;
    ft->n_points++;
}

static void ft_outline_close(PVG_FT_Outline* ft)
{
    ft->contours_flag[ft->n_contours] = 0;
    int index = ft->n_contours ? ft->contours[ft->n_contours - 1] + 1 : 0;
    if(index == ft->n_points)
        return;

    ft->points[ft->n_points].x = ft->points[index].x;
    ft->points[ft->n_points].y = ft->points[index].y;
    ft->tags[ft->n_points] = PVG_FT_CURVE_TAG_ON;
    ft->n_points++;
}

static void ft_outline_end(PVG_FT_Outline* ft)
{
    if(ft->n_points) {
        ft->contours[ft->n_contours] = ft->n_points - 1;
        ft->n_contours++;
    }
}

static void ft_outline_convert(PVG_FT_Outline* outline, plutovg_t* pluto, const plutovg_path_t* path, const plutovg_matrix_t* matrix)
{
    ft_outline_init(outline, pluto, path->points.size, path->contours);
    plutovg_path_element_t* elements = path->elements.data;
    plutovg_point_t* points = path->points.data;
    plutovg_point_t p[3];
    for(int i = 0;i < path->elements.size;i++) {
        switch(elements[i]) {
        case plutovg_path_element_move_to:
            plutovg_matrix_map_point(matrix, &points[0], &p[0]);
            ft_outline_move_to(outline, p[0].x, p[0].y);
            points += 1;
            break;
        case plutovg_path_element_line_to:
            plutovg_matrix_map_point(matrix, &points[0], &p[0]);
            ft_outline_line_to(outline, p[0].x, p[0].y);
            points += 1;
            break;
        case plutovg_path_element_cubic_to:
            plutovg_matrix_map_point(matrix, &points[0], &p[0]);
            plutovg_matrix_map_point(matrix, &points[1], &p[1]);
            plutovg_matrix_map_point(matrix, &points[2], &p[2]);
            ft_outline_cubic_to(outline, p[0].x, p[0].y, p[1].x, p[1].y, p[2].x, p[2].y);
            points += 3;
            break;
        case plutovg_path_element_close:
            ft_outline_close(outline);
            points += 1;
            break;
        }
    }

    ft_outline_end(outline);
}

static void ft_outline_convert_dash(PVG_FT_Outline* outline, plutovg_t* pluto, const plutovg_path_t* path, const plutovg_matrix_t* matrix, const plutovg_dash_t* dash)
{
    plutovg_path_t* dashed = plutovg_dash_path(dash, path);
    ft_outline_convert(outline, pluto, dashed, matrix);
    plutovg_path_destroy(dashed);
}

static void generation_callback(int count, const PVG_FT_Span* spans, void* user)
{
    plutovg_rle_t* rle = user;
    plutovg_array_ensure(rle->spans, count);
    plutovg_span_t* data = rle->spans.data + rle->spans.size;
    memcpy(data, spans, (size_t)count * sizeof(plutovg_span_t));
    rle->spans.size += count;
}

plutovg_rle_t* plutovg_rle_create(void)
{
    plutovg_rle_t* rle = malloc(sizeof(plutovg_rle_t));
    plutovg_array_init(rle->spans);
    rle->x = 0;
    rle->y = 0;
    rle->w = 0;
    rle->h = 0;
    return rle;
}

void plutovg_rle_destroy(plutovg_rle_t* rle)
{
    if(rle==NULL)
        return;

    free(rle->spans.data);
    free(rle);
}

void plutovg_rle_rasterize(plutovg_t* pluto, plutovg_rle_t* rle, const plutovg_path_t* path, const plutovg_matrix_t* matrix, const plutovg_rect_t* clip, const plutovg_stroke_data_t* stroke, plutovg_fill_rule_t winding)
{
    PVG_FT_Raster_Params params;
    params.flags = PVG_FT_RASTER_FLAG_DIRECT | PVG_FT_RASTER_FLAG_AA;
    params.gray_spans = generation_callback;
    params.user = rle;
    if(clip) {
        params.flags |= PVG_FT_RASTER_FLAG_CLIP;
        params.clip_box.xMin = (PVG_FT_Pos)(clip->x);
        params.clip_box.yMin = (PVG_FT_Pos)(clip->y);
        params.clip_box.xMax = (PVG_FT_Pos)(clip->x + clip->w);
        params.clip_box.yMax = (PVG_FT_Pos)(clip->y + clip->h);
    }

    if(stroke) {
        PVG_FT_Outline outline;
        if(stroke->dash == NULL)
            ft_outline_convert(&outline, pluto, path, matrix);
        else
            ft_outline_convert_dash(&outline, pluto, path, matrix, stroke->dash);
        PVG_FT_Stroker_LineCap ftCap;
        PVG_FT_Stroker_LineJoin ftJoin;
        PVG_FT_Fixed ftWidth;
        PVG_FT_Fixed ftMiterLimit;

        plutovg_point_t p1 = {0, 0};
        plutovg_point_t p2 = {plutovg_sqrt2, plutovg_sqrt2};
        plutovg_point_t p3;

        plutovg_matrix_map_point(matrix, &p1, &p1);
        plutovg_matrix_map_point(matrix, &p2, &p2);

        p3.x = p2.x - p1.x;
        p3.y = p2.y - p1.y;

        double scale = sqrt(p3.x*p3.x + p3.y*p3.y) / 2.0;

        ftWidth = (PVG_FT_Fixed)(stroke->width * scale * 0.5 * (1 << 6));
        ftMiterLimit = (PVG_FT_Fixed)(stroke->miterlimit * (1 << 16));

        switch(stroke->cap) {
        case plutovg_line_cap_square:
            ftCap = PVG_FT_STROKER_LINECAP_SQUARE;
            break;
        case plutovg_line_cap_round:
            ftCap = PVG_FT_STROKER_LINECAP_ROUND;
            break;
        default:
            ftCap = PVG_FT_STROKER_LINECAP_BUTT;
            break;
        }

        switch(stroke->join) {
        case plutovg_line_join_bevel:
            ftJoin = PVG_FT_STROKER_LINEJOIN_BEVEL;
            break;
        case plutovg_line_join_round:
            ftJoin = PVG_FT_STROKER_LINEJOIN_ROUND;
            break;
        default:
            ftJoin = PVG_FT_STROKER_LINEJOIN_MITER_FIXED;
            break;
        }

        PVG_FT_Stroker stroker;
        PVG_FT_Stroker_New(&stroker);
        PVG_FT_Stroker_Set(stroker, ftWidth, ftCap, ftJoin, ftMiterLimit);
        PVG_FT_Stroker_ParseOutline(stroker, &outline);

        PVG_FT_UInt points;
        PVG_FT_UInt contours;
        PVG_FT_Stroker_GetCounts(stroker, &points, &contours);

        ft_outline_init(&outline, pluto, points, contours);
        PVG_FT_Stroker_Export(stroker, &outline);
        PVG_FT_Stroker_Done(stroker);

        outline.flags = PVG_FT_OUTLINE_NONE;
        params.source = &outline;
        PVG_FT_Raster_Render(&params);
    } else {
        PVG_FT_Outline outline;
        ft_outline_convert(&outline, pluto, path, matrix);
        switch(winding) {
        case plutovg_fill_rule_even_odd:
            outline.flags = PVG_FT_OUTLINE_EVEN_ODD_FILL;
            break;
        default:
            outline.flags = PVG_FT_OUTLINE_NONE;
            break;
        }

        params.source = &outline;
        PVG_FT_Raster_Render(&params);
    }

    if(rle->spans.size == 0) {
        rle->x = 0;
        rle->y = 0;
        rle->w = 0;
        rle->h = 0;
        return;
    }

    plutovg_span_t* spans = rle->spans.data;
    int x1 = INT_MAX;
    int y1 = spans[0].y;
    int x2 = 0;
    int y2 = spans[rle->spans.size - 1].y;
    for(int i = 0;i < rle->spans.size;i++)
    {
        if(spans[i].x < x1) x1 = spans[i].x;
        if(spans[i].x + spans[i].len > x2) x2 = spans[i].x + spans[i].len;
    }

    rle->x = x1;
    rle->y = y1;
    rle->w = x2 - x1;
    rle->h = y2 - y1 + 1;
}

plutovg_rle_t* plutovg_rle_intersection(const plutovg_rle_t* a, const plutovg_rle_t* b)
{
    int count = plutovg_max(a->spans.size, b->spans.size);
    plutovg_rle_t* result = malloc(sizeof(plutovg_rle_t));
    plutovg_array_init(result->spans);
    plutovg_array_ensure(result->spans, count);

    plutovg_span_t* a_spans = a->spans.data;
    plutovg_span_t* a_end = a_spans + a->spans.size;

    plutovg_span_t* b_spans = b->spans.data;
    plutovg_span_t* b_end = b_spans + b->spans.size;

    while(count && a_spans < a_end && b_spans < b_end)
    {
        if(b_spans->y > a_spans->y)
        {
            ++a_spans;
            continue;
        }

        if(a_spans->y != b_spans->y)
        {
            ++b_spans;
            continue;
        }

        int ax1 = a_spans->x;
        int ax2 = ax1 + a_spans->len;
        int bx1 = b_spans->x;
        int bx2 = bx1 + b_spans->len;

        if(bx1 < ax1 && bx2 < ax1)
        {
            ++b_spans;
            continue;
        }
        else if(ax1 < bx1 && ax2 < bx1)
        {
            ++a_spans;
            continue;
        }

        int x = plutovg_max(ax1, bx1);
        int len = plutovg_min(ax2, bx2) - x;
        if(len)
        {
            plutovg_span_t* span = result->spans.data + result->spans.size;
            span->x = (short)x;
            span->len = (unsigned short)len;
            span->y = a_spans->y;
            span->coverage = plutovg_div255(a_spans->coverage * b_spans->coverage);
            ++result->spans.size;
            --count;
        }

        if(ax2 < bx2)
        {
            ++a_spans;
        }
        else
        {
            ++b_spans;
        }
    }

    if(result->spans.size==0)
    {
        result->x = 0;
        result->y = 0;
        result->w = 0;
        result->h = 0;
        return result;
    }

    plutovg_span_t* spans = result->spans.data;
    int x1 = INT_MAX;
    int y1 = spans[0].y;
    int x2 = 0;
    int y2 = spans[result->spans.size - 1].y;
    for(int i = 0;i < result->spans.size;i++)
    {
        if(spans[i].x < x1) x1 = spans[i].x;
        if(spans[i].x + spans[i].len > x2) x2 = spans[i].x + spans[i].len;
    }

    result->x = x1;
    result->y = y1;
    result->w = x2 - x1;
    result->h = y2 - y1 + 1;
    return result;
}

void plutovg_rle_clip_path(plutovg_rle_t* rle, const plutovg_rle_t* clip)
{
    if(rle==NULL || clip==NULL)
        return;

    plutovg_rle_t* result = plutovg_rle_intersection(rle, clip);
    plutovg_array_ensure(rle->spans, result->spans.size);
    memcpy(rle->spans.data, result->spans.data, (size_t)result->spans.size * sizeof(plutovg_span_t));
    rle->spans.size = result->spans.size;
    rle->x = result->x;
    rle->y = result->y;
    rle->w = result->w;
    rle->h = result->h;
    plutovg_rle_destroy(result);
}

plutovg_rle_t* plutovg_rle_clone(const plutovg_rle_t* rle)
{
    if(rle==NULL)
        return NULL;

    plutovg_rle_t* result = malloc(sizeof(plutovg_rle_t));
    plutovg_array_init(result->spans);
    plutovg_array_ensure(result->spans, rle->spans.size);

    memcpy(result->spans.data, rle->spans.data, (size_t)rle->spans.size * sizeof(plutovg_span_t));
    result->spans.size = rle->spans.size;
    result->x = rle->x;
    result->y = rle->y;
    result->w = rle->w;
    result->h = rle->h;
    return result;
}

void plutovg_rle_clear(plutovg_rle_t* rle)
{
    rle->spans.size = 0;
    rle->x = 0;
    rle->y = 0;
    rle->w = 0;
    rle->h = 0;
}
