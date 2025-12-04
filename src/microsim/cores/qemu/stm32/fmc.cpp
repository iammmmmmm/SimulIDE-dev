#include "fmc.h"

#include <cstdint>
#include <iostream>
#include <iomanip> // 用于 std::hex, std::setw

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
  // FMC_CTRL1 (0x00) 复位值: 0x0000 0030
  m_registers.CTRL1 = (uint32_t) 0x00000030;
  // FMC_STS (0x0C) 复位值: 0x0000 0000
  m_registers.STS = (uint32_t) 0x00000000;
  // FMC_CTRL2 (0x10) 复位值: 0x0000 0080 (LOCK 位为 1)
  m_registers.CTRL2 = (uint32_t) 0x00000080;
  // FMC_OBCS (0x1C) 复位值: 0x03FF FFFC
  m_registers.OBCS = (uint32_t) 0x03FFFFFC;
  // FMC_WRTPROT (0x20) 复位值: 0xFFFF FFFF (读保护无效)
  // WRTPROT 是只读，不在这里设置，但在 read 中返回。
  // KEY, OBKEY, ADDR 为只写寄存器，在结构体中标记为 const，不显式初始化。

  // 锁定状态由 CTRL2 寄存器的 LOCK 位决定
  m_is_locked = (m_registers.CTRL2 & FMC_CTRL2_LOCK) != 0;
}

// /**
//  * @brief 更新/清除状态标志位 (PEF, WPEF, OCF)
//  * 状态寄存器 (FMC_STS) 的这些标志位是通过软件写入 1 来清除的 (Write 1 to Clear, W1C)
//  */
// void Fmc::update_sts_flags(uint32_t clear_mask) {
//     // 只有 PEF, WPEF, OCF 可以 W1C
//     uint32_t clr_mask = clear_mask & (FMC_STS_PEF | FMC_STS_WPEF | FMC_STS_OCF);
//
//     // 使用 NAND 逻辑实现 W1C
//     m_registers.STS &= ~clr_mask;
// }


// -----------------------------------------------------------------------------
// 读操作处理
// -----------------------------------------------------------------------------

/**
 * @brief 处理对 FMC 寄存器的读操作
 */
bool Fmc::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value, void *user_data) {
  if (size != 4) {
    qWarning("[FMC] Error: Only 32-bit (4 byte) access is supported for %s", getName().c_str());
    return false;
  }

  auto offset = (uint32_t) (address - m_base_addr);
  auto reg_value = 0;

  switch (offset) {
    case FMC_CTRL1_OFFSET:
      reg_value = m_registers.CTRL1;
      break;
    case FMC_KEY_OFFSET:
    case FMC_OBKEY_OFFSET:
      // 关键字寄存器 (KEY, OBKEY) 是只写寄存器，执行读操作时返回 0
      reg_value = 0x00000000;
      break;
    case FMC_STS_OFFSET:
      reg_value = m_registers.STS;
      break;
    case FMC_CTRL2_OFFSET:
      reg_value = m_registers.CTRL2;
      break;
    case FMC_ADDR_OFFSET:
      // ADDR 寄存器在文档中描述为 W，但硬件中通常可读，这里模拟只写，返回 0
      // 实际读取的值需要查阅更详细的硬件手册。这里暂返回 0。
      reg_value = 0x00000000;
      break;
    case FMC_OBCS_OFFSET:
      reg_value = m_registers.OBCS;
      break;
    case FMC_WRTPROT_OFFSET:
      // WRTPROT 是只读寄存器，返回其当前值。
      // 复位值: 0xFFFF FFFF (写保护无效)
      reg_value = m_registers.WRTPROT;
      break;
    default:
      // 0x18 是保留地址
      qWarning("[FMC] Warning: Read from unsupported offset 0x%llX in %s",
               static_cast<unsigned long long>(offset),
               getName().c_str());
      reg_value = 0;
      break;
  }

  *read_value = (int64_t) reg_value;
  qDebug("[FMC] Read 0x%llX from 0x%llX",
         static_cast<unsigned long long>(reg_value),
         static_cast<unsigned long long>(address));
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
    std::cerr << "[FMC] Error: Only 32-bit (4 byte) access is supported for " << getName() <<
        std::endl;
    return false;
  }

  auto offset = (uint32_t) (address - m_base_addr);
  auto write_value = (uint32_t) value;

  switch (offset) {
    case FMC_CTRL1_OFFSET:
      // 写入 CTRL1
      m_registers.CTRL1 = write_value;
      // TODO: 可以在这里添加对 CTRL1 中 PBEN (预取缓存) 位设置的模拟反馈，例如设置 PBSF 位
      break;
    case FMC_KEY_OFFSET: {
      // 关键字寄存器: 用于解锁 FMC
      // 这里简化模拟解锁过程
      if (write_value == FMC_KEY1) {
        m_key_sequence_step = 1;
      } else if (m_key_sequence_step == 1 && write_value == FMC_KEY2) {
        // 解锁成功
        m_is_locked = false;
        m_registers.CTRL2 &= ~FMC_CTRL2_LOCK; // 清除 LOCK 位
        m_key_sequence_step = 0;
        std::cout << "[FMC] **Flash Unlocked**" << std::endl;
      } else {
        // 错误的序列，重置
        m_key_sequence_step = 0;
      }
      break;
    }
    case FMC_OBKEY_OFFSET:
      // 选项字节关键字寄存器: 用于解锁选项字节写操作
      // 简化: 忽略写操作，但可以添加实际解锁逻辑
      break;
    case FMC_STS_OFFSET:
      // 状态寄存器: 标志位 (PEF, WPEF, OCF) 是 W1C (Write 1 to Clear)
      //update_sts_flags(write_value);
      break;
    case FMC_CTRL2_OFFSET:
      // 控制寄存器2
      // LOCK 位 (位 7) 只能写 1，用于锁定，锁定后 CTRL2 和 FMC 寄存器被锁定
      if (write_value & FMC_CTRL2_LOCK) {
        m_is_locked = true;
        write_value |= FMC_CTRL2_LOCK; // 确保 LOCK 位被设置
        qDebug("[FMC] **Flash Locked**");
      }

      // 只有当 FMC 未锁定时，其他控制位才可写
      if (!m_is_locked) {
        m_registers.CTRL2 = write_value;
        // TODO: 在这里添加对编程 (PG)、擦除 (PAGEERA/MASSERA) 操作的模拟逻辑
        // 并在操作完成后设置 FMC_STS_OCF 或 FMC_STS_PEF/WPEF 标志
        // 如果写入 PG 等操作位，需要模拟 BUSY 状态
      } else {
        // 仅更新 LOCK 位 (如果 LOCK 位被写入 1)
        m_registers.CTRL2 |= (write_value & FMC_CTRL2_LOCK);
      }

      break;
    case FMC_ADDR_OFFSET:
      // 地址寄存器: 写入要编程/擦除的地址
      // 由于结构体中标记为 const，我们不能直接写 m_registers.ADDR。这里使用内存来跟踪。
      // 实际模拟中，这个值会被用于后续的编程/擦除操作
      // m_registers.ADDR = write_value;
      break;
    case FMC_OBCS_OFFSET:
      // OBCS: 主要是状态寄存器，只有在选项字节编程时才可能写入
      // 简化: 忽略写操作
      break;
    case FMC_WRTPROT_OFFSET:
      // 写保护寄存器: 用于编程写保护配置
      // 简化: 忽略写操作
      break;
    default:
      qWarning("[FMC] Warning: Write to unsupported offset 0x%llX in %s. Value: 0x%08llX",
            static_cast<unsigned long long>(offset),
            getName().c_str(),
            static_cast<unsigned long long>(write_value));
      return false;
  }

  qDebug("[FMC] Write 0x%llX to 0x%llX",
       static_cast<unsigned long long>(write_value),
       static_cast<unsigned long long>(address));
  return true;
}
