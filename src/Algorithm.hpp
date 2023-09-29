#include "botcraft/AI/BehaviourClient.hpp"
#include <string>

void SimpleBFS(Botcraft::BehaviourClient& c);
void SimpleDFS(Botcraft::BehaviourClient& c);
void SliceDFS(Botcraft::BehaviourClient& c);

// Used for material collecting optimization
struct MaterialCompare {
  bool operator()(const std::string& lhs, const std::string& rhs) const {
    if (lhs.find("_wool") != std::string::npos && rhs.find("_wool") != std::string::npos) {
      // If both have "_wool", sort by default
      return lhs < rhs;
    } else if (lhs.find("_wool") != std::string::npos) {
      // only left has "_wool", put at front
      return true;
    } else if (rhs.find("_wool") != std::string::npos) {
      // only right has "_wool", put at front
      return false;
    } else if (lhs.find("_terracotta") != std::string::npos && rhs.find("_terracotta") != std::string::npos) {
      return lhs < rhs;
    } else if (lhs.find("_terracotta") != std::string::npos) {
      return true;
    } else if (rhs.find("_terracotta") != std::string::npos) {
      return false;
    } else if (lhs.find("_block") != std::string::npos && rhs.find("_block") != std::string::npos) {
      return lhs < rhs;
    } else if (lhs.find("_block") != std::string::npos) {
      return true;
    } else if (rhs.find("_block") != std::string::npos) {
      return false;
    } else if (lhs.find("_planks") != std::string::npos && rhs.find("_planks") != std::string::npos) {
      return lhs < rhs;
    } else if (lhs.find("_planks") != std::string::npos) {
      return true;
    } else if (rhs.find("_planks") != std::string::npos) {
      return false;
    } else {
      // sort by default
      return lhs < rhs;
    }
  }
};