#include "TerrainRasterizer.h"
#include "utils/misc/terrain.h"
#include "LiteScene/scene.h"
#include <filesystem>
#include "stb_image.h"
namespace demo_app
{

    std::filesystem::path CreateCachedTerrainMesh(const std::vector<size_t> &params)
    {
        std::filesystem::path cache_dir = "./saves/terrain-cache";
        std::string scene_name;
        for (auto p : params)
        {
            scene_name.append(std::to_string(p));
            scene_name.append("_");
        }
        std::filesystem::path scene_dir = cache_dir / scene_name;
        std::filesystem::path scene_file = scene_dir / "scene.xml";
        std::filesystem::path scene_data_dir = scene_dir / "data";
        if (std::filesystem::exists(scene_dir))
        {
            std::cout << "Found cached terrain mesh " << scene_file << std::endl;
            return scene_file;
        }

        std::cout << "Creating terrain mesh " << scene_file << std::endl;

        auto mesh = terrain::create_mesh(params);
        LiteScene::HydraScene scene;
        uint32_t geomId = scene.add_mesh(mesh);
        uint32_t instId = scene.add_instance(geomId, {});

        std::filesystem::create_directories(scene_dir);
        scene.save(scene_file.string(), scene_data_dir.string());

        return scene_file;
    }

    void TerrainRasterizer::LoadScene(const std::string &path)
    {

        std::string s = CreateCachedTerrainMesh({1000}).string();
        // Somehow backslashes break SceneManager...
        for (char &c : s)
            if (c == '\\')
                c = '/';
        SimpleRasterizer::LoadScene(s);

        if (path == "test.png")
        {
            constexpr size_t N = 1024;
            constexpr size_t w = N;
            constexpr size_t h = N;

            std::vector<float> data(w * h);

            for (size_t j = 0; j < h; j++)
            {
                for (size_t i = 0; i < w; i++)
                {
                    float x = (float)i / w;
                    float y = (float)j / h;
                    data[i + j * w] = (x * x * sin(30 * y) + 1) / 2;
                }
            }

            LoadHeightMap(data.data(), w, h);
        }
        else
        {

            int w, h, c;
            float *data = stbi_loadf(path.c_str(), &w, &h, &c, 1);
            if (!data)
            {
                printf("Failed to load height map image %s\n", path.c_str());
                exit(1);
            }
            LoadHeightMap(data, w, h);
            stbi_image_free(data);
        }
    }

}
