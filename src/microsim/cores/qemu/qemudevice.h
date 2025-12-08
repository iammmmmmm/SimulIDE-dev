/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#pragma once

#include <QProcess>
#include <QDebug>
#include <queue>
#include "chip.h"
#include "unicorn/unicorn.h"
#include "peripheral_factory.h"
class UnicornEmulator;
typedef struct qemuArena{
    uint64_t simuTime;    // in ps
    uint64_t qemuTime;    // in ps
    uint32_t data32;
    uint32_t mask32;
    uint16_t data16;
    uint16_t mask16;
    uint8_t  data8;
    uint8_t  mask8;
    uint8_t  simuAction;
    uint8_t  qemuAction;
    double   ps_per_inst;
    bool     running;
} qemuArena_t;

enum simuAction{
    SIM_I2C=10,
    SIM_SPI,
    SIM_USART,
    SIM_TIMER,
    SIM_GPIO_IN,
    SIM_EVENT=1<<7
};
enum class EventActionType {
    NONE = 0,
    GPIO_PIN_SET,
    //TODO
};
struct EventParams {
    union {
        struct {
            uint8_t port;
            uint8_t  next_state;
        } gpio_data{};
    };
    EventParams() {}
};
struct ScheduledEvent {
    uint64_t scheduled_time{};
    EventActionType type = EventActionType::NONE;
    EventParams params;
};
struct EventComparator {
    bool operator()(const ScheduledEvent& a, const ScheduledEvent& b) const {
        return a.scheduled_time > b.scheduled_time;
    }
};
class IoPin;
class QemuUsart;
class QemuTimer;
class QemuTwi;
class QemuSpi;
class LibraryItem;

class QemuDevice : public Chip
{
    public:
        QemuDevice( QString type, QString id );
        ~QemuDevice();

        void initialize() override;
        void print_memory_map_debug() const;
        void stamp() override;
        //void updateStep() override;
        void voltChanged() override;
        void runEvent() override;

        QString firmware() { return m_firmware; }
        void setFirmware( QString file );

        QString extraArgs()  { return m_extraArgs; }
        void setExtraArgs( QString a ){ m_extraArgs = a; }

        void setPackageFile( QString package );

        volatile qemuArena_t* getArena() { return m_arena; }

        virtual void runToTime( uint64_t time );

        void slotLoad();
        void slotReload();
        void slotOpenTerm( int num );
 static QemuDevice* self() { return m_pSelf; }
 static Component* construct( QString type, QString id );
 static LibraryItem* libraryItem();
        //允许外设访问
        std::unique_ptr<PeripheralRegistry> peripheral_registry;
        /**
         * @brief 将一个新事件调度到指定的时间。
         * * @param time 事件发生的绝对时间。
         * @param type 事件类型。
         * @param params 事件参数。
         */
        void schedule_event(uint64_t time, EventActionType type, EventParams params);
    protected:
 static QemuDevice* m_pSelf;
       std::priority_queue<ScheduledEvent,std::vector<ScheduledEvent>,EventComparator> event_heap;
        virtual bool createArgs(){ return false;}

        virtual void doAction(){;}

        void contextMenu( QGraphicsSceneContextMenuEvent* e, QMenu* m ) override;

        QString m_lastFirmDir;  // Last firmware folder used
        QString m_firmware;
        QString m_executable;
        QString m_packageFile;

        QString m_extraArgs;

        volatile qemuArena_t* m_arena;

        int m_gpioSize{};
        std::vector<IoPin*> m_ioPin;
        IoPin* m_rstPin;

        QString m_shMemKey;
        int64_t m_shMemId;

        void* m_wHandle;

        QProcess m_qemuProcess;
        QStringList m_arguments;

        uint8_t m_portN{};
        uint8_t m_usartN{};
        //uint8_t m_timerN;
        uint8_t m_i2cN{};
        uint8_t m_spiN{};

        std::vector<QemuTwi*> m_i2cs;
        std::vector<QemuSpi*> m_spis;
        std::vector<QemuUsart*> m_usarts;

        std::unique_ptr<UnicornEmulator> unicorn_emulator_ptr;
        uc_err uc_err;
        uc_arch arch = UC_ARCH_ARM;
        uc_mode mode = UC_MODE_THUMB;
        //The following variables must be set to the correct value before map
        struct MemoryParams {
            uint64_t FLASH_START = 0x08000000;
            uint64_t FLASH_SIZE = (32 * 1024);
            uint64_t SRAM_START = 0x20000000;
            uint64_t SRAM_SIZE = (4 * 1024);
            uint64_t SYS_MEM_START = 0x00020000;
            uint64_t SYS_MEM_SIZE = (1 * 1024);
            uint64_t ROM_START = 0x00000000;
            uint64_t ROM_SIZE = 0x00000000;
            uint64_t PERIPHERAL_START = 0x40000000;
            uint64_t PERIPHERAL_END = 0x400114FF;
            uint64_t PPB_START = 0xE0000000;
            uint64_t PPB_SIZE = 0x100000;
        };
        MemoryParams m_mem_params;
        uint64_t target_instr_count=0;
        uint64_t target_instr_begin=0;
        //uint64_t target_instr_end=0;
        uint64_t last_target_time=0;
        uint64_t target_time=0;

        virtual void setupDeviceParams(){};
        void set_hooks();
        void hook_mem_wr(uc_engine *uc,uc_mem_type type,uint64_t address,int size,int64_t value,void *user_data) const;
        static  void hook_mem_wr_wappler
        (uc_engine *uc,uc_mem_type type,uint64_t address,int size,int64_t value,void *user_data) {
            auto qemudevice = static_cast<QemuDevice*>(user_data);
            qemudevice->hook_mem_wr(uc,type,address,size,value,user_data);
        };
        virtual bool hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *user_data);
        static  bool hook_code_wappler(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
            auto qemudevice = static_cast<QemuDevice*>(user_data);
            return qemudevice->hook_code(uc,address,size,user_data);
        }
        static bool hook_mem_unmapped(uc_engine *uc,uc_mem_type type,uint64_t address,int size,int64_t value,void *user_data);
        static  bool hook_mem_unmapped_wappler(uc_engine *uc,uc_mem_type type,uint64_t address,int size,int64_t value,void *user_data) {
            auto qemudevice = static_cast<QemuDevice*>(user_data);
            return qemudevice->hook_mem_unmapped(uc,type,address,size,value,user_data);
        }
        //ask son register peripheral by this
        virtual bool registerPeripheral();
        bool loadFirmware();
        uint64_t getStartPc();
        /**
 * @brief 计算在给定的时间内需要执行的指令数。
 * * @param target_time_ps 目标时间点 (单位：皮秒 ps)
 * @param current_time_ps 当前时间点 (单位：皮秒 ps)
 * @param ps_per_inst 每条指令执行所需的皮秒数
 * @return uint64_t 需要执行的指令数
 */
        static uint64_t calculateInstructionsToExecute(uint64_t target_time_ps, uint64_t current_time_ps, double ps_per_inst);
        uint64_t getCurrentPc() const;
};

#include <stdexcept>
#include <string>

/**
 * @brief 使用 RAII 封装 Unicorn Engine 句柄 (uc_engine*)。
 * * 利用构造函数获取资源 (uc_open)，析构函数释放资源 (uc_close)，
 * 确保即使在异常发生时也能安全清理。
 */
class UnicornEmulator {
public:
    UnicornEmulator(uc_arch arch, uc_mode mode) {
        uc_err err = uc_open(arch, mode, &uc_);
        if (err != UC_ERR_OK) {
            std::string msg = "Error opening Unicorn Engine: ";
            msg += uc_strerror(err);

            throw std::runtime_error(msg);
        }
    }
    ~UnicornEmulator() {
        if (uc_ != nullptr) {
            uc_close(uc_);
            uc_ = nullptr; //
        }
    }
    /**
     * @brief Get the underlying uc_engine* pointer.
     * @return A valid uc_engine* handle.
     */
    uc_engine* get() const {
        if (uc_ == nullptr) {
             throw std::runtime_error("Attempted to access uninitialized or closed Unicorn engine.");
        }
        return uc_;
    }
        UnicornEmulator(const UnicornEmulator&) = delete;
    UnicornEmulator& operator=(const UnicornEmulator&) = delete;

private:
    uc_engine* uc_ = nullptr;
};