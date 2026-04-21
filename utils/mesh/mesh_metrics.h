#pragma once
#include "utils/mesh/mesh.h"

#ifdef USE_LIBIGL
#include <Eigen/Core>
#include <igl/hausdorff.h>
#include <igl/point_mesh_squared_distance.h>
#include <igl/random_points_on_mesh.h>
#endif

namespace cmesh4
{
#ifdef USE_LIBIGL
  void SimpleMesh_to_eigen(const SimpleMesh& mesh,
                           Eigen::MatrixXd& V, Eigen::MatrixXi& F)
  {
    size_t N = mesh.VerticesNum();
    V.resize(N, 3);
    for (size_t i = 0; i < N; ++i)
    {
      V(i, 0) = mesh.vPos4f[i].x;
      V(i, 1) = mesh.vPos4f[i].y;
      V(i, 2) = mesh.vPos4f[i].z;
    }
    
    size_t M = mesh.TrianglesNum();
    F.resize(M, 3);
    for (size_t i = 0; i < M; ++i)
    {
      F(i, 0) = mesh.indices[3*i + 0];
      F(i, 1) = mesh.indices[3*i + 1];
      F(i, 2) = mesh.indices[3*i + 2];
    }
  }

  Eigen::MatrixXd sample_mesh_random(const Eigen::MatrixXd& V, const Eigen::MatrixXi& F, int n_samples)
  {
    srand(time(NULL));
    Eigen::MatrixXd B;
    Eigen::VectorXi FI;
    Eigen::MatrixXd X;
    igl::random_points_on_mesh(n_samples, V, F, B, FI, X);
    
    /*Eigen::MatrixXd P(B.rows() + V.rows(), 3);
    for (int i = 0; i < B.rows(); ++i)
    {
      int f_idx = FI(i);
      P.row(i) = B(i, 0)*V.row(F(f_idx, 0)) + 
                 B(i, 1)*V.row(F(f_idx, 1)) + 
                 B(i, 2)*V.row(F(f_idx, 2));
    }
    for (int i = 0; i < V.rows(); ++i)
    {
      P.row(B.rows() + i) = V.row(i);
    }
    return P;*/
    return X;
  }
#endif

  double chamfer_distance(cmesh4::SimpleMesh a, cmesh4::SimpleMesh b, int samples = 1000000)
  {
#ifdef USE_LIBIGL
    Eigen::MatrixXd V1, V2;
    Eigen::MatrixXi F1, F2;

    SimpleMesh_to_eigen(a, V1, F1);
    SimpleMesh_to_eigen(b, V2, F2);

    Eigen::VectorXd sqrD2;
    Eigen::MatrixXi I2;
    Eigen::MatrixXd C2;
    int num2 = V2.rows();
    {
      Eigen::MatrixXd V = sample_mesh_random(V2, F2, samples);
      num2 = V.rows();
      igl::point_mesh_squared_distance(V, V1, F1, sqrD2, I2, C2);
    }

    return sqrD2.cwiseSqrt().sum() / num2;

    // Eigen::VectorXd sqrD1;
    // Eigen::MatrixXi I1;
    // Eigen::MatrixXd C1;
    // int num1 = V1.rows();
    // {
    //   Eigen::MatrixXd V = sample_mesh_random(V1, F1, samples);
    //   num1 = V.rows();
    //   igl::point_mesh_squared_distance(V, V2, F2, sqrD1, I1, C1);
    // }

    // float f1 = sqrD1.cwiseSqrt().sum() / num1;
    // float f2 = sqrD2.cwiseSqrt().sum() / num2;
    // return 0.5f*(f1 + f2);
#endif
    printf("WARNING: LIBIGL is DISABLED and chamfer distance will return -1.0\n");
    return -1.0;
  }

  double chamfer_recounted(cmesh4::SimpleMesh a, cmesh4::SimpleMesh b, double bbox_side = 2.0, double multiply = 1000.0)
  {
    auto res = chamfer_distance(a, b);
    return res * multiply / (bbox_side * std::sqrt(3));
  }

  double chamfer_bbox(cmesh4::SimpleMesh a, cmesh4::SimpleMesh b, double multiply = 1000.0)
  {
    float3 min_a, max_a, min_b, max_b;
    cmesh4::get_bbox(a, &min_a, &max_a);
    cmesh4::get_bbox(b, &min_b, &max_b);
    float3 min_p = min(min_a, min_b);
    float3 max_p = max(max_a, max_b);

    auto res = chamfer_distance(a, b);
    return res * multiply / (length(max_p - min_p));
  }

  double hausdorff_distance(cmesh4::SimpleMesh a, cmesh4::SimpleMesh b)
  {
#ifdef USE_LIBIGL
    Eigen::MatrixXd V1, V2;
    Eigen::MatrixXi F1, F2;

    SimpleMesh_to_eigen(a, V1, F1);
    SimpleMesh_to_eigen(b, V2, F2);

    double h1;
    igl::hausdorff(V1, F1, V2, F2, h1);
    double h2;
    igl::hausdorff(V2, F2, V1, F1, h2);

    return std::max(h1, h2);
#endif
    printf("WARNING: LIBIGL is DISABLED and hausdorff distance will return -1.0\n");
    return -1.0;
  }
};