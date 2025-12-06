#include "stm32Gpio.h"

#include <cstdint>
#include <iostream>
#include <utility>

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
  initialize_registers();
  qDebug() << "[GPIO] Initialized" << Gpio::getName().c_str()
      << "at base address 0x" << Qt::hex << m_base_addr;
}
/**
 * @brief 初始化 GPIO 寄存器到复位值
 */
void Gpio::initialize_registers() {
  // 端口配置低寄存器 (GPIOx_CFGLOW)，偏移地址 0x00
  m_registers[CFGLOW_OFFSET] = std::make_unique<stm32Gpio::GPIOx_CFGLOW>(CFGLOW_OFFSET);
  // 端口配置高寄存器 (GPIOx_CFGHIG)，偏移地址 0x04
  m_registers[CFGHIG_OFFSET] = std::make_unique<stm32Gpio::GPIOx_CFGHIG>(CFGHIG_OFFSET);
  // 端口输入数据寄存器 (GPIOx_IDATA)，偏移地址 0x08
  m_registers[IDATA_OFFSET] = std::make_unique<stm32Gpio::GPIOx_IDATA>(IDATA_OFFSET);
  // 端口输出数据寄存器 (GPIOx_ODATA)，偏移地址 0x0C
  m_registers[ODATA_OFFSET] = std::make_unique<stm32Gpio::GPIOx_ODATA>(ODATA_OFFSET);
  // 端口位设置/清除寄存器 (GPIOx_BSC)，偏移地址 0x10
  m_registers[BSC_OFFSET] = std::make_unique<stm32Gpio::GPIOx_BSC>(BSC_OFFSET);
  // 端口位清除寄存器 (GPIOx_BC)，偏移地址 0x14
  m_registers[BC_OFFSET] = std::make_unique<stm32Gpio::GPIOx_BC>(BC_OFFSET);
  // 端口配置锁定寄存器 (GPIOx_LOCK)，偏移地址 0x18
  m_registers[LOCK_OFFSET] = std::make_unique<stm32Gpio::GPIOx_LOCK>(LOCK_OFFSET);}
bool Gpio::is_clock_enabled(void *user_data) const {
  //a,b,c,d,e,均挂在ahpb2上,所以从AHPB2上查询
  const auto stm32_instance = static_cast<Stm32*>(user_data);
  const auto rcm_device = dynamic_cast<stm32Rcm::Rcm*>(stm32_instance->peripheral_registry->findDevice(RCM_BASE));
  string name="PAEN";
  switch (m_base_addr) {
    case GPIOA_BASE:
      name="PAEN";
      break;
      case GPIOB_BASE:
      name="PBEN";
      break;
      case GPIOC_BASE:
      name="PCEN";
      break;
      case GPIOD_BASE:
      name="PDEN";
      break;
      case GPIOE_BASE:
      name="PEEN";
      break;
      default:
      qWarning() << "[GPIO] Gpio::is_clock_enabled Error: Unknown base address 0x" << Qt::hex << m_base_addr<<"name be PAEN ";
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
    std::cerr << "[GPIO] Error: Only 32-bit (4 byte) access is supported for " << getName() <<
        std::endl;
    return false;
  }

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
    qWarning("[GPIO] Error: Only 32-bit (4 byte) access is supported for %s", getName().c_str());
    return false;
  }

  auto offset = (uint32_t) (address - m_base_addr);
  auto write_value = (uint32_t) value;

  // std::cout << "[GPIO] Write 0x" << std::hex << write_value << " to 0x" << address << std::endl;
  return true;
}
