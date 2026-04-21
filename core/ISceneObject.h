#pragma once

#include <cstdint>
#include <cstddef>
#include <unordered_set>
#include <memory>

#include "LiteMath.h"
#include "../CrossRT/CrossRT.h" //slicer requires relative path to CrossRT.h from LiteRT root
#include "core/render_settings.h"

std::shared_ptr<ISceneObject> CreateSceneRT(const char* a_implName, const char* a_buildName, const char* a_layoutName);
