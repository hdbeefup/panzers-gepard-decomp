#ifndef _CHEETAH_H_
#define _CHEETAH_H_

#include "defines.h"
#include "file_parser.h"
#include <vector>
#include <d3d9.h>


typedef struct {
    char* file_name;
    uint8_t shader_id;
    uint8_t bit_depth;
    uint8_t field19;
    uint32_t width;
    uint32_t height;
    uint32_t format;
    uint8_t* raw_data_ptr;
    IDirect3DTexture9* texture_object;
} cheetah_texture_entry_t;


typedef enum {
    VTYPE_UNSET         = 0x0,
    VTYPE_UNK_152       = 0x152,    // maybe terrain
    VTYPE_UNK_042       = 0x042,    // also maybe terrain
    VTYPE_MESH_2102     = 0x2102,
    VTYPE_MESH_ANIMESH  = 0x112,
    VTYPE_MESH_SKINMESH = 0x4112,
} cheetah_vertex_type_t;


typedef struct 
{
    uint16_t a, vertex_descriptor_int;
    uint8_t b, c, d, e;
} cheetah_vertex_descriptor_t;


typedef struct
{
    uint32_t width, height, format;
    uint8_t* data;
} cheetah_bitmap_t;


class cheetah 
{
    public:
        cheetah(){};
        virtual int load_texture_from_dxt(char* file_path, uint8_t mip_maps){ return 0; };
        virtual int load_texture_from_data(file_parser* file) { return 0; };
        virtual int create_index_buffer(uint8_t* buffer, uint32_t length){ return 0; };
        virtual void render_frame(){};
        virtual void draw_mesh(uint32_t index_buffer, int32_t declaration_id, uint32_t vertex_buffer, uint32_t fvf, uint32_t vertex_descriptor_int, uint32_t vertex_count, uint32_t face_count){};
        virtual int create_vertex_declaration(cheetah_vertex_type_t vertex_type, uint32_t* vertex_descriptor_int, uint32_t* fvf){ return 0; };
        virtual uint32_t create_vertex_buffer(uint32_t length, uint32_t fvf, uint8_t* vertex_data) { return 0; };
        virtual uint32_t get_texture_shader(int32_t texture_index){ return 0; };

        LPDIRECT3D9        _d3d_handle;
        LPDIRECT3DDEVICE9  _d3d_device;
};


class cheetah_d3d : public cheetah 
{

    public:
        cheetah_d3d(HWND hWnd);
        void render_frame();
        ~cheetah_d3d();

        int load_texture_from_dxt(char* file_path, uint8_t mip_maps);
        int load_texture_from_data(file_parser* file);
        uint32_t get_texture_shader(int32_t texture_index);
        void init_pixel_format(cheetah_texture_entry_t* texture);

        void draw_mesh(uint32_t index_buffer, int32_t declaration_id, uint32_t vertex_buffer, uint32_t fvf, uint32_t vertex_descriptor_int, uint32_t vertex_count, uint32_t face_count);
        
        int create_index_buffer(uint8_t* buffer, uint32_t length);
        int create_vertex_declaration(cheetah_vertex_type_t vertex_type, uint32_t* vertex_descriptor_int, uint32_t* fvf);
        uint32_t create_vertex_buffer(uint32_t length, uint32_t fvf, uint8_t* vertex_data);
        
        

    //private:
        LPDIRECT3D9        _d3d_handle;
        LPDIRECT3DDEVICE9  _d3d_device;


        uint32_t            _vertex_buffer_count;
        uint32_t            vertex_memory_occupation;
        
        std::vector<cheetah_texture_entry_t*> _texture_bank;

        std::vector<IDirect3DIndexBuffer9*>  _index_buffers;
        std::vector<IDirect3DVertexDeclaration9*> _vertex_declarations;
        std::vector<IDirect3DVertexBuffer9*> _vertex_buffers;

        uint32_t convert_format_dxt_to_d3d(uint32_t game_format);
        uint32_t convert_format_d3d_to_dxt(uint32_t file_format);

        uint32_t opaqueTexType;
        uint32_t alpha1BitTexType;
        uint32_t fullAlphaTexType;

};

#endif
