#include "c4d_model.h"
#include "file_parser.h"
#include "cheetah.h"
#include <malloc.h>
#include <cassert>

/*
    This is a reimplementation of parts of the SModel and SPModel classes which
    load .4d models and their textures
*/

extern cheetah* engine;


c_status_t c4d_model::load_from_file(char* file_name, float scale)
{
    bool is_older_file = false;

    file_parser model_file(file_name, true, false);

    model_file.validate_header();
    cheetah_file_kind_t kind;
    c4d_node* node = nullptr;

    kind = model_file.read_kind();
    if (kind != KIND_SCEN)
    {
        return C_FILE_ERROR;
    }

    uint32_t file_version = model_file.read4();

    if (file_version == KIND_FILE_v101)
    {
        is_older_file = false;
    }
    else if (file_version == KIND_FILE_v100)
    {
        is_older_file = true;
    }
    else
    {
        return C_FILE_ERROR;
    }

    while (!model_file.is_end_of_file())
    {
        kind = model_file.read_kind();
        bool done = true;

        switch(kind)
        {
            case KIND_SSQS:
                parse_ssqs(&model_file);
                break;

            case KIND_RCON:
                // not valid apparently
                return C_FILE_ERROR;

            case KIND_CROT:
            case KIND_CPOS:
            case KIND_CLNK:
            case KIND_FRM2:
                // not implemented yet
                return C_FILE_UNSUPPORTED;

            case KIND_FLYZ:
                _has_FLYZ = true;
                break;

            case KIND_AMBI:
                _ambi[0] = model_file.read_float();
                _ambi[1] = model_file.read_float();
                _ambi[2] = model_file.read_float();
                break;

            default:
                done = false;
                break;
        }

        if (done)
        {
			model_file.pop();
            continue;
        }

        

        if ((kind == KIND_MESH) || (kind == KIND_DUMY) || (kind == KIND_BSP_))
        {
            uint16_t node_index = _nodes.size();
            _nodes.push_back(new c4d_node);
            node = _nodes[node_index];

            node->_name_length = model_file.read_string(&node->_name);
            node->_parent_id = model_file.read4();

            printf("%s\n", _nodes[node_index]->_name);

            if (!is_older_file)
            {
                _nodes[node_index]->_old_a = (int32_t)model_file.read4();
                _nodes[node_index]->_old_b = (int32_t)model_file.read4();
            }
            else
            {
                _nodes[node_index]->_old_a = -1;
                _nodes[node_index]->_old_b = -1;
            }

            node->_transform.r11 = model_file.read_float();
            node->_transform.r12 = model_file.read_float();
            node->_transform.r13 = model_file.read_float();

            node->_transform.r21 = model_file.read_float();
            node->_transform.r22 = model_file.read_float();
            node->_transform.r23 = model_file.read_float();

            node->_transform.r31 = model_file.read_float();
            node->_transform.r32 = model_file.read_float();
            node->_transform.r33 = model_file.read_float();
            
            model_file.read_bytes_exactly((uint8_t*)&node->_position, sizeof(node->_position));

            if (_nodes[node_index]->_name[0] == ':')
            {
                // something special with fx
                printf("specialFx alert!\n");
            }
        }

        cheetah_file_kind_t version;
        c4d_material* material;
        uint8_t* buffer;
        uint16_t nodes;
        uint8_t format;

        
        switch(kind)
        {
            case KIND_MESH:
                _mesh_count++;
            case KIND_DUMY:
                break;

            case KIND_BBOX:
                model_file.read_bytes_exactly((uint8_t*)&node->_mesh._bbox, sizeof(node->_mesh._bbox));
                break;

            case KIND_BONS:
                node->_mesh._num_bones = model_file.read4();
                node->_mesh._bones = (cheetah_bone_t*)malloc(node->_mesh._num_bones * sizeof(cheetah_bone_t));
                for (uint32_t i = 0; i < node->_mesh._num_bones; i++)
                {
                    node->_mesh._bones[i].id = model_file.read4();
                    model_file.read_bytes_exactly((uint8_t*)&node->_mesh._bones[i].m1.x, 0x30);
                }

            case KIND_ANIM:
                break;

            case KIND_SKIN:
                break;

            case KIND_POLY:
                node->_collision_poly = new c4d_collision_convex_poly();
                if (node->_collision_poly->read_from_file(&model_file) != C_OK)
                {
                    return C_FILE_ERROR;
                }
                break;

            kind = model_file.read_kind();
                
            case KIND_BOND:
                for (int i = 0; i < 10; i++)
                {
                    model_file.read_float();
                }
                break;

            case KIND_BTRE:
                nodes = model_file.read4();
                format = model_file.read4();
                if (format != 0)
                {
                    return C_FILE_UNSUPPORTED;
                }
                model_file.pop();
                //model_file.read_bytes_exactly(buffer, nodes * 20);
                break;

            case KIND_BSP_:
                break;

            case KIND_CAM_:
                if (is_older_file)
                {
                    return C_FILE_ERROR;
                }
                break;

            case KIND_LITE:
                version = (cheetah_file_kind_t)model_file.read4();
                if (version != KIND_FILE_v100)
                {
                    return C_FILE_ERROR;
                }

                node->_light = new c4d_light();
                if (node->_light->read_from_file(&model_file) != C_OK)
                {
                    return C_FILE_ERROR;
                }
                break;

            case KIND_MTLS:
                node->_mesh._material_count = model_file.read4();

                for (int i = 0; i < node->_mesh._material_count; i++)
                {
                    kind = model_file.read_kind();
                    if ((kind == KIND_MATE) || (kind == KIND_STRP))
                    {
                        material = new c4d_material();
                        material->_is_strp = kind == KIND_STRP;
                        material->_num_faces = model_file.read4();
                        material->_vertex_start = model_file.read4();
                        material->_vertex_end = model_file.read4();

                        while (!model_file.peek())
                        {
                            char* texture_name = nullptr;
                            kind = model_file.read_kind();
                            model_file.read_string(&texture_name);

                            printf("parse TEX %s\n", texture_name);

                            if (kind == KIND_REFL)
                            {
                                material->set_texture(MATERIAL_REFL, engine->load_texture_from_dxt(texture_name, 1)); 
                            }
                            else if (kind == KIND_SPEC)
                            {
                                material->set_texture(MATERIAL_SPEC, engine->load_texture_from_dxt(texture_name, 1)); 
                            }
                            else if (kind == KIND_DIFF)
                            {
                                material->set_texture(MATERIAL_DIFF, engine->load_texture_from_dxt(texture_name, 1)); 
                            }
                            else if (kind == KIND_SILL)
                            {
                                material->set_texture(MATERIAL_SILL, engine->load_texture_from_dxt(texture_name, 1)); 
                            }
                            else if (kind == KIND_BUMP)
                            {
                                material->set_texture(MATERIAL_BUMP, engine->load_texture_from_dxt(texture_name, 1)); 
                            }
                            
                            if (texture_name != nullptr)
                            {
                                free(texture_name);
                            }
                            
                            model_file.pop();
                        }

                        node->_mesh._materials.push_back(material);
                    }
                    else
                    {
                        return C_FILE_ERROR;
                    }
                    model_file.pop();
                }
                break;

            case KIND_VERT:
                node->_mesh._num_vertices = model_file.read4();
                node->_mesh._vertex_format = model_file.read4();
                node->_mesh._vertices = (cheetah_vertex_t*)malloc(sizeof(cheetah_vertex_t) * node->_mesh._num_vertices);

                if (node->_mesh._vertex_format == 0) 
                {
                    // make with 0x112
                    model_file.read_bytes_exactly((uint8_t*)node->_mesh._vertices, sizeof(cheetah_vertex_t) * node->_mesh._num_vertices);
                    if (node->_mesh._num_vertices > 10)
                    {
                        for (int i = 0; i < 10; i++)
                        {
                            printf("%f, %f, %f\n", node->_mesh._vertices[i*3].vertices_pos.x,  node->_mesh._vertices[i*3].vertices_pos.y, node->_mesh._vertices[i*3].vertices_pos.z);
                        }
                    }
                    printf("read %u bytes of VERT data\n", sizeof(cheetah_vertex_t) * node->_mesh._num_vertices);
                }
                else if (node->_mesh._vertex_format == 1)
                {
                    // make with 0x2102
                    for (uint32_t i = 0; i < node->_mesh._num_vertices; i++)
                    {
                        // the original has this as numVerts * 56
                        model_file.read_bytes_exactly((uint8_t*)&node->_mesh._vertices[i], sizeof(cheetah_vertex_t));
                        node->_mesh.vertices_extra[i] = model_file.read_float();
                    }
                }
                else if (node->_mesh._vertex_format == 2)
                {
                    model_file.read4(); // seems to be thrown away
                    // make with 0x4112
                }
                else
                {
                    printf("unknown vertex format %u\n", node->_mesh._vertex_format);
                    return C_FILE_UNSUPPORTED;
                }
                break;

            case KIND_FACE:
                assert(node->_mesh._index_buffer_ref == -1);
                node->_mesh._num_faces = model_file.read4();
                buffer = (uint8_t*)malloc(node->_mesh._num_faces * 3);
                assert(buffer != nullptr);
                model_file.read_bytes_exactly(buffer, node->_mesh._num_faces * 3);
                node->_mesh._index_buffer_ref = engine->create_index_buffer(buffer, node->_mesh._num_faces * 3);
                free(buffer);
                break;

            case KIND_INDI:
                assert(node->_mesh._index_buffer_ref == -1);
                node->_mesh._num_indis = model_file.read4();
                buffer = (uint8_t*)malloc(node->_mesh._num_indis * 2);
                assert(buffer != nullptr);
                model_file.read_bytes_exactly(buffer, node->_mesh._num_indis * 2);
                node->_mesh._index_buffer_ref = engine->create_index_buffer(buffer, node->_mesh._num_indis);
                free(buffer);
                break;

            default:
                printf("someone forgot about %08X\n", kind);
                break;
        }
        if (model_file.peek())
        {
		    model_file.pop();
        }
    }

    printf("> file load completed OK\n");

    return C_OK;
}


void c4d_model::print_stats(uint8_t indent)
{
    printf("Animations %u\n", _anim_count);

    for (int i = 0; i < _nodes.size(); i++)
    {
        printf("Node %s\n", _nodes[i]->_name);
        _nodes[i]->_mesh.print_stats(indent + 1);
    }

}


void c4d_model::generate_meshes(void)
{
    printf("generate meshes...\n");
    uint16_t count = 0;
    for (int i = 0; i < _nodes.size(); i++)
    {
        if (_nodes[i]->_mesh._num_vertices)
        {
            _nodes[i]->_mesh.create_mesh_in_engine();
            count++;
        }
    }
    printf("created %u meshes\n", count);
}


void c4d_model::draw(void)
{
    for (int i = 0; i < _nodes.size(); i++)
    {
        if (_nodes[i]->_mesh._num_vertices)
        {
            _nodes[i]->_mesh.draw();
        }
    }
}


void c4d_model::parse_ssqs(file_parser* model_file)
{
    cheetah_file_kind_t kind;
    _anim_count += model_file->read4();
    uint16_t things_to_read = 0;
    char* string;

    for (int i = 0; i < _anim_count; i++)
    {
        kind = model_file->read_kind();

        if (kind == KIND_SSQE)
        {
            model_file->read_string(&string);
            printf("anim ref %s\n", string);
            model_file->read4();
            model_file->read_float();
            model_file->read4();
            model_file->read4();
            model_file->read_float();
            model_file->read_float();

            for (int i = 0; i < _mesh_count; i++)
            {
                kind = model_file->read_kind();

                if (kind == KIND_SCON)
                {
                    if (model_file->peek())
                    {

                    }
                    else
                    {
                        model_file->read_bytes_exactly(nullptr, 12);
                        model_file->read_bytes_exactly(nullptr, 16);
                        model_file->read_bytes_exactly(nullptr, 48);
                        while (!model_file->peek())
                        {
                            kind = model_file->read_kind();

                            switch (kind)
                            {
                                case KIND_CEUP:
                                    things_to_read = model_file->read4();
                                    model_file->read_float();
                                    model_file->read_float();
                                    model_file->read1();
                                    model_file->read1();
                                    model_file->read_bytes_exactly(nullptr, things_to_read * 12);
                                    break;
                            
                                case KIND_CEUL:
                                    while (!model_file->peek())
                                    {
                                        kind = model_file->read_kind();
                                        switch (kind)
                                        {
                                            case KIND_CZBX:
                                            case KIND_CZBY:
                                            case KIND_CZBZ:
                                                break;
                                        }
                                        things_to_read = model_file->read4();
                                        model_file->read1();
                                        model_file->read1();
                                        model_file->read_bytes_exactly(nullptr, things_to_read * 16);

                                        model_file->pop();
                                    }
                                    break;
                            
                                case KIND_CPSP:
                                    things_to_read = model_file->read4();
                                    model_file->read_float();
                                    model_file->read_float();
                                    model_file->read1();
                                    model_file->read1();
                                    model_file->read_bytes_exactly(nullptr, things_to_read * 12);
                                    break;

                                case KIND_CLNK:
                                    things_to_read = model_file->read4();
                                    for (int i = 0; i < things_to_read; i++)
                                    {
                                        model_file->read_float();
                                        model_file->read4();
                                        model_file->read_bytes_exactly(nullptr, 12);
                                        model_file->read_bytes_exactly(nullptr, 16);
                                        model_file->read_bytes_exactly(nullptr, 48);
                                    }
                                    break;

                                case KIND_VISI:
                                    things_to_read = model_file->read4();
                                    model_file->read1();
                                    model_file->read1();
                                    model_file->read_bytes_exactly(nullptr, things_to_read * 8);
                                    break;

                                case KIND_INHE:
                                    model_file->read4();
                                    break;

                                case KIND_CROT:
                                    things_to_read = model_file->read4();
                                    model_file->read1();
                                    model_file->read1();

                                    for (int i = 0; i < things_to_read; i++)
                                    {
                                        model_file->read_float();
                                        model_file->read_float();

                                        model_file->read_float();
                                        model_file->read_float();
                                        model_file->read_float();
                                        model_file->read_float();
                                        model_file->read_float();
                                    }
                                    break;


                                case KIND_CPOS:
                                    things_to_read = model_file->read4();
                                    model_file->read1();
                                    model_file->read1();

                                    for (int i = 0; i < things_to_read; i++)
                                    {
                                        model_file->read_float();

                                        model_file->read_float();       // vec3
                                        model_file->read_float();
                                        model_file->read_float();

                                        model_file->read_float();       // vec3
                                        model_file->read_float();
                                        model_file->read_float();

                                        model_file->read_float();       // vec3
                                        model_file->read_float();
                                        model_file->read_float();
                                    }
                                    break;
                            
                                case KIND_CFOV:
                                case KIND_CXYZ:
                                    break;
                            }
                            model_file->pop();
                        }
                    }
                    model_file->pop();
                }
            }
        }
        else if (kind == KIND_SREF)
        {
            model_file->read_string(&string);
            printf("SREF - %s\n", string);
        }
        model_file->pop();
    }
}