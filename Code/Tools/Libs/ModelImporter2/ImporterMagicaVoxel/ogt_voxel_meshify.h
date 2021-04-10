/*
    opengametools voxel meshifier - v0.9 - MIT license - Justin Paver, April 2020

    This is a single-header-file library that provides easy-to-use
    support for converting paletted voxel grid data into an indexed triangle mesh. 

    Please see the MIT license information at the end of this file.

    Also, please consider sharing any improvements you make.

    For more information and more tools, visit:
      https://github.com/jpaver/opengametools

    USAGE

    1. load your voxel grid data and palette data (see ogt_vox_model inside ogt_vox_loader.h for example)
       and obtain the x, y and z dimensions of the grid.

    2. convert into a mesh

        ogt_mesh* mesh = ogt_mesh_from_paletted_voxels_simple( voxel_data, size_x, size_y, size_z, voxel_palette );
    
    3. use the indexed triangle list in the mesh to construct renderable geometry, collision geometry.

        // This is old sceen OpenGL immediate mode rendering for demonstration purposes only.
        // Ideally you'd use more modern practices for rendering, including converting ogt_mesh data to 
        // your own engine's layout.

        glBegin(GL_TRIANGLES);
        for (uint32_t i = 0; i < mesh->index_count; i+=3)
        {
          uint32_t i0 = mesh->indices[i + 0];
          uint32_t i1 = mesh->indices[i + 1];
          uint32_t i2 = mesh->indices[i + 2];
          const ogt_mesh_vertex* v0 = &mesh->vertices[i0];
          const ogt_mesh_vertex* v1 = &mesh->vertices[i1];
          const ogt_mesh_vertex* v2 = &mesh->vertices[i2];
          glColor4ubv(&v0->color);
          glNormal3fv(&v0->normal);
          glVertex3fv(&v0->pos);
          glColor4ubv(&v1->color);
          glNormal3fv(&v1->normal);
          glVertex3fv(&v1->pos);
          glColor4ubv(&v2->color);
          glNormal3fv(&v2->normal);
          glVertex3fv(&v2->pos);
        }
        glEnd();

    EXPLANATION

        We currently only support paletted voxel data as input to the meshing algorithms here.

        Paletted voxel mesh data assumes each voxel within the grid is a single byte
        that represents a color index into a 256 color palette.

        If the color index is 0, the voxel is assumed to be empty, otherwise it is solid.
        For this reason, palette[0] will never be used.

        Voxel data is laid out in x, then y, then z order. In other words, given
        a coordinate (x,y,z) within your grid, you can compute where it is in your voxel 
        array using the following logic:

            voxel_index = x + (y * size_x) + (z * size_x * size_y);

        We support the following algorithms for meshing the voxel data for now:

        * ogt_mesh_from_paletted_voxels_simple:  creates 2 triangles for every visible voxel face.
        * ogt_mesh_from_paletted_voxels_greedy:  creates 2 triangles for every rectangular region of voxel faces with the same color
        * ogt_mesh_from_paletted_voxels_polygon: determines the polygon contour of every connected voxel face with the same color and then triangulates that.
*/
#ifndef OGT_VOXEL_MESHIFY_H__
#define OGT_VOXEL_MESHIFY_H__


#if _MSC_VER == 1400	
    // VS2005 doesn't have inttypes or stdint so we just define what we need here.
    typedef unsigned char uint8_t;
    typedef signed int    int32_t;
    typedef unsigned int  uint32_t;
    typedef unsigned short uint16_t;
    #ifndef UINT32_MAX
        #define UINT32_MAX	0xFFFFFFFF
    #endif
#elif defined(_MSC_VER)
    // general VS* 
    #include <inttypes.h>
#elif __APPLE__
    // general Apple compiler
#elif defined(__GNUC__)
    // any GCC*
    #include <inttypes.h>
    #include <stdlib.h> // for size_t
#else
    #error some fixup needed for this platform?
#endif


// a 3 dimensional quantity
struct ogt_mesh_vec3 
{
    float x, y, z;
};

// a color
struct ogt_mesh_rgba
{
    uint8_t r,g,b,a;
};

// represents a vertex
struct ogt_mesh_vertex
{
    ogt_mesh_vec3  pos;
    ogt_mesh_vec3  normal;
    ogt_mesh_rgba  color;
};

// a mesh that contains an indexed triangle list of vertices
struct ogt_mesh 
{
    uint32_t         vertex_count;	// number of vertices
    uint32_t         index_count;	// number of indices
    ogt_mesh_vertex* vertices;		// array of vertices
    uint32_t*        indices;		// array of indices
};

// allocate memory function interface. pass in size, and get a pointer to memory with at least that size available.
typedef void* (*ogt_voxel_meshify_alloc_func)(size_t size, void* user_data);

// free memory function interface. pass in a pointer previously allocated and it will be released back to the system managing memory.
typedef void  (*ogt_voxel_meshify_free_func)(void* ptr, void* user_data);

// stream function can receive a batch of triangles for each voxel processed by ogt_stream_from_paletted_voxels_simple. (i,j,k) 
typedef void (*ogt_voxel_simple_stream_func)(uint32_t x, uint32_t y, uint32_t z, const ogt_mesh_vertex* vertices, uint32_t vertex_count, const uint32_t* indices, uint32_t index_count, void* user_data);

// a context that allows you to override various internal operations of the below api functions.
struct ogt_voxel_meshify_context
{
    ogt_voxel_meshify_alloc_func                alloc_func;                 // override allocation function
    ogt_voxel_meshify_free_func                 free_func;                  // override free function
    void*                                       alloc_free_user_data;       // alloc/free user-data (passed to alloc_func / free_func )
};

// returns the number of quad faces that would be generated by tessellating the specified voxel field using the simple algorithm. Useful for preallocating memory.
// number of vertices needed would 4x this value, and number of indices needed would be 6x this value.
uint32_t ogt_face_count_from_paletted_voxels_simple(const uint8_t* voxels, uint32_t size_x, uint32_t size_y, uint32_t size_z);

// The simple meshifier returns the most naieve mesh possible, which will be tessellated at voxel granularity. 
ogt_mesh* ogt_mesh_from_paletted_voxels_simple(const ogt_voxel_meshify_context* ctx, const uint8_t* voxels, uint32_t size_x, uint32_t size_y, uint32_t size_z, const ogt_mesh_rgba* palette);

// The greedy meshifier will use a greedy box-expansion pass to replace the polygons of adjacent voxels of the same color with a larger polygon that covers the box.
// It will generally produce t-junctions which can make rasterization not water-tight based on your camera/project/distances.
ogt_mesh* ogt_mesh_from_paletted_voxels_greedy(const ogt_voxel_meshify_context* ctx, const uint8_t* voxels, uint32_t size_x, uint32_t size_y, uint32_t size_z, const ogt_mesh_rgba* palette);

// The polygon meshifier will polygonize and triangulate connected voxels that are of the same color. The boundary of the polygon
// will be tessellated only to the degree that is necessary to there are tessellations at color discontinuities.
// This will mostly be water-tight, except for a very small number of cases.
ogt_mesh* ogt_mesh_from_paletted_voxels_polygon(const ogt_voxel_meshify_context* ctx, const uint8_t* voxels, uint32_t size_x, uint32_t size_y, uint32_t size_z, const ogt_mesh_rgba* palette);

// ogt_mesh_remove_duplicate_vertices will in-place remove identical vertices and remap indices to produce an identical mesh.
// Use this after a call to ogt_mesh_from_paletted_voxels_* functions to remove duplicate vertices with the same attributes.
void	  ogt_mesh_remove_duplicate_vertices(const ogt_voxel_meshify_context* ctx, ogt_mesh* mesh);

// Removes faceted normals on the mesh and averages vertex normals based on the faces that are adjacent.
// It is recommended only to call this on  ogt_mesh_from_paletted_voxels_simple.
void      ogt_mesh_smooth_normals(const ogt_voxel_meshify_context* ctx, ogt_mesh* mesh);

// destroys the mesh returned by ogt_mesh_from_paletted_voxels* functions.
void      ogt_mesh_destroy(const ogt_voxel_meshify_context* ctx, ogt_mesh* mesh );
    
// The simple stream function will stream geometry for the specified voxel field, to the specified stream function, which will be invoked on each voxel that requires geometry. 
void     ogt_stream_from_paletted_voxels_simple(const uint8_t* voxels, uint32_t size_x, uint32_t size_y, uint32_t size_z, const ogt_mesh_rgba* palette, ogt_voxel_simple_stream_func stream_func, void* stream_func_data);


#endif // OGT_VOXEL_MESHIFY_H__

//-----------------------------------------------------------------------------------------------------------------
//
// If you're only interested in using this library, everything you need is above this point.
// If you're interested in how this library works, everything you need is below this point.
//
//-----------------------------------------------------------------------------------------------------------------
#ifdef OGT_VOXEL_MESHIFY_IMPLEMENTATION
#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

// a set of up to 65536 bits
struct ogt_mesh_bitset_64k {
    uint8_t bits[8192];
    void clear(uint32_t max_bits) {
        if (max_bits > (8192*8))
            max_bits = (8192*8);
        memset(bits, 0, (max_bits+7)/8);
    }
    uint8_t is_set(uint32_t index) { return bits[index/8] & (1<<(index%8)); }
    void set(uint32_t index)	{ bits[index/8] |=  (1<<(index%8)); }
    void unset(uint32_t index)  { bits[index/8] &= ~(1<<(index%8)); }
};

static void* _voxel_meshify_malloc(const ogt_voxel_meshify_context* ctx, size_t size) {
    return size ? (ctx->alloc_func ? ctx->alloc_func(size, ctx->alloc_free_user_data) : malloc(size)) : NULL;
}

static void* _voxel_meshify_calloc(const ogt_voxel_meshify_context* ctx, size_t size) {
    void* pMem = _voxel_meshify_malloc(ctx, size);
    if (pMem)
        memset(pMem, 0, size);
    return pMem;
}

static void _voxel_meshify_free(const ogt_voxel_meshify_context* ctx, void* old_ptr) {
    if (old_ptr) {
        if (ctx->free_func)
            ctx->free_func(old_ptr, ctx->alloc_free_user_data);
        else
            free(old_ptr);
    }
}

// column-major 4x4 matrix
struct ogt_mesh_transform  {
    float m00, m01, m02, m03;   // column 0 of 4x4 matrix, 1st three elements = x axis vector, last element always 0.0
    float m10, m11, m12, m13;   // column 1 of 4x4 matrix, 1st three elements = y axis vector, last element always 0.0
    float m20, m21, m22, m23;   // column 2 of 4x4 matrix, 1st three elements = z axis vector, last element always 0.0
    float m30, m31, m32, m33;   // column 3 of 4x4 matrix. 1st three elements = translation vector, last element always 1.0
};

// replaces transforms the point computes: transform * (vec.x, vec.y, vec.z, 1.0)
inline ogt_mesh_vec3 _transform_point(const ogt_mesh_transform& transform, const ogt_mesh_vec3& vec) {
    ogt_mesh_vec3 ret;
    ret.x = transform.m30 + (transform.m00 * vec.x) + (transform.m10 * vec.y) + (transform.m20 * vec.z);
    ret.y = transform.m31 + (transform.m01 * vec.x) + (transform.m11 * vec.y) + (transform.m21 * vec.z);
    ret.z = transform.m32 + (transform.m02 * vec.x) + (transform.m12 * vec.y) + (transform.m22 * vec.z);
    return ret;
}

// replaces transforms the point computes: transform * (vec.x, vec.y, vec.z, 0.0)
inline ogt_mesh_vec3 _transform_vector(const ogt_mesh_transform& transform, const ogt_mesh_vec3& vec) {
    ogt_mesh_vec3 ret;
    ret.x = (transform.m00 * vec.x) + (transform.m10 * vec.y) + (transform.m20 * vec.z);
    ret.y = (transform.m01 * vec.x) + (transform.m11 * vec.y) + (transform.m21 * vec.z);
    ret.z = (transform.m02 * vec.x) + (transform.m12 * vec.y) + (transform.m22 * vec.z);
    return ret;
}

inline ogt_mesh_transform _make_transform(
    float m00, float m01, float m02, float m03,
    float m10, float m11, float m12, float m13,
    float m20, float m21, float m22, float m23,
    float m30, float m31, float m32, float m33)
{
    ogt_mesh_transform ret;
    ret.m00 = m00;    ret.m01 = m01;	ret.m02 = m02;	ret.m03 = m03;
    ret.m10 = m10;    ret.m11 = m11;	ret.m12 = m12;	ret.m13 = m13;
    ret.m20 = m20;    ret.m21 = m21;	ret.m22 = m22;	ret.m23 = m23;
    ret.m30 = m30;    ret.m31 = m31;	ret.m32 = m32;	ret.m33 = m33;
    return ret;
}

inline ogt_mesh_vec3 _make_vec3(float x, float y, float z ) {
    ogt_mesh_vec3 ret;
    ret.x = x;	ret.y = y;	ret.z = z;
    return ret;
}

static inline const ogt_mesh_vec3* _make_vec3_ptr(const float* xyz_elements) {
    return (ogt_mesh_vec3*)xyz_elements;
}

static inline float _dot3(const ogt_mesh_vec3& a, const ogt_mesh_vec3& b) {
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

static inline ogt_mesh_vec3 _cross3(const ogt_mesh_vec3& a, const ogt_mesh_vec3& b) {
    ogt_mesh_vec3 ret;
    ret.x = (a.y * b.z) - (a.z * b.y);
    ret.y = (a.z * b.x) - (a.x * b.z);
    ret.z = (a.x * b.y) - (a.y * b.x);
    return ret;
}

static inline ogt_mesh_vec3 _sub3(const ogt_mesh_vec3 & a, const ogt_mesh_vec3 & b) {
    ogt_mesh_vec3 ret;
    ret.x = a.x - b.x;
    ret.y = a.y - b.y;
    ret.z = a.z - b.z;
    return ret;
}
static inline ogt_mesh_vec3 _add3(const ogt_mesh_vec3 & a, const ogt_mesh_vec3 & b) {
    ogt_mesh_vec3 ret;
    ret.x = a.x + b.x;
    ret.y = a.y + b.y;
    ret.z = a.z + b.z;
    return ret;
}
static inline ogt_mesh_vec3 _normalize3(const ogt_mesh_vec3 & a) {
    float len = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
    assert(len > 0.0f);
    float len_inv = 1.0f / len;
    ogt_mesh_vec3 ret;
    ret.x = a.x * len_inv;
    ret.y = a.y * len_inv;
    ret.z = a.z * len_inv;
    return ret;
}

static inline ogt_mesh_vertex _mesh_make_vertex(const ogt_mesh_vec3& pos, const ogt_mesh_vec3& normal, const ogt_mesh_rgba color ) {
    ogt_mesh_vertex ret;
    ret.pos    = pos;
    ret.normal = normal;
    ret.color  = color;
    return ret;
}

static inline ogt_mesh_vertex _mesh_make_vertex(float pos_x, float pos_y, float pos_z, float normal_x, float normal_y, float normal_z, ogt_mesh_rgba color ) {
    return _mesh_make_vertex(_make_vec3(pos_x, pos_y, pos_z), _make_vec3(normal_x, normal_y, normal_z), color);
}

// counts the number of voxel sized faces that are needed for this voxel grid.
static uint32_t _count_voxel_sized_faces( const uint8_t* voxels, uint32_t size_x, uint32_t size_y, uint32_t size_z ) {
    const int32_t k_stride_x = 1;
    const int32_t k_stride_y = size_x;
    const int32_t k_stride_z = size_x * size_y;
    const int32_t k_max_x = size_x - 1;
    const int32_t k_max_y = size_y - 1;
    const int32_t k_max_z = size_z - 1;

    uint32_t face_count  = 0;
        
    const uint8_t* current_voxel = voxels;
    for (uint32_t k = 0; k < size_z; k++)
    {
        for (uint32_t j = 0; j < size_y; j++)
        {
            for (uint32_t i = 0; i < size_x; i++, current_voxel++)
            {
                if (current_voxel[0] != 0) // voxel is not empty.
                {
                    // check each of the -X,+X,-Y,+Y,-Z,+Z directions to see if a face is needed in that direction.
                    face_count += ((i == 0)       || (current_voxel[-k_stride_x] == 0 )) ? 1 : 0; // if on min x boundary of voxel grid, or neighbor to -1 on x is empty
                    face_count += ((i == k_max_x) || (current_voxel[ k_stride_x] == 0 )) ? 1 : 0; // if on max x boundary of voxel grid, or neighbor to +1 on x is empty
                    face_count += ((j == 0)       || (current_voxel[-k_stride_y] == 0 )) ? 1 : 0; // if on min y boundary of voxel grid, or neighbor to -1 on y is empty
                    face_count += ((j == k_max_y) || (current_voxel[ k_stride_y] == 0 )) ? 1 : 0; // if on max y boundary of voxel grid, or neighbor to +1 on y is empty
                    face_count += ((k == 0)       || (current_voxel[-k_stride_z] == 0 )) ? 1 : 0; // if on min z boundary of voxel grid, or neighbor to -1 on z is empty
                    face_count += ((k == k_max_z) || (current_voxel[ k_stride_z] == 0 )) ? 1 : 0; // if on max z boundary of voxel grid, or neighbor to +1 on z is empty
                }
            }
        }
    }
    return face_count;
}


// murmur_hash2 - this variant deals with only 4 bytes at a time
static uint32_t murmur_hash2_size4(uint32_t h, const uint32_t* data, uint32_t data_len) {
    assert(data_len % 4 == 0);	
    const uint32_t m = 0x5bd1e995;
    while (data_len >= 4) {
        uint32_t k = data[0];
        k *= m;
        k ^= k >> (signed)24;
        k *= m;
        h *= m;
        h ^= k;
        data++;
        data_len -= 4;
    }

    return h;
}

// quadratic probing in the hash table.
static uint32_t* hash_table_find_vertex(uint32_t* table, uint32_t table_index_mask, const uint8_t* vertex_data, uint32_t vertex_size, uint32_t vertex_index) {
    uint32_t* this_vertex  = (uint32_t*)&vertex_data[vertex_index*vertex_size];
    uint32_t  bucket_index = murmur_hash2_size4(0, this_vertex, vertex_size) & table_index_mask;

    for (uint32_t probe_count = 0; probe_count <= table_index_mask; probe_count++) {
        uint32_t* existing_index = &table[bucket_index];
        // if there is an uninitialized value at this bucket, the vertex is definitely not already in the hash table.
        if (*existing_index == UINT32_MAX)
            return existing_index;
        // this item is potentially in the table, we compare to see if the existing vertex matches the one we're trying to find.
        uint32_t* existing_vertex = (uint32_t*)&vertex_data[*existing_index*vertex_size];
        if (memcmp(this_vertex, existing_vertex, vertex_size) == 0) {
            assert(*existing_index < vertex_index);
            return existing_index;
        }
        // use quadratic probing to find the next bucket in the case of a collision.
        bucket_index = (bucket_index + probe_count + 1) & table_index_mask;
    }
    // hash table is full. We should technically never get here because we always allocate more buckets in the table than vertices we search for.
    assert(false); 
    return NULL;
}


// quadratic probing in the hash table 
static uint32_t* hash_table_find_vertex_position(uint32_t* table, uint32_t table_index_mask, const ogt_mesh_vertex* vertex_data, uint32_t vertex_index) {
    const ogt_mesh_vertex* this_vertex = &vertex_data[vertex_index];
    uint32_t  bucket_index = murmur_hash2_size4(0, (uint32_t*)& this_vertex->pos, sizeof(this_vertex->pos)) & table_index_mask;

    for (uint32_t probe_count = 0; probe_count <= table_index_mask; probe_count++) {
        uint32_t* existing_index = &table[bucket_index];
        // if there is an uninitialized value at this bucket, the vertex is definitely not already in the hash table.
        if (*existing_index == UINT32_MAX)
            return existing_index;
        // this item is potentially in the table, we compare to see if the existing vertex matches the one we're trying to find.
        const ogt_mesh_vertex * existing_vertex = &vertex_data[*existing_index];
        if (memcmp(&this_vertex->pos, &existing_vertex->pos, sizeof(this_vertex->pos)) == 0) {
            assert(*existing_index < vertex_index);
            return existing_index;
        }
        // use quadratic probing to find the next bucket in the case of a collision.
        bucket_index = (bucket_index + probe_count + 1) & table_index_mask;
    }
    // hash table is full. We should technically never get here because we always allocate more buckets in the table than vertices we search for.
    assert(false);
    return NULL;
}

// removes duplicate vertices in-place from the specified mesh.
void ogt_mesh_remove_duplicate_vertices(const ogt_voxel_meshify_context* ctx, ogt_mesh* mesh) {
    
    uint32_t* indices      = mesh->indices;
    uint32_t  index_count  = mesh->index_count;
    uint8_t*  vertices     = (uint8_t*)mesh->vertices;
    uint32_t  vertex_count = mesh->vertex_count;
    uint32_t  vertex_size  = sizeof(ogt_mesh_vertex);
    assert(indices && index_count && vertices && vertex_count && vertex_size);
    
    // allocate a hash table that is sized at the next power of 2 above the vertex count
    uint32_t hash_table_size = 1;
    while (hash_table_size < vertex_count)
        hash_table_size *= 2;
    uint32_t hash_table_mask = hash_table_size - 1;
    uint32_t* hash_table = (uint32_t*)_voxel_meshify_malloc(ctx, sizeof(uint32_t) * hash_table_size);
    memset(hash_table, -1, hash_table_size * sizeof(uint32_t));

    // generate an remap table for vertex indices
    uint32_t* remap_indices = (uint32_t*)_voxel_meshify_malloc(ctx, sizeof(uint32_t) * vertex_count);
    memset(remap_indices, -1, vertex_count * sizeof(uint32_t));
    uint32_t num_unique_vertices = 0;
    for (uint32_t vertex_index = 0; vertex_index < vertex_count; vertex_index++) {
        uint32_t* hash_table_entry = hash_table_find_vertex(hash_table, hash_table_mask, vertices, vertex_size, vertex_index);
        if (*hash_table_entry == UINT32_MAX) {
            // vertex is not already in the hash table. allocate a unique index for it.
            *hash_table_entry = vertex_index;;
            remap_indices[vertex_index] = num_unique_vertices++;
        }
        else {
            // vertex is already in the hash table. Point this to the index that is already existing!
            assert(remap_indices[*hash_table_entry] != UINT32_MAX);
            remap_indices[vertex_index] = remap_indices[*hash_table_entry];
        }
    }

    // compact all vertices using the remap_indices map.
    for (uint32_t i = 0; i < vertex_count; i++) {
        uint32_t dst_index = remap_indices[i];
        uint32_t src_index = i;
        assert(dst_index <= src_index);
        memcpy(&vertices[dst_index*vertex_size], &vertices[src_index*vertex_size], vertex_size);
    }
    // remap all indices now
    for (uint32_t i = 0; i < index_count; i++) {
        indices[i] = remap_indices[indices[i]];
    }

    _voxel_meshify_free(ctx, hash_table);
    _voxel_meshify_free(ctx, remap_indices);

    assert(num_unique_vertices <= mesh->vertex_count);
    mesh->vertex_count = num_unique_vertices;
}

// resets normals for the mesh so they are based on triangle connectivity, while preserving triangle colors.
void ogt_mesh_smooth_normals(const ogt_voxel_meshify_context* ctx, ogt_mesh* mesh) {
    // generate an remap table for vertex indices based on the vertex positions.
    uint32_t* remap_indices = (uint32_t*)_voxel_meshify_malloc(ctx, sizeof(uint32_t) * mesh->vertex_count);
    memset(remap_indices, -1, mesh->vertex_count * sizeof(uint32_t));
    {
        // allocate a hash table that is sized at the next power of 2 above the vertex count
        uint32_t hash_table_size = 1;
        while (hash_table_size < mesh->vertex_count)
            hash_table_size *= 2;
        uint32_t hash_table_mask = hash_table_size - 1;
        uint32_t* hash_table = (uint32_t*)_voxel_meshify_malloc(ctx, sizeof(uint32_t) * hash_table_size);
        memset(hash_table, -1, hash_table_size * sizeof(uint32_t));

        // create a unique mapping for each vertex based purely on its position
        uint32_t num_unique_vertices = 0;
        for (uint32_t vertex_index = 0; vertex_index < mesh->vertex_count; vertex_index++) {
            uint32_t* hash_table_entry = hash_table_find_vertex_position(hash_table, hash_table_mask, mesh->vertices, vertex_index);
            if (*hash_table_entry == UINT32_MAX) {
                // vertex is not already in the hash table. allocate a unique index for it.
                *hash_table_entry = vertex_index;
                remap_indices[vertex_index] = num_unique_vertices++;
            }
            else {
                // vertex is already in the hash table. Point this to the index that is already existing!
                assert(remap_indices[*hash_table_entry] != UINT32_MAX);
                remap_indices[vertex_index] = remap_indices[*hash_table_entry];
            }
        }
        // now that we have remap_indices, we no longer need the hash table. 
        _voxel_meshify_free(ctx, hash_table);
    }

    // for each triangle face, add the normal of the face to the unique normal for the vertex
    ogt_mesh_vec3* remap_normals = (ogt_mesh_vec3*)_voxel_meshify_malloc(ctx, sizeof(ogt_mesh_vec3) * mesh->vertex_count);
    memset(remap_normals, 0, sizeof(ogt_mesh_vec3) * mesh->vertex_count);

    for (uint32_t i = 0; i < mesh->index_count; i += 3) {
        uint32_t i0 = mesh->indices[i + 0];
        uint32_t i1 = mesh->indices[i + 1];
        uint32_t i2 = mesh->indices[i + 2];
        ogt_mesh_vertex v0 = mesh->vertices[i0];
        ogt_mesh_vertex v1 = mesh->vertices[i1];
        ogt_mesh_vertex v2 = mesh->vertices[i2];
        ogt_mesh_vec3 normal = _cross3(_sub3(v1.pos, v0.pos), _sub3(v2.pos, v0.pos));

        uint32_t ri0 = remap_indices[i0];
        uint32_t ri1 = remap_indices[i1];
        uint32_t ri2 = remap_indices[i2];
        remap_normals[ri0] = _add3(remap_normals[ri0], normal);
        remap_normals[ri1] = _add3(remap_normals[ri1], normal);
        remap_normals[ri2] = _add3(remap_normals[ri2], normal);
    }

    // for each vertex, copy over remap normal if it's non-zero.
    for (uint32_t vertex_index = 0; vertex_index < mesh->vertex_count; vertex_index++) {
        ogt_mesh_vec3 accumulated_normal = remap_normals[remap_indices[vertex_index]];
        if (_dot3(accumulated_normal, accumulated_normal) > 0.001f)
            mesh->vertices[vertex_index].normal = _normalize3(accumulated_normal);
    }

    _voxel_meshify_free(ctx, remap_normals);
    _voxel_meshify_free(ctx, remap_indices);
}

static void _streaming_add_to_mesh(uint32_t x, uint32_t y, uint32_t z, const ogt_mesh_vertex* vertices, uint32_t vertex_count, const uint32_t* indices, uint32_t index_count, void* stream_func_data) {
    // these params are unused for now.
    (void)x; (void)y; (void)z;
    // copy the specified vertices and indices into the mesh
    ogt_mesh* mesh = (ogt_mesh*)stream_func_data;
    memcpy(&mesh->vertices[mesh->vertex_count], vertices, vertex_count * sizeof(ogt_mesh_vertex));
    memcpy(&mesh->indices[mesh->index_count], indices, index_count * sizeof(uint32_t));
    mesh->vertex_count += vertex_count;
    mesh->index_count += index_count;
}

// returns the number of quad faces that would be generated by tessellating the specified voxel field using the simple algorithm.
uint32_t ogt_face_count_from_paletted_voxels_simple(const uint8_t* voxels, uint32_t size_x, uint32_t size_y, uint32_t size_z)
{
    return _count_voxel_sized_faces( voxels, size_x, size_y, size_z );
}

// constructs and returns a mesh from the specified voxel grid with no optimization to the geometry.
ogt_mesh* ogt_mesh_from_paletted_voxels_simple(
    const ogt_voxel_meshify_context* ctx,
    const uint8_t* voxels, uint32_t size_x, uint32_t size_y, uint32_t size_z, const ogt_mesh_rgba* palette) 
{
    uint32_t max_face_count   = _count_voxel_sized_faces( voxels, size_x, size_y, size_z );
    uint32_t max_vertex_count = max_face_count * 4;
    uint32_t max_index_count  = max_face_count * 6;
    
    uint32_t mesh_size = sizeof(ogt_mesh) + (max_vertex_count * sizeof(ogt_mesh_vertex)) + (max_index_count * sizeof(uint32_t));
    ogt_mesh* mesh = (ogt_mesh*)_voxel_meshify_malloc(ctx, mesh_size);
    if (!mesh)
        return NULL;
    
    mesh->vertices = (ogt_mesh_vertex*)&mesh[1];
    mesh->indices  = (uint32_t*)&mesh->vertices[max_vertex_count];
    mesh->vertex_count = 0;
    mesh->index_count  = 0;
    
    ogt_stream_from_paletted_voxels_simple(voxels, size_x, size_y, size_z, palette, _streaming_add_to_mesh, mesh);
    
    assert( mesh->vertex_count == max_vertex_count);
    assert( mesh->index_count == max_index_count);	
    return mesh;
}

// streams geometry for each voxel at a time to a specified user function.
void ogt_stream_from_paletted_voxels_simple(
    const uint8_t* voxels, uint32_t size_x, uint32_t size_y, uint32_t size_z, const ogt_mesh_rgba* palette,
    ogt_voxel_simple_stream_func stream_func, void* stream_func_data) 
{
    assert(stream_func);
    const int32_t k_stride_x = 1;
    const int32_t k_stride_y = size_x;
    const int32_t k_stride_z = size_x * size_y;
    const int32_t k_max_x = size_x - 1;
    const int32_t k_max_y = size_y - 1;
    const int32_t k_max_z = size_z - 1;
    
    const uint8_t* current_voxel = voxels;

    uint32_t total_vertex_count = 0;
    uint32_t total_index_count = 0;
    for (uint32_t k = 0; k < size_z; k++)
    {
        const float min_z = (float)k;
        const float max_z = min_z + 1.0f;
        for (uint32_t j = 0; j < size_y; j++)
        {
            const float min_y = (float)j;
            const float max_y = min_y + 1.0f;
            for (uint32_t i = 0; i < size_x; i++, current_voxel++)
            {
                // current voxel slot is empty? skip it.
                if (current_voxel[0] == 0)
                    continue;

                ogt_mesh_rgba color = palette[ current_voxel[0]];
                
                // determine the min/max coords of the voxel for each dimension.
                const float min_x = (float)i;
                const float max_x = min_x + 1.0f;

                // determine which faces we need to generate
                uint32_t neg_x = ((i == 0)       || (current_voxel[-k_stride_x] == 0));
                uint32_t pos_x = ((i == k_max_x) || (current_voxel[ k_stride_x] == 0));
                uint32_t neg_y = ((j == 0)       || (current_voxel[-k_stride_y] == 0));
                uint32_t pos_y = ((j == k_max_y) || (current_voxel[ k_stride_y] == 0));
                uint32_t neg_z = ((k == 0)       || (current_voxel[-k_stride_z] == 0));
                uint32_t pos_z = ((k == k_max_z) || (current_voxel[ k_stride_z] == 0));

                // count the number of faces. skip if zero.
                const uint32_t face_count_needed = (neg_x + pos_x + neg_y + pos_y + neg_z + pos_z);
                if (!face_count_needed)
                    continue;

                // generate geometry for this voxel to a local buffer first.
                ogt_mesh_vertex  local_vertex[24];
                uint32_t         local_index[36];
                ogt_mesh_vertex* current_vertex = local_vertex;
                uint32_t*        current_index  = local_index;

                // -X direction face
                if (neg_x)
                {
                    current_vertex[0] = _mesh_make_vertex( min_x, min_y, min_z, -1.0f, 0.0f, 0.0f, color );
                    current_vertex[1] = _mesh_make_vertex( min_x, max_y, min_z, -1.0f, 0.0f, 0.0f, color );
                    current_vertex[2] = _mesh_make_vertex( min_x, max_y, max_z, -1.0f, 0.0f, 0.0f, color );
                    current_vertex[3] = _mesh_make_vertex( min_x, min_y, max_z, -1.0f, 0.0f, 0.0f, color );
                    current_index[0] = total_vertex_count + 2;
                    current_index[1] = total_vertex_count + 1;
                    current_index[2] = total_vertex_count + 0;
                    current_index[3] = total_vertex_count + 0;
                    current_index[4] = total_vertex_count + 3;
                    current_index[5] = total_vertex_count + 2;
                    total_vertex_count += 4;
                    total_index_count  += 6;
                    current_vertex += 4;
                    current_index += 6;
                }
                
                // +X direction face
                if (pos_x)
                {
                    current_vertex[0] = _mesh_make_vertex( max_x, min_y, min_z, 1.0f, 0.0f, 0.0f, color );
                    current_vertex[1] = _mesh_make_vertex( max_x, max_y, min_z, 1.0f, 0.0f, 0.0f, color );
                    current_vertex[2] = _mesh_make_vertex( max_x, max_y, max_z, 1.0f, 0.0f, 0.0f, color );
                    current_vertex[3] = _mesh_make_vertex( max_x, min_y, max_z, 1.0f, 0.0f, 0.0f, color );
                    current_index[0] = total_vertex_count + 0;
                    current_index[1] = total_vertex_count + 1;
                    current_index[2] = total_vertex_count + 2;
                    current_index[3] = total_vertex_count + 2;
                    current_index[4] = total_vertex_count + 3;
                    current_index[5] = total_vertex_count + 0;
                    total_vertex_count += 4;
                    total_index_count  += 6;
                    current_vertex += 4;
                    current_index += 6;
                }
                
                // -Y direction face
                if (neg_y)
                {
                    current_vertex[0] = _mesh_make_vertex( min_x, min_y, min_z, 0.0f,-1.0f, 0.0f, color );
                    current_vertex[1] = _mesh_make_vertex( max_x, min_y, min_z, 0.0f,-1.0f, 0.0f, color );
                    current_vertex[2] = _mesh_make_vertex( max_x, min_y, max_z, 0.0f,-1.0f, 0.0f, color );
                    current_vertex[3] = _mesh_make_vertex( min_x, min_y, max_z, 0.0f,-1.0f, 0.0f, color );
                    current_index[0] = total_vertex_count + 0;
                    current_index[1] = total_vertex_count + 1;
                    current_index[2] = total_vertex_count + 2;
                    current_index[3] = total_vertex_count + 2;
                    current_index[4] = total_vertex_count + 3;
                    current_index[5] = total_vertex_count + 0;
                    total_vertex_count += 4;
                    total_index_count  += 6;
                    current_vertex += 4;
                    current_index += 6;
                }
                // +Y direction face
                if (pos_y)
                {
                    current_vertex[0] = _mesh_make_vertex( min_x, max_y, min_z, 0.0f, 1.0f, 0.0f, color );
                    current_vertex[1] = _mesh_make_vertex( max_x, max_y, min_z, 0.0f, 1.0f, 0.0f, color );
                    current_vertex[2] = _mesh_make_vertex( max_x, max_y, max_z, 0.0f, 1.0f, 0.0f, color );
                    current_vertex[3] = _mesh_make_vertex( min_x, max_y, max_z, 0.0f, 1.0f, 0.0f, color );
                    current_index[0] = total_vertex_count + 2;
                    current_index[1] = total_vertex_count + 1;
                    current_index[2] = total_vertex_count + 0;
                    current_index[3] = total_vertex_count + 0;
                    current_index[4] = total_vertex_count + 3;
                    current_index[5] = total_vertex_count + 2;
                    total_vertex_count += 4;
                    total_index_count  += 6;
                    current_vertex += 4;
                    current_index += 6;
               }
                // -Z direction face
                if (neg_z)
                {
                    current_vertex[0] = _mesh_make_vertex( min_x, min_y, min_z, 0.0f, 0.0f,-1.0f, color );
                    current_vertex[1] = _mesh_make_vertex( max_x, min_y, min_z, 0.0f, 0.0f,-1.0f, color );
                    current_vertex[2] = _mesh_make_vertex( max_x, max_y, min_z, 0.0f, 0.0f,-1.0f, color );
                    current_vertex[3] = _mesh_make_vertex( min_x, max_y, min_z, 0.0f, 0.0f,-1.0f, color );
                    current_index[0] = total_vertex_count + 2;
                    current_index[1] = total_vertex_count + 1;
                    current_index[2] = total_vertex_count + 0;
                    current_index[3] = total_vertex_count + 0;
                    current_index[4] = total_vertex_count + 3;
                    current_index[5] = total_vertex_count + 2;
                    total_vertex_count += 4;
                    total_index_count  += 6;
                    current_vertex += 4;
                    current_index += 6;
                }
                // +Z direction face
                if (pos_z)
                {
                    current_vertex[0] = _mesh_make_vertex( min_x, min_y, max_z, 0.0f, 0.0f, 1.0f, color );
                    current_vertex[1] = _mesh_make_vertex( max_x, min_y, max_z, 0.0f, 0.0f, 1.0f, color );
                    current_vertex[2] = _mesh_make_vertex( max_x, max_y, max_z, 0.0f, 0.0f, 1.0f, color );
                    current_vertex[3] = _mesh_make_vertex( min_x, max_y, max_z, 0.0f, 0.0f, 1.0f, color );
                    current_index[0] = total_vertex_count + 0;
                    current_index[1] = total_vertex_count + 1;
                    current_index[2] = total_vertex_count + 2;
                    current_index[3] = total_vertex_count + 2;
                    current_index[4] = total_vertex_count + 3;
                    current_index[5] = total_vertex_count + 0;
                    total_vertex_count += 4;
                    total_index_count  += 6;
                    current_vertex += 4;
                    current_index += 6;
                }

                // geometry for this voxel is provided to a caller-specified stream function/callback
                stream_func(i, j, k, local_vertex, face_count_needed*4, local_index, face_count_needed*6, stream_func_data);
            }
        }
    }	

}


// The base algorithm that is used here, is as follows:
// On a per slice basis, we find a voxel that has not yet been polygonized. We then try to 
// grow a rectangle from that voxel within the slice that can be represented by a polygon.
// We create the quad polygon to represent the voxel, mark the voxels in the slice that are
// covered by the rectangle as having been polygonized, and continue on the search through 
// the rest of the slice.
void _greedy_meshify_voxels_in_face_direction(
    const uint8_t* voxels,
    const ogt_mesh_rgba* palette,
    int32_t size_x, int32_t size_y, int32_t size_z,                // how many voxels in each of X,Y,Z dimensions
    int32_t k_stride_x, int32_t k_stride_y, int32_t k_stride_z,            // the memory stride for each of those X,Y,Z dimensions within the voxel data.
    const ogt_mesh_transform& transform,                                    // transform to convert from X,Y,Z to "objectSpace"
    ogt_mesh* out_mesh)
{

    // enable aggressive voxel optimization for now.
    uint32_t max_voxels_per_slice = size_x * size_y;

    // allocate a structure that is used for tracking which voxels in a slice have already been included in output mesh.
    assert(max_voxels_per_slice <= 65536);	// 
    ogt_mesh_bitset_64k voxel_polygonized;

    ogt_mesh_vec3 normal = _transform_vector(transform, _make_vec3(0.0f, 0.0f, 1.0f));

#define VOXELDATA_INDEX(_x,_y,_z)        ((_x) * k_stride_x) + ((_y) * k_stride_y) + ((_z) * k_stride_z)
#define LOCALDATA_INDEX(_x,_y)            ((_x) + ((_y) * size_x))

    // use this to remap parity where necessary.
    uint32_t base_index_start = out_mesh->index_count;

    uint32_t* index_data         = &out_mesh->indices[out_mesh->index_count];
    ogt_mesh_vertex* vertex_data = &out_mesh->vertices[out_mesh->vertex_count];

    // determine if the transform parity has flipped in a way that winding would have been switched.
    const ogt_mesh_vec3* side = _make_vec3_ptr(&transform.m00);
    const ogt_mesh_vec3* up   = _make_vec3_ptr(&transform.m10);
    const ogt_mesh_vec3* fwd  = _make_vec3_ptr(&transform.m20);
    bool is_parity_flipped = _dot3(*fwd, _cross3(*side, *up)) < 0.0f;

    for (int32_t k0 = 0; k0 < size_z; k0++) {
        // k0 = current slice, k1 = next slice
        int32_t k1 = k0 + 1;

        // reset the per-voxel X/Y status for this slice.
        voxel_polygonized.clear(max_voxels_per_slice);

        // is this the last slice? If yes, we don't check
        bool is_last_k_slice = (k1 == size_z);

        // here, we search for the first unprocessed voxel
        for (int32_t j0 = 0; j0 < size_y; j0++) {
            for (int32_t i0 = 0; i0 < size_x; i0++) {
                // determine the polygon color index
                uint8_t  color_index = voxels[VOXELDATA_INDEX(i0, j0, k0)];

                // this voxel doesn't need to be polygonized if...
                if ((color_index == 0) ||                                             // (1) voxel is empty
                    voxel_polygonized.is_set(LOCALDATA_INDEX(i0, j0)) ||              // (2) voxel is already part of a polygon for the zslice.
                    (!is_last_k_slice && voxels[VOXELDATA_INDEX(i0, j0, k1)] != 0))   // (3) voxel in the next slice (+z direction) is solid
                {
                    continue;
                }

                // compute i1. This is the coord bounding the longest span of identical voxels in the +i direction.
                int32_t i1 = i0 + 1;
                for (i1 = i0 + 1; i1 < size_x; i1++) {
                    // stop extending i1 if...
                    if ((voxels[VOXELDATA_INDEX(i1, j0, k0)] != color_index) ||			// (1) this voxel doesn't match the match color
                        (voxel_polygonized.is_set(LOCALDATA_INDEX(i1, j0))) ||          // (2) voxel is already part of a polygon for the zslice
                        (!is_last_k_slice && voxels[VOXELDATA_INDEX(i1, j0, k1)] != 0)) // (3) voxel in the next slice (+z direction) is solid
                    {
                        break;
                    }
                }


                // compute j1. The is the coord bounding the longest span of identical voxels [i0..i1] in the +j direction
                int32_t j1 = j0 + 1;
                for (j1 = j0 + 1; j1 < size_y; j1++) {
                    bool got_j1 = false;
                    for (int32_t a = i0; a < i1; a++) {
                        // stop extending i1 if...
                        if ((voxels[VOXELDATA_INDEX(a, j1, k0)] != color_index) ||        // (1) this voxel doesn't match the match color
                            (voxel_polygonized.is_set(LOCALDATA_INDEX(a,j1))) ||          // (2) voxel is already part of a polygon for the zslice
                            (!is_last_k_slice && voxels[VOXELDATA_INDEX(a,j1,k1)] != 0))  // (3) voxel in the next slice (+z direction) is solid
                        {
                            got_j1 = true;
                            break;
                        }
                    }
                    if (got_j1)
                        break;
                }


                // now j1 and i1 are the upper bound (exclusive) of the rectangle starting from i0,j0. 
                // mark all of this slice voxels in that rectangle as processed.
                for (int32_t b = j0; b < j1; b++)
                    for (int32_t a = i0; a < i1; a++)
                        voxel_polygonized.set(LOCALDATA_INDEX(a,b));

                // determine the min/max coords of the polygon for each dimension.
                float min_x = (float)i0;
                float max_x = (float)i1;
                float min_y = (float)j0;
                float max_y = (float)j1;
                float max_z = (float)k1;

                // cache the color
                ogt_mesh_rgba color = palette[color_index];

                // write the verts for this face
                vertex_data[0] = _mesh_make_vertex(_transform_point(transform, _make_vec3(min_x, min_y, max_z)), normal, color);
                vertex_data[1] = _mesh_make_vertex(_transform_point(transform, _make_vec3(max_x, min_y, max_z)), normal, color);
                vertex_data[2] = _mesh_make_vertex(_transform_point(transform, _make_vec3(max_x, max_y, max_z)), normal, color);
                vertex_data[3] = _mesh_make_vertex(_transform_point(transform, _make_vec3(min_x, max_y, max_z)), normal, color);

                // reserve the index order to ensure parity/winding is still correct.
                if (is_parity_flipped) {
                    index_data[0] = out_mesh->vertex_count + 0;
                    index_data[1] = out_mesh->vertex_count + 3;
                    index_data[2] = out_mesh->vertex_count + 2;
                    index_data[3] = out_mesh->vertex_count + 2;
                    index_data[4] = out_mesh->vertex_count + 1;
                    index_data[5] = out_mesh->vertex_count + 0;
                }
                else {
                    index_data[0] = out_mesh->vertex_count + 0;
                    index_data[1] = out_mesh->vertex_count + 1;
                    index_data[2] = out_mesh->vertex_count + 2;
                    index_data[3] = out_mesh->vertex_count + 2;
                    index_data[4] = out_mesh->vertex_count + 3;
                    index_data[5] = out_mesh->vertex_count + 0;
                }

                vertex_data += 4;
                index_data  += 6;

                out_mesh->vertex_count += 4;
                out_mesh->index_count += 6;
            }
        }
    }

#undef VOXELDATA_INDEX
#undef LOCALDATA_INDEX

}

ogt_mesh* ogt_mesh_from_paletted_voxels_greedy(
    const ogt_voxel_meshify_context* ctx,
    const uint8_t* voxels, uint32_t size_x, uint32_t size_y, uint32_t size_z, const ogt_mesh_rgba* palette) 
{
    uint32_t max_face_count   = _count_voxel_sized_faces( voxels, size_x, size_y, size_z );
    uint32_t max_vertex_count = max_face_count * 4;
    uint32_t max_index_count  = max_face_count * 6;
    
    uint32_t mesh_size = sizeof(ogt_mesh) + (max_vertex_count * sizeof(ogt_mesh_vertex)) + (max_index_count * sizeof(uint32_t));
    ogt_mesh* mesh = (ogt_mesh*)_voxel_meshify_malloc(ctx, mesh_size);
    if (!mesh)
        return NULL;
    
    mesh->vertices = (ogt_mesh_vertex*)&mesh[1];
    mesh->indices  = (uint32_t*)&mesh->vertices[max_vertex_count];
    mesh->vertex_count = 0;
    mesh->index_count  = 0;
    
    const int32_t k_stride_x = 1;
    const int32_t k_stride_y = size_x;
    const int32_t k_stride_z = size_x * size_y;
    
    // do the +y PASS
    {
        ogt_mesh_transform transform_pos_y = _make_transform(
            0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f);

        _greedy_meshify_voxels_in_face_direction(
            voxels, palette,
            size_z, size_x, size_y,
            k_stride_z, k_stride_x, k_stride_y,
            transform_pos_y,
            mesh);
    }
    // do the -y PASS
    {
        ogt_mesh_transform transform_neg_y = _make_transform(
            0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f,-1.0f, 0.0f, 0.0f,
            0.0f, (float)(size_y), 0.0f, 0.0f);

        _greedy_meshify_voxels_in_face_direction(
            voxels + (size_y - 1) * k_stride_y, 
            palette,
            size_z, size_x, size_y,
            k_stride_z, k_stride_x,-k_stride_y,
            transform_neg_y,
            mesh);
    }
    // do the +X pass
    {
        ogt_mesh_transform transform_pos_x = _make_transform(
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f);

        _greedy_meshify_voxels_in_face_direction(
            voxels, palette,
            size_y, size_z, size_x,
            k_stride_y, k_stride_z, k_stride_x,
            transform_pos_x,
            mesh);
    }
    // do the -X pass
    {
        ogt_mesh_transform transform_neg_x = _make_transform(
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
           -1.0f, 0.0f, 0.0f, 0.0f,
        (float)size_x, 0.0f, 0.0f, 0.0f);

        _greedy_meshify_voxels_in_face_direction(
            voxels + (size_x - 1) * k_stride_x,
            palette,
            size_y, size_z, size_x,
            k_stride_y, k_stride_z, -k_stride_x,
            transform_neg_x,
            mesh);
    }
    // do the +Z pass
    {
        ogt_mesh_transform transform_pos_z = _make_transform(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f);

        _greedy_meshify_voxels_in_face_direction(
            voxels, palette,
            size_x, size_y, size_z,
            k_stride_x, k_stride_y, k_stride_z,
            transform_pos_z,
            mesh);
    }
    // do the -Z pass
    {
        ogt_mesh_transform transform_neg_z = _make_transform(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, -1.0f, 0.0f,
            0.0f, 0.0f,(float)size_z, 0.0f);

        _greedy_meshify_voxels_in_face_direction(
            voxels + (size_z-1) * k_stride_z, 
            palette,
            size_x, size_y, size_z,
            k_stride_x, k_stride_y, -k_stride_z,
            transform_neg_z,
            mesh);
    }

    assert( mesh->vertex_count <= max_vertex_count);
    assert( mesh->index_count <= max_index_count);	
    return mesh;
}

struct ogt_mesh_vec2i {
    int32_t x, y;
};

inline ogt_mesh_vec2i make_vec2i(int32_t x, int32_t y) {
    ogt_mesh_vec2i ret;
    ret.x = x;
    ret.y = y;
    return ret;
}

inline ogt_mesh_vec2i operator + (const ogt_mesh_vec2i& lhs, const ogt_mesh_vec2i& rhs) {
    ogt_mesh_vec2i ret;
    ret.x = lhs.x + rhs.x;
    ret.y = lhs.y + rhs.y;
    return ret;
}

inline ogt_mesh_vec2i operator - (const ogt_mesh_vec2i& lhs, const ogt_mesh_vec2i& rhs) {
    ogt_mesh_vec2i ret;
    ret.x = lhs.x - rhs.x;
    ret.y = lhs.y - rhs.y;
    return ret;
}

inline ogt_mesh_vec2i operator * (const ogt_mesh_vec2i& lhs, const int32_t& rhs) {
    ogt_mesh_vec2i ret;
    ret.x = lhs.x * rhs;
    ret.y = lhs.y * rhs;
    return ret;
}

inline bool is_vec2i_equal (const ogt_mesh_vec2i& lhs, const ogt_mesh_vec2i& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

ogt_mesh_vec2i get_cardinal_unit_vector(const ogt_mesh_vec2i& vec) {
    assert((vec.x == 0 && vec.y != 0) || (vec.y == 0 && vec.x != 0));	// assumes this is a cardinal vector	
    if (vec.x <= -1) return make_vec2i(-1, 0);
    if (vec.x >=  1) return make_vec2i( 1, 0);
    if (vec.y <= -1) return make_vec2i( 0,-1);
    if (vec.y >=  1) return make_vec2i( 0, 1);
    assert(0); // unreachable unless the above assert already failed!
    return make_vec2i(0,0);
}

int32_t get_cardinal_vector_length(const ogt_mesh_vec2i& vec) {
    assert((vec.x == 0 && vec.y != 0) || (vec.y == 0 && vec.x != 0));
    return vec.x == 0 ? abs(vec.y) : 
           vec.y == 0 ? abs(vec.x) : 
           0;
}

// gets the signed area of the triangle
inline int32_t get_triangle_signed_area(const ogt_mesh_vec2i& v0,const ogt_mesh_vec2i& v1,const ogt_mesh_vec2i& v2) {
    return ((v0.x - v1.x) * (v2.y - v1.y)) - ((v0.y - v1.y) * (v2.x - v1.x));
}

// determines whether the triangle is convex or not
inline bool is_triangle_convex(const ogt_mesh_vec2i& v0,const ogt_mesh_vec2i& v1,const ogt_mesh_vec2i& v2) {
    return get_triangle_signed_area(v0, v1, v2) > 0;
}

// determines whether the point p is inside the triangle v0,v1,v2
bool is_point_in_triangle(const ogt_mesh_vec2i& v0,const ogt_mesh_vec2i& v1,const ogt_mesh_vec2i& v2, const ogt_mesh_vec2i& p) {
    bool convex_v0v1 = get_triangle_signed_area(v0,v1,p) >= 0;
    bool convex_v1v2 = get_triangle_signed_area(v1,v2,p) >= 0;
    bool convex_v2v0 = get_triangle_signed_area(v2,v0,p) >= 0;
    return (convex_v0v1 == convex_v1v2) && (convex_v0v1 == convex_v2v0);
}

uint32_t _tessellate_polygon(uint32_t* indices, const ogt_mesh_vec2i* verts, uint32_t vert_count) {
    static const uint32_t k_max_polygon_size = 16384;   // 32KB of stack!
    uint16_t ring_indices[k_max_polygon_size]; 
    assert(vert_count >= 3 && vert_count <= k_max_polygon_size);

    for (uint16_t i = 0; i < vert_count; i++)
        ring_indices[i] = i;
    uint32_t ring_count = vert_count;

    uint32_t index_count = 0;

    // naieve "ear clipping" algorithm. Start with a ring of polygon corners.
    // Find 3 sequential corners on the polygon and make sure no other points of the polygon are contained within it.
    // If so, remove the inner point from the ring. Rinse and repeat.
    uint32_t no_progress_counter = 0;
    uint32_t i0 = 0;
    while (ring_count > 3) {
                 i0 = (i0 + 0) % ring_count;
        uint32_t i1 = (i0 + 1) % ring_count;
        uint32_t i2 = (i0 + 2) % ring_count;

        ogt_mesh_vec2i v0 = verts[ring_indices[i0]];
        ogt_mesh_vec2i v1 = verts[ring_indices[i1]];
        ogt_mesh_vec2i v2 = verts[ring_indices[i2]];

        // check whether we can carve off this ear.
        bool can_triangulate = is_triangle_convex(v0, v1, v2);

        if (can_triangulate) {
            // make sure that no other points are inside this triangle. We do allow points to be coincident with corners of the triangle though.
            for (uint32_t i = 0; i < ring_count; i++) {
                if (i == i0 || i == i1 || i == i2)
                    continue;
                const ogt_mesh_vec2i& p = verts[ring_indices[i]];
                bool point_on_corner = is_vec2i_equal(v0, p) || is_vec2i_equal(v1,p) || is_vec2i_equal(v2,p);
                if (!point_on_corner && is_point_in_triangle(v0,v1,v2,verts[ring_indices[i]])) {
                    can_triangulate = false;
                    break;
                }
            }
        }

        if (can_triangulate) {
            indices[index_count++] = (uint32_t)ring_indices[i2];
            indices[index_count++] = (uint32_t)ring_indices[i1];
            indices[index_count++] = (uint32_t)ring_indices[i0];
            // compact verts down in the ring indices
            ring_count--;
            for (uint32_t i = i1; i < ring_count; i++)
                ring_indices[i] = ring_indices[i+1];
            // reset no progress counter because we just made progress!
            no_progress_counter = 0;
        }
        else {
            no_progress_counter++;
            i0++;
        }
        // we haven't made progress in a full trip around the ring -- the geometry is probably malformed and cannot be tessellated
        if (no_progress_counter == ring_count) {
            return index_count;
        }
    }
    // trailing case, just have one triangle left -- emit it.
    indices[index_count++] = (uint32_t)ring_indices[2];
    indices[index_count++] = (uint32_t)ring_indices[1];
    indices[index_count++] = (uint32_t)ring_indices[0];

    return index_count;
}

// where do we sample for the specified edge
ogt_mesh_vec2i get_edge_bias(const ogt_mesh_vec2i& edge_vert0, const ogt_mesh_vec2i& edge_vert1) {
    if (edge_vert0.x < edge_vert1.x) {
        assert(edge_vert0.y == edge_vert1.y);
        return make_vec2i(0, 0);
    } 
    else if (edge_vert0.x > edge_vert1.x ) {
        assert(edge_vert0.y == edge_vert1.y);
        return make_vec2i(-1, -1);
    }
    else if (edge_vert0.y < edge_vert1.y) {
        assert(edge_vert0.x == edge_vert1.x);
        return make_vec2i(-1, 0);
    }
    else if (edge_vert0.y > edge_vert1.y) {
        assert(edge_vert0.x == edge_vert1.x);
        return make_vec2i(0,-1);
    }
    else {
        assert(0);
        return make_vec2i(0,0);
    }
}

uint32_t _tessellate_edge(ogt_mesh_vec2i* tess, uint32_t max_tess, const ogt_mesh_vec2i& edge_vert0, const ogt_mesh_vec2i& edge_vert1, 
                         const uint8_t* slice_colors, int32_t size_x, int32_t size_y ) {

    uint32_t num_tess = 0;
    int32_t  edge_len       = get_cardinal_vector_length(edge_vert1 - edge_vert0);
    ogt_mesh_vec2i edge_bias = get_edge_bias(edge_vert0, edge_vert1);
    ogt_mesh_vec2i edge_step = get_cardinal_unit_vector(edge_vert1 - edge_vert0);
    ogt_mesh_vec2i curr_pos  = edge_vert0 + edge_bias;

    // do early-out logic if the entire edge is out-of-bounds
    if (edge_vert0.x < edge_vert1.x) {
        // handle left-to-right edges
        assert(curr_pos.y <= size_y);
        if (curr_pos.y == size_y)
            return 0;
    }
    else if (edge_vert0.x > edge_vert1.x ) {
        // handle right-to-left edges
        assert(curr_pos.y >= -1);
        if (curr_pos.y == -1)
            return 0;
    }
    else if (edge_vert0.y < edge_vert1.y) {
        // handle bottom-to-top edges
        assert(curr_pos.x >= -1);
        if (curr_pos.x == -1)
            return 0;
    
    }
    else if (edge_vert0.y > edge_vert1.y) {
        // handle top-to-bottom
        assert(curr_pos.x <= size_x);
        if (curr_pos.x == size_x)
            return 0;
    }
    else {
        assert(0);
        return 0;
    }

    uint8_t last_color_index = slice_colors[curr_pos.x + (curr_pos.y * size_x)];
    curr_pos = curr_pos + edge_step;

    for (int32_t i = 1; i < edge_len; i++) {
        uint8_t curr_color_index = slice_colors[curr_pos.x + (curr_pos.y * size_x)];
        if (curr_color_index != last_color_index) {
            assert(num_tess < max_tess);
            tess[num_tess++] = curr_pos - edge_bias;
            last_color_index = curr_color_index;
        }
        curr_pos = curr_pos + edge_step;
    }
    return num_tess;
}

// My algorithm for flood-filling the polygon.
//
// We start with a simple set of verts that represents a polygon ring
// surrounding a single voxel.
//  
//    v1 +---+ v2
//       | C |
//    v0 +---+ v3
//
// This initial state has 4 verts on the polygon boundary which 
// represents 4 edges on the polygon boundary, and the color of
// the polygon is the voxel color.
//
// The algorithm from here is fairly simple:
// 1. select the next edge from the polygon ring
// 2. try extrude the edge as far as possible along its outward
//    facing normal one voxel at a time.
//    - stop pushing the edge outward if we'd consume an already polygonized voxel, or if the edge 
//      would push a different color to the inside of the polygon ring.
// 3. if the edge could be extruded, insert new points for the edge and each of its left/right 
//    neighbor edges such that color discontinuities on the outside of the edge have a vertex between them.
// 4. if the edge could not be extruded and we have gone around the entire ring with no progress,
//    terminate, otherwise goto 1. 
// 5. we now have a polygon ring that can be tessellated.
//
// Example 
// step(1):
//
//        (e1)
//   v1 +-------+ v2
//(e0)  | X | X |	 (e2)		X is the color inside the polygon
//   v0 +       + v3
//
// edge1 corresponds to the next_edge_index edge within the current polygon ring.
//
// step(2)
// We now try to push edge1 out as far as we can by holding v0 & v3 fixed 
// and moving v1/v2 in the direction of the edge normal:
//
//          ^
//          ^
//   v1 +-------+ v2
//      | X | X |
//      |   |   |          
//      | X | X |
//      |   |   |          
//      | X | X |
//   v0 +       + v3
//
// We can only extend the edge as long as by doing so the polygon remains
// filled with it's current color and if the polygon does not cover a
// voxel that was already polygonized by a prior pass.
//
// step(3)
// Once we've finished pushing v1/v2 forward, we now check whether we have
// to tessellate any of the edges e0, e1, e2. This is now mostly a function
// of which colors are on the outside of the polygon boundary.
// 
//        X   C
//      +---*---+ 
//    B | X | X | D			A,B,C,D are colors along the outside of the edges.
//      *   |   |           Here * are new points of the edges because of an 
//    A | X | X | D			exterior change of color along the edge.
//      |   |   |          
//    A | X | X | D
//      +       + 
//
// We then insert the new edges into the polygon ring and select a new
// edge to start extruding.
// If we tessellated e1, then we set next_edge_index to the first child-edge 
// on that edge again:
//
//         v2  
//   v1 +---*---+ v3
//      | X   X
//   v0 *
//
// When we can no longer extrude any of the polygon ring edges, we
// terminate, as that'll mean we've flood filled the space.
//
int32_t _construct_polygon_for_slice(ogt_mesh_vec2i* verts, uint32_t max_verts, int32_t i, int32_t j, int32_t size_x, int32_t size_y, const uint8_t* slice_colors, ogt_mesh_bitset_64k& voxel_polygonized) {
    assert(max_verts > 4);
    // start with just a single 4 vertex closed polygon
    verts[0] = make_vec2i(i,   j  );
    verts[1] = make_vec2i(i,   j+1);
    verts[2] = make_vec2i(i+1, j+1);
    verts[3] = make_vec2i(i+1, j  );
    int32_t vert_count = 4;

    uint8_t polygon_color_index = slice_colors[i+(j*size_x)];

    // mark this voxel as polygonized so it can't be further considered.
    voxel_polygonized.set(i+(j*size_x));

    int32_t next_edge_index = 0;
    int32_t no_progress_counter = 0;
    while (no_progress_counter < vert_count ) {
        // this is just figuring out the vertices for 3 successive edges.
        // edge0 from v0...v1, edge1 from v1...v2, edge2 from v2...v3
        int32_t v0 = next_edge_index < 1 ? vert_count-1 : next_edge_index-1;
        int32_t v1 = next_edge_index;
        int32_t v2 = (v1 < (vert_count-1)) ? v1 + 1 : 0;
        int32_t v3 = (v2 < (vert_count-1)) ? v2 + 1 : 0;

        // compute the direction of edge1.
        ogt_mesh_vec2i edge0_unitvec  = get_cardinal_unit_vector(verts[v1] - verts[v0]);
        ogt_mesh_vec2i edge1_unitvec  = get_cardinal_unit_vector(verts[v2] - verts[v1]);
        ogt_mesh_vec2i edge2_unitvec  = get_cardinal_unit_vector(verts[v3] - verts[v2]);
        ogt_mesh_vec2i edge1_normal   = make_vec2i(-edge1_unitvec.y, edge1_unitvec.x);

        bool can_extrude_edge1 = !is_vec2i_equal(edge1_normal, (edge0_unitvec * -1)) && !is_vec2i_equal(edge1_normal, edge2_unitvec);

        int32_t edge1_pushed_distance = 0;
        if (can_extrude_edge1)
        {
            int32_t		   edge1_len      = get_cardinal_vector_length(verts[v2] - verts[v1]);
            // start 1 unit pushed forward along the edge normal
            ogt_mesh_vec2i edge1_origin = verts[v1] + get_edge_bias(verts[v1], verts[v2]);

            while (edge1_origin.x >= 0 && edge1_origin.y >= 0 && edge1_origin.x < size_x && edge1_origin.y < size_y) {
                // scan along the entire edge to see if the following criteria are met by pushing the edge forward to this position:
                // 1. all of the cells it occupies match the existing polygon color
                // 2. none of the cells are already voxelized.
                ogt_mesh_vec2i edge1_cursor = edge1_origin;
                bool can_push_edge = true;
                for (int32_t i = 0; i < edge1_len; i++) {
                    assert(edge1_cursor.x >= 0 && edge1_cursor.x < size_x && edge1_cursor.y >= 0 && edge1_cursor.y < size_y);
                    int32_t slice_index = edge1_cursor.x + (edge1_cursor.y * size_x);
                    if ( slice_colors[slice_index] != polygon_color_index || voxel_polygonized.is_set(slice_index)) {
                        can_push_edge = false;
                        break;
                    }
                    edge1_cursor = edge1_cursor + edge1_unitvec;
                }
                // we can't push the edge to this location, we've gone as far as we can go with this edge, so jump out immediately.
                if (!can_push_edge)
                    break;
                // mark these cells as voxelized as they'll now be part of the polygon.
                {
                    ogt_mesh_vec2i edge1_cursor = edge1_origin;
                    for (int32_t i = 0; i < edge1_len; i++) {
                        int32_t slice_index = edge1_cursor.x + (edge1_cursor.y * size_x);
                        voxel_polygonized.set(slice_index);
                        edge1_cursor = edge1_cursor + edge1_unitvec;
                    }
                }

                // we can push the edge, bump up the distance we've pushed it.
                edge1_pushed_distance++;
                edge1_origin = edge1_origin + edge1_normal; // step to the next candidate origin.
            }
        }

        if (!edge1_pushed_distance) {
            // step to the next edge 
            next_edge_index = (next_edge_index + 1) % vert_count;
            // bump the exit counter
            no_progress_counter++;
        }
        else {
            // if edge1 was pushed out, we now replace
            //  v0, v1, v2, v3 in the poly ringbuffer with t0, t1, t2, t3  where t* represents multiple points.

            // (0) cache verts v0,v3 and update verts v1, v2
            ogt_mesh_vec2i cached_v0 = verts[v0];
            ogt_mesh_vec2i cached_v1 = verts[v1];
            ogt_mesh_vec2i cached_v2 = verts[v2];
            ogt_mesh_vec2i cached_v3 = verts[v3];
            ogt_mesh_vec2i extruded_v1 = verts[v1] + (edge1_normal * edge1_pushed_distance);
            ogt_mesh_vec2i extruded_v2 = verts[v2] + (edge1_normal * edge1_pushed_distance);

            // determine if edge1 is an extrude from edge0 and if edge1 is an extrude from edge2.
            // If it is not an extrude, it is an extension.
            bool is_e0e1_extrude = is_vec2i_equal(edge0_unitvec, edge1_unitvec); 
            bool is_e1e2_extrude = is_vec2i_equal(edge1_unitvec, edge2_unitvec); 

            // (1) try tessellate edge0, edge1, edge2.
            const uint32_t k_max_tessellations = 512;
            assert(edge1_pushed_distance < k_max_tessellations);
            ogt_mesh_vec2i tess_buffer[k_max_tessellations];
            uint32_t tess_offset = 0;
            
            // allocate tess_e0
            uint32_t e0_offset = tess_offset;
            if (is_e0e1_extrude) {
                tess_buffer[tess_offset++] = cached_v0;
                tess_buffer[tess_offset++] = cached_v1;
            }
            else {
                tess_buffer[tess_offset++] = cached_v0;
                tess_offset += _tessellate_edge(&tess_buffer[tess_offset], k_max_tessellations-tess_offset, cached_v0, extruded_v1, slice_colors, size_x, size_y );
            }
            uint32_t tess_count_e0 = tess_offset - e0_offset;
            // allocate tess_e1
            uint32_t e1_offset = tess_offset;
            if (is_e0e1_extrude)
                tess_offset += _tessellate_edge(&tess_buffer[tess_offset], k_max_tessellations-tess_offset, cached_v1, extruded_v1, slice_colors, size_x, size_y );
            tess_buffer[tess_offset++] = extruded_v1;
            tess_offset += _tessellate_edge(&tess_buffer[tess_offset], k_max_tessellations-tess_offset, extruded_v1, extruded_v2, slice_colors, size_x, size_y );
            tess_buffer[tess_offset++] = extruded_v2;
            if (is_e1e2_extrude)
                tess_offset += _tessellate_edge(&tess_buffer[tess_offset], k_max_tessellations-tess_offset, extruded_v2, cached_v2, slice_colors, size_x, size_y );
            uint32_t tess_count_e1 = tess_offset - e1_offset;
            // allocate tess_e2
            uint32_t e2_offset = tess_offset;
            if (is_e1e2_extrude)
                tess_buffer[tess_offset++] = cached_v2;
            else
                tess_offset += _tessellate_edge(&tess_buffer[tess_offset], k_max_tessellations-tess_offset, extruded_v2, cached_v3, slice_colors, size_x, size_y );
            uint32_t tess_count_e2 = tess_offset - e2_offset;
            // allocate tess_e3
            uint32_t e3_offset = tess_offset;
            tess_buffer[tess_offset++] = cached_v3;
            uint32_t tess_count_e3 = tess_offset - e3_offset;

            const ogt_mesh_vec2i* new_tess_e0 = &tess_buffer[e0_offset];
            const ogt_mesh_vec2i* new_tess_e1 = &tess_buffer[e1_offset];
            const ogt_mesh_vec2i* new_tess_e2 = &tess_buffer[e2_offset];
            const ogt_mesh_vec2i* new_tess_e3 = &tess_buffer[e3_offset];

            // (2) insert the tessellations into the polygon ring. 
            // This bit is tricky as v0,v1,v2,v3 can straddle the end of the polygon ring in 4 different combinations which we handle here:
            int32_t other1_count = vert_count - 4;
            int32_t other2_count = 0;
            int32_t old_other1 = (v3 + 1) % vert_count;
            int32_t old_other2 = 0;
            int32_t new_other2 = -1;
            int32_t new_v0, new_v1, new_v2, new_v3, new_other1; 

            if (v1 < v0) {
                //   [v1][v2][v3] (.... other1 ....) [v0]
                assert(v1 == 0 && v0 == (vert_count-1));
                new_v1     = 0;
                new_v2     = new_v1 + tess_count_e1;
                new_v3     = new_v2 + tess_count_e2;
                new_other1 = new_v3 + tess_count_e3;
                new_v0     = new_other1 + other1_count;
            }
            else if (v2 < v0) {
                //   [v2][v3] (.... other1 ....) [v0][v1]
                assert(v2 == 0 && v1 == (vert_count-1));
                new_v2     = 0;
                new_v3     = new_v2 + tess_count_e2;
                new_other1 = new_v3 + tess_count_e3;
                new_v0     = new_other1  + other1_count;
                new_v1     = new_v0 + tess_count_e0;
            }
            else if (v3 < v0) {
                //   [v3] (.... other1 ....) [v0][v1][v2]
                assert(v3 == 0 && v2 == (vert_count-1));
                new_v3     = 0;
                new_other1 = new_v3 + tess_count_e3;
                new_v0     = new_other1 + other1_count;
                new_v1     = new_v0 + tess_count_e0;
                new_v2     = new_v1 + tess_count_e1;
            }
            else {
                //   (.... other1 ....)  [v0][v1][v2][v3] (...other2...)
                other2_count = vert_count - v3 - 1;
                old_other2   = v3+1;
                other1_count = vert_count - 4 - other2_count;
                old_other1   = 0;
                new_other1   = 0;
                new_v0       = new_other1 + other1_count;
                new_v1       = new_v0 + tess_count_e0;
                new_v2       = new_v1 + tess_count_e1;
                new_v3       = new_v2 + tess_count_e2;
                new_other2   = new_v3 + tess_count_e3;
            }
            uint32_t new_vert_count = vert_count - 4 + tess_count_e0 + tess_count_e1 + tess_count_e2 + tess_count_e3;
            // make sure we wouldn't exceed our polygon ring memory size by inserting these tessellated points
            assert(new_vert_count <= max_verts);

            if (old_other2 != new_other2 && other2_count)
                memmove(&verts[new_other2], &verts[old_other2], other2_count * sizeof(ogt_mesh_vec2i));
            if (old_other1 != new_other1 && other1_count)
                memmove(&verts[new_other1], &verts[old_other1], other1_count * sizeof(ogt_mesh_vec2i));
            if (tess_count_e0)
                memcpy(&verts[new_v0], new_tess_e0, tess_count_e0 * sizeof(ogt_mesh_vec2i));
            if (tess_count_e1)
                memcpy(&verts[new_v1], new_tess_e1, tess_count_e1 * sizeof(ogt_mesh_vec2i));
            if (tess_count_e2)
                memcpy(&verts[new_v2], new_tess_e2, tess_count_e2 * sizeof(ogt_mesh_vec2i));
            if (tess_count_e3)
                memcpy(&verts[new_v3], new_tess_e3, tess_count_e3 * sizeof(ogt_mesh_vec2i));
            // grow the polygon ring size
            vert_count = new_vert_count;

            // (3) set next edge to first tessellated edge of edge1.
            next_edge_index = new_v1;

            // (4) we've progressed, clear the no-progress counter
            no_progress_counter = 0;

        }
    }
    return vert_count;
}

void _polygon_meshify_voxels_in_face_direction(
    const uint8_t* voxels,
    const ogt_mesh_rgba* palette,
    int32_t size_x, int32_t size_y, int32_t size_z,                // how many voxels in each of X,Y,Z dimensions
    int32_t k_stride_x, int32_t k_stride_y, int32_t k_stride_z,    // the memory stride for each of those X,Y,Z dimensions within the voxel data.
    const ogt_mesh_transform& transform,                           // transform to convert from X,Y,Z to "objectSpace"
    ogt_mesh* mesh)
{
    // enable aggressive voxel optimization for now.
    uint32_t max_voxels_per_slice = size_x * size_y;
    assert(max_voxels_per_slice <= 65536);	// voxel_polygonized and slice_colors requires this.
    ogt_mesh_bitset_64k voxel_polygonized;
    uint8_t slice_colors[65536];

    // determine if the transform parity has flipped in a way that winding would have been switched.
    const ogt_mesh_vec3* side = _make_vec3_ptr(&transform.m00);
    const ogt_mesh_vec3* up   = _make_vec3_ptr(&transform.m10);
    const ogt_mesh_vec3* fwd  = _make_vec3_ptr(&transform.m20);
    bool is_parity_flipped    = _dot3(*fwd, _cross3(*side, *up)) < 0.0f;

    ogt_mesh_vec3 normal = _transform_vector(transform, _make_vec3(0.0f, 0.0f, 1.0f));

    for ( int32_t k = 0; k < (int32_t)size_z; k++ ) {
        bool is_last_slice = (k == (size_z-1)) ? true : false;

        // clear this slice
        voxel_polygonized.clear(max_voxels_per_slice);

        // first, fill this slice with all colors for the voxel grid but set to zero where the
        // slice has a non-empty voxel in the corresponding location in the k+1 slice.
        uint32_t num_non_empty_cells = 0;
        for (int32_t j = 0; j < size_y; j++) {
            for (int32_t i = 0; i < size_x; i++) {
                int32_t index_in_slice = i+(j*size_x);
                uint8_t cell_color = voxels[i*k_stride_x + j*k_stride_y + (k+0)*k_stride_z];

                // if the this cell on this slice is occluded by the corresponding cell on the next slice, we 
                // mark this polygon as voxelized already so it doesn't get included in any polygons for the current slice.
                // we also inherit the next slice's color to ensure the polygon flood fill inserts 
                // discontinuities where necessary in order to generate a water-tight tessellation
                // to the next slice.
                uint8_t next_cell_color = !is_last_slice ? voxels[i * k_stride_x + j * k_stride_y + (k + 1) * k_stride_z] : 0;
                if (next_cell_color != 0) {
                    cell_color = next_cell_color;
                    voxel_polygonized.set(index_in_slice);
                }
                slice_colors[index_in_slice] = cell_color;

                num_non_empty_cells += (cell_color != 0) ? 1 : 0;
            }
        }
        // skip to the next slice if the entire slice is empty.
        if (!num_non_empty_cells)
            continue;
    
        // polygonize all voxels for this slice.
        for (int32_t j = 0; j < (int32_t)size_y; j++) {
            for (int32_t i = 0; i < (int32_t)size_x; i++) {
                int32_t index_in_slice = i + j * size_x;
                // this voxel does not need to be polygonized on this slice? 
                // early out: empty-cell, we don't consider it.
                uint8_t color_index = slice_colors[index_in_slice];
                if ( color_index == 0 )
                    continue;

                // this voxel is already polygonized, skip it.
                if (voxel_polygonized.is_set(index_in_slice))
                    continue;

                // we always start polygon rasterization with any lower-left corner in (i,j) 
                // space and fill outward from there. So skip any coords that don't match this
                // criteria.
                //if ((i > 0 && slice_colors[index_in_slice-1] == color_index) || 
                //	(j > 0 && slice_colors[index_in_slice-size_x] == color_index))
                //	continue;

                const uint32_t MAX_VERTS = 4096;
                ogt_mesh_vec2i verts[MAX_VERTS];
                uint32_t vert_count = _construct_polygon_for_slice(verts, MAX_VERTS, i, j, size_x, size_y, slice_colors, voxel_polygonized);
                
                const ogt_mesh_rgba& color = palette[color_index];

                // generate the verts in the output mesh
                uint32_t base_vertex_index = mesh->vertex_count;
                for (uint32_t i = 0; i < vert_count; i++) {
                    mesh->vertices[mesh->vertex_count++] = _mesh_make_vertex(_transform_point(transform, _make_vec3((float)verts[i].x,   (float)verts[i].y,   (float)(k+1))), normal, color);
                }

                // generate the indices in the output mesh.
                uint32_t tessellated_index_count = _tessellate_polygon(&mesh->indices[mesh->index_count], verts, vert_count);

                // flip the winding of tessellated triangles to account for an inversion in the transform.
                if (is_parity_flipped) {
                    for (uint32_t i = 0; i < tessellated_index_count; i += 3) {
                        uint32_t i0 = mesh->indices[mesh->index_count + i + 0];
                        uint32_t i1 = mesh->indices[mesh->index_count + i + 1];
                        uint32_t i2 = mesh->indices[mesh->index_count + i + 2];
                        mesh->indices[mesh->index_count + i + 0] = base_vertex_index + i2;
                        mesh->indices[mesh->index_count + i + 1] = base_vertex_index + i1;
                        mesh->indices[mesh->index_count + i + 2] = base_vertex_index + i0;
                    }
                }
                else {
                    for (uint32_t i = 0; i < tessellated_index_count; i += 3) {
                        uint32_t i0 = mesh->indices[mesh->index_count + i + 0];
                        uint32_t i1 = mesh->indices[mesh->index_count + i + 1];
                        uint32_t i2 = mesh->indices[mesh->index_count + i + 2];
                        mesh->indices[mesh->index_count + i + 0] = base_vertex_index + i0;
                        mesh->indices[mesh->index_count + i + 1] = base_vertex_index + i1;
                        mesh->indices[mesh->index_count + i + 2] = base_vertex_index + i2;
                    }
                }

                mesh->index_count += tessellated_index_count;
            }
        }
    }

    #undef SLICE_INDEX
}

// for each slice
//   for each voxel cell
//     if not already polygonized
//        initialize polygon to a single cell
//        while (can expand polygon)
//          choose an edge and expand it out as far as possible, tessellating surrounding edges if neccessary, marking newly expanded cells as polygonized
//        triangulate the output polygon.
ogt_mesh* ogt_mesh_from_paletted_voxels_polygon(
    const ogt_voxel_meshify_context* ctx, 
    const uint8_t* voxels, uint32_t size_x, uint32_t size_y, uint32_t size_z, const ogt_mesh_rgba* palette) {
    uint32_t max_face_count   = _count_voxel_sized_faces( voxels, size_x, size_y, size_z );
    uint32_t max_vertex_count = max_face_count * 4;
    uint32_t max_index_count  = max_face_count * 6;

    uint32_t mesh_size = sizeof(ogt_mesh) + (max_vertex_count * sizeof(ogt_mesh_vertex)) + (max_index_count * sizeof(uint32_t));
    ogt_mesh* mesh = (ogt_mesh*)_voxel_meshify_malloc(ctx, mesh_size);
    if (!mesh)
        return NULL;

    mesh->vertices = (ogt_mesh_vertex*)&mesh[1];
    mesh->indices  = (uint32_t*)&mesh->vertices[max_vertex_count];
    mesh->vertex_count = 0;
    mesh->index_count  = 0;
    
    const int32_t k_stride_x = 1;
    const int32_t k_stride_y = size_x;
    const int32_t k_stride_z = size_x * size_y;
    
    // do the +y PASS
    {
        ogt_mesh_transform transform_pos_y = _make_transform(
            0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f);

        _polygon_meshify_voxels_in_face_direction(
            voxels, palette,
            size_z, size_x, size_y,
            k_stride_z, k_stride_x, k_stride_y,
            transform_pos_y,
            mesh);
    }
    
    // do the -y PASS
    {
        ogt_mesh_transform transform_neg_y = _make_transform(
            0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f,-1.0f, 0.0f, 0.0f,
            0.0f, (float)(size_y), 0.0f, 0.0f);

        _polygon_meshify_voxels_in_face_direction(
            voxels + (size_y - 1) * k_stride_y, 
            palette,
            size_z, size_x, size_y,
            k_stride_z, k_stride_x,-k_stride_y,
            transform_neg_y,
            mesh);
    }
    // do the +X pass
    {
        ogt_mesh_transform transform_pos_x = _make_transform(
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f);

        _polygon_meshify_voxels_in_face_direction(
            voxels, palette,
            size_y, size_z, size_x,
            k_stride_y, k_stride_z, k_stride_x,
            transform_pos_x,
            mesh);
    }
    // do the -X pass
    {
        ogt_mesh_transform transform_neg_x = _make_transform(
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
           -1.0f, 0.0f, 0.0f, 0.0f,
        (float)size_x, 0.0f, 0.0f, 0.0f);

        _polygon_meshify_voxels_in_face_direction(
            voxels + (size_x - 1) * k_stride_x,
            palette,
            size_y, size_z, size_x,
            k_stride_y, k_stride_z, -k_stride_x,
            transform_neg_x,
            mesh);
    }
    // do the +Z pass
    {
        ogt_mesh_transform transform_pos_z = _make_transform(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f);

        _polygon_meshify_voxels_in_face_direction(
            voxels, palette,
            size_x, size_y, size_z,
            k_stride_x, k_stride_y, k_stride_z,
            transform_pos_z,
            mesh);
    }
    // do the -Z pass
    {
        ogt_mesh_transform transform_neg_z = _make_transform(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, -1.0f, 0.0f,
            0.0f, 0.0f,(float)size_z, 0.0f);

        _polygon_meshify_voxels_in_face_direction(
            voxels + (size_z-1) * k_stride_z, 
            palette,
            size_x, size_y, size_z,
            k_stride_x, k_stride_y, -k_stride_z,
            transform_neg_z,
            mesh);
    }

    assert( mesh->vertex_count <= max_vertex_count);
    assert( mesh->index_count <= max_index_count);

    return mesh;
}


void ogt_mesh_destroy(const ogt_voxel_meshify_context* ctx, ogt_mesh* mesh )
{
    _voxel_meshify_free(ctx, mesh);
}

#endif // #ifdef OGT_VOXEL_MESHIFY_IMPLEMENTATION

/* -------------------------------------------------------------------------------------------------------------------------------------------------

    MIT License

    Copyright (c) 2020 Justin Paver

    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.

------------------------------------------------------------------------------------------------------------------------------------------------- */
