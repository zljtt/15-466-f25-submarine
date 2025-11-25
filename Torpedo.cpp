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

    //tracking torpedoes(rotating the velocity vector) (Johnny's implementation)
    // for(auto p: game->connection_to_player){
        
    //     Player *playa = p.second;
    //     if(playa->id == owner) continue;

    //     //if the player is within a certain radius 
    //     glm::vec2 target = playa->position-position;
    //     float dist = glm::length(target);
    //     if((dist < 10.0f)){
    //         //depending on how close it is to the player, and how aligned is the torped
    //         //also the speed of the player(so that when a player is not moving, it doesn't get tracked)
            
    //         glm::vec2 velocity_unit =glm::normalize(velocity);
    //         glm::vec2 target_unit = glm::normalize(target);
    //         float cross_product = velocity.x * target_unit.y - velocity.y * target_unit.x;
    //         float sign = (cross_product > 0) ? 1.0f : -1.0f;
    //         //std::cout<<target_unit.x<<" "<<target_unit.y<<" "<<velocity_unit.x<<" "<<velocity_unit.y<<std::endl;
    //         float angle_to_player = sign * std::acos(glm::dot(target_unit, velocity_unit));

    //         // std::cout<<"dot "<<glm::dot(target_unit, velocity_unit)<<"acos "<<std::acos(glm::dot(target_unit, velocity_unit))<<std::endl;
    //         // std::cout<<"cross p: "<<cross_product<<" angle: "<<angle_to_player<<std::endl;

    //         //rotate towards that angle (manual 2d rotational matrix)
    //         float cos_theta = std::cos(angle_to_player * elapsed);
    //         float sin_theta = std::sin(angle_to_player * elapsed);

    //         glm::mat2 rot = glm::mat2(cos_theta,sin_theta,-sin_theta,cos_theta);
    //         velocity = rot * velocity;
    //         // std::cout<<"velocity vector length "<<glm::length(velocity)<<std::endl;
    //     }
    // }

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
