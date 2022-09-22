#include <glm/common.hpp>

#include <unordered_set>

// struct int2_key {
//     int x, y;
//     int2_key(int x, int y): x(x), y(y) {}
//     bool operator < (const int2_key& t) const {
//         return x < t.x && y < t.y;
//     }
//     bool operator == (const int2_key& t) const {
//         return x == t.x && y == t.y;
//     }
// };


// struct int3_key {
//     int x, y, z;
//     int3_key(int x, int y, int z): x(x), y(y), z(z) {}
//     bool operator < (const int3_key& t) const {
//         return x < t.x && y < t.y && z < t.z;
//     }
// };

namespace std {
    template<>
    class hash <glm::ivec2> {
        public:
        size_t operator()(const glm::ivec2& t) const {
            return hash<int>()(t.x) ^ hash<int>()(t.y);
        }
    };
    template<>
    class hash <glm::ivec3> {
        public:
        size_t operator()(const glm::vec3& t) const {
            return hash<int>()(t.x) ^ hash<int>()(t.y) ^ hash<int>()(t.z);
        }
    };
}


class Voxelizer {

public:

    Voxelizer(const std::vector<glm::vec3>& vertices, const std::vector<unsigned int>& indices, bool fill): vertices(vertices), indices(indices), fill(fill) {
        mx_edge_len = 0.0f;
        for(size_t i = 0; i < indices.size() / 3; ++i) {
            auto ind_a = indices[3*i];
            auto ind_b = indices[3*i+1];
            auto ind_c = indices[3*i+2];
            auto a = vertices[ind_a];
            auto b = vertices[ind_b];
            auto c = vertices[ind_c];
            mx_edge_len = std::max(mx_edge_len, glm::length(a-b));
            mx_edge_len = std::max(mx_edge_len, glm::length(b-c));
            mx_edge_len = std::max(mx_edge_len, glm::length(c-a));
        }
        calc_bound();
    }

    bool check_watertight() {
        std::unordered_map<glm::ivec2, int> edge_map;
        for(size_t i = 0; i < indices.size() / 3; ++i) {
            auto ind_a = indices[3*i];
            auto ind_b = indices[3*i+1];
            auto ind_c = indices[3*i+2];
            for (size_t j = 0; j < 3; ++j) {
                auto ind_a = indices[3*i+j];
                auto ind_b = indices[3*i+(j+1)%3];
                if (ind_a > ind_b) std::swap(ind_a, ind_b);
                auto e = glm::ivec2(ind_a, ind_b);
                if (edge_map.find(e) == edge_map.end()) {
                    edge_map[e] = 1;
                } else {
                    edge_map[e] ++;
                }

            }
        }
        for(auto& it : edge_map) {
            if(it.second != 2){
                return false;
            }
        }
        return true;
    }

    void calc_bound() {
        float inf = 1e30;
        bound[0] = glm::vec3(inf, inf, inf);
        bound[1] = glm::vec3(-inf, -inf, -inf);
        for(auto& v : vertices) {
            bound[0] = glm::min(bound[0], v);
            bound[1] = glm::max(bound[1], v);
        }
    }

    void voxelize(float pitch_, std::vector<glm::vec3>& cell_positions) {

        this->pitch = pitch_;
        subdivide();

        std::cout << "subdivision finished. " << "vn: " << vertices.size() << ", in: " << indices.size() << std::endl;

        std::unordered_set<glm::ivec3> occupied_cells;
        for(size_t i = 0; i < indices.size() / 3; ++i) {
            auto ind_a = indices[3*i];
            auto ind_b = indices[3*i+1];
            auto ind_c = indices[3*i+2];
            auto a = vertices[ind_a];
            auto b = vertices[ind_b];
            auto c = vertices[ind_c];
            occupied_cells.insert(get_cell_id(a));
            occupied_cells.insert(get_cell_id(b));
            occupied_cells.insert(get_cell_id(c));
            occupied_cells.insert(get_cell_id((a+b+c)/3.0f));
        }

        std::cout << "occupied cells: " << occupied_cells.size() << std::endl;

        // std::vector<glm::vec3> cell_positions;
        cell_positions.clear();
        int cnt = 0;
        for(const auto& cid : occupied_cells) {
            cnt ++;
            cell_positions.push_back(get_cell_center_pos(cid));
        }
        std::cout << "cnt: " << cnt << std::endl;

        // return cell_positions;

    }

private:
    void subdivide() {

        if(mx_edge_len < pitch) {
            return ;
        }

        float cur_mx_edge_len = mx_edge_len;

        while(cur_mx_edge_len > pitch) {

            std::vector<glm::vec3> cur_vertices = vertices;
            std::vector<unsigned int> cur_indices = indices;

            cur_mx_edge_len = 0.0f;

            for(size_t i = 0; i < cur_indices.size() / 3; ++i) {
                auto ind_a = cur_indices[3*i];
                auto ind_b = cur_indices[3*i+1];
                auto ind_c = cur_indices[3*i+2];
                auto a = cur_vertices[ind_a];
                auto b = cur_vertices[ind_b];
                auto c = cur_vertices[ind_c];
                auto new_a = 0.5f * (b+c);
                auto new_b = 0.5f * (c+a);
                auto new_c = 0.5f * (a+b);
                vertices.push_back(new_a);
                vertices.push_back(new_b);
                vertices.push_back(new_c);
                auto ind_new_a = vertices.size() - 3;
                auto ind_new_b = vertices.size() - 2;
                auto ind_new_c = vertices.size() - 1;
                
                // Add new face [new_a, new_b, new_c], also remove original face [a, b, c]
                indices[3*i] = ind_new_a;
                indices[3*i+1] = ind_new_b;
                indices[3*i+2] = ind_new_c;

                // Add new face
                indices.push_back(ind_a);
                indices.push_back(ind_new_c);
                indices.push_back(ind_new_b);

                // Add new face
                indices.push_back(ind_b);
                indices.push_back(ind_new_a);
                indices.push_back(ind_new_c);

                // Add new face
                indices.push_back(ind_c);
                indices.push_back(ind_new_b);
                indices.push_back(ind_new_a);

                cur_mx_edge_len = std::max(cur_mx_edge_len, glm::length(a-new_b));
                cur_mx_edge_len = std::max(cur_mx_edge_len, glm::length(new_b-c));
                cur_mx_edge_len = std::max(cur_mx_edge_len, glm::length(c-new_a));
                cur_mx_edge_len = std::max(cur_mx_edge_len, glm::length(new_a-b));
                cur_mx_edge_len = std::max(cur_mx_edge_len, glm::length(b-new_c));
                cur_mx_edge_len = std::max(cur_mx_edge_len, glm::length(new_c-a));
                cur_mx_edge_len = std::max(cur_mx_edge_len, glm::length(new_a-new_b));
                cur_mx_edge_len = std::max(cur_mx_edge_len, glm::length(new_b-new_c));
                cur_mx_edge_len = std::max(cur_mx_edge_len, glm::length(new_c-new_a));
            }
            std::cout << "max edge: " << cur_mx_edge_len << std::endl;
        }

        mx_edge_len = cur_mx_edge_len;

        calc_bound();

    }

    glm::ivec3 get_cell_id(glm::vec3 pos) {
        auto cid = glm::ivec3((pos-bound[0])/pitch);
        // std::cout << "pos: " << glm::to_string(pos) << ", cid: " << glm::to_string(cid) << std::endl;
        return cid;
    }

    glm::vec3 get_cell_center_pos(glm::ivec3 cell_id) {
        return bound[0] + glm::vec3(cell_id) * pitch;
    }

public:
    glm::vec3 bound[2] = {};

private:
    float pitch = 0.0f;
    float mx_edge_len = 0.0f;
    bool fill = false;

    std::vector<glm::vec3> vertices{};
    std::vector<unsigned int> indices{};

};