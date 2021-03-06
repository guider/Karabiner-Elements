#pragma once

// `krbn::device_observer` can be used safely in a multi-threaded environment.

#include "boost_defs.hpp"

#include "grabbable_state_manager.hpp"
#include "grabber_client.hpp"
#include "hid_manager.hpp"
#include "hid_observer.hpp"
#include "logger.hpp"
#include "time_utility.hpp"
#include "types.hpp"
#include <pqrs/dispatcher.hpp>
#include <pqrs/osx/iokit_types.hpp>

namespace krbn {
class device_observer final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  device_observer(const device_observer&) = delete;

  device_observer(std::weak_ptr<grabber_client> grabber_client) : dispatcher_client(),
                                                                  grabber_client_(grabber_client) {
    // grabbable_state_manager_

    grabbable_state_manager_ = std::make_unique<grabbable_state_manager>();

    grabbable_state_manager_->grabbable_state_changed.connect([this](auto&& grabbable_state) {
      if (auto client = grabber_client_.lock()) {
        client->async_grabbable_state_changed(grabbable_state);
      }
    });

    // hid_manager_

    std::vector<std::pair<krbn::hid_usage_page, krbn::hid_usage>> targets({
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_keyboard),
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_mouse),
        std::make_pair(hid_usage_page::generic_desktop, hid_usage::gd_pointer),
        std::make_pair(krbn::hid_usage_page::leds, krbn::hid_usage::led_caps_lock),
    });

    hid_manager_ = std::make_unique<hid_manager>(targets);

    hid_manager_->device_detecting.connect([](auto&& registry_entry_id, auto&& device) {
      iokit_utility::log_matching_device(registry_entry_id, device);

      return true;
    });

    hid_manager_->device_detected.connect([this](auto&& weak_hid) {
      if (auto hid = weak_hid.lock()) {
        logger::get_logger().info("{0} is detected.", hid->get_name_for_log());

        grabbable_state_manager_->update(grabbable_state(hid->get_registry_entry_id(),
                                                         grabbable_state::state::device_error,
                                                         grabbable_state::ungrabbable_temporarily_reason::none,
                                                         time_utility::mach_absolute_time_point()));

        if (hid->is_karabiner_virtual_hid_device()) {
          // Handle caps_lock_state_changed event only if the hid is Karabiner-VirtualHIDDevice.
          hid->values_arrived.connect([this](auto&& shared_event_queue) {
            for (const auto& e : shared_event_queue->get_entries()) {
              if (e.get_event().get_type() == event_queue::event::type::caps_lock_state_changed) {
                if (auto client = grabber_client_.lock()) {
                  if (auto state = e.get_event().get_integer_value()) {
                    client->async_caps_lock_state_changed(*state);
                  }
                }
              }
            }
          });
        } else {
          hid->values_arrived.connect([this](auto&& shared_event_queue) {
            grabbable_state_manager_->update(*shared_event_queue);
          });
        }

        auto observer = std::make_shared<hid_observer>(hid);

        observer->device_observed.connect([this, weak_hid] {
          if (auto hid = weak_hid.lock()) {
            logger::get_logger().info("{0} is observed.",
                                      hid->get_name_for_log());

            if (auto state = grabbable_state_manager_->get_grabbable_state(hid->get_registry_entry_id())) {
              // Keep grabbable_state if the state is already changed by value_callback.
              if (state->get_state() == grabbable_state::state::device_error) {
                grabbable_state_manager_->update(grabbable_state(hid->get_registry_entry_id(),
                                                                 grabbable_state::state::grabbable,
                                                                 grabbable_state::ungrabbable_temporarily_reason::none,
                                                                 time_utility::mach_absolute_time_point()));
              }
            }
          }
        });

        observer->async_observe();

        hid_observers_[hid->get_registry_entry_id()] = observer;
      }
    });

    hid_manager_->device_removed.connect([this](auto&& weak_hid) {
      if (auto hid = weak_hid.lock()) {
        logger::get_logger().info("{0} is removed.", hid->get_name_for_log());

        hid_observers_.erase(hid->get_registry_entry_id());
      }
    });

    hid_manager_->async_start();

    logger::get_logger().info("device_observer is started.");
  }

  virtual ~device_observer(void) {
    detach_from_dispatcher([this] {
      hid_manager_ = nullptr;

      hid_observers_.clear();

      grabbable_state_manager_ = nullptr;
    });

    logger::get_logger().info("device_observer is stopped.");
  }

private:
  std::weak_ptr<grabber_client> grabber_client_;

  std::unique_ptr<hid_manager> hid_manager_;
  std::unordered_map<pqrs::osx::iokit_registry_entry_id, std::shared_ptr<hid_observer>> hid_observers_;
  std::unique_ptr<grabbable_state_manager> grabbable_state_manager_;
};
} // namespace krbn
