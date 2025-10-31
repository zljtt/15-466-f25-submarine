#include "Raycast.hpp"
#include "BBox.hpp"
#include "GameObject.hpp"
#include <algorithm>
#include <iostream>
#include <vector>

bool Node::is_leaf() const
{
    return l == r;
}

void BVH::build(std::vector<GameObject> &&prims, size_t max_leaf_size)
{
    nodes.clear();
    obstacles = std::move(prims);

    const int partitionPerAxis = 8;
    // Construct a BVH from the given vector of primitives and maximum leaf
    // size configuration.
    if (obstacles.size() == 0)
        return;

    auto compute_bbox_all = [&](auto begin, auto end)
    {
        SAHBucketData out{};
        for (auto it = begin; it != end; it++)
        {
            if (out.num_prims == 0)
            {
                // bbox_all = BBox(i->bbox().min, i->bbox().max);
                out.bb = it->get_BBox();
            }
            else
            {
                out.bb.enclose(it->get_BBox());
            }
            out.num_prims++;
        }
        return out;
    };

    auto find_best_partition = [&](auto &&self, SAHBucketData bucket_data, auto begin, auto end) -> size_t
    {
        // is leaf
        size_t n = static_cast<size_t>(end - begin);
        if (n <= max_leaf_size)
        {
            size_t start = static_cast<size_t>(begin - obstacles.begin());
            return new_node(bucket_data.bb, start, n, 0, 0);
        }

        float c_min = std::numeric_limits<float>::infinity();
        auto best_split = begin;

        for (int axis = 0; axis < 2; axis++)
        {
            float bucket_size = bucket_data.bb.max[axis] - bucket_data.bb.min[axis];
            if (!(bucket_size > 0.0f))
                continue;
            for (int split = 0; split < partitionPerAxis - 1; split++)
            {
                float left_bucket_size = bucket_size * float(split + 1) / float(partitionPerAxis);
                auto split_point = std::partition(begin, end, [&](const GameObject &p)
                                                  { return p.get_BBox().center()[axis] < left_bucket_size; });

                SAHBucketData bucket_left = compute_bbox_all(begin, split_point);
                SAHBucketData bucket_right = compute_bbox_all(split_point, end);

                if (bucket_left.num_prims == 0 || bucket_right.num_prims == 0)
                    break;

                float c = bucket_left.bb.surface_area() * bucket_left.num_prims + bucket_right.bb.surface_area() * bucket_right.num_prims;
                if (c < c_min)
                {
                    c_min = c;
                    best_split = split_point;
                }
            }
        }

        SAHBucketData bucket_left = compute_bbox_all(begin, best_split);
        SAHBucketData bucket_right = compute_bbox_all(best_split, end);
        if (bucket_left.num_prims == 0 || bucket_right.num_prims == 0)
        {
            // fall back, become leaf
            size_t start = static_cast<size_t>(begin - obstacles.begin());
            return new_node(bucket_data.bb, start, n, 0, 0);
        }

        size_t start = static_cast<size_t>(begin - obstacles.begin());
        size_t size = static_cast<size_t>(end - begin);
        // add left
        size_t left = self(self, bucket_left, begin, best_split);
        // add right
        size_t right = self(self, bucket_right, best_split, end);
        // add self
        size_t node_id = new_node(bucket_data.bb, start, size, left, right);
        return node_id;
    };
    // partition
    SAHBucketData bb_all = compute_bbox_all(obstacles.begin(), obstacles.end());
    root_idx = find_best_partition(find_best_partition, bb_all, obstacles.begin(), obstacles.end());
}

Trace BVH::hit(const Ray2D &ray) const
{
    Trace closest_hit;
    closest_hit.hit = false;
    closest_hit.distance = ray.dist_bounds.y;

    if (nodes.size() == 0)
        return closest_hit;

    auto find_closest_hit = [&](auto &&self, size_t n, glm::vec2 t) -> void
    {
        // std::cout << "Finding cloest hit on node " << n << "\n";
        if (t.x >= closest_hit.distance)
            return;
        t.y = std::min(t.y, closest_hit.distance);
        if (t.x > t.y)
            return;

        const Node &node = nodes[n];

        if (node.is_leaf())
        {
            // std::cout << "Node " << n << " is leaf\n";
            Ray2D local = ray;
            local.dist_bounds.x = std::max(local.dist_bounds.x, t.x);
            local.dist_bounds.y = std::min(local.dist_bounds.y, t.y);
            for (size_t i = 0; i < node.size; i++)
            {
                Trace trace = obstacles[node.start + i].hit(local);
                if (trace.hit && trace.distance < closest_hit.distance)
                {
                    closest_hit = trace;
                    local.dist_bounds.y = closest_hit.distance;
                }
            }
            return;
        }
        else
        {
            // std::cout << "Node " << n << " is parent\n";
            const Node &left = nodes[node.l];
            const Node &right = nodes[node.r];
            glm::vec2 trace_left = t;
            glm::vec2 trace_right = t;

            bool hit_left = left.bbox.hit(ray, trace_left);
            bool hit_right = right.bbox.hit(ray, trace_right);

            // clamp trace
            if (!hit_left && !hit_right)
                return;
            if (hit_left)
                trace_left.y = std::min(trace_left.y, closest_hit.distance);
            if (hit_right)
                trace_right.y = std::min(trace_right.y, closest_hit.distance);

            if (hit_left && !hit_right)
            {
                if (trace_left.x < closest_hit.distance)
                {
                    // std::cout << "- Only hit left\n";
                    self(self, node.l, trace_left);
                }
                return;
            }
            else if (hit_right && !hit_left)
            {
                if (trace_right.x < closest_hit.distance)
                {
                    // std::cout << "- Only hit right\n";
                    self(self, node.r, trace_right);
                }
                return;
            }
            else if (hit_right && hit_left)
            {
                // std::cout << "- Hit both left and right\n";
                size_t first = node.l;
                size_t second = node.r;
                glm::vec2 trace_first = trace_left;
                glm::vec2 trace_second = trace_right;
                if (trace_first.x < trace_second.x)
                {
                    std::swap(first, second);
                    std::swap(trace_first, trace_second);
                }

                if (trace_first.x < closest_hit.distance)
                {
                    self(self, first, trace_first);
                }
                if (trace_second.x < closest_hit.distance)
                {
                    trace_second.y = std::min(trace_second.y, closest_hit.distance);
                    if (trace_second.x <= trace_second.y)
                    {
                        self(self, second, trace_second);
                    }
                }
            }
        }
    };

    find_closest_hit(find_closest_hit, root_idx, ray.dist_bounds);
    return closest_hit;
}
size_t BVH::new_node(BBox box, size_t start, size_t size, size_t l, size_t r)
{
    Node n;
    n.bbox = box;
    n.start = start;
    n.size = size;
    n.l = l;
    n.r = r;
    nodes.push_back(n);
    return nodes.size() - 1;
}