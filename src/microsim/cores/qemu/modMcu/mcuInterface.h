#ifndef SIMULIDE_DEV_MCUINTERFACE_H
#define SIMULIDE_DEV_MCUINTERFACE_H
#include "iopin.h"


extern "C" {
  struct McuConfig {
    const char *packageFile;
    const char *itemName;
    const char *iconName;
    const char *type;
    // const char *version;
    // const int usartN;
    // const int i2cN;
    // const int pinN;
    // const int canN;
    // const int spiN;
  };

  McuConfig mcu_get_config(const char* str);
  void mcu_run_to_time(uint64_t timePs);
  void mcu_stamp();
  void mcu_initialize();
  void mcu_on_pin_changed(int port,int number, double voltage);
  void mcu_on_scheduled_event();
  void mcu_set_firmware(const char* firmwareFileName);
  typedef void (*SetPinStateFunc)(void* ctx, int port,int number, bool state);
  typedef void (*SetPortStateFunc)(void* ctx, int port, uint16_t state);

  typedef void (*AddEventFunc)(void* ctx, uint64_t timePs);

  // from ioPin.h,
 //   enum pinMode_t{
 //     undef_mode=0,
 //     input,
 //     openCo,
 //     output,
 //     source
 // };
  struct PinConfig {
    int port;
    int number;
    pinMode_t mode;
    bool pullEn;
    bool altEn;
  };
  typedef void (*ConfigurePinsFunc)(void* ctx, PinConfig* configs, int count);



  struct ModMcuHost {
    void* context;
    SetPinStateFunc setPinState;
    SetPortStateFunc setPortState;
    AddEventFunc addEvent;
    ConfigurePinsFunc configurePins;
    // PinVoltCallback pinVoltCallback;
  };
  void mcu_setup_host(ModMcuHost host);
}

#endif //SIMULIDE_DEV_MCUINTERFACE_H