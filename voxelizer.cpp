class Voxelizer {
    float pitch = 0.0f;
    float mx_edge_len = 0.0f;
    bool fill = false;
    glm::vec3 bound[2] = {};

    std::vector<glm::vec3> vertices{};
    std::vector<unsigned int> indices{};

    Voxelizer(const std::vector<glm::vec3>& vertices, const std::vector<unsigned int>& indices, float pitch, bool fill): vertices(vertices), indices(indices), pitch(pitch), fill(fill) {
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

    void calc_bound() {
        float inf = 1e30;
        bound[0] = glm::vec3(-inf, -inf, -inf);
        bound[1] = glm::vec3(inf, inf, inf);
        for(auto& v : vertices) {
            bound[0] = glm::min(bound[0], v);
            bound[1] = glm::max(bound[1], v);
        }
    }

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
                auto new_a = 0.5 * (b+c);
                auto new_b = 0.5 * (c+a);
                auto new_c = 0.5 * (a+b);
                vertices.push_back(new_a);
                vertices.push_back(new_b);
                vertices.push_back(new_c);
                int ind_new_a = vertices.size()-3;
                int ind_new_b = vertices.size()-2;
                int ind_new_c = vertices.size()-1;

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
        }

        mx_edge_len = cur_mx_edge_len;

        calc_bound();

    }

    glm::int3 get_cell_id(glm::vec3 pos) {
        return glm::int3((pos-bound[0])/pitch);
    }
    
    glm::vec3 get_cell_center_pos(glm::int3 cell_id) {
        return bound[0] + cell_id * pitch;
    }

    std::vector<glm::vec3> voxelize(float pitch_) {

        this->pitch = pitch_;
        subdivide();

        std::unordered_set<glm::int3> occupied_cells;
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
            occupied_cells.insert(get_cell_id((a+b+c)/3));
        }

        std::vector<glm::vec3> cell_positions;
        for(auto& cid : occupied_cells) {
            cell_positions.push_back(get_cell_center_pos(cid));
        }

    }
};