#include "Afio.h"
#include <cstdint>
#include <iomanip>

/**
 * @brief Afio 类的构造函数
 */
Afio::Afio() : m_base_addr(AFIO_BASE_ADDR) {
  // 初始化寄存器到复位值
  initialize_registers();
  qDebug("[AFIO] Initialized %s at base address 0x%x\n",
         Afio::getName().c_str(),
         static_cast<unsigned>(m_base_addr));
}

/**
 * @brief 初始化 AFIO 寄存器到复位值
 */
void Afio::initialize_registers() {
  // 所有寄存器的复位值均为 0x0000 0000
  m_registers.EVCTRL = (uint32_t) 0x00000000;
  m_registers.REMAP1 = (uint32_t) 0x00000000;
  m_registers.EINTSEL1 = (uint32_t) 0x00000000;
  m_registers.EINTSEL2 = (uint32_t) 0x00000000;
  m_registers.EINTSEL3 = (uint32_t) 0x00000000;
  m_registers.EINTSEL4 = (uint32_t) 0x00000000;
}

/**
 * @brief 检查 AFIO 时钟是否开启
 * @note 这是一个简化的检查，实际模拟中需要与 RCM 模块交互
 */
bool Afio::is_clock_enabled(uc_engine *uc) const {
  // 简化: 总是返回 true，或根据您的模拟环境实现 APB2CLKEN_ADDR 的读取逻辑
  // 实际应读取 RCM_APB2CLKEN 寄存器的 AFIOEN 位 (位 0)
  // 假设 RCM_APB2CLKEN 寄存器已被正确映射
  return true;
}


// -----------------------------------------------------------------------------
// 读操作处理
// -----------------------------------------------------------------------------

/**
 * @brief 处理对 AFIO 寄存器的读操作
 */
bool Afio::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value, void *user_data) {
  if (size != 4) {
    qWarning("[AFIO] Error: Only 32-bit (4 byte) access is supported for%s", getName().c_str());
    return false;
  }

  uint32_t offset = (uint32_t) (address - m_base_addr);
  uint32_t reg_value = 0;

  switch (offset) {
    case AFIO_EVCTRL_OFFSET:
      reg_value = m_registers.EVCTRL;
      break;
    case AFIO_REMAP1_OFFSET:
      // SWJCFG 位 26:24 只写，读这些位将返回未定义的数值
      // 简单模拟：将这些位清零
      reg_value = m_registers.REMAP1 & (~AFIO_REMAP1_SWJCFG_MASK);
      break;
    case AFIO_EINTSEL1_OFFSET:
      reg_value = m_registers.EINTSEL1;
      break;
    case AFIO_EINTSEL2_OFFSET:
      reg_value = m_registers.EINTSEL2;
      break;
    case AFIO_EINTSEL3_OFFSET:
      reg_value = m_registers.EINTSEL3;
      break;
    case AFIO_EINTSEL4_OFFSET:
      reg_value = m_registers.EINTSEL4;
      break;
    default:
      qWarning("[AFIO] Warning: Read from unsupported offset 0x%X in %s",
               offset,
               getName().c_str());
      break;
  }

  *read_value = (int64_t) reg_value;
  qDebug("[AFIO] Read 0x%08llX from %s at 0x%X",
         static_cast<unsigned long long>(reg_value),
         getName().c_str(),
         static_cast<unsigned>(address));
  return true;
}


// -----------------------------------------------------------------------------
// 写操作处理
// -----------------------------------------------------------------------------

/**
 * @brief 处理对 AFIO 寄存器的写操作
 */
bool Afio::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value, void *user_data) {
  if (size != 4) {
    qWarning("[AFIO] Error: Only 32-bit (4 byte) access is supported for %s",getName().c_str());
    return false;
  }

  auto offset = (uint32_t) (address - m_base_addr);
  auto write_value = (uint32_t) value;

  // 根据文档，操作前应当首先打开 AFIO 的时钟
  // 暂时跳过时钟检查
  // if (!is_clock_enabled(uc)) {
  //     std::cerr << "[AFIO] Error: Attempted write when clock is disabled." << std::endl;
  //     return true; // 硬件通常会忽略写操作，但模拟器可能继续执行
  // }

  switch (offset) {
    case AFIO_EVCTRL_OFFSET:
      m_registers.EVCTRL = write_value;
      break;
    case AFIO_REMAP1_OFFSET:
      // SWJCFG (位 26:24) 是只写域
      // 其他位是 R/W
      // 仅更新 R/W 位和只写位
      m_registers.REMAP1 = (m_registers.REMAP1 & AFIO_REMAP1_SWJCFG_MASK) | (write_value & (~
        AFIO_REMAP1_SWJCFG_MASK));
      // 确保只写位被更新
      m_registers.REMAP1 = (m_registers.REMAP1 & (~AFIO_REMAP1_SWJCFG_MASK)) | (write_value &
        AFIO_REMAP1_SWJCFG_MASK);

      // 简单实现：直接写入 (但请注意 SWJCFG 是只写，写入后不能读出)
      // m_registers.REMAP1 = write_value;
      break;
    case AFIO_EINTSEL1_OFFSET:
      m_registers.EINTSEL1 = write_value;
      break;
    case AFIO_EINTSEL2_OFFSET:
      m_registers.EINTSEL2 = write_value;
      break;
    case AFIO_EINTSEL3_OFFSET:
      m_registers.EINTSEL3 = write_value;
      break;
    case AFIO_EINTSEL4_OFFSET:
      m_registers.EINTSEL4 = write_value;
      break;
    default:
      qWarning("[AFIO] Warning: Write to unsupported offset 0x%llX in %s. Value: 0x%08llX",
         static_cast<unsigned long long>(offset),
         getName().c_str(),
         static_cast<unsigned long long>(write_value));
      return false;
  }
  qDebug("[AFIO] Write 0x%08llX to %s at 0x%llX",
         static_cast<unsigned long long>(write_value),
         getName().c_str(),
         static_cast<unsigned long long>(address));
  return true;
}
