/***************************************************************************
 *   Copyright (C) 2025 by Santiago González                               *
 *                                                                         *
 ***( see copyright.txt file at root folder )*******************************/

#include <qtconcurrentrun.h>
#include <QLibrary>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSignalMapper>
#include <QDir>
#include <memory>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <unistd.h>

#include "peripheral_factory.h"
#include "Register.h"
#ifdef __linux__
#include <sys/mman.h>
#include <sys/shm.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

#include "qemudevice.h"
#include "itemlibrary.h"
#include "qemuusart.h"
#include "circuitwidget.h"
#include "iopin.h"

#include "stm32.h"
#include "esp32.h"

#include "circuit.h"
#include "simulator.h"
#include "componentlist.h"
#include "utils.h"

#include "stringprop.h"

#define tr(str) simulideTr("QemuDevice",str)

QemuDevice* QemuDevice::m_pSelf = nullptr;

Component* QemuDevice::construct( QString type, QString id )
{
    if( QemuDevice::self() )
    {
        qDebug() << "\nQemuDevice::construct ERROR: only one QemuDevice allowed\n";
        return nullptr;
    }
    QString device = Chip::getDevice( id );

    QemuDevice* qdev = nullptr;

    if     ( device.startsWith("STM32") ) qdev = new Stm32( type, id, device );
    else if( device.startsWith("Esp32") ) qdev = new Esp32( type, id, device );
    return qdev;
}

LibraryItem* QemuDevice::libraryItem()
{
    return new LibraryItem(
        "QemuDevice",
        "Micro",
        "ic2.png",
        "QemuDevice",
        QemuDevice::construct );
}
void QemuDevice::schedule_event(uint64_t time, EventActionType type, EventParams params) {
    ScheduledEvent new_event;
    new_event.scheduled_time = time;
    new_event.type = type;
    new_event.params = params;
    event_heap.push(new_event);
    if (this->eventTime==0) {
      //  const auto uc=this->unicorn_emulator_ptr->get();
        //uc_emu_stop(uc);
        Simulator::self()->addEvent(time,this);
    }
}

QemuDevice::QemuDevice( QString type, QString id )
          : Chip( type, id )
{
    m_pSelf = this;
    m_rstPin = nullptr;
    addPropGroup( { tr("Main"),{
        new StrProp<QemuDevice>("Program", tr("Firmware"),""
                               , this, &QemuDevice::firmware, &QemuDevice::setFirmware ),

        new StrProp<QemuDevice>("Args", tr("Extra arguments"),""
                               , this, &QemuDevice::extraArgs, &QemuDevice::setExtraArgs )
    }, 0 } );
    unicorn_emulator_ptr=std::make_unique<UnicornEmulator>(arch,mode);

}
QemuDevice::~QemuDevice()
{

}

void QemuDevice::initialize()
{
     target_instr_count=0;
     target_instr_begin=0;
     last_target_time=0;
     target_time=0;
}
void QemuDevice::print_memory_map_debug() const {
    qDebug() << "============================================";
    qDebug() << "          Unicorn 内存映射配置 (Memory Map)       ";
    qDebug() << "============================================";

    // 格式化输出函数，使用QDebug << QHex
    auto print_map_info = [](const QString& name, uint64_t start, uint64_t size, uint32_t prot) {
        QString prot_str;
        if (prot & UC_PROT_READ)  prot_str += "R";
        if (prot & UC_PROT_WRITE) prot_str += "W";
        if (prot & UC_PROT_EXEC)  prot_str += "X";

        qDebug().nospace() << "✅ [" << name << "]"
                           << " | START: 0x" << QString::number(start, 16).toUpper()
                           << " | SIZE: " << (size / 1024) << " KB (0x" << QString::number(size, 16).toUpper() << ")"
                           << " | END: 0x" << QString::number(start + size, 16).toUpper()
                           << " | PROT: " << prot_str;
    };

    // 1. FLASH 映射 (代码和常量)
    print_map_info("FLASH", m_mem_params.FLASH_START,  m_mem_params.FLASH_SIZE, UC_PROT_READ | UC_PROT_EXEC);

    // 2. SRAM 映射 (运行时数据/堆栈)
    print_map_info("SRAM",  m_mem_params.SRAM_START,  m_mem_params.SRAM_SIZE, UC_PROT_READ | UC_PROT_WRITE | UC_PROT_EXEC);

    // 3. ROM 映射 (启动向量/重映射)
    print_map_info("ROM (Boot)",  m_mem_params.ROM_START,  m_mem_params.ROM_SIZE, UC_PROT_READ | UC_PROT_EXEC);

    // 4. PERIPHERAL 映射 (硬件寄存器)
    print_map_info("PERIPHERAL",  m_mem_params.PERIPHERAL_START,  m_mem_params.PERIPHERAL_END-m_mem_params.PERIPHERAL_START, UC_PROT_READ | UC_PROT_WRITE);

    // 5. PPB 映射 (内核私有外设)
    print_map_info("PPB (Core)",  m_mem_params.PPB_START,  m_mem_params.PPB_SIZE, UC_PROT_READ | UC_PROT_WRITE);

    qDebug() << "============================================";
}
// 1. 定义 MMIO 读取回调
uint64_t mmio_read_callback(uc_engine *uc, uint64_t address, unsigned size, void *user_data) {
    const auto* device = static_cast<QemuDevice*>(user_data);
    PeripheralDevice *peripheral = device->peripheral_registry->findDevice(0x40000000+address);

    if (peripheral) {
        int64_t val = 0;
        peripheral->handle_read(uc, 0x40000000+address, size, &val, user_data);
        return static_cast<uint64_t>(val);
    }
    qDebug()<<"mmio_read_callback peripheral==0";
    return 0;
}

// 2. 定义 MMIO 写入回调
void mmio_write_callback(uc_engine *uc, uint64_t address, unsigned size, uint64_t value, void *user_data) {
    auto* device = static_cast<QemuDevice*>(user_data);
    PeripheralDevice *peripheral = device->peripheral_registry->findDevice(0x40000000+address);

    if (peripheral) {
        peripheral->handle_write(uc, 0x40000000+address, size, value, user_data);
    }
}
void QemuDevice::stamp()
{
    setupDeviceParams();
    print_memory_map_debug();
    unicorn_emulator_ptr=std::make_unique<UnicornEmulator>(arch,mode);
    uc_err = uc_mem_map(unicorn_emulator_ptr->get(),  m_mem_params.FLASH_START,  m_mem_params.FLASH_SIZE, UC_PROT_READ | UC_PROT_EXEC);
    uc_err=uc_mem_map(unicorn_emulator_ptr->get(),  m_mem_params.SRAM_START,  m_mem_params.SRAM_SIZE, UC_PROT_READ | UC_PROT_WRITE | UC_PROT_EXEC);
    uc_err=uc_mem_map(unicorn_emulator_ptr->get(),  m_mem_params.ROM_START,  m_mem_params.ROM_SIZE, UC_PROT_READ | UC_PROT_EXEC);
    // uc_err=uc_mem_map(unicorn_emulator_ptr->get(), m_mem_params.PERIPHERAL_START, m_mem_params.PERIPHERAL_END-m_mem_params.PERIPHERAL_START,UC_PROT_READ | UC_PROT_WRITE);
     uc_err = uc_mmio_map(
            unicorn_emulator_ptr->get(),
            m_mem_params.PERIPHERAL_START,
            m_mem_params.PERIPHERAL_END-m_mem_params.PERIPHERAL_START, // 确保大小对齐
            mmio_read_callback,           // 读回调
            static_cast<void*>(this),      // user_data
            mmio_write_callback,          // 写回调
            static_cast<void*>(this)       // user_data
        );
    uc_err=uc_mem_map(unicorn_emulator_ptr->get(),  m_mem_params.PPB_START,  m_mem_params.PPB_SIZE, UC_PROT_READ | UC_PROT_WRITE);

    if (uc_err) {
        qWarning() <<"uc map err；"<<uc_strerror(uc_err);
        return;
    }
    peripheral_registry = std::make_unique<PeripheralRegistry>();
    if (!registerPeripheral()) {
        qWarning() <<"registerPeripheral err"<< Qt::endl;
        return;
    }
    set_hooks();
    if (!loadFirmware()) {
        qWarning() <<"loadFirmware err"<< Qt::endl;
        return;
    }
   const uint64_t start_pc=getStartPc();
    if (start_pc) {
        qDebug()<<"uc emu start!"<< Qt::endl;
        target_instr_begin=start_pc;
    }else {
        qWarning() <<"start_pc err；"<<uc_strerror(uc_err)<< Qt::endl;
    }
}

//void QemuDevice::updateStep()
//{
//    return;
//
//    QString output = m_qemuProcess.readAllStandardOutput();
//    if( !output.isEmpty() )
//    {
//        QStringList lines = output.split("\n");
//        for( QString line : lines ) qDebug() << line.remove("\"");
//    }
//}

void QemuDevice::voltChanged()
{
    if( m_shMemId == -1 ) return;

    bool reset = !m_rstPin->getInpState();

    if( reset )
    {
        m_arena->running = 0;

        if( m_qemuProcess.state() > QProcess::NotRunning )
        {
            Simulator::self()->cancelEvents( this );
            m_qemuProcess.kill();
        }
    }
    else if( m_qemuProcess.state() == QProcess::NotRunning ) stamp();
}

void QemuDevice::runToTime( uint64_t time )
{
    // if( this->eventTime ) return;// Our event still not executed
    // //if( m_arena->qemuTime ) return; // Our event still not executed
    // //if( m_arena->simuTime ) return; // Our event still not executed
    //
    // //qDebug() << "\nQemuDevice::runToTime"<< time;
    //
    // m_arena->simuTime = 0;
    // m_arena->qemuTime = time; // Tell Qemu to run up to time
    // //m_arena->qemuAction = SIM_EVENT;
    //
    // while( m_arena->simuTime == 0 ) { ; } // Wait for Qemu action  //   Simulator::self()->simState() == SIM_RUNNING )
    //
    // uint64_t eventTime = m_arena->simuTime - Simulator::self()->circTime();
    // //if( m_arena->action < SIM_EVENT )
    // Simulator::self()->addEvent( eventTime, this );
    // //qDebug() << "QemuDevice::runToTime event"<< m_arena->simuTime;

    // uc_err = uc_emu_start(unicorn_emulator_ptr->get(), target_instr_begin, FLASH_END, 0, 0);

}
uint64_t QemuDevice::calculateInstructionsToExecute(uint64_t target_time_ps,uint64_t current_time_ps,double ps_per_inst) {
    if (target_time_ps <= current_time_ps) {
        return 0;
    }
    const uint64_t delta_time_ps = target_time_ps - current_time_ps;
    if (ps_per_inst < 1.0) {
        qWarning() << "ps_per_inst is too small! Check frequency calculation.";
        return 0;
    }
    const double instructions_double = static_cast<double>(delta_time_ps) / ps_per_inst;
    const auto instructions_count = static_cast<uint64_t>(std::round(instructions_double));
    // 如果计算出 0 但时间差很大，可能需要关注
    return instructions_count;
}
uint64_t QemuDevice::getCurrentPc() const {
    uint64_t current_pc = 0;
    uc_reg_read(unicorn_emulator_ptr->get(), UC_ARM_REG_PC, &current_pc);
    return current_pc;
}
void QemuDevice::runEvent()
{

}

void QemuDevice::slotOpenTerm( int num )
{
    m_usarts.at(num-1)->openMonitor( findIdLabel(), num );
    //m_serialMon = num;
}

void QemuDevice::slotLoad()
{
    QDir dir( m_lastFirmDir );
    if( !dir.exists() ) m_lastFirmDir = Circuit::self()->getFilePath();

    QString fileName = QFileDialog::getOpenFileName( nullptr, tr("Load Firmware"), m_lastFirmDir,
                                                    tr("All files (*.*);;Hex Files (*.hex)"));

    if( fileName.isEmpty() ) return; // User cancels loading

    setFirmware( fileName );
}

void QemuDevice::slotReload()
{
    if( !m_firmware.isEmpty() ) setFirmware( m_firmware );
    else QMessageBox::warning( 0, "QemuDevice::slotReload", tr("No File to reload ") );
}

void QemuDevice::setFirmware( QString file )
{
    if( Simulator::self()->isRunning() ) CircuitWidget::self()->powerCircOff();

    //QDir    circuitDir   = QFileInfo( Circuit::self()->getFilePath() ).absoluteDir();
    //QString fileNameAbs  = circuitDir.absoluteFilePath( file );
    //QString cleanPathAbs = circuitDir.cleanPath( fileNameAbs );
    m_firmware = file;//cleanPathAbs;
}

void QemuDevice::setPackageFile( QString package )
{
    if( !QFile::exists( package ) )
    {
        qDebug() <<"File does not exist:"<< package<< Qt::endl;
        return;
    }
    QString pkgText = fileToString( package, "QemuDevice::setPackageFile");
    QString pkgStr  = convertPackage( pkgText );
    m_isLS = package.endsWith("_LS.package");
    initPackage( pkgStr );
    setLogicSymbol( m_isLS );

    m_label.setPlainText( m_name );

    Circuit::self()->update();
}

void QemuDevice::contextMenu( QGraphicsSceneContextMenuEvent* event, QMenu* menu )
{
    //if( m_eMcu.flashSize() )
    {
        QAction* loadAction = menu->addAction( QIcon(":/load.svg"),tr("Load firmware") );
        QObject::connect( loadAction, &QAction::triggered, [=](){ slotLoad(); } );

        QAction* reloadAction = menu->addAction( QIcon(":/reload.svg"),tr("Reload firmware") );
        QObject::connect( reloadAction, &QAction::triggered, [=](){ slotReload(); } );

        menu->addSeparator();
    }

    //QAction* openRamTab = menu->addAction( QIcon(":/terminal.svg"),tr("Open Mcu Monitor.") );
    //QObject::connect( openRamTab, &QAction::triggered, [=](){ slotOpenMcuMonitor(); } );

    if( m_usarts.size() )
    {
        QMenu* serMonMenu = menu->addMenu( QIcon(":/serialterm.png"),tr("Open Serial Monitor.") );

        QSignalMapper* sm = new QSignalMapper();
        for( uint i=0; i<m_usarts.size(); ++i )
        {
            QAction* openSerMonAct = serMonMenu->addAction( "USart"+QString::number(i+1) );
            QObject::connect( openSerMonAct, &QAction::triggered, sm, QOverload<>::of(&QSignalMapper::map) );
            sm->setMapping( openSerMonAct, i+1 );
        }
        QObject::connect( sm, QOverload<int>::of(&QSignalMapper::mapped), [=](int n){ slotOpenTerm(n);} );
    }
    menu->addSeparator();
    Component::contextMenu( event, menu );
}

void QemuDevice::set_hooks() {
  //  uc_hook hh_mem_write;
    uc_hook hh_code;
    uc_hook hh_mem_unmap;
   // uc_err = uc_hook_add(
   //     unicorn_emulator_ptr->get(),
   //     &hh_mem_write,
   //     UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,
   //     reinterpret_cast<void*>(hook_mem_wr_wappler),
   //     static_cast<void*>(this),
   //     m_mem_params.PERIPHERAL_START,
   //     m_mem_params.PERIPHERAL_END);
    //hook code带来的开销很大，但又必须知道”固件的当前时间“
    /*一个办法是使用hook block，但是block不提供指令数，只有块大小
     *      使用block 的话要么根据size 估算指令数，然后根据模拟器时间不断修正  精度差 性能好点 实现难度一般
     *      或者自己反汇编计算block里的指令数 精度好 性能无法评估 很难实现
     *          可以在模拟启动前预先烘焙一个”表格“，根据表格获取block里的指令数 精度好 性能好点 几乎难以实现
     *
     */
    uc_err = uc_hook_add(
           unicorn_emulator_ptr->get(),
           &hh_code,
           UC_HOOK_CODE,
           reinterpret_cast<void*>(hook_code_wappler),
           static_cast<void*>(this),
           0,
           m_mem_params.FLASH_START+m_mem_params.FLASH_SIZE);
    uc_err = uc_hook_add(
           unicorn_emulator_ptr->get(),
           &hh_mem_unmap,
           UC_HOOK_MEM_READ_UNMAPPED | UC_HOOK_MEM_WRITE_UNMAPPED |UC_HOOK_MEM_FETCH_UNMAPPED,
           reinterpret_cast<void*>(hook_mem_unmapped_wappler),
           static_cast<void*>(this),
           0,
           0xFFFFFFFF);
    if (uc_err) {
        qWarning() << "QemuDevice::set_hooks(): uc_err=" <<uc_strerror(uc_err)<< Qt::endl;
    }
}
// void QemuDevice::hook_mem_wr(uc_engine *uc,uc_mem_type type,uint64_t address,int size,int64_t value,void *user_data) const {
//     //qDebug()<<"QemuDevice::hook_mem_wr";
//     auto* device = static_cast<QemuDevice*>(user_data);
//     if (address>=device->m_mem_params.PERIPHERAL_START&&address<=device->m_mem_params.PERIPHERAL_END) {
//         PeripheralDevice *peripheral = peripheral_registry->findDevice(address);
//         if (peripheral) {
//             //qDebug().noquote()<<"指令PC：0x"<<Qt::hex<<getCurrentPc()<<Qt::endl;
//             if (type==UC_MEM_WRITE) {
//                 peripheral->handle_write(uc,address,size,value, user_data);
//             }else {
//                 peripheral->handle_read(uc,address,size,&value, user_data);
//             }
//         }else {
//     qWarning() << "QemuDevice::hook_mem_wr(): peripheral not found, address: 0x" << Qt::hex << address << Qt::endl;
//         }
//     }
// }
bool QemuDevice::hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
  //  qDebug() << "QemuDevice::hook_code(): address=" << Qt::hex << address << Qt::endl;
    //auto* device = static_cast<QemuDevice*>(user_data);
// while (!event_heap.empty()) {
//     qDebug()<<"!event_heap.empty()=true";
// }
    return true;
}
bool QemuDevice::hook_mem_unmapped(uc_engine *uc,uc_mem_type type,uint64_t address,int size,int64_t value,void *user_data) {
    uint32_t current_pc;
    uc_reg_read(uc, UC_ARM_REG_PC, &current_pc);
    qWarning() << "QemuDevice::hook_mem_unmapped(): current_pc="<<Qt::hex << current_pc<< Qt::endl;
    //Maybe we need to do something?
    return false;
}
bool QemuDevice::registerPeripheral() {
    qWarning() << "QemuDevice::registerPeripheral() must def by son !!!!!!!!!!!!!!!!!!!!"<< Qt::endl;
    return false;
}
bool QemuDevice::loadFirmware() {
    //FIXME: 文件不存在时没法检测到，不会报错，也不会中止无效的模拟启动...<(＿　＿)>
    std::ifstream file(m_firmware.toStdString(), std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        qWarning() << " ERROR UNABLE TO OPEN FIRMWARE FILE" << m_firmware<< Qt::endl;
        file.close();
        return false;
    }

    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);

    if (file.read(reinterpret_cast<char *>(buffer.data()), size)) {
        qDebug() << "Successfully read firmware, size；" << size << " byte。"<< Qt::endl;

        uc_err = uc_mem_write(unicorn_emulator_ptr->get(), m_mem_params.FLASH_START, buffer.data(), size);
        uc_err = uc_mem_write(unicorn_emulator_ptr->get(), m_mem_params.ROM_START, buffer.data(), size);
        if (uc_err) {
            qWarning() << "failed to write firmware to mem error : " <<uc_strerror( uc_err);
            file.close();
            return false;
        }
    } else {
        qWarning() << "Error: An error occurred while reading the file。"<< Qt::endl;
        uc_close(unicorn_emulator_ptr->get());
        file.close();
        return false;
    }
    file.close();
    return true;
}
uint64_t QemuDevice::getStartPc() {
    uint32_t sp_val, pc_val;
    uint64_t start_pc;
    uc_err = uc_mem_read(unicorn_emulator_ptr->get(), m_mem_params.FLASH_START, &sp_val, sizeof(sp_val));
    if (uc_err == UC_ERR_OK) {
        uc_reg_write(unicorn_emulator_ptr->get(), UC_ARM_REG_SP, &sp_val);
        qDebug( "the initial sp is set to:%x", sp_val) ;
    } else {
        qWarning() << "failed to read sp initial value error code: " << uc_strerror(uc_err) << Qt::endl;
        return false;
    }
    uc_err = uc_mem_read(unicorn_emulator_ptr->get(), m_mem_params.FLASH_START + 4, &pc_val, sizeof(pc_val));
    if (uc_err == UC_ERR_OK) {
        // start_pc = (FLASH_START + pc_val) | 1; //NOTE 这里很怪，apm32f003x系列使用这个也可以启动，使用下面哪个都能启动
        start_pc = pc_val | 1;
        qDebug("the initial pc setup is: %x" ,static_cast<unsigned>(start_pc));
    } else {
        qWarning()<<"Failed to read PC initial value. Error code； "<<uc_err<< Qt::endl;
        return false;
    }
    return start_pc;
}
