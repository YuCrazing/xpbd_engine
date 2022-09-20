#define NdArray(name, ele_shape, data_type, arr_shape) ;

class PBDEngine {

    int object_num;
    int particle_num;
    int max_object_num;
    int max_particle_num;

    NdArray(p_oid, 1, ti.i32, max_particle_num); // particle's object id
    NdArray(p_id, 1, ti.i32, max_particle_num); // particle's id
    NdArray(p_pos, 3, ti.f32, max_particle_num);
    NdArray(p_rest_pos, 3, ti.f32, max_particle_num); // only for shape matching
    NdArray(p_prev_pos, 3, ti.f32, max_particle_num);
    NdArray(p_vel, 3, ti.f32, max_particle_num);
    NdArray(p_inv_m, 1, ti.f32, max_particle_num);
    NdArray(p_den, 1, ti.f32, max_particle_num);
    NdArray(p_color, 3, ti.f32, max_particle_num);
    NdArray(p_index, 3, ti.f32, max_particle_num); // for sorting
    NdArray(p_cid, 3, ti.f32, max_particle_num); // particle's cell id

    // NdArray(o_rest_den, 1, ti.f32, max_object_num); // just use p_den
    NdArray(o_rest_center, 3, ti.f32, max_object_num);
    NdArray(o_center, 3, ti.f32, max_object_num);
    NdArray(o_color, 3, ti.f32, max_object_num);
    NdArray(o_phase, 3, ti.f32, max_object_num);
    NdArray(o_state, 3, ti.f32, max_object_num);
    NdArray(o_friction, 3, ti.f32, max_object_num);
    NdArray(o_restitution, 3, ti.f32, max_object_num);

    int time_step;
    int pbd_iter_n;
    float fluid_relaxation_parameter = 1000.0;
    float fluid_relaxation_parameter_2 = 0.0;
    float fluid_viscosity = 0.1;
    float fluid_pressure_k = 0.00002;
    float fluid_pressure_n = 4;
    float xsph_iter_n = 1;
    float particle_radius = 0.0f;
    float particle_diameter = 0.0f;
    float smooth_radius = 0.0f;


    void add_cube(glm::vec3 center, glm::vec3 half_extent, float density, int phase, int state, glm::vec2 friction, float restitution, glm::vec3 color) {

    }

    // Do not provide a voxilization tool?
    // void add_mesh(glm::vec3 center, std::vector<glm::vec3> vertices, std::vector<unsigned int> indices, float density, int phase, int state, glm::vec2 friction, float restitution, glm::vec3 color) {

    // }

    void add_custom_particles(glm::vec3 center, std::vector<glm::vec3> particles, float density, int phase, int state, glm::vec2 friction, float restitution, glm::vec3 color) {

    }

    void add_cube_emitter(glm::vec3 center, glm::vec3 half_extent, float speed, float density, int phase, int state, glm::vec2 friction, float restitution, glm::vec3 color) {

    }

    void set_boundary_box() {

    }

    // slip, sticky, separate
    void set_static_collision() {
        
    }

};