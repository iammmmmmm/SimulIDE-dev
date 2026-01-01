#pragma once

#include "iopin.h"
#include "qemumodule.h"

class ModMcu;
class IoPort;

class ModMcuPin:public IoPin, public QemuModule{
  public:
    ModMcuPin( uint8_t port, int i, QString id, QemuDevice* mcu );
    ~ModMcuPin();

    void initialize() override;
    void stamp() override;

    void voltChanged() override;

    void setPinMode( pinMode_t mode );

    void setOutState( bool high ) override;
    void scheduleState( bool high, uint64_t time ) override;

    void setPortState( bool high );

    void setPull( bool p );
    bool setAlternate( bool a );
    void setAnalog( bool a );

  protected:
    void paint( QPainter* p, const QStyleOptionGraphicsItem* o, QWidget* w ) override;

    void setPinState( bool high );
    //QString m_id;

    uint8_t m_port;

    bool m_pull;
    bool m_analog;
    bool m_alternate;

    double m_pullAdmit;

    uint16_t m_pinMask;

    ModMcu* modMcu = nullptr;
};