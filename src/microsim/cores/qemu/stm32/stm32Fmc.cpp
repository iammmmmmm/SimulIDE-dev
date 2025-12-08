#include "stm32Fmc.h"
#include <iostream>
#include <iomanip>
#include "stm32FmcReg.h"
using namespace stm32Fmc;
/**
 * @brief Fmc 类的构造函数
 */
Fmc::Fmc() {
  // 初始化寄存器到复位值
  initialize_registers();
  qDebug("[FMC] Initialized %s at base address 0x%llX",
         Fmc::getName().c_str(),
         static_cast<unsigned long long>(m_base_addr));
}

/**
 * @brief 初始化 FMC 寄存器到复位值
 */
void Fmc::initialize_registers() {
  m_registers[FMC_CTRL1_OFFSET] = std::make_unique<FMC_CTRL1>(FMC_CTRL1_OFFSET);
  m_registers[FMC_KEY_OFFSET] = std::make_unique<FMC_KEY>(FMC_KEY_OFFSET);
  m_registers[FMC_OBKEY_OFFSET] = std::make_unique<FMC_OBKEY>(FMC_OBKEY_OFFSET);
  m_registers[FMC_STS_OFFSET] = std::make_unique<FMC_STS>(FMC_STS_OFFSET);
  m_registers[FMC_CTRL2_OFFSET] = std::make_unique<FMC_CTRL2>(FMC_CTRL2_OFFSET);
  m_registers[FMC_ADDR_OFFSET] = std::make_unique<FMC_ADDR>(FMC_ADDR_OFFSET);
  m_registers[FMC_OBCS_OFFSET] = std::make_unique<FMC_OBCS>(FMC_OBCS_OFFSET);
  m_registers[FMC_WRTPROT_OFFSET] = std::make_unique<FMC_WRTPROT>(FMC_WRTPROT_OFFSET);
}

/**
 * @brief 处理对 FMC 寄存器的读操作
 */
bool Fmc::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value, void *user_data) {
  if (size != 4) {
    qWarning("[FMC R] Error: Only 32-bit (4 byte) access is supported ");
    return false;
  }
  const auto offset = static_cast<uint32_t>(address - FMC_BASE_ADDR);
  uint32_t stored_value = 0;
  if (m_registers.count(offset)) {
    stored_value = m_registers[offset]->read(user_data);
  }else {
    qWarning().noquote() << QString("   [FMC R: 0x%1] 警告: 访问未注册寄存器!").arg(offset, 0, 16);
    *read_value = 0;
    return false;
  }
  *read_value = static_cast<int64_t>(stored_value);
  // *read_value = (int64_t) reg_value;
  // qDebug("[FMC] Read 0x%llX from 0x%llX",
  //        static_cast<unsigned long long>(reg_value),
  //        static_cast<unsigned long long>(address));
  return true;
}


// -----------------------------------------------------------------------------
// 写操作处理
// -----------------------------------------------------------------------------

/**
 * @brief 处理对 FMC 寄存器的写操作
 */
bool Fmc::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value, void *user_data) {
  if (size != 4) {
    std::cerr << "[FMC W] Error: Only 32-bit (4 byte) access is supported for " << getName() <<
        std::endl;
    return false;
  }
  const auto offset = static_cast<uint32_t>(address - FMC_BASE_ADDR);
  if (m_registers.count(offset)) {
    const auto new_value = static_cast<uint32_t>(value);
    m_registers[offset]->write(new_value,size, user_data);
  }else {
    qWarning().noquote() << QString("   [FMC W: 0x%1] 警告: 访问未注册寄存器!").arg(offset, 0, 16);
    return false;
  }

 // qDebug("[FMC] Write 0x%llX to 0x%llX",static_cast<unsigned long long>(write_value),static_cast<unsigned long long>(address));
  return true;
}
