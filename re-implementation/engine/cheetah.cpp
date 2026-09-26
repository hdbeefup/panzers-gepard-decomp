#include "cheetah.h"
#include "file_parser.h"
#include <dxerr.h>

/*
    This is a partial and growing re-implementation of the "SGepard" class from the original engine
    This is primarily used to provide a basis for testing the various D3D calls as they are decompiled

*/

cheetah_d3d::cheetah_d3d(HWND hWnd)
{
    _d3d_handle = Direct3DCreate9(D3D_SDK_VERSION);    // create the Direct3D interface

    D3DPRESENT_PARAMETERS d3dpp;    // create a struct to hold various device information

    ZeroMemory(&d3dpp, sizeof(d3dpp));    // clear out the struct for use
    d3dpp.Windowed = TRUE;    // program windowed, not fullscreen
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;    // discard old frames
    d3dpp.hDeviceWindow = hWnd;    // set the window to be used by Direct3D
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferWidth = 800;
    d3dpp.BackBufferHeight = 600;
    d3dpp.EnableAutoDepthStencil = true;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;


    // create a device class using this information and the info from the d3dpp stuct
    HRESULT ok = _d3d_handle->CreateDevice(D3DADAPTER_DEFAULT,
                      D3DDEVTYPE_HAL,
                      hWnd,
                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                      &d3dpp,
                      &_d3d_device);
    if (ok < 0)
    {
        printf("error creating d3d device\n");
        exit(1);
    }

    D3DFORMAT adapter_format = D3DFMT_R5G6B5;

    opaqueTexType = 0;
    alpha1BitTexType = 0;
    fullAlphaTexType = 0;

    HRESULT a = _d3d_handle->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, adapter_format, 0, D3DRTYPE_TEXTURE, D3DFMT_DXT1);
    if (a == 0)
    {
        opaqueTexType = 0x18;
        alpha1BitTexType = 0x18;
        fullAlphaTexType = 0x1c;
    }
    else
    {
        HRESULT b = _d3d_handle->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, adapter_format, 0, D3DRTYPE_TEXTURE, D3DFMT_R5G6B5);
        if (b == 0)
        {
            opaqueTexType = 4;
        }
        b = _d3d_handle->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, adapter_format, 0, D3DRTYPE_TEXTURE, D3DFMT_A1R5G5B5);
        if (b == 0)
        {
            alpha1BitTexType = 6;
        }
        b = _d3d_handle->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, adapter_format, 0, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8);
        if (b == 0)
        {
            fullAlphaTexType = 2;
        }
        else
        {
            b = _d3d_handle->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, adapter_format, 0, D3DRTYPE_TEXTURE, D3DFMT_A4R4G4B4);
            if (b == 0)
            {
                fullAlphaTexType = 7;
            }
        }
    }

    printf("texture types are %u / %u / %u\n", opaqueTexType, alpha1BitTexType, fullAlphaTexType);
}


uint32_t cheetah_d3d::get_texture_shader(int32_t texture_index)
{
    return _texture_bank[texture_index]->shader_id;
}


int cheetah_d3d::load_texture_from_data(file_parser* file)
{
    cheetah_texture_entry_t* object = (cheetah_texture_entry_t*)malloc(sizeof(cheetah_texture_entry_t));

    object->width = file->read4();
    object->height = file->read4();
    object->format = file->read4();
    init_pixel_format(object);

    uint32_t data_size = 0;
    if (object->field19)
    {
        data_size = (object->width + 3 >> 2) * (object->height + 3 >> 2) * object->bit_depth;
    }
    else
    {
        data_size = object->bit_depth * object->width * object->height;
    }

    object->raw_data_ptr = (uint8_t*)malloc(data_size);
    file->read_bytes_exactly(object->raw_data_ptr, data_size);
    int reference = _texture_bank.size();
    _texture_bank.push_back(object);
    return reference;
}


int cheetah_d3d::load_texture_from_dxt(char* file_path, uint8_t mip_maps)
{
    char qualified_path[256];
    D3DLOCKED_RECT rect;
    D3DSURFACE_DESC surfac_desc;
    sprintf(qualified_path, "../data/units/train/%s", file_path);

    for (uint32_t i = 0; i < _texture_bank.size(); i++)
    {
        if (strcmp(qualified_path, _texture_bank[i]->file_name) == 0)
        {
            printf("cheetah_tex: already loaded %s\n", qualified_path);
            return i;
        }
    }

    file_parser file(qualified_path, true, false);

    cheetah_texture_entry_t* object = (cheetah_texture_entry_t*)malloc(sizeof(cheetah_texture_entry_t));
    object->file_name = (char*)malloc(strlen(qualified_path));
    memcpy(object->file_name, qualified_path, strlen(qualified_path));

    file.validate_header();
    cheetah_file_kind_t kind = file.read_kind();
    
    if (kind != KIND_TEXT)
    {
        return -1;
    }

    object->shader_id = file.read4();
    object->width = file.read4();
    object->height = file.read4();
    object->format = file.read4();
    uint32_t format;

    if (object->shader_id == 2)
    {
        // uses full transparency
        printf("full alpha texture\n");
        format = fullAlphaTexType;
    }
    else if (object->shader_id == 1)
    {
        // 1 bit alpha type
        printf("1 bit alpha texture\n");
        format = alpha1BitTexType;
    }
    else if (object->shader_id == 0)
    {
        printf("opaque texture\n");
        format = opaqueTexType;
    }
    else
    {
        printf("unknown texture style\n");
        exit(1);
    }

    HRESULT result = _d3d_device->CreateTexture(object->width, object->height, 1, 0, (D3DFORMAT)(format), D3DPOOL_MANAGED, &object->texture_object, 0);
    printf("Create %d\n", result);
    printf("%s\n", DXGetErrorStringA(result));
    result = object->texture_object->LockRect(0, &rect, NULL, 0);
    printf("Lock %d\n", result);
    result = object->texture_object->GetLevelDesc(0, &surfac_desc);
    printf("GetLevelDesc %d\n", result);
    
    object->format = convert_format_d3d_to_dxt(surfac_desc.Format);
    init_pixel_format(object);

    printf("read %u x %u with %u bpp (f19 = %u, fmt = %u, fmtfileConv = %u)\n", object->width, object->height, object->bit_depth, object->field19, format, object->format);

    if (format == object->format)
    {
        uint32_t memory_size = object->bit_depth * object->height * object->width;
        if (object->field19)
        {
            memory_size = (object->height + 3 >> 2) * (object->width + 3 >> 2) * object->bit_depth;
        }
        file.read_bytes_exactly((uint8_t*)&rect.pBits, memory_size);
    }
    else
    {
        printf("unimpl\n");
    }

    object->texture_object->UnlockRect(0);

    int reference = _texture_bank.size();
    _texture_bank.push_back(object);

    printf("cheetah_tex: added %s\n", qualified_path);

    return reference;
}


void cheetah_d3d::draw_mesh(uint32_t index_buffer, int32_t declaration_id, uint32_t vertex_buffer, uint32_t fvf, uint32_t vertex_descriptor_int, uint32_t vertex_count, uint32_t face_count)
{
    printf("drawcall %u %u %u\n", vertex_descriptor_int, vertex_count, face_count);
    _d3d_device->SetIndices(_index_buffers[index_buffer]);
    _d3d_device->SetStreamSource(0, _vertex_buffers[vertex_buffer], 0, vertex_descriptor_int);
    _d3d_device->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1);
    _d3d_device->SetRenderState(D3DRS_LIGHTING, false);

    //_d3d_device->DrawPrimitive(D3DPT_POINTLIST, 0, vertex_count);
    _d3d_device->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, 0, 0, vertex_count, 0, face_count);
    return;

    // -- 
    if (declaration_id < 0)
    {
        _d3d_device->SetFVF(fvf);
    }
    else
    {
        _d3d_device->SetVertexDeclaration(_vertex_declarations[declaration_id]);
    }
}


int cheetah_d3d::create_index_buffer(uint8_t* buffer, uint32_t length)
{
    IDirect3DIndexBuffer9* index_buffer;
    void* buffer_data_ptr;

    HRESULT ok = _d3d_device->CreateIndexBuffer(length * 2, 8, D3DFMT_INDEX16, D3DPOOL_MANAGED, &index_buffer, 0);
    if (ok != D3D_OK)
    {
        printf("cib0 %s\n", DXGetErrorStringA(ok));
        exit(1);
    }
    ok = index_buffer->Lock(0, 0, &buffer_data_ptr, 0);
    if (ok != D3D_OK)
    {
        printf("cib1 %s\n", DXGetErrorStringA(ok));
        exit(1);
    }
    memcpy(buffer_data_ptr, buffer, length * 2);
    index_buffer->Unlock();

    int size = _index_buffers.size();
    _index_buffers.push_back(index_buffer);
    return size;
}


uint32_t cheetah_d3d::create_vertex_buffer(uint32_t length, uint32_t fvf, uint8_t* vertex_data)
{
    IDirect3DVertexBuffer9* vertex_buffer;

    printf("cvb %u, fvf %u\n", length, fvf);

    HRESULT ok = _d3d_device->CreateVertexBuffer(length, 8, D3DFVF_XYZ, D3DPOOL_MANAGED, &vertex_buffer, 0);
    if (ok != D3D_OK)
    {
        printf("cvb1 %s\n", DXGetErrorStringA(ok));
        exit(1);
    }

    void* buffer;
    ok = vertex_buffer->Lock(0, 0, &buffer, 0);
    if (ok != D3D_OK)
    {
        printf("cvb2 %s\n", DXGetErrorStringA(ok));
        exit(1);
    }

    memcpy(buffer, vertex_data, length);

    vertex_buffer->Unlock();

    vertex_memory_occupation += length;

    int size = _vertex_buffers.size();
    _vertex_buffers.push_back(vertex_buffer);
    return size;
}


void cheetah_d3d::render_frame()
{
    _d3d_device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 40, 100), 1.0f, 0);

    _d3d_device->BeginScene();    // begins the 3D scene

    // do 3D rendering on the back buffer here

    _d3d_device->EndScene();    // ends the 3D scene

    _d3d_device->Present(NULL, NULL, NULL, NULL);   // displays the created frame on the screen
}


cheetah_d3d::~cheetah_d3d()
{   
    _d3d_device->Release();    // close and release the 3D device
    _d3d_handle->Release();    // close and release Direct3D
}


uint32_t cheetah_d3d::convert_format_d3d_to_dxt(uint32_t file_format)
{
    if (file_format < 0x47)
    {
        if (file_format == 0x46)
        {
            return 0x1d;
        }

        switch(file_format) {
            case 0x14:
                return 1;
            case 0x15:
                return 2;
            case 0x16:
                return 3;
            case 0x17:
                return 4;
            case 0x18:
                return 5;
            case 0x19:
                return 6;
            case 0x1a:
                return 7;
            case 0x1b:
                return 8;
            case 0x1c:
                return 9;
            case 0x1d:
                return 10;
            case 0x1e:
                return 0xb;
            case 0x28:
                return 0xc;
            case 0x29:
                return 0xd;
            case 0x32:
                return 0xe;
            case 0x33:
                return 0xf;
            case 0x34:
                return 0x10;
            case 0x3c:
                return 0x11;
            case 0x3d:
                return 0x12;
            case 0x3e:
                return 0x13;
            case 0x3f:
                return 0x14;
            case 0x40:
                return 0x15;
        }
    }
    else if (file_format < 0x31545845) {
    if (file_format == 0x31545844) {
        return 0x18;
    }
    switch(file_format) {
    case 0x6f:
        return 0x20;
    case 0x70:
        return 0x21;
    case 0x71:
        return 0x22;
    case 0x72:
        return 0x23;
    case 0x73:
        return 0x24;
    case 0x74:
        return 0x25;
    }
    }
    else if (file_format < 0x34545845) {
    if (file_format == 0x34545844) {
        return 0x1b;
    }
    if (file_format == 0x32545844) {
        return 0x19;
    }
    if (file_format == 0x32595559) {
        return 0x17;
    }
    if (file_format == 0x33545844) {
        return 0x1a;
    }
    }
    else {
    if (file_format == 0x35545844) {
        return 0x1c;
    }
    if (file_format == 0x59565955) {
        return 0x16;
    }
    }
    printf("unknown d3d->dxt format %08X\n", file_format);
    assert(false);
}


uint32_t cheetah_d3d::convert_format_dxt_to_d3d(uint32_t game_format)
{
    switch(game_format) {
        case 1:
            return 0x14;
        case 2:
            return 0x15;
        case 3:
            return 0x16;
        case 4:
            return 0x17;
        case 5:
            return 0x18;
        case 6:
            return 0x19;
        case 7:
            return 0x1a;
        case 8:
            return 0x1b;
        case 9:
            return 0x1c;
        case 10:
            return 0x1d;
        case 0xb:
            return 0x1e;
        case 0xc:
            return 0x28;
        case 0xd:
            return 0x29;
        case 0xe:
            return 0x32;
        case 0xf:
            return 0x33;
        case 0x10:
            return 0x34;
        case 0x11:
            return 0x3c;
        case 0x12:
            return 0x3d;
        case 0x13:
            return 0x3e;
        case 0x14:
            return 0x3f;
        case 0x15:
            return 0x40;
        case 0x16:
            return 0x59565955;
        case 0x17:
            return 0x32595559;
        case 0x18:
            return 0x31545844;
        case 0x19:
            return 0x32545844;
        case 0x1a:
            return 0x33545844;
        case 0x1b:
            return 0x34545844;
        case 0x1c:
            return 0x35545844;
        case 0x1d:
            return 0x46;
        default:
            printf("invalid game texture format! %08X\n", game_format);
            assert(false);
        case 0x20:
            return 0x6f;
        case 0x21:
            return 0x70;
        case 0x22:
            return 0x71;
        case 0x23:
            return 0x72;
        case 0x24:
            return 0x73;
        case 0x25:
            return 0x74;
    }
}


int cheetah_d3d::create_vertex_declaration(cheetah_vertex_type_t vertex_type, uint32_t* vertex_descriptor_int, uint32_t* fvf)
{
    IDirect3DVertexDeclaration9* dec;

    uint32_t d3d_fvf = 0;
    uint32_t next_inst = 0;
    uint16_t descriptor_int = 0;
    bool vtx_condition = false;     // unknown

    cheetah_vertex_type_t unk_4v = VTYPE_UNSET;
    cheetah_vertex_descriptor_t vertex_descriptor[16];

    if ((vertex_type & 2) == 0)
    {
        printf("invalid vertex type no xyz\n");
    }

    d3d_fvf = 2;
    vertex_descriptor[next_inst].a = 0;
    vertex_descriptor[next_inst].vertex_descriptor_int = descriptor_int;
    vertex_descriptor[next_inst].b = 2;
    vertex_descriptor[next_inst].c = 0;
    vertex_descriptor[next_inst].d = 0;
    vertex_descriptor[next_inst].e = 0;

    descriptor_int += 0xc;
    unk_4v = (cheetah_vertex_type_t)(vertex_type & 0xfffffffd);
    
    if ((vertex_type & 0x10) != 0) {
        d3d_fvf = d3d_fvf | 0x10;

        next_inst++;
        vertex_descriptor[next_inst].a = 0;
        vertex_descriptor[next_inst].vertex_descriptor_int = descriptor_int;
        vertex_descriptor[next_inst].b = 2;
        vertex_descriptor[next_inst].c = 0;
        vertex_descriptor[next_inst].d = 3;
        vertex_descriptor[next_inst].e = 0;
        descriptor_int += 0xc;

        unk_4v = (cheetah_vertex_type_t)(vertex_type & 0xffffffed);
    }
    
    vertex_type = unk_4v;
    vtx_condition = (vertex_type & 0x2000) != 0;

    if (vtx_condition)
    {
        vertex_type = (cheetah_vertex_type_t)(vertex_type & 0xffffdfff);

        next_inst++;
        vertex_descriptor[next_inst].a = 0;
        vertex_descriptor[next_inst].vertex_descriptor_int = descriptor_int;
        vertex_descriptor[next_inst].b = 2;
        vertex_descriptor[next_inst].c = 0;
        vertex_descriptor[next_inst].d = 6;
        vertex_descriptor[next_inst].e = 0;
        descriptor_int += 0xc;

        next_inst++;
        vertex_descriptor[next_inst].a = 0;
        vertex_descriptor[next_inst].vertex_descriptor_int = descriptor_int;
        vertex_descriptor[next_inst].b = 2;
        vertex_descriptor[next_inst].c = 0;
        vertex_descriptor[next_inst].d = 6;
        vertex_descriptor[next_inst].e = 1;
        descriptor_int += 0xc;

        next_inst++;
        vertex_descriptor[next_inst].a = 0;
        vertex_descriptor[next_inst].vertex_descriptor_int = descriptor_int;
        vertex_descriptor[next_inst].b = 2;
        vertex_descriptor[next_inst].c = 0;
        vertex_descriptor[next_inst].d = 6;
        vertex_descriptor[next_inst].e = 2;

        descriptor_int += 0xc;
    }

    vtx_condition = !vtx_condition;

    if ((vertex_type & 0x40) != 0)
    {
        vertex_type = (cheetah_vertex_type_t)(vertex_type & 0xffffffbf);
        d3d_fvf = d3d_fvf | 0x40;
        
        next_inst++;
        vertex_descriptor[next_inst].a = 0;
        vertex_descriptor[next_inst].vertex_descriptor_int = descriptor_int;
        vertex_descriptor[next_inst].b = 4;
        vertex_descriptor[next_inst].c = 0;
        vertex_descriptor[next_inst].d = 10;
        vertex_descriptor[next_inst].e = 0;
        descriptor_int += 4;
    }

    if ((char)vertex_type < 0) 
    {
        vertex_type = (cheetah_vertex_type_t)(vertex_type & 0xffffff7f);
        d3d_fvf = d3d_fvf | 0x80;

        next_inst++;
        vertex_descriptor[next_inst].a = 0;
        vertex_descriptor[next_inst].vertex_descriptor_int = descriptor_int;
        vertex_descriptor[next_inst].b = 4;
        vertex_descriptor[next_inst].c = 0;
        vertex_descriptor[next_inst].d = 10;
        vertex_descriptor[next_inst].e = 1;
        descriptor_int += 4;
    }

    if ((vertex_type & 0xf00) == 0x100) 
    {
        vertex_type = (cheetah_vertex_type_t)(vertex_type & 0xfffff0ff);
        d3d_fvf = d3d_fvf | 0x100;

        next_inst++;
        vertex_descriptor[next_inst].a = 0;
        vertex_descriptor[next_inst].vertex_descriptor_int = descriptor_int;
        vertex_descriptor[next_inst].b = 1;
        vertex_descriptor[next_inst].c = 0;
        vertex_descriptor[next_inst].d = 5;
        vertex_descriptor[next_inst].e = 0;
        descriptor_int += 8;
    }

    if ((vertex_type & 0xf00) == 0x200) 
    {
        vertex_type = (cheetah_vertex_type_t)(vertex_type & 0xfffff0ff);
        d3d_fvf = d3d_fvf | 0x200;

        next_inst++;
        vertex_descriptor[next_inst].a = 0;
        vertex_descriptor[next_inst].vertex_descriptor_int = descriptor_int;
        vertex_descriptor[next_inst].b = 1;
        vertex_descriptor[next_inst].c = 0;
        vertex_descriptor[next_inst].d = 5;
        vertex_descriptor[next_inst].e = 0;
        descriptor_int += 8;

        next_inst++;
        vertex_descriptor[next_inst].a = 0;
        vertex_descriptor[next_inst].vertex_descriptor_int = descriptor_int;
        vertex_descriptor[next_inst].b = 1;
        vertex_descriptor[next_inst].c = 0;
        vertex_descriptor[next_inst].d = 5;
        vertex_descriptor[next_inst].e = 1;
        descriptor_int += 8;
    }

    if ((vertex_type & 0xf00) == 0x300) 
    {
        vertex_type = (cheetah_vertex_type_t)(vertex_type & 0xfffff0ff);
        d3d_fvf = d3d_fvf | 0x300;
        
        next_inst++;
        vertex_descriptor[next_inst].a = 0;
        vertex_descriptor[next_inst].vertex_descriptor_int = descriptor_int;
        vertex_descriptor[next_inst].b = 1;
        vertex_descriptor[next_inst].c = 0;
        vertex_descriptor[next_inst].d = 5;
        vertex_descriptor[next_inst].e = 0;
        descriptor_int += 8;

        next_inst++;
        vertex_descriptor[next_inst].a = 0;
        vertex_descriptor[next_inst].vertex_descriptor_int = descriptor_int;
        vertex_descriptor[next_inst].b = 1;
        vertex_descriptor[next_inst].c = 0;
        vertex_descriptor[next_inst].d = 5;
        vertex_descriptor[next_inst].e = 1;
        descriptor_int += 8;

        next_inst++;
        vertex_descriptor[next_inst].a = 0;
        vertex_descriptor[next_inst].vertex_descriptor_int = descriptor_int;
        vertex_descriptor[next_inst].b = 1;
        vertex_descriptor[next_inst].c = 0;
        vertex_descriptor[next_inst].d = 5;
        vertex_descriptor[next_inst].e = 2;
        descriptor_int += 8;
    }

    if ((vertex_type & 0x4000) != 0) 
    {
        vertex_type = (cheetah_vertex_type_t)(vertex_type & 0xffffbfff);
        vtx_condition = false;
        
        next_inst++;
        vertex_descriptor[next_inst].a = 0;
        vertex_descriptor[next_inst].vertex_descriptor_int = descriptor_int;
        vertex_descriptor[next_inst].b = 4;
        vertex_descriptor[next_inst].c = 0;
        vertex_descriptor[next_inst].d = 2;
        vertex_descriptor[next_inst].e = 0;
        descriptor_int += 4;

        next_inst++;
        vertex_descriptor[next_inst].a = 0;
        vertex_descriptor[next_inst].vertex_descriptor_int = descriptor_int;
        vertex_descriptor[next_inst].b = 3;
        vertex_descriptor[next_inst].c = 0;
        vertex_descriptor[next_inst].d = 1;
        vertex_descriptor[next_inst].e = 0;
        descriptor_int += 0x10;
    }

    if (vertex_type != 0) 
    {
        printf("invalid vertex type, remaining bits are %08X\n", vertex_type);
    }

    *vertex_descriptor_int = descriptor_int;
    *fvf = d3d_fvf;

    if (!vtx_condition) 
    {
        next_inst++;
        vertex_descriptor[next_inst].a = 0xff;
        vertex_descriptor[next_inst].b = 0x11;

        printf("vtx %d fvf %u\n", descriptor_int, fvf);

        *fvf = 0;
        HRESULT ok = _d3d_device->CreateVertexDeclaration((D3DVERTEXELEMENT9*)&vertex_descriptor, &dec);
        if (ok != D3D_OK)
        {
            printf("cvd %s\n", DXGetErrorStringA(ok));
            exit(1);
        }

        int size = _vertex_declarations.size();
        _vertex_declarations.push_back(dec);
        return size;
    }
    else
    {
        printf("no vertex declaration to make\n");
        return -1;
    }
}


void cheetah_d3d::init_pixel_format(cheetah_texture_entry_t* texture)
{
    switch(texture->format) {
    case 1:
        texture->field19 = 0;
        texture->bit_depth = 3;
        return;
    case 2:
    case 3:
    case 0x21:
    case 0x23:
        texture->field19 = 0;
        texture->bit_depth = 4;
        return;
    case 4:
    case 5:
    case 6:
    case 7:
    case 10:
    case 0xb:
    case 0xf:
    case 0x1d:
    case 0x20:
        texture->field19 = 0;
        texture->bit_depth = 2;
        return;
    case 8:
    case 9:
    case 0xe:
    case 0x10:
        texture->field19 = 0;
        texture->bit_depth = 1;
        return;
    default:
        printf("unknown pixel format %08X\n", texture->format);
        assert(false);
    case 0x18:
        texture->field19 = 1;
        texture->bit_depth = 8;
        return;
    case 0x19:
    case 0x1a:
    case 0x1b:
    case 0x1c:
        texture->field19 = 1;
        texture->bit_depth = 0x10;
        return;
    case 0x22:
    case 0x24:
        texture->field19 = 0;
        texture->bit_depth = 8;
        return;
    case 0x25:
        texture->field19 = 0;
        texture->bit_depth = 0x10;
        return;
    }
}