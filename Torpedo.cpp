#include "Game.hpp"

#include "Connection.hpp"
#include "PlayMode.hpp"
#include "Scene.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring>

#include "BBox.hpp"

void Torpedo::init()
{
    NetworkObject::init();
    type = ObjectType::Torpedo;
    scale = glm::vec2(0.5f, 0.5f);
    tracking = false;
    age = 0;
}

int Torpedo::can_collide(const NetworkObject *other) const
{
    if (other->id == this->id)
        return 0;
    // don't collide with owner
    if (other->id == this->owner)
        return 0;
    if (other->type == ObjectType::SubmitPoint || other->type == ObjectType::Flag)
        return 0;
    return 1;
}

void Torpedo::update(float elapsed, Game *game)
{
    Player *target = nullptr;
    float best_dist2 = std::numeric_limits<float>::max();

    for (auto *p : game->get_objects<Player>())
    {
        glm::vec2 to_player = p->position - position;
        float d2 = dot(to_player, to_player);
        if (d2 < best_dist2)
        {
            best_dist2 = d2;
            target = p;
        }
    }

    if (target)
    {
        glm::vec2 to_target = target->position - position;

        if (dot(to_target, to_target) > 0.0001f && dot(velocity, velocity) > 0.0001f)
        {
            glm::vec2 desired_dir = normalize(to_target);
            glm::vec2 current_dir = normalize(velocity);

            const float homing_strength = 2.0f;
            float t = homing_strength * elapsed;
            if (t > 1.0f)
                t = 1.0f;

            glm::vec2 new_dir = normalize(current_dir * (1.0f - t) + desired_dir * t);

            float speed = length(velocity);
            velocity = new_dir * speed;
        }
    }

    auto hits = move_with_collision(game, velocity * elapsed);

    age += elapsed;
    if (age > TORPEDO_LIFETIME)
    {
        deleted = true;
    }
    auto player_hit = get_colliders<Player>(hits);
    if (player_hit)
    {
        // PLAY SOUND : torpeto hit
        player_hit->take_damage(game, TORPEDO_DAMAGE, this);
    }
    if (hits.size() > 0)
    {
        deleted = true;
    }
}
