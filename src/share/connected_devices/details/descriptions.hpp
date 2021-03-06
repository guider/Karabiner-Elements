#pragma once

#include "json_utility.hpp"

namespace krbn {
namespace connected_devices {
namespace details {
class descriptions {
public:
  descriptions(const std::string& manufacturer,
               const std::string& product) : manufacturer_(manufacturer),
                                             product_(product) {
  }
  descriptions(const nlohmann::json& json) {
    if (auto v = json_utility::find_optional<std::string>(json, "manufacturer")) {
      manufacturer_ = *v;
    }

    if (auto v = json_utility::find_optional<std::string>(json, "product")) {
      product_ = *v;
    }
  }

  nlohmann::json to_json(void) const {
    return nlohmann::json({
        {"manufacturer", manufacturer_},
        {"product", product_},
    });
  }

  const std::string& get_manufacturer(void) const {
    return manufacturer_;
  }

  const std::string& get_product(void) const {
    return product_;
  }

  bool operator==(const descriptions& other) const {
    return manufacturer_ == other.manufacturer_ &&
           product_ == other.product_;
  }
  bool operator!=(const descriptions& other) const {
    return !(*this == other);
  }

private:
  std::string manufacturer_;
  std::string product_;
};

inline void to_json(nlohmann::json& json, const descriptions& descriptions) {
  json = descriptions.to_json();
}
} // namespace details
} // namespace connected_devices
} // namespace krbn
