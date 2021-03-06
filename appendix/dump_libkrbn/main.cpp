#include "../../src/lib/libkrbn/include/libkrbn.h"
#include <CoreFoundation/CoreFoundation.h>
#include <iostream>
#include <thread>

namespace {
void hid_value_observer_callback(libkrbn_hid_value_type type,
                                 uint32_t value,
                                 libkrbn_hid_value_event_type event_type,
                                 void* refcon) {
  char buffer[256];

  switch (type) {
    case libkrbn_hid_value_type_key_code:
      libkrbn_get_key_code_name(buffer, sizeof(buffer), value);
      std::cout << "hid_value_observer_callback"
                << " " << buffer
                << " event_type:" << event_type
                << std::endl;
      break;

    case libkrbn_hid_value_type_consumer_key_code:
      libkrbn_get_consumer_key_code_name(buffer, sizeof(buffer), value);
      std::cout << "hid_value_observer_callback"
                << " " << buffer
                << " event_type:" << event_type
                << std::endl;
      break;
  }
}
} // namespace

int main(int argc, const char* argv[]) {
  libkrbn_initialize();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  {
    libkrbn_complex_modifications_assets_manager* manager = nullptr;
    if (libkrbn_complex_modifications_assets_manager_initialize(&manager)) {
      libkrbn_complex_modifications_assets_manager_reload(manager);

      {
        auto size = libkrbn_complex_modifications_assets_manager_get_files_size(manager);
        std::cout << "libkrbn_complex_modifications_assets_manager_get_files_size: " << size << std::endl;

        for (size_t i = 0; i < size; ++i) {
          std::cout << "  " << libkrbn_complex_modifications_assets_manager_get_file_title(manager, i) << std::endl;

          auto rules_size = libkrbn_complex_modifications_assets_manager_get_file_rules_size(manager, i);
          std::cout << "    libkrbn_complex_modifications_assets_manager_get_file_rules_size: " << rules_size << std::endl;

          for (size_t j = 0; j < rules_size; ++j) {
            std::cout << "      " << libkrbn_complex_modifications_assets_manager_get_file_rule_description(manager, i, j) << std::endl;
          }
        }
      }

      libkrbn_complex_modifications_assets_manager_terminate(&manager);
    }
  }

  libkrbn_hid_value_observer* observer = nullptr;
  libkrbn_hid_value_observer_initialize(&observer, hid_value_observer_callback, nullptr);

  std::cout << std::endl;
  for (int i = 0; i < 10; ++i) {
    std::cout << "." << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::cout << std::endl;

  std::cout << "observed_device_count: "
            << libkrbn_hid_value_observer_calculate_observed_device_count(observer)
            << std::endl;

  CFRunLoopRun();

  libkrbn_hid_value_observer_terminate(&observer);

  std::cout << std::endl;

  libkrbn_terminate();

  return 0;
}
