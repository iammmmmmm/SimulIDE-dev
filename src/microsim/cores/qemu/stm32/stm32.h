/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>

#include "qemudevice.h"
#include "ioport.h"
#include "stm32Rcm.h"

class Stm32Pin;

typedef std::vector<Stm32Pin*> Stm32Port;

class Stm32 : public QemuDevice
{
    public:
        Stm32( QString type, QString id, QString device );
        ~Stm32();

        void stamp() override;
        static LibraryItem* libraryItem();
        static Component* construct( QString type, QString id );
        void runToTime( uint64_t time ) override;
        void runEvent() override;

      protected:
        void setPinState( uint8_t port, uint8_t pin, bool state );
        void setPortState( uint8_t port, uint16_t state );
        Pin* addPin( QString id, QString type, QString label,
                     int n, int x, int y, int angle , int length=8, int space=0 ) override;

        void createPort( std::vector<Stm32Pin*>* port, uint8_t number, QString pId, uint8_t n );
        void createPins();
        void doAction() override;

        void setupDeviceParams() override;
        void cofigPort( uint8_t port,  uint32_t config, uint8_t shift );



        uint16_t readInputs( uint8_t port );

        //std::vector<Stm32Pin*>* getPort( uint8_t number )
        //{
        //    switch( number ) {
        //    case 1: return &m_portA;
        //    case 2: return &m_portB;
        //    case 3: return &m_portC;
        //    case 4: return &m_portD;
        //    }
        //    return nullptr;
        //}

        uint16_t m_state[7]; // Port states

        std::vector<Stm32Port> m_ports;

        //std::vector<Stm32Pin*> m_portA;
        //std::vector<Stm32Pin*> m_portB;
        //std::vector<Stm32Pin*> m_portC;
        //std::vector<Stm32Pin*> m_portD;
        //std::vector<Stm32Pin*> m_portE;

        uint32_t m_model;
        bool registerPeripheral() override;
        std::unique_ptr<stm32Rcm::Rcm> m_rcm;
        bool hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) override;

      struct EmuTask {
      uint64_t count;
       };

        std::thread m_emuThread;
        std::queue<EmuTask> m_taskQueue;
        std::mutex m_queueMutex;
        std::condition_variable m_queueCv;
        std::atomic<bool> m_stopThread{false};

        void emuLoop();
        double m_psRemainder = 0.0; // 记录上次不够跑一条指令剩下的皮秒
    };
