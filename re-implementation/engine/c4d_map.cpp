#include "c4d_map.h"
#include "file_parser.h"
#include "string.h"
#include <cassert>

/*
    This is a reimplementation of parts of the "STerrain" and "SWorld" classes and some smaller objects (SRoad, SJunction etc)
    which provides a way to load the entirety of a .map file into a series of data structures.
    This is a toolbox to validate assumptions about data and provide a way to browse through it.

*/

extern cheetah* engine;

c4d_map::c4d_map()
{

}


c_status_t c4d_map::load_from_file(char* file_name)
{
    bool is_older_file = false;

    file_parser map_file(file_name, true, false);

    map_file.validate_header();
    cheetah_file_kind_t kind;
    uint32_t count, block_sig;

    kind = map_file.read_kind();
    if (kind != KIND_MAPF)
    {
        return C_FILE_ERROR;
    }

    uint32_t file_version = map_file.read4();

    if (file_version == KIND_FILE_v201)
    {
        // phase1
    }
    else if (file_version == KIND_FILE_0004)
    {
        // phase2
    }
    else
    {
        return C_FILE_ERROR;
    }

    while (!map_file.is_end_of_file())
    {
        kind = map_file.read_kind();

        switch(kind)
        {
            case KIND_CAM:
                _camera_data = (uint8_t*)malloc(20);
                map_file.read_bytes_exactly(_camera_data, 20);
                break;

            case KIND_TRIG:
                // load_triggers(&model_file);
                break;

            case KIND_WIR3:
                // load_wires(&model_file);
                break;

            case KIND_MINA:
                map_file.read_string(&_mina);
                break;

            case KIND_ATMS:
                map_file.read_string(&_atmosphere);
                break;

            case KIND_KSYB:
                map_file.read_string(&_skybox);
                break;

            case KIND_TERR:
                load_terrain(&map_file);
                break;

            case KIND_LITE:
                count = map_file.read4();
                _weather_data = (uint8_t*)malloc(count * 4);
                map_file.read_bytes_exactly(_weather_data, count * 4);
                break;

            case KIND_ENTS:
                load_entities(&map_file);
                break;

            case KIND_LOCS:
                count = map_file.read4();
                map_file.read4();
                map_file.read4();
                _locations = (c4d_location*)malloc(count * sizeof(c4d_location));
                for (int i = 0; i < count; i++)
                {
                    block_sig = map_file.read4();
                    while (block_sig != KIND_BLOCK_HAS_NEXT)
                    {
                        block_sig = map_file.read4();
                        i++;
                    }

                    _locations[i].x1 = map_file.read4();
                    _locations[i].y1 = map_file.read4();
                    _locations[i].x2 = map_file.read4();
                    _locations[i].y2 = map_file.read4();
                    map_file.read_string(&_locations[i].name);
                    _locations[i].a = map_file.read4();
                }
                break;

            case KIND_PATH:
                count = map_file.read4();
                map_file.read4();
                map_file.read4();
                _paths = (c4d_path*)malloc(count * sizeof(c4d_path));
                for (int i = 0; i < count; i++)
                {
                    block_sig = map_file.read4();
                    while (block_sig != KIND_BLOCK_HAS_NEXT)
                    {
                        block_sig = map_file.read4();
                        i++;
                    }

                    map_file.read_string(&_paths[i].name);
                    _paths[i].a = map_file.read4();
                    _paths[i].b = map_file.read1();
                    _paths[i].node_count = map_file.read4();
                    _paths[i].nodes = (cheetah_vec2_t*)malloc(sizeof(cheetah_vec2_t) * _paths[i].node_count);
                    for (int j = 0; j < _paths[i].node_count; j++)
                    {
                        _paths[i].nodes[j].x = map_file.read_float();
                        _paths[i].nodes[j].y = map_file.read_float();
                    }
                    map_file.pop();
                }
                break;

            case KIND_MINI:
                engine->load_texture_from_data(&map_file);
                break;

            case KIND_ROD2:
                count = map_file.read4();
                map_file.read4();
                map_file.read4();
                _roads = (c4d_road*)malloc(count * sizeof(c4d_road));

                for (int i = 0; i < count; i++)
                {
                    block_sig = map_file.read4();
                    while (block_sig != KIND_BLOCK_HAS_NEXT)
                    {
                        if (block_sig == KIND_BLOCK_END)
                        {
                            break;
                        }
                        block_sig = map_file.read4();
                        i++;
                    }
                    if (block_sig == KIND_BLOCK_END)
                    {
                        break;
                    }

                    map_file.read_string(&_roads[i].name);
                    _roads[i].tesselation_distance = map_file.read_float();
                    _roads[i].b = map_file.read_float();
                    _roads[i].mirror_flags = map_file.read4();
                    _roads[i].node_count = map_file.read4();
                    _roads[i].nodes = (c4d_road_node*)malloc(sizeof(c4d_road_node) * _roads[i].node_count);

                    for (int j = 0; j < _roads[i].node_count; j++)
                    {
                        _roads[i].nodes[j].x = map_file.read_float();
                        _roads[i].nodes[j].y = map_file.read_float();
                        _roads[i].nodes[j].r1 = map_file.read_float();
                        _roads[i].nodes[j].r2 = map_file.read_float();
                        _roads[i].nodes[j].a = map_file.read_float();
                        _roads[i].nodes[j].jcn = map_file.read4();
                    }
                }
                break;

            case KIND_RODJ:
                count = map_file.read4();
                map_file.read4();
                map_file.read4();
                _junctions = (c4d_road_junction*)malloc(count * sizeof(c4d_road_junction));

                for (int i = 0; i < count; i++)
                {
                    block_sig = map_file.read4();
                    while (block_sig != KIND_BLOCK_HAS_NEXT)
                    {
                        if (block_sig == KIND_BLOCK_END)
                        {
                            break;
                        }
                        block_sig = map_file.read4();
                        i++;
                    }
                    if (block_sig == KIND_BLOCK_END)
                    {
                        break;
                    }

                    map_file.read_string(&_junctions[i].name);
                    _junctions[i].a = map_file.read_float();
                    _junctions[i].b = map_file.read_float();
                    _junctions[i].c = map_file.read4();
                    _junctions[i].d = map_file.read_float();
                    _junctions[i].e = map_file.read_float();
                    _junctions[i].f = map_file.read_float();
                    _junctions[i].g = map_file.read_float();
                    _junctions[i].h = map_file.read_float();
                    _junctions[i].i = map_file.read4();
                }
                break;
        }

        map_file.pop();

    }
}


c_status_t c4d_map::load_entities(file_parser* map_file)
{
    cheetah_file_kind_t kind;
    uint32_t temp;
    uint32_t block_sig;
    c4d_unit* unit;
    uint32_t obj_type;

    uint32_t version = map_file->read4();
    if (version != KIND_FILE_v100)
    {
        printf("unknown ENTS version %u\n", version);
        return C_FILE_UNSUPPORTED;
    }

    while (!map_file->peek())
    {
        kind = map_file->read_kind();

        switch (kind)
        {
            case KIND_DODS:
                temp = map_file->read4();
                _doodads = (c4d_doodad*)malloc(sizeof(c4d_doodad) * temp);
                map_file->read4();
                map_file->read4();

                for (int i = 0; i < temp-1; i++)
                {
                    block_sig = map_file->read4();
                    while (block_sig != KIND_BLOCK_HAS_NEXT)
                    {
                        block_sig = map_file->read4();
                        i++;
                    }

                    assert(map_file->read_kind() == KIND_DOOD);
                    assert(map_file->read4() == KIND_FILE_v100);
                    map_file->read_string(&_doodads[i].name);
                    _doodads[i].r = map_file->read_float();
                    _doodads[i].x = map_file->read_float();
                    _doodads[i].y = map_file->read_float();
                    _doodads[i].z = map_file->read_float();
                    _doodads[i].b = map_file->read_float();
                    _doodads[i].c = map_file->read_float();
                    map_file->pop();                    
                }
                break;

            case KIND_DECS:
                temp = map_file->read4();
                _decals = (c4d_decal*)malloc(temp * sizeof(c4d_decal));
                for (int i = 0; i < temp; i++)
                {
                    assert(map_file->read_kind() == KIND_DECA);
                    assert(map_file->read4() == KIND_FILE_v100);
                    map_file->read_string(&_decals[i].name);
                    _decals[i].a = map_file->read4();
                    _decals[i].b = map_file->read4();
                    _decals[i].c = map_file->read4();
                    map_file->pop();
                }
                break;

            case KIND_AMBS:
                temp = map_file->read4();
                _ambient_sounds = (c4d_ambient_sound*)malloc(temp * sizeof(c4d_ambient_sound));
                for (int i = 0; i < temp; i++)
                {
                    block_sig = map_file->read4();
                    while (block_sig != KIND_BLOCK_HAS_NEXT)
                    {
                        block_sig = map_file->read4();
                        i++;
                    }

                    assert(map_file->read_kind() == KIND_AMBI);
                    assert(map_file->read4() == KIND_FILE_v100);
                    map_file->read_string(&_ambient_sounds[i].name);
                    _ambient_sounds[i].x = map_file->read_float();
                    _ambient_sounds[i].y = map_file->read_float();
                    _ambient_sounds[i].z = map_file->read_float();
                    _ambient_sounds[i].range = map_file->read_float();
                    map_file->pop();    
                }
                break;

            case KIND_AIGP:
                break;

            case KIND_UNDS:
                if (0)
                {
                    while (1)
                    {
                        unit = (c4d_unit*)malloc(sizeof(c4d_unit));
                        assert(map_file->read_kind() == KIND_UNTD);
                        assert(map_file->read4() == KIND_FILE_v100);
                        /*
                            read gvar type, field name(str), value(as gvar type)
                        obj_type = map_file->read4();
                        if (obj_type == )
                        map_file->read_string(&unit->class_name);
                        */
                        
                        map_file->pop();  
                    }
                }
                break;

            case KIND_UNIS:
                if (0)
                {
                    temp = map_file->read4();
                    for (int i = 0; i < temp; i++)
                    {
                        if (map_file->read4() != 0)
                        {
                            assert(map_file->read_kind() == KIND_UNIT);
                            assert(map_file->read4() == KIND_FILE_v100);
                            // name
                            // -- tbd
                        }
                    }
                }
                break;

            case KIND_EEFS:
                temp = map_file->read4();
                map_file->read4();
                map_file->read4();
                _effects = (c4d_effect*)malloc(temp * sizeof(c4d_effect));

                for (int i = 0; i < temp; i++)
                {
                    block_sig = map_file->read4();
                    while (block_sig != KIND_BLOCK_HAS_NEXT)
                    {
                        block_sig = map_file->read4();
                        i++;
                    }

                    assert(map_file->read_kind() == KIND_EFFE);
                    assert(map_file->read4() == KIND_FILE_v100);
                    map_file->read_string(&_effects[i].name);
                    _effects[i].a = map_file->read_float();
                    _effects[i].b = map_file->read_float();
                    _effects[i].c = map_file->read_float();
                    _effects[i].d = map_file->read_float();
                    _effects[i].e = map_file->read_float();
                    // Pixie::loadEffectPrototype
                    map_file->pop();  
                }
                break;

            case KIND_LAKS:
                temp = map_file->read4();
                map_file->read4();
                map_file->read4();
                _lakes = (c4d_lake*)malloc(temp * sizeof(c4d_lake));

                for (int i = 0; i < temp; i++)
                {
                    block_sig = map_file->read4();
                    while (block_sig != KIND_BLOCK_HAS_NEXT)
                    {
                        if (block_sig == KIND_BLOCK_END)
                        {
                            break;
                        }
                        block_sig = map_file->read4();
                        i++;
                    }
                    if (block_sig == KIND_BLOCK_END)
                    {
                        break;
                    }

                    assert(map_file->read_kind() == KIND_LAKE);
                    obj_type = map_file->read4();      // v100 or v101
                    map_file->read_string(&_lakes[i].name);
                    _lakes[i].a = map_file->read_float();
                    _lakes[i].b = map_file->read_float();
                    _lakes[i].c = map_file->read_float();
                    _lakes[i].e = map_file->read4();
                    _lakes[i].f = map_file->read4();
                    if (obj_type == KIND_FILE_v101)
                    {
                        _lakes[i].g = map_file->read4();
                        _lakes[i].d = map_file->read_float();
                    }
                    // loadTexture(name)
                    map_file->pop();  
                }
                break;
        }

        map_file->pop();
    }
}


c_status_t c4d_map::load_terrain(file_parser* map_file)
{
    cheetah_file_kind_t kind;
    uint32_t temp;

    uint32_t version = map_file->read4();
    if (version != KIND_FILE_v100)
    {
        printf("unknown TERR version %u\n", version);
        return C_FILE_UNSUPPORTED;
    }

    while (!map_file->peek())
    {
        kind = map_file->read_kind();

        switch (kind)
        {
            case KIND_HMAP:
                _map_width = map_file->read4();
                _map_height = map_file->read4();
                temp = (_map_width + 1) * (_map_height + 1) * 4;
                _heightmap_data = (float*)malloc(temp);
                map_file->read_bytes_exactly((uint8_t*)_heightmap_data, temp);
                break;

            case KIND_TLAY:
                _layer_count = map_file->read4();
                assert (_layer_count < TERR_MAX_LAYERS);
                for (int i = 0; i < _layer_count; i++)
                {
                    map_file->read_string(&_layers[i].name);
                    _layers[i].attributes = map_file->read4();

                    if (_layers[i].attributes & 2)
                    {
                        map_file->read_string(&_layers[i].extrastr);
                    }
                    else if (_layers[i].attributes & 8)
                    {
                        _layers[i].attributes = (_layers[i].attributes & 0xfffffff7) | 2;
                        _layers[i].extrastr = (char*)malloc(9);
                        strcpy(_layers[i].extrastr, "99 Regi");
                    }
                }
                break;

            case KIND_BLND:
                assert(_layer_count > 0);
                temp = (_map_width + 1) * (_map_height + 1);
                for (int i = 0; i < _layer_count; i++)
                {
                    _layers[i].blend_strength = (uint8_t*)malloc(temp);
                    map_file->read_bytes_exactly(_layers[i].blend_strength, temp);
                }
                break;

            case KIND_DIFF:
                temp = (_map_width + 1) * (_map_height + 1) * 4;
                _diff_data = (float*)malloc(temp);
                map_file->read_bytes_exactly((uint8_t*)_diff_data, temp);
                break;

            case KIND_BLCK:
                temp = (_map_width + 1) * (_map_height + 1);
                _block_map = (uint16_t*)malloc(temp * 2);
                for (int i = 0; i < temp; i++)
                {
                    _block_map[i] = map_file->read2();
                }
                break;

            case KIND_TMAP:
                temp = ((_map_height + (_map_height >> 0x1f & 7)) >> 3) * ((_map_width + (_map_width >> 0x1f & 7)) >> 3) * 2;
                _tile_map = (uint8_t*)malloc(temp);
                map_file->read_bytes_exactly(_tile_map, temp);
                break;
            
        }

        map_file->pop();
    }

}


void c4d_map::print_stats(void)
{
    for (int i = 0; i < _layer_count; i++)
    {
        printf("Layer %u: %s with attr %08X\n", i, _layers[i].name, _layers[i].attributes);
    }
}