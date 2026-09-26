#ifndef _C4D_MAP_H_
#define _C4D_MAP_H_

#include "defines.h"
#include "file_parser.h"
#include "cheetah.h"
#include "c4d_map_objects.h"
#include <vector>

#define TERR_MAX_LAYERS				16

#define TERR_LAYER_NORMAL			0
#define TERR_LAYER_BLOCKER			1
#define TERR_LAYER_GRASS			2
#define TERR_LAYER_FORD 			4
#define TERR_LAYER_MASK_TYPE 		0xF
#define TERR_LAYER_MASK_WALKER		0x10
#define TERR_LAYER_MASK_INVISIBLE	0x80

class c4d_map
{
	public:
		c4d_map();
		c_status_t load_from_file(char* file_name);

		c_status_t load_entities(file_parser* map_file);
		c_status_t load_terrain(file_parser* map_file);

		void print_stats(void);

	private:
		char* _mina;
		char* _skybox;
		char* _atmosphere;
		uint32_t _map_width, _map_height;
		cheetah_bitmap_t _minimap;

		float* _heightmap_data;
		uint32_t _layer_count;
		c4d_layer _layers[TERR_MAX_LAYERS];
		float* _diff_data;
		uint16_t* _block_map;
		uint8_t* _tile_map;

		uint8_t* _camera_data;
		uint8_t* _weather_data;

		c4d_doodad* _doodads;
		c4d_ambient_sound* _ambient_sounds;
		c4d_decal* _decals;
		std::vector<c4d_unit*> _units;
		c4d_lake* _lakes;
		c4d_effect* _effects;
		c4d_location* _locations;
		c4d_path* _paths;
		c4d_road* _roads;
		c4d_road_junction* _junctions;
};

#endif