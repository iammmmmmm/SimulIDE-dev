#pragma once
#include <QLibrary>

#include "../qemudevice.h"
#include "mcuInterface.h"
#include "ModMcuPin.h"
#include "simulator.h"

typedef std::vector<ModMcuPin*> ModMcuPort;


class ModMcu : public QemuDevice {
  ModMcu(QString type, QString id, QString device);
  ~ModMcu();
  public:
    void stamp() override;
    void initialize() override;
    static Component* construct( QString type, QString id );
    void runToTime( uint64_t time ) override;
    void runEvent() override;
    void voltChanged() override;
    void pinVoltChanged(int port,int pin,double volt) {
      m_onPinChanged(port,pin,volt);
    }
     void setFirmware( QString file ) override;
  private:
    void createPins();
    void setPinState(int port, int number, bool state);
    void setPortState(uint8_t port, uint16_t state);
    void cofigPort(uint8_t port, uint32_t config, uint8_t shift);
    Pin *addPin(QString id,
                QString type,
                QString label,
                int n,
                int x,
                int y,
                int angle,
                int length,
                int space);

    std::vector<ModMcuPort> m_ports;


    McuConfig m_cfg;
    QLibrary mylibrary;
    using RunToTimeFunc      = void (*)(uint64_t timePs);
    using StampFunc          = void (*)();
    using InitializeFunc     = void (*)();
    using PinChangedFunc     = void (*)(int port,int number, double voltage);
    using ScheduledEventFunc = void (*)();
    // using ScheduledEventFunc = void (*)(uint64_t currentTimePs);
    using McuSetUpHostFunc       = void (*)(ModMcuHost host);
    using McuGetConfigFunc      = McuConfig (*)(const char* str);
    using McuSetFirmwareFunc     = void (*)(const char* firmware);

    RunToTimeFunc      m_runToTime      = nullptr;
    StampFunc          m_stamp          = nullptr;
    InitializeFunc     m_initialize     = nullptr;
    PinChangedFunc     m_onPinChanged   = nullptr;
    ScheduledEventFunc m_onScheduled    = nullptr;
    McuSetUpHostFunc   m_mcuSetUpFunc   = nullptr;
    McuGetConfigFunc   m_mcuGetConfigFunc = nullptr;
    McuSetFirmwareFunc   m_mcuSetFirmwareFunc = nullptr;

    static void handleSetPinState(void* ctx, const int port, const int number, const bool state) {
      if (ctx) {
        static_cast<ModMcu*>(ctx)->setPinState( port, number, state);
      }
    }
    static void handleSetPortState(void* ctx, const int port, const uint16_t state) {
      if (ctx) {
        static_cast<ModMcu*>(ctx)->setPortState( port, state);
      }
    }
    static void handleAddEvent(void* ctx, const uint64_t timePs) {
      if (ctx) {
        Simulator::self()->addEvent( timePs, static_cast<ModMcu*>(ctx) );;
      }
    }
    static void handleConfigurePins(void* ctx, PinConfig* configs, int count) {
      const auto* mcu = static_cast<ModMcu*>(ctx);
      for (int i = 0; i < count; ++i) {
        const PinConfig& cfg = configs[i];
        ModMcuPin* pin = mcu->m_ports[cfg.port - 1].at(cfg.number); // 获取引脚
        pin->setPinMode(cfg.mode );
        pin->setAnalog(cfg.mode == 3);
        pin->setPull(cfg.pullEn);
        pin->setAlternate(cfg.altEn);
      }
    }


    uint16_t m_state[7]; // Port states
};
