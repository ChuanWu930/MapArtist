#include <Evaluate/Evaluate.hpp>
#include <Finder/Finder.hpp>
#include <Goal/Goal.hpp>
#include <Weighted/Weighted.hpp>

#include "botcraft/AI/SimpleBehaviourClient.hpp"
#include "botcraft/AI/Tasks/PathfindingTask.hpp"
#include "botcraft/Game/Entities/EntityManager.hpp"
#include "botcraft/Game/Entities/LocalPlayer.hpp"
#include "botcraft/Game/World/World.hpp"
#include "botcraft/Utilities/Logger.hpp"

namespace pf = pathfinding;

template <template <typename, typename> class TFinder = pf::AstarFinder,
          class TWeight = pf::weight::ConstWeighted<2, 3>>
class BotCraftFinder final
    : public TFinder<BotCraftFinder<TFinder, TWeight>, TWeight> {
 public:
  virtual std::string getBlockNameImpl(const pf::Position& pos) const override {
    // get block information
    auto world = client->GetWorld();
    if (world->IsLoaded(Botcraft::Position{pos.x, pos.y, pos.z})) {
      const Botcraft::Blockstate* block =
          world->GetBlock(Botcraft::Position{pos.x, pos.y, pos.z});
      return (block != nullptr ? block->GetName() : "minecraft:air");
    } else {
      return "";
    }
  }

  virtual std::vector<std::string> getBlockNameImpl(
      const std::vector<pf::Position>& pos) const override {
    std::vector<Botcraft::Position> botcraftPos;
    std::transform(pos.begin(), pos.end(), std::back_inserter(botcraftPos),
                   [](const pf::Position& p) {
                     return Botcraft::Position{p.x, p.y, p.z};
                   });

    // get block information
    auto world = client->GetWorld();
    std::vector<const Botcraft::Blockstate*> blocks =
        world->GetBlocks(botcraftPos);

    std::vector<std::string> blockNames;
    for (int i = 0; i < pos.size(); ++i) {
      if (world->IsLoaded(botcraftPos[i])) {
        blockNames.push_back(
            (blocks[i] != nullptr ? blocks[i]->GetName() : "minecraft:air"));
      } else {
        blockNames.push_back("");
      }
    }

    return blockNames;
  }

  virtual inline float getFallDamageImpl(
      [[maybe_unused]] const pf::Position& landingPos,
      [[maybe_unused]] const typename pf::Position::value_type& height)
      const override {
    return 0.0;
  }

  virtual inline bool playerMoveImpl(const pf::Position& offset) override {
    std::shared_ptr<Botcraft::LocalPlayer> local_player =
        client->GetEntityManager()->GetLocalPlayer();

    pf::Vec3<double> targetPos, realOffset;
    {
      std::lock_guard<std::mutex> player_lock(local_player->GetMutex());
      pf::Vec3<double> now{local_player->GetPosition().x,
                           local_player->GetPosition().y,
                           local_player->GetPosition().z};
      targetPos = (now.floor() + offset)
                      .offset(0.5, 0, 0.5);  // stand in the middle of the block
      realOffset = targetPos - now;
      const auto lookAtPos = targetPos.offset(0.0, 1.62, 0.0);
      local_player->LookAt(
          Botcraft::Vector3<double>(lookAtPos.x, lookAtPos.y, lookAtPos.z),
          true);
    }

    // jump
    if (offset.y > 0) {
      std::lock_guard<std::mutex> player_lock(local_player->GetMutex());
      local_player->Jump();
    }

    const double speed = Botcraft::LocalPlayer::WALKING_SPEED;
    double norm = std::sqrt(realOffset.getXZ().template squaredNorm<double>());

    auto startTime = std::chrono::steady_clock::now(), preTime = startTime;
    pf::Vec3<double> total_v;
    while (true) {
      auto nowTime = std::chrono::steady_clock::now();
      const double elapsed_t = static_cast<double>(
          std::chrono::duration_cast<std::chrono::milliseconds>(nowTime -
                                                                startTime)
              .count());
      const double delta_t = static_cast<double>(
          std::chrono::duration_cast<std::chrono::milliseconds>(nowTime -
                                                                preTime)
              .count());
      pf::Vec3<double> delta_v =
          (static_cast<pf::Vec3<double>>(realOffset) / norm) *
          ((delta_t / 1000.0) * speed);
      {
        std::lock_guard<std::mutex> player_lock(local_player->GetMutex());
        if ((elapsed_t / 1000.0) > norm / speed) {
          local_player->SetX(targetPos.x);

          auto targetBlock = client->GetWorld()->GetBlock(
              Botcraft::Position{static_cast<int>(std::floor(targetPos.x)),
                                 static_cast<int>(std::floor(targetPos.y) - 1),
                                 static_cast<int>(std::floor(targetPos.z))});
          if (targetBlock->GetName().find("slab") != std::string::npos) {
            if (targetBlock->GetVariableValue("type") == "top") {
              local_player->SetY(targetPos.y);
            } else {
              local_player->SetY(targetPos.y + 0.5);  // down slab, y + 0.5
            }
          }

          local_player->SetZ(targetPos.z);
          break;
        } else {
          local_player->AddPlayerInputsX(delta_v.x);
          local_player->AddPlayerInputsZ(delta_v.z);
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      preTime = nowTime;
    }

    // wait for falling
    while (!local_player->GetOnGround()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    {
      std::lock_guard<std::mutex> player_lock(local_player->GetMutex());
      local_player->SetY(targetPos.y);
    }
    return true;
  }

  BotCraftFinder(std::shared_ptr<Botcraft::BehaviourClient> _client)
      : TFinder<BotCraftFinder<TFinder, TWeight>, TWeight>({true, 9999999}), client(_client) {}

 private:
  std::shared_ptr<Botcraft::BehaviourClient> client;
};