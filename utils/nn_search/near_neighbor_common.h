#pragma once
#include "utils/common/aligned_allocator.h"
#include <functional>
#include <math.h>
#include <string>
#include <vector>
#include <cstdint>

namespace nn_search
{
  using aligned_vector_f = std::vector<float, AlignmentAllocator<float, 64>>;
  struct DataPoint
  {
    uint32_t original_id;
    size_t   data_offset;
    uint32_t transform_id;
    float    average_val;
  };

  struct Dataset
  {
    unsigned dim;
    aligned_vector_f all_points; //R^n vector for each data point
    std::vector<DataPoint> data_points;
  };

  void save_dataset(const Dataset &dataset, const std::string &filename);
  void load_dataset(const std::string &filename, Dataset &dataset);

  /// Callback function for near neighbor search
  /// @param dist distance to point
  /// @param idx index of point in dataset
  /// @param point point info
  /// @param data  N-dimensional vector
  /// @return finish should we finish traversal or not
  using ScanFunction = std::function<bool(float dist, unsigned idx, const DataPoint &, const float *)>;

  //Interface for acceleration structure for near neighbor search
  class INNSearchAS
  {
  public:
    virtual ~INNSearchAS() = default;

    /// Build acceleration structure. Dataset is copied inside acceleration structure
    //  Max leaf size can mean nothing for some acceleration structures
    virtual void build(const Dataset &dataset, int max_leaf_size) = 0; 

    /// Get closest point to query within max_dist
    //  Returns nullptr if no point is found
    //  writes to *dist_to_nearest distance to closest point
    virtual const float *get_closest_point(const float *query, float max_dist, float *dist_to_nearest) const = 0;

    /// Scan all points within max_dist
    //  Returns number of points found
    virtual int scan_near(const float *query, float max_dist, ScanFunction callback) const = 0;
  };

  /// Interface for distance function to be used in near neighbor search
  /// User must set properly set the type of distance function, as it affects
  /// what acceleration structures can be used with it. Any AS will exhibit
  /// undefined behavior if distance function is not symmetric or negative.
  class IDistanceFunction
  {
    public:
      // Type of distance, determines what acceleration structure can be used
      // For L1, L2, LInf faster versions of near neighbor search are implemented
      // If Type is CustomMetric, distance function met metric definition
      // If Type is CustomDistance, distance function must met distance definition
      enum class Type
      {
        L1,            // |x1 - x2| + |y1 - y2|
        L2,            // sqrt((x1 - x2)^2 + (y1 - y2)^2)
        LInf,          // max(|x1 - x2|, |y1 - y2|)
        CustomMetric,  // any other metric (i.e. triangle inequality required)
        CustomDistance // arbitrary distance
      };
      virtual ~IDistanceFunction() = default;
      virtual Type getType() = 0;
      bool isMetric() { return getType() <= Type::CustomMetric; }

      // Distance function
      virtual float calc(uint32_t dim, const float *a, const float *b) = 0;

      // Calculate truncated distance. If dist < max_dist, returns dist,
      // otherwise returns arbitrary value >= max_dist. In many cases, this
      // can be faster than calc(a, b).
      virtual float calcTruncated(uint32_t dim, const float *a, const float *b, float max_dist) { return calc(dim, a, b); }
  };

  class ILossFunction
  {
    public:
      virtual ~ILossFunction() = default;
      virtual bool preserveMetric() const { return false; }
      virtual float calc(uint32_t dim, const float *a) = 0;
  };
}