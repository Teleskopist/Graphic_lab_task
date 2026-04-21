#include "bvh_info.h"
#include <queue>

namespace bvh_info
{

    GeomNodes get_geometry_nodes(const BVHRT *bvh, uint32_t geomId)
    {
        GeomData geom = bvh->m_geomData[geomId];
        uint32_t bvhOffset = geom.bvhOffset;

        BVHNodePair root = bvh->m_allNodePairs[bvhOffset];

        GeomNodes out;

        struct Q
        {
            BVHNode node;
            uint32_t layer;
        };

        std::queue<Q> q;

        q.push({root.left, 0});
        q.push({root.right, 0});

        while (!q.empty())
        {
            auto [node, layer] = q.front();
            q.pop();
            if (!(node.leftOffset & LEAF_BIT))
            {
                auto [left, right] = bvh->m_allNodePairs[bvhOffset + node.leftOffset];
                q.push({left, layer + 1});
                q.push({right, layer + 1});
            }
            if (out.nodes.size() <= layer)
            {
                out.nodes.resize(layer + 1);
            }
            out.nodes[layer].push_back(node);
        }
        return out;
    }

}
