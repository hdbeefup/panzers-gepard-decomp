#include "defines.h"
#include "c4d_mesh.h"
#include "c4d_node.h"
#include "c4d_collision_poly.h"
#include <vector>

class c4d_model
{
    public:
		c4d_model() {};
        c_status_t load_from_file(char* file_name, float scale);
        void parse_ssqs(file_parser* model_file);

        void generate_meshes(void);
        void draw(void);
        void print_stats(uint8_t indent);

    private:
        std::vector<c4d_node*> _nodes;
        bool _has_FLYZ;
        float _ambi[3];
        uint16_t _mesh_count;
        uint16_t _anim_count;
};
