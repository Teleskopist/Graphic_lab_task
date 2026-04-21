

include(kslicer)

#
# VolumeRenderer
#
kslicer_add_config(
    config/slicer/options_volume_comp.json
)

#
# VoxelRenderer
#
kslicer_add_config(
    config/slicer/options_voxel_comp.json
)

#
# MultiRenderer
#
kslicer_add_config(
    config/slicer/options_comp.json
)

kslicer_add_config(
    config/slicer/options_rq.json
)

#
# PostEffects::image_metrics
#
kslicer_add_config(
    config/slicer/options_metrics.json
)

#
# PostEffects::antialiaser
#
kslicer_add_config(
    config/slicer/options_aa.json
)

#
# PostEffects::denoiser
#
kslicer_add_config(
    config/slicer/options_denoiser.json
)