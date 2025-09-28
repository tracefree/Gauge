#pragma once

#include <gauge/common.hpp>

namespace YAML {
class Node;
}

namespace Gauge {

class Node;
class StringID;

class Scene {
   private:
    Ref<YAML::Node> state;

   public:
    Ref<Node> FromData(YAML::Node p_data);
    Ref<Node> Instantiate();

    Scene(Ref<YAML::Node> p_state = nullptr) : state(p_state) {}

    // --- Resource interface ---
    static Scene Load(StringID p_id);
    void Unload();
};

}  // namespace Gauge