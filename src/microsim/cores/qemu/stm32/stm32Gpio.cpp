#include "stm32Gpio.h"

#include <cstdint>
#include <iostream>
#include <utility>

#include "simulator.h"
#include "stm32.h"
#include "stm32GpioReg.h"
using namespace stm32Gpio;
/**
 * @brief Gpio 类的构造函数
 * @param base_addr GPIO 端口的基地址
 * @param port_name 端口名称 (如 "Port A")
 */
Gpio::Gpio(const uint64_t base_addr, std::string port_name) : m_port_name(std::move(port_name)),
                                                              m_base_addr(base_addr) {
  // 初始化寄存器到复位值
  initialize_registers(port_name);
  uint8_t port = 1;
  switch (m_base_addr) {
    case GPIOA_BASE:
      port = 1;
      break;
    case GPIOB_BASE:
      port = 2;
      break;
    case GPIOC_BASE:
      port = 3;
      break;
    case GPIOD_BASE:
      port = 4;
      break;
    case GPIOE_BASE:
      port = 5;
      break;
    default:
      qWarning() << "[GPIO] Error: Unknown base address 0x" << Qt::hex << Qt::endl;
      break;
  }
  m_port = port;
  qDebug() << "[GPIO] Initialized" << Gpio::getName().c_str()
      << "at base address 0x" << Qt::hex << m_base_addr;
}
/**
 * @brief 初始化 GPIO 寄存器到复位值
 */
void Gpio::initialize_registers(const string &portName) {
  // 端口配置低寄存器 (GPIOx_CFGLOW)，偏移地址 0x00
  m_registers[CFGLOW_OFFSET] = std::make_unique<stm32Gpio::GPIOx_CFGLOW>(CFGLOW_OFFSET, portName);
  // 端口配置高寄存器 (GPIOx_CFGHIG)，偏移地址 0x04
  m_registers[CFGHIG_OFFSET] = std::make_unique<stm32Gpio::GPIOx_CFGHIG>(CFGHIG_OFFSET, portName);
  // 端口输入数据寄存器 (GPIOx_IDATA)，偏移地址 0x08
  m_registers[IDATA_OFFSET] = std::make_unique<stm32Gpio::GPIOx_IDATA>(IDATA_OFFSET, portName);
  // 端口输出数据寄存器 (GPIOx_ODATA)，偏移地址 0x0C
  m_registers[ODATA_OFFSET] = std::make_unique<stm32Gpio::GPIOx_ODATA>(ODATA_OFFSET, portName);
  // 端口位设置/清除寄存器 (GPIOx_BSC)，偏移地址 0x10
  m_registers[BSC_OFFSET] = std::make_unique<stm32Gpio::GPIOx_BSC>(BSC_OFFSET, portName);
  // 端口位清除寄存器 (GPIOx_BC)，偏移地址 0x14
  m_registers[BC_OFFSET] = std::make_unique<stm32Gpio::GPIOx_BC>(BC_OFFSET, portName);
  // 端口配置锁定寄存器 (GPIOx_LOCK)，偏移地址 0x18
  m_registers[LOCK_OFFSET] = std::make_unique<stm32Gpio::GPIOx_LOCK>(LOCK_OFFSET, portName);
  // =========================================================================
  // 寄存器写监控 (WR Monitor)
  // =========================================================================

  // GPIOx_ODATA (0x0C): 端口输出数据寄存器
  // 写入 ODATA 会直接改变引脚的输出状态（如果引脚配置为推挽或开漏输出）
  auto *odata_reg = m_registers[ODATA_OFFSET]->find_field("ODATA");
  if (odata_reg) {
    odata_reg->write_callback = [this](Register &reg,
                                       const BitField &field,
                                       uint32_t new_field_value,
                                       void *user_data) {
      const uint32_t current_odata = m_registers[ODATA_OFFSET]->
          get_field_value_non_intrusive("ODATA");
      const uint32_t set_mask = new_field_value;
      const uint32_t new_odata = current_odata | set_mask;
      const auto stm32_instance = static_cast<Stm32 *>(user_data);
      const auto rcm_device = dynamic_cast<stm32Rcm::Rcm *>(stm32_instance->peripheral_registry->
        findDevice(RCM_BASE));
      const auto port_state_16bit = static_cast<uint16_t>(new_odata & 0xFFFF);
      const uint64_t time = rcm_device->getMcuTime();
      // qDebug()<<"[GPIO w] 引脚操作事件，mcu时间：" << time << "模拟器时间:"<< Simulator::self()->circTime();
      // qDebug()<<"[GPIO w] RCM频率："<<rcm_device->getSysClockFrequency()<<"RCM ticks:"<<rcm_device->getRcmTicks();
      EventParams params;
      params.gpio_pin_set_data.port = m_port;
      params.gpio_pin_set_data.next_state = port_state_16bit;
      stm32_instance->schedule_event(time- Simulator::self()->circTime(), EventActionType::GPIO_PIN_SET, params);
     // Simulator::self()->addEvent(time - Simulator::self()->circTime(), stm32_instance);
    };
  }

  // GPIOx_BSC (0x10): 端口位设置/清除寄存器
  // 写入 BSC 寄存器会触发对 ODATA 寄存器的修改
  auto *bsc_reg = m_registers[BSC_OFFSET].get();
  if (bsc_reg) {
    // bsc_reg->find_field("BS")->write_callback=[this](Register &reg, BitField field,uint32_t new_reg_value, void *user_data) {
    //
    // };
    // bsc_reg->find_field("BC")->write_callback=[this](Register &reg,BitField field, uint32_t new_reg_value, void *user_data) {
    //
    // };
    // 设置整个寄存器的写入回调，处理完整的 32 位值
    bsc_reg->write_callback = [this](Register &reg, uint32_t new_reg_value, void *user_data) {
      // qDebug()<<"[GPIO] bsc寄存器操作";
      // new_reg_value 是写入 BSC 寄存器的 32 位完整值
      // 1. 提取 BCy (Bit Clear) 掩码（ 31:16）
      const uint32_t clear_mask = (new_reg_value >> 16) & 0x0000FFFF;
      // 2. 提取 BSy (Bit Set) 掩码（ 15:0）
      const uint32_t set_mask = (new_reg_value & 0x0000FFFF);
      // 如果既没有设置位，也没有清除位，则直接返回
      // qDebug() << "  [BSC CB] Clear Mask (shifted): 0x" << Qt::hex << clear_mask;
      // qDebug() << "  [BSC CB] Set Mask: 0x" << Qt::hex << set_mask;
      if (set_mask == 0 && clear_mask == 0) {
        // qDebug()<<"bsc set mask=0,clear_mask=0";
        return;
      }
      // 获取当前的 ODATA 寄存器值
      const uint32_t current_odata = m_registers[ODATA_OFFSET]->
          get_field_value_non_intrusive("ODATA");

      uint32_t temp_odata = current_odata & (~clear_mask);
      const uint32_t new_odata = temp_odata | set_mask;
      if (new_odata == current_odata) {
        // qDebug() << "  [BSC CB] ODATA value UNCHANGED: 0x" << Qt::hex << current_odata;
        return;
      }
      m_registers[ODATA_OFFSET]->set_field_value("ODATA", sizeof(uint32_t), new_odata, user_data);
    };
  }
  const auto bc_reg = m_registers[BC_OFFSET].get();
  if (bc_reg) {
    bc_reg->write_callback = [this](Register &reg, uint32_t new_reg_value, void *user_data) {
      const uint32_t clear_mask = new_reg_value & 0x0000FFFF;
      if (clear_mask == 0) {
        // qDebug() << "bc clear_mask=0";
        return;
      }
      const uint32_t current_odata = m_registers[ODATA_OFFSET]->
          get_field_value_non_intrusive("ODATA");
      const uint32_t new_odata = current_odata & (~clear_mask);
      if (new_odata == current_odata) {
        // qDebug() << "  [gpio BC ] ODATA value UNCHANGED: 0x" << Qt::hex << current_odata;
        return;
      }
      m_registers[ODATA_OFFSET]->set_field_value("ODATA", sizeof(uint32_t), new_odata, user_data);
    };
  }


  // GPIOx_CFGLOW (0x00) / GPIOx_CFGHIG (0x04): 模式和功能配置
  // 写入配置寄存器会影响引脚的输入/输出/复用功能，需要外部 IO 模块进行适配。
  auto *cfglow_reg = m_registers[CFGLOW_OFFSET].get();
  auto *cfghig_reg = m_registers[CFGHIG_OFFSET].get();

  if (cfglow_reg) {
    cfglow_reg->write_callback = [this](Register &reg, uint32_t new_register_value, void *user_data) {
      const auto stm32_instance = static_cast<Stm32 *>(user_data);
      const auto rcm_device = dynamic_cast<stm32Rcm::Rcm *>(stm32_instance->peripheral_registry->
        findDevice(RCM_BASE));
      EventParams params;
      params.gpio_config_data.port=m_port;
      params.gpio_config_data.config=new_register_value;
      //qDebug()<<"cfglow_reg new_register_value:0x"<<Qt::hex<<new_register_value;
      params.gpio_config_data.shift=0;
      const uint64_t time = rcm_device->getMcuTime();
      stm32_instance->schedule_event(time- Simulator::self()->circTime(), EventActionType::GPIO_CONFIG, params);
      // Simulator::self()->addEvent(time - Simulator::self()->circTime(), stm32_instance);
    };
  }
  if (cfghig_reg) {
    cfghig_reg->write_callback = [this](Register &reg, uint32_t new_register_value, void *user_data) {
      const auto stm32_instance = static_cast<Stm32 *>(user_data);
      const auto rcm_device = dynamic_cast<stm32Rcm::Rcm *>(stm32_instance->peripheral_registry->
        findDevice(RCM_BASE));
      EventParams params;
      params.gpio_config_data.port=m_port;
      params.gpio_config_data.config=new_register_value;
      qDebug()<<"cfglow_reg new_register_value:0x"<<Qt::hex<<new_register_value;
      params.gpio_config_data.shift=8;
      const uint64_t time = rcm_device->getMcuTime();
      stm32_instance->schedule_event(time- Simulator::self()->circTime(), EventActionType::GPIO_CONFIG, params);
     // Simulator::self()->addEvent(time - Simulator::self()->circTime(), stm32_instance);
    };
  }

  // --- 5. GPIOx_LOCK (0x18): 锁定寄存器
  // 监控 LOCKKEY 的写入序列，这可能会影响 CFGLOW/CFGHIG 的可写性
  auto *lock_reg = m_registers[LOCK_OFFSET].get();
  if (lock_reg) {
    // 监控 LOCKKEY 字段
    auto *lockkey_field = lock_reg->find_field("LOCKKEY");
    if (lockkey_field) {
      lockkey_field->write_callback = [](Register &reg,
                                         const BitField &field,
                                         uint32_t new_field_value,
                                         void *user_data) {
        // TODO: 实现锁定序列检查和锁定/解锁配置寄存器的逻辑。
      };
    }
  }

  // TODO: 如果 GPIOx_IDATA 是输入，您还需要一个机制（例如外部 I/O 模块或回调）
  // 来**非侵入式**地更新 GPIOx_IDATA 寄存器的内部值，以模拟引脚的外部输入变化。
}
bool Gpio::is_clock_enabled(void *user_data) const {
  //a,b,c,d,e,均挂在ahpb2上,所以从AHPB2上查询
  const auto stm32_instance = static_cast<Stm32 *>(user_data);
  const auto rcm_device = dynamic_cast<stm32Rcm::Rcm *>(stm32_instance->peripheral_registry->
    findDevice(RCM_BASE));
  string name = "PAEN";
  switch (m_base_addr) {
    case GPIOA_BASE:
      name = "PAEN";
      break;
    case GPIOB_BASE:
      name = "PBEN";
      break;
    case GPIOC_BASE:
      name = "PCEN";
      break;
    case GPIOD_BASE:
      name = "PDEN";
      break;
    case GPIOE_BASE:
      name = "PEEN";
      break;
    default:
      qWarning() << "[GPIO] Gpio::is_clock_enabled Error: Unknown base address 0x" << Qt::hex <<
          m_base_addr << "name be PAEN ";
      break;
  }
  return rcm_device->isAHPB2ClockEnabled(name);
}


// -----------------------------------------------------------------------------
// 读操作处理
// -----------------------------------------------------------------------------

/**
 * @brief 处理对 GPIO 寄存器的读操作
 */
bool Gpio::handle_read(uc_engine *uc,
                       uint64_t address,
                       int size,
                       int64_t *read_value,
                       void *user_data) {
  // 仅支持 32 位读写
  if (size != 4) {
    qWarning() << "[GPIO R] Error: Only 32-bit (4 byte) access is supported for" << getName().
        c_str() << Qt::endl;
    return false;
  }
  const auto offset = static_cast<uint32_t>(address - m_base_addr);
  uint32_t stored_value = 0;
  if (m_registers.count(offset)) {
    stored_value = m_registers[offset]->read(user_data);
  } else {
    qWarning().noquote() << "   [GPIO R: 0x" << Qt::hex << offset << "] 警告: 访问未注册寄存器!";
    *read_value = 0;
    return false;
  }
  *read_value = static_cast<int64_t>(stored_value);
  return true;
}


// -----------------------------------------------------------------------------
// 写操作处理
// -----------------------------------------------------------------------------

/**
 * @brief 处理对 GPIO 寄存器的写操作
 */
bool Gpio::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value, void *user_data) {
  // 仅支持 32 位读写
  if (size != 4) {
    qWarning("[GPIO W] Error: Only 32-bit (4 byte) access is supported for %s", getName().c_str());
    return false;
  }
  const auto offset = static_cast<uint32_t>(address - m_base_addr);
  if (m_registers.count(offset)) {
    const auto new_value = static_cast<uint32_t>(value);
   // qDebug()<<"Gpio::handle_write:0x"<<Qt::hex<<new_value;
    m_registers[offset]->write(new_value, size, user_data);
  } else {
    qWarning().noquote() << "   [GPIO W: 0x" << Qt::hex << offset << "] 警告: 访问未注册寄存器!";
    return false;
  }
  return true;
}
