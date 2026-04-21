#include "LiteMath.h"
#include <array>
#include <optional>

namespace rotated_bbox
{

    using LiteMath::float3;
    using LiteMath::float4;
    using LiteMath::float4x4;

    struct TransformedBBox
    {
        float4 rotate;
        float3 center;
        float3 half_size;
    };

    TransformedBBox compute_bbox_of_points(const std::vector<float3> &points);

    /*
        Computes bbox for
        {x[i] / w[i], y[i] / w[i], z[i] / w[i]}
    */
    TransformedBBox compute_bbox_of_weighted_points(const std::vector<float4> &points);

}
