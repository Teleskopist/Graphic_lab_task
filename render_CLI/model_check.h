#pragma once
#include <vector>
#include <string>

//given a path to mesh (obj or vsgf) check if it is valid and 
//if good-quality SDF can be built from it
void check_model(const std::string &path);

//Validates models in dir
void check_models(const std::string &models_dir, const std::string &saves_dir);