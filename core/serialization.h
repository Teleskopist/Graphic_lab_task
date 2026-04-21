#pragma once
#include "blk/blk.h"
#include "core/render_settings.h"
#include "utils/misc/camera.h"
#include "utils/mesh/mesh.h"
#include "core/IRenderer.h"

// forces the caller to include serialization.o translation unit 
// and therefore register enums declared there. It is useful for
// some executable that work with blks and have to reason about
// them, but do not use the actual data (e.g. benchmark, testing).
extern void force_register_common_enums();

void load_from_blk(MultiRenderPreset &preset, const Block *block);
void save_to_blk(const MultiRenderPreset &preset, Block *block);

void load_from_blk(Camera &camera, const Block *block);
void save_to_blk(const Camera &camera, Block *block);

void load_from_blk(cmesh4::PreprocessSettings &settings, const Block *block);
void save_to_blk(const cmesh4::PreprocessSettings &settings, Block *block);

void load_from_blk(Light &light, const Block *block);
void save_to_blk(const Light &light, Block *block);