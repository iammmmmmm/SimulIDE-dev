#pragma once
#include <QLibrary>

#include "../qemudevice.h"
#include "mcuInterface.h"
class ModMcu : public QemuDevice {
  ModMcu(QString type, QString id, QString device);
  ~ModMcu();
  public:
    void stamp() override;
    void initialize() override;
    static LibraryItem* libraryItem();
    static Component* construct( QString type, QString id );
    void runToTime( uint64_t time ) override;
    void runEvent() override;
  private:
    QLibrary mylibrary;
    using RunToTimeFunc      = void (*)(uint64_t timePs);
    using StampFunc          = void (*)();
    using InitializeFunc     = void (*)();
    using PinChangedFunc     = void (*)(int pinIndex, double voltage);
    using ScheduledEventFunc = void (*)();
    // using ScheduledEventFunc = void (*)(uint64_t currentTimePs);
    using McuSetUpHostFunc       = void (*)(ModMcuHost host);

    RunToTimeFunc      m_runToTime      = nullptr;
    StampFunc          m_stamp          = nullptr;
    InitializeFunc     m_initialize     = nullptr;
    PinChangedFunc     m_onPinChanged   = nullptr;
    ScheduledEventFunc m_onScheduled    = nullptr;
    McuSetUpHostFunc       m_mcuSetUpFunc    = nullptr;

    static void handleSetVoltage(void* ctx, int pinIndex, double voltage) {
      if (ctx) {
       // static_cast<ModMcu*>(ctx)->doSetVoltage(pinIndex, voltage);
      }
    }
    static void handleAddEvent(void* ctx) {
      if (ctx) {
       // static_cast<ModMcu*>(ctx)->;
      }
    }

};