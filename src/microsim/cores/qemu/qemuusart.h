/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include "usartmodule.h"
#include <functional>
class QemuDevice;

class QemuUsart : public UsartModule
{
    public:
        QemuUsart( /*QemuDevice* mcu, QString name, int number*/ );
        virtual ~QemuUsart();

        enum usart_action_t{
            QUSART_READ=0,
            QUSART_WRITE,
            QUSART_BAUD
        };

        void enable( bool e );

        void sendByte( uint8_t data ) override{ UsartModule::sendByte( data ); }
        void bufferEmpty() override;
        void frameSent( uint8_t data ) override;
        void readByte( uint8_t data ) override;
        void setRxFlags( uint16_t frame ) override;

        uint8_t getBit9Tx() override;
        void setBit9Rx( uint8_t bit ) override;

        void setPins( QList<IoPin*> pinList );

        void doAction( uint32_t action, uint32_t data );

        void setMcuReadByte( std::function<void(uint8_t)> handler );

    protected:
        int m_number;

        bool m_speedx2;

        regBits_t m_bit9Tx;
        regBits_t m_bit9Rx;

        std::function<void(uint8_t)> m_mcuReadByteCallback;
};
