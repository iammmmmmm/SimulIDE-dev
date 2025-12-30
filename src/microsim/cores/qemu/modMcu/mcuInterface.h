#ifndef SIMULIDE_DEV_MCUINTERFACE_H
#define SIMULIDE_DEV_MCUINTERFACE_H
#include <cstdint>
extern "C" {
  struct McuConfig {
    const char *packageFilePath;
    const char *itemName;
    const char *iconName;
    const char *type;
  };

  McuConfig mcu_get_config(const char *type, const char *id);
  void mcu_run_to_time(uint64_t timePs);
  void mcu_stamp();
  void mcu_initialize();
  void mcu_on_pin_changed(int pinIndex, double voltage);
  void mcu_on_scheduled_event();
  // void mcu_on_scheduled_event(uint64_t currentTimePs);
  typedef void (*SetPinVoltFunc)(void* ctx, int pinIndex, double voltage);
  typedef void (*AddEventFunc)(void* ctx, uint64_t timePs);
  //typedef void (*PinVoltCallback)(void* ctx, uint64_t timePs, int pinIndex, double voltage);
  struct ModMcuHost {
    void* context;
    SetPinVoltFunc setVoltage;
    AddEventFunc addEvent;
   // PinVoltCallback pinVoltCallback;
  };
  void mcu_setup_host(ModMcuHost host);
}

#endif //SIMULIDE_DEV_MCUINTERFACE_H