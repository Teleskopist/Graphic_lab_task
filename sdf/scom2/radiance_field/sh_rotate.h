#pragma once
#include "sh_compute.h"

// calculates Spherical Harmonics rotate matrices for an arbitrary rotation
void calculate_sh_rotate_matrix(uint32_t deg, LiteMath::float4x4 rot, std::vector<float> &out_mat);

// calculates Spherical Harmonics rotate matrices for 48 coded rotations
void calculate_and_print_all_sh_rotate_matrices();