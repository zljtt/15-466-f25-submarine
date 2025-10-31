
#include "BBox.hpp"
#include "Raycast.hpp"
#include "GameObject.hpp"

// copied and modified from Scotty3D (from zhijianw)
bool BBox::hit(const Ray2D &ray, glm::vec2 &times) const
{
    float t0 = times.x;
    float t1 = times.y;

    auto hit_in_axis = [&](float o, float d, float min, float max) -> bool
    {
        if (std::abs(d) < 1e-8f)
        {
            if (o < min || o > max)
                return false;
            return true;
        }

        float tmin = (min - o) / d;
        float tmax = (max - o) / d;
        if (tmin > tmax)
            std::swap(tmin, tmax);

        t0 = std::max(tmin, t0);
        t1 = std::min(tmax, t1);

        return t1 >= t0;
    };

    if (!hit_in_axis(ray.point.x, ray.dir.x, min.x, max.x))
        return false;
    if (!hit_in_axis(ray.point.y, ray.dir.y, min.y, max.y))
        return false;

    times.x = t0;
    times.y = t1;

    return true;
}
