#include <iostream>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <chrono>
#include <iostream>
#include <signal.h>
#include <inttypes.h>
#include <unistd.h>
#include <memory>
#include <vector>

#include <taichi/ui/gui/gui.h>
#include <taichi/ui/backends/vulkan/renderer.h>
#include "taichi_core_impl.h"
#include "c_api_test_utils.h"
#include "taichi/taichi_core.h"
#include "taichi/taichi_vulkan.h"
#include "taichi/ui/backends/vulkan/scene.h"
#include "taichi/ui/backends/vulkan/window.h"
#include "taichi/ui/backends/vulkan/canvas.h"
#include "glm/gtx/string_cast.hpp"
#include "utils.hpp"
#include <taichi/cpp/taichi.hpp>



const std::vector<uint32_t> NONE_SHAPE = {};
const std::vector<uint32_t> VEC2_SHAPE = {2};
const std::vector<uint32_t> VEC3_SHAPE = {3};
const std::vector<uint32_t> MAT2_SHAPE = {2, 2};
const std::vector<uint32_t> MAT3_SHAPE = {3, 3};


#define NR_PARTICLES 8000

#include <unistd.h>

constexpr int SUBSTEPS = 5;




void scene_traverse(aiNode* cur_node, const aiScene* scene){
    if (cur_node->mNumMeshes > 0) {
        static int mesh_count = 0;
        for(int i = 0; i < cur_node->mNumMeshes; ++i) {
            auto mesh_id = cur_node->mMeshes[i];
            auto* mesh = scene->mMeshes[mesh_id];
            std::cout << "mesh " << mesh_count << ": vn: " << mesh->mNumVertices << "; in: " << mesh->mNumFaces << std::endl;
            ++mesh_count;
        }
    }
    if(cur_node->mNumChildren > 0){
        for(int i = 0; i < cur_node->mNumChildren; ++i){
            scene_traverse(cur_node->mChildren[i], scene);
        }
    }
}

bool TestImporting( const std::string& pFile)
{
  // Create an instance of the Importer class
  Assimp::Importer importer;
  // And have it read the given file with some example postprocessing
  // Usually - if speed is not the most important aspect for you - you'll 
  // propably to request more postprocessing than we do in this example.
  const aiScene* scene = importer.ReadFile( pFile, 
        // aiProcess_CalcTangentSpace       | 
        aiProcess_Triangulate            |
        aiProcess_JoinIdenticalVertices  |
        aiProcess_SortByPType);
  
  // If the import failed, report it
  if( !scene)
  {
    std::cout << ( importer.GetErrorString()) << std::endl;
    return false;
  }

  scene_traverse(scene->mRootNode, scene);

  return true;
}


#define MAKETIDATA_WITHVALUE_FLOAT(runtime_, value_name)                                       \
value_name = TiAotNdArray::Make(runtime_,                                             \
                                    TiDataType::TI_DATA_TYPE_F32,                       \
                                    {static_cast<uint32_t>(tmp_##value_name.size())},   \
                                    NONE_SHAPE);                                       \
TiAotNdArray::LoadDataFromHost<float>(value_name, tmp_##value_name);                 \
// ti_name_arges[#value_name] = BindTiNameArgs(#value_name, value_name)                 \


void run(TiArch arch, const std::string& folder_dir, const std::string& package_path) {

    /* --------------------- */
    /* Render Initialization */
    /* --------------------- */
    // Init gl window
    int win_width = 800;
    int win_height = 800;
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(win_width, win_height, "Taichi show", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }

    // Create a GGUI configuration
    taichi::ui::AppConfig app_config;
    app_config.name         = "SPH";
    app_config.width        = win_width;
    app_config.height       = win_height;
    app_config.vsync        = true;
    app_config.show_window  = false;
    app_config.package_path = package_path; // make it flexible later
    app_config.ti_arch      = get_taichi_arch(arch);

    // Create GUI & renderer
    auto renderer  = std::make_unique<taichi::ui::vulkan::Renderer>();
    renderer->init(nullptr, window, app_config);

    /* ---------------------------------- */
    /* Runtime & Arguments Initialization */
    /* ---------------------------------- */
    TiRuntime runtime = ti_create_runtime(arch);

    // Load Aot and Kernel
    TiAotModule aot_mod = ti_load_aot_module(runtime, folder_dir.c_str());
    
    // TiKernel k_initialize = ti_get_aot_module_kernel(aot_mod, "initialize");
    // TiKernel k_initialize_particle = ti_get_aot_module_kernel(aot_mod, "initialize_particle");
    // TiKernel k_update_density = ti_get_aot_module_kernel(aot_mod, "update_density");
    // TiKernel k_update_force = ti_get_aot_module_kernel(aot_mod, "update_force");
    // TiKernel k_advance = ti_get_aot_module_kernel(aot_mod, "advance");
    // TiKernel k_boundary_handle = ti_get_aot_module_kernel(aot_mod, "boundary_handle");
    TiKernel k_particle_fall = ti_get_aot_module_kernel(aot_mod, "particle_fall");

    const std::vector<int> shape_1d = {NR_PARTICLES};
    const std::vector<int> vec3_shape = {3};
    
    // auto N_ = capi::utils::make_ndarray(runtime,
    //                        TiDataType::TI_DATA_TYPE_I32,
    //                        shape_1d.data(), 1,
    //                        vec3_shape.data(), 1,
    //                        false /*host_read*/, false /*host_write*/
    //                        );
    // auto den_ = capi::utils::make_ndarray(runtime,
    //                          TiDataType::TI_DATA_TYPE_F32,
    //                          shape_1d.data(), 1,
    //                          nullptr, 0,
    //                          false /*host_read*/, false /*host_write*/
    //                          );
    // auto pre_ = capi::utils::make_ndarray(runtime,
    //                      TiDataType::TI_DATA_TYPE_F32,
    //                      shape_1d.data(), 1,
    //                      nullptr, 0,
    //                      false /*host_read*/, false /*host_write*/
    //                      );
    
    // auto vel_ = capi::utils::make_ndarray(runtime,
    //                      TiDataType::TI_DATA_TYPE_F32,
    //                      shape_1d.data(), 1,
    //                      vec3_shape.data(), 1,
    //                      false /*host_read*/, false /*host_write*/
    //                      );
    // auto acc_ = capi::utils::make_ndarray(runtime,
    //                      TiDataType::TI_DATA_TYPE_F32,
    //                      shape_1d.data(), 1,
    //                      vec3_shape.data(), 1,
    //                      false /*host_read*/, false /*host_write*/
    //                      );
    // auto boundary_box_ = capi::utils::make_ndarray(runtime,
    //                               TiDataType::TI_DATA_TYPE_F32,
    //                               shape_1d.data(), 1,
    //                               vec3_shape.data(), 1,
    //                               false /*host_read*/, false /*host_write*/
    //                               );
    // auto spawn_box_ = capi::utils::make_ndarray(runtime,
    //                            TiDataType::TI_DATA_TYPE_F32,
    //                            shape_1d.data(), 1,
    //                            vec3_shape.data(), 1,
    //                            false /*host_read*/, false /*host_write*/
    //                            );
    // auto gravity_ = capi::utils::make_ndarray(runtime,
    //                          TiDataType::TI_DATA_TYPE_F32,
    //                          nullptr, 0, 
    //                          vec3_shape.data(), 1,
    //                          false/*host_read*/,
    //                          false/*host_write*/);



    capi::utils::TiNdarrayAndMem pos_;
    taichi::lang::DeviceAllocation pos_devalloc;
    if(arch == TiArch::TI_ARCH_VULKAN) {
        taichi::lang::vulkan::VulkanDevice *device = &(renderer->app_context().device());
        pos_devalloc = get_ndarray_with_imported_memory(runtime, TiDataType::TI_DATA_TYPE_F32, shape_1d, vec3_shape, device, pos_);

    } else {
        pos_ = capi::utils::make_ndarray(runtime,
                            TiDataType::TI_DATA_TYPE_F32,
                            shape_1d.data(), 1,
                            vec3_shape.data(), 1,
                            false /*host_read*/, false /*host_write*/
                            );
        pos_devalloc = get_devalloc(pos_.runtime_, pos_.memory_);
    }


    std::vector<float> pos_host;
    for(size_t i = 0; i < NR_PARTICLES; ++i) {
        pos_host.push_back(i*1.0f * 0.01);
        pos_host.push_back(i*1.0f * 0.01);
        pos_host.push_back(i*1.0f * 0.01);
    }

    float *dst = (float *)ti_map_memory(pos_.runtime_, pos_.memory_);
    if (dst != nullptr) {
        std::memcpy(dst, pos_host.data(), pos_host.size() * sizeof(float));
    }
    ti_unmap_memory(pos_.runtime_, pos_.memory_);


    // TiArgument k_initialize_args[3];
    // TiArgument k_initialize_particle_args[4];
    // TiArgument k_update_density_args[3];
    // TiArgument k_update_force_args[6];
    // TiArgument k_advance_args[3];
    // TiArgument k_boundary_handle_args[3];
    TiArgument k_particle_fall_args[1];

    // k_initialize_args[0] = boundary_box_.arg_;
    // k_initialize_args[1] = spawn_box_.arg_;
    // k_initialize_args[2] = N_.arg_;

    // k_initialize_particle_args[0] = pos_.arg_;
    // k_initialize_particle_args[1] = spawn_box_.arg_;
    // k_initialize_particle_args[2] = N_.arg_;
    // k_initialize_particle_args[3] = gravity_.arg_;

    // k_update_density_args[0] = pos_.arg_;
    // k_update_density_args[1] = den_.arg_;
    // k_update_density_args[2] = pre_.arg_;

    // k_update_force_args[0] = pos_.arg_;
    // k_update_force_args[1] = vel_.arg_;
    // k_update_force_args[2] = den_.arg_;
    // k_update_force_args[3] = pre_.arg_;
    // k_update_force_args[4] = acc_.arg_;
    // k_update_force_args[5] = gravity_.arg_;

    // k_advance_args[0] = pos_.arg_;
    // k_advance_args[1] = vel_.arg_;
    // k_advance_args[2] = acc_.arg_;

    // k_boundary_handle_args[0] = pos_.arg_;
    // k_boundary_handle_args[1] = vel_.arg_;
    // k_boundary_handle_args[2] = boundary_box_.arg_;
    k_particle_fall_args[0] = pos_.arg_;

    /* --------------------- */
    /* Kernel Initialization */
    /* --------------------- */
    // ti_launch_kernel(runtime, k_initialize, 3, &k_initialize_args[0]);
    // ti_launch_kernel(runtime, k_initialize_particle, 4, &k_initialize_particle_args[0]);
    // ti_wait(runtime);

    /* -------------- */
    /* Initialize GUI */
    /* -------------- */
    auto gui = std::make_shared<taichi::ui::vulkan::Gui>(&renderer->app_context(), &renderer->swap_chain(), window);

    // Describe information to render the circle with Vulkan
    taichi::ui::FieldInfo f_info;
    f_info.valid        = true;
    f_info.field_type   = taichi::ui::FieldType::Scalar;
    f_info.matrix_rows  = 1;
    f_info.matrix_cols  = 1;
    f_info.shape        = {NR_PARTICLES};
    f_info.field_source = get_field_source(arch);
    f_info.dtype        = taichi::lang::PrimitiveType::f32;
    f_info.snode        = nullptr;
    f_info.dev_alloc    = pos_devalloc;

    renderer->set_background_color({0.6, 0.6, 0.6});

    auto scene = std::make_unique<taichi::ui::vulkan::Scene>();

    taichi::ui::RenderableInfo renderable_info;
    renderable_info.vbo = f_info;
    renderable_info.has_user_customized_draw = false;
    renderable_info.has_per_vertex_color = false;

    taichi::ui::ParticlesInfo particles;
    particles.renderable_info = renderable_info;
    particles.color = glm::vec3(0.3, 0.5, 0.8);
    particles.radius = 0.01f;
    particles.object_id = 0;

    // taichi::ui::MeshInfo meshes;
    // meshes.renderable_info = renderable_info;
    // meshes.color = glm::vec3(0.3, 0.5, 0.8);
    // meshes.object_id = 0;

    auto camera = std::make_unique<taichi::ui::Camera>();
    camera->position = glm::vec3(0.0, 1.5, 1.5);
    camera->lookat = glm::vec3(0.0, 0.0, 0.0);
    camera->up = glm::vec3(0.0, 1.0, 0);

    /* --------------------- */
    /* Execution & Rendering */
    /* --------------------- */
    while (!glfwWindowShouldClose(window)) {
        // for(int i = 0; i < SUBSTEPS; i++) {
        //     ti_launch_kernel(runtime, k_update_density, 3, &k_update_density_args[0]);
        //     ti_launch_kernel(runtime, k_update_force, 6, &k_update_force_args[0]);
        //     ti_launch_kernel(runtime, k_advance, 3, &k_advance_args[0]);
        //     ti_launch_kernel(runtime, k_boundary_handle, 3, &k_boundary_handle_args[0]);
        // }
        ti_launch_kernel(runtime, k_particle_fall, 1, &k_particle_fall_args[0]);
        ti_wait(runtime);

        handle_user_inputs(camera.get(), window, win_width, win_height);

        scene->set_camera(*camera);
        scene->point_light(glm::vec3(2.0, 2.0, 2.0), glm::vec3(1.0, 1.0, 1.0));
        scene->particles(particles);
        // scene->mesh(meshes);

        renderer->scene(scene.get());

        // Render elements
        renderer->draw_frame(gui.get());
        renderer->swap_chain().surface().present_image();
        renderer->prepare_for_next_frame();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    renderer->cleanup();
}

int main(int argc, char *argv[]) {
    // TestImporting("./data/Collision_lowmesh3.obj");
    assert(argc == 4);
    std::string aot_path = argv[1];
    std::string package_path = argv[2];
    std::string arch_name = argv[3];

    TiArch arch;
    if(arch_name == "cuda") arch = TiArch::TI_ARCH_CUDA;
    else if(arch_name == "vulkan") arch = TiArch::TI_ARCH_VULKAN;
    else if(arch_name == "x64") arch = TiArch::TI_ARCH_X64;
    else assert(false);

    run(arch, aot_path, package_path);
}