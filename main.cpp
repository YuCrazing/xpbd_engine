#include <iostream>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <glm/common.hpp>

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

constexpr int SUBSTEPS = 5;




void scene_traverse(aiNode* cur_node, const aiScene* scene, std::vector<glm::vec3>& vbo, std::vector<unsigned int>& ibo) {
    if (cur_node->mNumMeshes > 0) {
        static int mesh_count = 0;
        for(int i = 0; i < cur_node->mNumMeshes; ++i) {
            auto mesh_id = cur_node->mMeshes[i];
            auto* mesh = scene->mMeshes[mesh_id];
            std::cout << "mesh " << mesh_count << ": vn: " << mesh->mNumVertices << "; fn: " << mesh->mNumFaces << std::endl;
            for(int vi = 0; vi < mesh->mNumVertices; ++vi){
                vbo.push_back(glm::vec3(mesh->mVertices[vi].x, mesh->mVertices[vi].y, mesh->mVertices[vi].z));
                std::cout << vi << " " << glm::to_string(vbo[vbo.size()-1]) << std::endl;
            }
            for(int fi = 0; fi < mesh->mNumFaces; ++fi) {
                for(int jj = 0; jj < mesh->mFaces[fi].mNumIndices; ++jj) {
                    ibo.push_back(mesh->mFaces[fi].mIndices[jj]);
                    std::cout << ibo[ibo.size()-1] << " ";
                }
                std::cout << std::endl;
            }
            ++mesh_count;
        }
    }
    if(cur_node->mNumChildren > 0){
        for(int i = 0; i < cur_node->mNumChildren; ++i){
            scene_traverse(cur_node->mChildren[i], scene, vbo, ibo);
        }
    }
}

bool import_scene(const std::string& pFile, std::vector<glm::vec3>& vbo, std::vector<unsigned int>& ibo)
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

    scene_traverse(scene->mRootNode, scene, vbo, ibo);

    return true;
}


// #define MAKETIDATA_WITHVALUE_FLOAT(runtime_, value_name)                                       \
// value_name = TiAotNdArray::Make(runtime_,                                             \
//                                     TiDataType::TI_DATA_TYPE_F32,                       \
//                                     {static_cast<uint32_t>(tmp_##value_name.size())},   \
//                                     NONE_SHAPE);                                       \
// TiAotNdArray::LoadDataFromHost<float>(value_name, tmp_##value_name);                 \
// // ti_name_arges[#value_name] = BindTiNameArgs(#value_name, value_name)                 \


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
    app_config.name         = "TaichiShow";
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
    TiKernel k_initialize_particles = ti_get_aot_module_kernel(aot_mod, "initialize_particles");
    TiKernel k_data_copy = ti_get_aot_module_kernel(aot_mod, "data_copy");

    const std::vector<int> shape_1d = {NR_PARTICLES};
    const std::vector<int> vec3_shape = {3};

    std::vector<glm::vec3> vbo_host;
    std::vector<unsigned int> ibo_host;
    import_scene("/home/yuzhang/Work/xpbd_engine/data/Collision_lowmesh3.obj", vbo_host, ibo_host);


    std::cout << "vn: " << vbo_host.size() << " in: " << ibo_host.size() << std::endl;

    capi::utils::TiNdarrayAndMem vbo, ibo, vbo_for_rendering;
    taichi::lang::DeviceAllocation vbo_devalloc, ibo_devalloc, vbo_for_rendering_devalloc;
    const std::vector<int> shape_vbo = {vbo_host.size()};

    if (arch == TiArch::TI_ARCH_VULKAN) {
        // taichi::lang::vulkan::VulkanDevice *device = &(renderer->app_context().device());
        // vbo_devalloc = get_ndarray_with_imported_memory(runtime, TiDataType::TI_DATA_TYPE_F32, shape_vbo, vec3_shape, device, vbo);
        assert(0);
    } else {
        vbo = capi::utils::make_ndarray(runtime,
                            TiDataType::TI_DATA_TYPE_F32,
                            shape_vbo.data(), 1,
                            vec3_shape.data(), 1,
                            false /*host_read*/, false /*host_write*/
                            );
        vbo_devalloc = get_devalloc(vbo.runtime_, vbo.memory_);
        vbo_for_rendering = capi::utils::make_ndarray(runtime,
                    TiDataType::TI_DATA_TYPE_F32,
                    shape_vbo.data(), 1,
                    (std::vector<int>{12}).data(), 1,
                    false /*host_read*/, false /*host_write*/
                    );
        vbo_for_rendering_devalloc = get_devalloc(vbo_for_rendering.runtime_, vbo_for_rendering.memory_);
    }


    ibo = capi::utils::make_ndarray(runtime,
                        TiDataType::TI_DATA_TYPE_I32,
                        (std::vector<int> {ibo_host.size()}).data(), 1,
                        nullptr, 0,
                        true /*host_read*/, true /*host_write*/
                        );
    ibo_devalloc = get_devalloc(ibo.runtime_,ibo.memory_);

    float *dst_vbo = (float *)ti_map_memory(vbo.runtime_, vbo.memory_);
    if (dst_vbo != nullptr) {
        std::memcpy(dst_vbo, vbo_host.data(), vbo_host.size() * sizeof(glm::vec3));
    }
    ti_unmap_memory(vbo.runtime_, vbo.memory_);

    int *dst_ibo = (int *)ti_map_memory(ibo.runtime_, ibo.memory_);
    if (dst_ibo != nullptr) {
        std::memcpy(dst_ibo, ibo_host.data(), ibo_host.size() * sizeof(int));
    }
    ti_unmap_memory(ibo.runtime_, ibo.memory_);

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



    // capi::utils::TiNdarrayAndMem pos_;
    // taichi::lang::DeviceAllocation pos_devalloc;
    // if(arch == TiArch::TI_ARCH_VULKAN) {
    //     taichi::lang::vulkan::VulkanDevice *device = &(renderer->app_context().device());
    //     pos_devalloc = get_ndarray_with_imported_memory(runtime, TiDataType::TI_DATA_TYPE_F32, shape_1d, vec3_shape, device, pos_);

    // } else {
    //     pos_ = capi::utils::make_ndarray(runtime,
    //                         TiDataType::TI_DATA_TYPE_F32,
    //                         shape_1d.data(), 1,
    //                         vec3_shape.data(), 1,
    //                         false /*host_read*/, false /*host_write*/
    //                         );
    //     pos_devalloc = get_devalloc(pos_.runtime_, pos_.memory_);
    // }


    // std::vector<float> pos_host;
    // for(size_t i = 0; i < NR_PARTICLES; ++i) {
    //     pos_host.push_back(i*1.0f * 0.01);
    //     pos_host.push_back(i*1.0f * 0.01);
    //     pos_host.push_back(i*1.0f * 0.01);
    // }

    // float *dst = (float *)ti_map_memory(pos_.runtime_, pos_.memory_);
    // if (dst != nullptr) {
    //     std::memcpy(dst, pos_host.data(), pos_host.size() * sizeof(float));
    // }
    // ti_unmap_memory(pos_.runtime_, pos_.memory_);


    // TiArgument k_initialize_args[3];
    // TiArgument k_initialize_particle_args[4];
    // TiArgument k_update_density_args[3];
    // TiArgument k_update_force_args[6];
    // TiArgument k_advance_args[3];
    // TiArgument k_boundary_handle_args[3];
    TiArgument k_particle_fall_args[1];
    TiArgument k_initialize_particles_args[1];
    TiArgument k_data_copy_args[2];

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
    k_particle_fall_args[0] = vbo.arg_;
    k_initialize_particles_args[0] = vbo.arg_;
    k_data_copy_args[0] = vbo_for_rendering.arg_;
    k_data_copy_args[1] = vbo.arg_;
    k_data_copy_args[2] = ibo.arg_;

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

    renderer->set_background_color({0.6, 0.6, 0.6});

    auto scene = std::make_unique<taichi::ui::vulkan::Scene>();

    // taichi::ui::RenderableInfo renderable_info;
    // renderable_info.vbo = f_info;
    // renderable_info.has_user_customized_draw = false;
    // renderable_info.has_per_vertex_color = false;

    // taichi::ui::ParticlesInfo particles;
    // particles.renderable_info = renderable_info;
    // particles.color = glm::vec3(0.3, 0.5, 0.8);
    // particles.radius = 0.01f;
    // particles.object_id = 0;

    taichi::ui::FieldInfo vbo_info_0;
    vbo_info_0.valid        = true;
    vbo_info_0.field_type   = taichi::ui::FieldType::Matrix;
    vbo_info_0.matrix_rows  = 12;
    vbo_info_0.matrix_cols  = 1;
    vbo_info_0.shape        = {vbo_host.size()};
    vbo_info_0.field_source = get_field_source(arch);
    vbo_info_0.dtype        = taichi::lang::PrimitiveType::f32;
    vbo_info_0.snode        = nullptr;
    vbo_info_0.dev_alloc    = vbo_for_rendering_devalloc;
    taichi::ui::FieldInfo ibo_info_0;
    ibo_info_0.valid        = true;
    ibo_info_0.field_type   = taichi::ui::FieldType::Scalar;
    ibo_info_0.matrix_rows  = 1;
    ibo_info_0.matrix_cols  = 1;
    ibo_info_0.shape        = {ibo_host.size()};
    ibo_info_0.field_source = get_field_source(arch);
    ibo_info_0.dtype        = taichi::lang::PrimitiveType::i32;
    ibo_info_0.snode        = nullptr;
    ibo_info_0.dev_alloc    = ibo_devalloc;
    taichi::ui::RenderableInfo renderable_info_0;
    renderable_info_0.vbo = vbo_info_0;
    renderable_info_0.indices = ibo_info_0;
    renderable_info_0.has_user_customized_draw = false;
    renderable_info_0.has_per_vertex_color = false;
    taichi::ui::MeshInfo meshes;
    meshes.renderable_info = renderable_info_0;
    meshes.color = glm::vec3(0.3, 0.5, 0.8);
    meshes.object_id = 0;
    taichi::ui::ParticlesInfo vertices;
    vertices.renderable_info = renderable_info_0;
    vertices.color = glm::vec3(0.9, 0.5, 0.8);
    vertices.radius = 0.1f;
    vertices.object_id = 0;


    auto camera = std::make_unique<taichi::ui::Camera>();
    camera->position = glm::vec3(0.0, 1.5, 1.5);
    camera->lookat = glm::vec3(0.0, 0.0, 0.0);
    camera->up = glm::vec3(0.0, 1.0, 0);


    // ti_launch_kernel(runtime, k_initialize_particles, 1, &k_initialize_particles_args[0]);
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
        ti_launch_kernel(runtime, k_data_copy, 3, &k_data_copy_args[0]);
        ti_wait(runtime);

        handle_user_inputs(camera.get(), window, win_width, win_height);

        scene->set_camera(*camera);
        scene->point_light(glm::vec3(2.0, 2.0, 2.0), glm::vec3(1.0, 1.0, 1.0));
        scene->mesh(meshes);
        // scene->particles(vertices);

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