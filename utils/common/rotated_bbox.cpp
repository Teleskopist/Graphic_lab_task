#include "rotated_bbox.h"
#include "utils/common/linalg3d.h"
#include "utils/common/quaternion.h"

namespace rotated_bbox
{

    using namespace LiteMath;

    double3 point(float3 v)
    {
        return {v.x, v.y, v.z};
    }

    double3 point(float4 v)
    {
        return {double(v.x) / v.w, double(v.y) / v.w, double(v.z) / v.w};
    }

    template <typename V>
    double3 compute_points_average(const V &points)
    {
        double3 avg = {0, 0, 0};
        for (auto p : points)
        {
            avg += point(p);
        }
        return avg / points.size();
    }

    template <typename V>
    double3x3 compute_covariance_matrix(const V &points, double3 average)
    {
        double3x3 c{};
        c.zero();
        for (size_t i = 0; i < 3; i++)
        {
            for (size_t j = 0; j < 3; j++)
            {
                for (size_t k = 0; k < points.size(); k++)
                {
                    c(i, j) += (point(points[k])[i] - average[i]) * (point(points[k])[j] - average[j]);
                }
                c(i, j) /= points.size() - 1;
            }
        }
        return c;
    }

    template <typename T>
    std::pair<double3x3, double3> compute_bbox_transform(const T &points)
    {
        double3 average = compute_points_average(points);
        double3x3 covariance_matrix = compute_covariance_matrix(points, average);
        auto eigenvectors = linalg3d::compute_eigenvectors(covariance_matrix);
        for (auto &i : eigenvectors)
            i = normalize(i);
        auto basis = linalg3d::complete_basis(linalg3d::make_linearly_independent(eigenvectors));
        for (auto &i : basis)
            i = normalize(i);
        auto ortho_basis = linalg3d::reorthogonalize(basis).to_array();

        double3x3 inverted_rotation;
        for (size_t i = 0; i < 3; i++)
        {
            for (size_t j = 0; j < 3; j++)
            {
                inverted_rotation(i, j) = ortho_basis[j][i];
            }
        }
        // Matrix is symmetric
        return {transpose(inverted_rotation), -average};
    }

    template <typename T>
    TransformedBBox compute_bbox(const T &points)
    {
        TransformedBBox out;

        auto [rotation, translate] = compute_bbox_transform(points);

        double4 q_rotation = quaternion_from_rotation_matrix(rotation);
        if (q_rotation.w < 0)
            q_rotation = -q_rotation;

        double3 q_xyz = to_double3(q_rotation);
        q_rotation.w = std::sqrt(std::abs(1.0 - dot(q_xyz, q_xyz)));

        out.rotate = float4(q_rotation.x, q_rotation.y, q_rotation.z, q_rotation.w);

        double3 box_min = quaternion_rotate_vector(q_rotation, point(points[0]) + translate);
        double3 box_max = box_min;

        for (size_t i = 1; i < points.size(); i++)
        {
            double3 v = quaternion_rotate_vector(q_rotation, point(points[i]) + translate);
            box_min = min(box_min, v);
            box_max = max(box_max, v);
        }

        double3 box_center = (box_max + box_min) / 2.0;

        double3 center = quaternion_rotate_vector(quaternion_conjugate(q_rotation), box_center) - translate;
        out.center = float3(center.x, center.y, center.z);

        double3 half_size = {0, 0, 0};

        for (auto p : points)
        {
            half_size = max(half_size, abs(quaternion_rotate_vector(q_rotation, point(p) - center)));
        }

        out.half_size = float3(half_size.x, half_size.y, half_size.z);

        return out;
    }

    TransformedBBox compute_bbox_of_points(const std::vector<float3> &points)
    {
        return compute_bbox(points);
    }

    TransformedBBox compute_bbox_of_weighted_points(const std::vector<float4> &points)
    {
        return compute_bbox(points);
    }

}
