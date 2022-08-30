#include <iostream>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

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

int main(int argc, char *argv[]) {
    TestImporting("./data/Collision_lowmesh3.obj");
}