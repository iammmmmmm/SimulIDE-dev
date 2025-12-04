#include "rcm.h"
#include <iomanip>
#include <iostream>
#include <qlogging.h>
#include <qtextstream.h>
#include <unicorn/unicorn.h>

#include "simulator.h"
#include "stm32.h"
Rcm::Rcm(uint64_t hsi_freq, uint64_t hse_freq, uint64_t max_cpu_freq):Rcm() {
  HSI_FREQ = hsi_freq;
  HSE_FREQ = hse_freq;
  MAX_CPU_FREQ = max_cpu_freq;
}
// 实现 CMU 构造函数
Rcm::Rcm() {
  // 调用初始化函数设置复位值
  initialize_registers();
  qDebug("[RCM] peripheral initialized at base address %x", RCM_BASE);
}

// 寄存器初始化
void Rcm::initialize_registers() {
  // 根据手册提供的复位值初始化寄存器。
  // 注意：手册中对于 RCM_CTRL 的复位值是 0x0000 XX83 (X代表未定义)，
  // 我们将其视为可读的 HSICAL 字段 (8:15) 的值，我们模拟将其初始化为 0x00
  // 对于 CSTS，复位值是 0x0C00 0000。

  // RCM_CTRL (0x00) - 复位值: 0x0000 XX83 (X=未定义，我们用0模拟)
  // 0x0000 0083
  // HSIEN=1, HSIRDYFLG=1, HSITRM=0x10 (默认值，但手册未给出具体值，这里假设为 0)
  // 根据描述：上电时HSICLK开启且稳定。HSICAL我们不能写，这里只初始化可写位。
  // RCM_CTRL 可写位复位后，只有 HSIEN=1，所以是 0x00000001。
  // 可读位 HSIRDYFLG 也会被置 1，HSICAL 会有值。我们初始化为所有可读位和可写位的默认状态：
  m_registers.CTRL = (uint32_t) 0x00000083; // 假设 HSICAL=0, HSITRM=0 (实际硬件值由硬件决定)

  // RCM_CFG (0x04) - 复位值: 0x0000 0000
  m_registers.CFG = (uint32_t) 0x00000000;

  // RCM_INT (0x08) - 复位值: 0x0000 0000
  m_registers.INT = (uint32_t) 0x00000000;

  // RCM_APB2RST (0x0C) - 复位值: 0x0000 0000
  m_registers.APB2RST = (uint32_t) 0x00000000;

  // RCM_APB1RST (0x10) - 复位值: 0x0000 0000
  m_registers.APB1RST = (uint32_t) 0x00000000;

  // RCM_AHBCLKEN (0x14) - 复位值: 0x0000 0014 (SRAMEN=1, FMCEN=1)
  m_registers.AHBCLKEN = (uint32_t) 0x00000014;

  // RCM_APB2CLKEN (0x18) - 复位值: 0x0000 0000
  m_registers.APB2CLKEN = (uint32_t) 0x00000000;

  // RCM_APB1CLKEN (0x1C) - 复位值: 0x0000 0000
  m_registers.APB1CLKEN = (uint32_t) 0x00000000;

  // RCM_BDCTRL (0x20) - 复位值: 0x0000 0000
  // 注意：BDCTRL只能由备份域复位有效复位
  m_registers.BDCTRL = (uint32_t) 0x00000000;

  // RCM_CSTS (0x24) - 复位值: 0x0C00 0000
  // PODRSTFLG=1, SWRSTFLG=1, 假设其他复位标志在系统启动时为0。
  // 0x0C00 0000 = (PODRSTFLG | SWRSTFLG) = 1<<27 | 1<<28 (根据描述 0x0C000000)
  // 0x0C00 0000 对应位 26, 27。 26: NRSTFLG, 27: PODRSTFLG。
  m_registers.CSTS = (uint32_t) 0x0C000000;

  // 假设 PLL/HSE 默认是关闭和未就绪的
  m_registers.CTRL &= ~((1 << 17) | (1 << 25));
}

// 处理写入操作
bool Rcm::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value, void *user_data) {
  if (size != 4) {
    // 仅支持 32 位写入
    qWarning("   [CMU W] WARing: Non-32-bit write operations are ignored. Address: %x",
             static_cast<unsigned>(address));
    return true;
  }
  uint32_t offset = address - RCM_BASE;
  uint32_t data = static_cast<uint32_t>(value);
  volatile unsigned *reg = nullptr;
  uint32_t w_mask = 0;
  uint32_t rc_mask = 0; // 写 1 清除标志位的掩码
  uint32_t hseen_bit = 1 << 16; // Bit 16: HSEEN
  uint32_t hserdyflg_bit = 1 << 17; // Bit 17: HSERDYFLG
  switch (offset) {
    case RCM_CTRL_OFFSET: {
      uint32_t current_ctrl = m_registers.CTRL;
      uint32_t data_value = data;

      // *** 强制定义 HSEEN 和整个可写掩码 (不再使用宏 RCM_CTRL_W_MASK) ***
      // Bit 16: HSEEN
      const uint32_t HSEEN_BIT = (1 << 16);
      // Bit 0: HSIEN
      const uint32_t HSIEN_BIT = (1 << 0);

      // 假设这是所有可写位的组合 (0x011C00BD)
      // 注意：这里手动组合 HSEEN 位，以确保它被包含
      const uint32_t W_MASK_FORCED = (HSEEN_BIT | HSIEN_BIT | (0x011C00BD & ~(HSEEN_BIT | HSIEN_BIT)));

      // *** 启动延迟检测 (使用 HSEEN_BIT) ***
      if ((data_value & HSEEN_BIT) && !(current_ctrl & HSEEN_BIT)) {
        m_clock_timing.hse_start_tick = m_clock_timing.rcm_ticks;
        qDebug()<<"rcm: set hse_start_tick:"<<m_clock_timing.rcm_ticks;
      }

      // *** 写入逻辑：使用 W_MASK_FORCED ***

      // 步骤 1: 提取只读状态 (R/O)
      // 关键修正：使用强制组合的掩码进行位反转
      uint32_t ro_state = current_ctrl & (~W_MASK_FORCED);

      // 步骤 2: 提取写入数据中的可写位 (R/W)
      uint32_t new_rw_bits = data_value & W_MASK_FORCED;

      // 步骤 3: 合并 R/O 状态和新的 R/W 状态
      uint32_t new_ctrl_value = ro_state | new_rw_bits;
      m_registers.CTRL = new_ctrl_value;

      // *** 检查最终结果 ***
      qDebug("[RCM] CTRL write: m_registers.CTRL=0x%x, data=0x%x, ticks=%llu, NEW_CTRL=0x%x",
          current_ctrl, data_value, m_clock_timing.rcm_ticks, m_registers.CTRL);

      if (m_registers.CTRL & HSEEN_BIT) {
        qDebug("RCM SUCCESS! HSEEN (Bit 16) successfully set! CTRL=0x%x", m_registers.CTRL);
      } else {
        // 如果到这一步仍然失败，则问题是 m_registers.CTRL 变量本身存在问题（例如，指针/内存问题），而不是逻辑或宏定义问题。
        qCritical("RCM FATAL ERROR: HSEEN (Bit 16) NOT SET after write. **MEMORY/POINTER ISSUE**");
      }

      return true;
    }
    case RCM_CFG_OFFSET:
      reg = &m_registers.CFG;
      w_mask = RCM_CFG_W_MASK;
      break;
    case RCM_INT_OFFSET:{
      volatile uint32_t *reg_int = &m_registers.INT;
      uint32_t current_reg = *reg_int;
      uint32_t data_value = data; // 写入的新数据

      // --- 1. 处理清除位 (W1C 逻辑) ---
      // 清除操作是根据写入值(data)中的 Bit 16-23 来清除寄存器中 Bit 0-7 的标志位。

      // CSSCLR (Bit 23) 清除 CSSFLG (Bit 7)
      if (data_value & (1 << 23)) {
        current_reg &= ~(1 << 7);
      }

      // PLLRDYCLR (Bit 20) 清除 PLLRDYFLG (Bit 4)
      if (data_value & (1 << 20)) {
        current_reg &= ~(1 << 4);
      }

      // HSERDYCLR (Bit 19) 清除 HSERDYFLG (Bit 3)
      if (data_value & (1 << 19)) {
        current_reg &= ~(1 << 3);
      }

      // HSIRDYCLR (Bit 18) 清除 HSIRDYFLG (Bit 2)
      if (data_value & (1 << 18)) {
        current_reg &= ~(1 << 2);
      }

      // LSERDYCLR (Bit 17) 清除 LSERDYFLG (Bit 1)
      if (data_value & (1 << 17)) {
        current_reg &= ~(1 << 1);
      }

      // LSIRDYCLR (Bit 16) 清除 LSIRDYFLG (Bit 0)
      if (data_value & (1 << 16)) {
        current_reg &= ~(1 << 0);
      }

      // --- 2. 处理使能位 (R/W 逻辑) ---
      // 使能位是 Bit 8-12 (*EN)。写入值中包含新的使能状态。
      uint32_t en_mask = 0x1F00; // Bit 8-12

      // A. 清除旧的 EN 位
      current_reg &= ~en_mask;
      // B. 写入新的 EN 位 (写入值中 Bit 8-12 的部分)
      current_reg |= (data_value & en_mask);

      // 3. 将清除和使能后的最终值写回寄存器
      *reg_int = current_reg;

      return true;
    }
      // reg = &m_registers.INT;
      // w_mask = RCM_INT_W_MASK;
      // rc_mask = RCM_INT_RC_MASK;
      // break;
    case RCM_APB2RST_OFFSET:
    case RCM_APB1RST_OFFSET:
      // 复位寄存器特殊处理：写 1 置位复位，触发外设复位行为
      reg = (offset == RCM_APB2RST_OFFSET) ? &m_registers.APB2RST : &m_registers.APB1RST;
      w_mask = (offset == RCM_APB2RST_OFFSET) ? RCM_APB2RST_W_MASK : RCM_APB1RST_W_MASK;

      // 仅写入 data 中被掩码允许的位
      *reg |= (data & w_mask);
      // 注意：此时需要通知对应的外设执行 reset() 方法！

      qDebug("[RCM]: Triggered reset for peripheral(s) at %x with mask %x", offset, *reg);
      break;
    case RCM_AHBCLKEN_OFFSET:
      reg = &m_registers.AHBCLKEN;
      w_mask = RCM_AHBCLKEN_W_MASK;
      break;
    case RCM_APB2CLKEN_OFFSET:
      reg = &m_registers.APB2CLKEN;
      w_mask = RCM_APB2CLKEN_W_MASK;
      break;
    case RCM_APB1CLKEN_OFFSET:
      reg = &m_registers.APB1CLKEN;
      w_mask = RCM_APB1CLKEN_W_MASK;
      break;
    case RCM_BDCTRL_OFFSET:
      reg = &m_registers.BDCTRL;
      w_mask = RCM_BDCTRL_W_MASK;
      break;
    case RCM_CSTS_OFFSET:
      reg = &m_registers.CSTS;
      w_mask = RCM_CSTS_W_MASK;
      rc_mask = RCM_CSTS_RC_MASK;
      break;
    default:
      qWarning("[RCM] Write error: Invalid offset %x%s%x",
               offset,
               " at 0x",
               static_cast<unsigned>(address));
      return false;
  }
  if (reg) {
    // 1. 处理写 1 清除 (W1C) 逻辑
    if (rc_mask != 0) {
      // 清除: ~(data & rc_mask) 只保留非清除位
      // 如果 data 的相应位为 1，则清零 *reg 中对应的位
      *reg &= ~(data & rc_mask);
    }

    // 2. 处理普通读写 (R/W) 逻辑
    // 对于 RCM_APB2RST/RCM_APB1RST，我们在 case 块中已经处理，跳过通用逻辑
    if (offset != RCM_APB2RST_OFFSET && offset != RCM_APB1RST_OFFSET) {
      // 步骤 1: 提取当前寄存器的只读位 (R/O) 状态，包括 HSERDYFLG
      // 只保留不在 w_mask 中的位
      uint32_t ro_state = *reg & (~w_mask);

      // 步骤 2: 提取写入数据中的可写位 (R/W)
      uint32_t new_rw_bits = data & w_mask;

      // 步骤 3: 合并 R/O 状态和新的 R/W 状态
      *reg = ro_state | new_rw_bits;
    }
    // 仅在 RCM_CTRL 写入发生时，或在关键时间段，输出详细信息
    if (offset == RCM_CTRL_OFFSET || (m_clock_timing.rcm_ticks > 498 && m_clock_timing.rcm_ticks < 3000)) {
      qDebug("[RCM DEBUG W] Address: 0x%x, Offset: 0x%x, Data: 0x%x, Tick: %llu",
             static_cast<unsigned>(address),
             offset,
             data,
             m_clock_timing.rcm_ticks);
    }
    return true;
  }
  return false;
}

// 读取处理
bool Rcm::handle_read(uc_engine *uc,
                      uint64_t address,
                      int size,
                      int64_t *read_value,
                      void *user_data) {
  uint32_t offset = (uint32_t) (address - RCM_BASE);
  uint32_t reg_value = 0;
  uint32_t read_mask = 0;
  auto stm32_instance = static_cast<Stm32 *>(user_data);
  if (!stm32_instance) {
    qWarning() <<
        "[RCM] stm32_instance=NULL!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
    return false;
  }
  if (size != 4 || !read_value) {
    qWarning("[RCM] Read error:  Invalid access size or null pointer at%x",
             static_cast<unsigned>(address));
    return false;
  }

  switch (offset) {
    case RCM_CTRL_OFFSET:
      reg_value = m_registers.CTRL;
      read_mask = RCM_CTRL_R_MASK;
      break;
    case RCM_CFG_OFFSET:
      reg_value = m_registers.CFG;
      read_mask = RCM_CFG_R_MASK;
      // SCLKSELSTS (3:2) 应该反映当前使用的时钟源，这里简单起见，假设它等于 SYSCLKSEL
      reg_value |= ((reg_value >> 0) & 0b11) << 2;
      break;
    case RCM_INT_OFFSET:
      reg_value = m_registers.INT;
      read_mask = RCM_INT_R_MASK;
      break;
    case RCM_APB2RST_OFFSET:
      reg_value = m_registers.APB2RST;
      read_mask = RCM_APB2RST_R_MASK;
      break;
    case RCM_APB1RST_OFFSET:
      reg_value = m_registers.APB1RST;
      read_mask = RCM_APB1RST_R_MASK;
      break;
    case RCM_AHBCLKEN_OFFSET:
      reg_value = m_registers.AHBCLKEN;
      read_mask = RCM_AHBCLKEN_R_MASK;
      break;
    case RCM_APB2CLKEN_OFFSET:
      reg_value = m_registers.APB2CLKEN;
      read_mask = RCM_APB2CLKEN_R_MASK;
      break;
    case RCM_APB1CLKEN_OFFSET:
      reg_value = m_registers.APB1CLKEN;
      read_mask = RCM_APB1CLKEN_R_MASK;
      break;
    case RCM_BDCTRL_OFFSET:
      reg_value = m_registers.BDCTRL;
      read_mask = RCM_BDCTRL_R_MASK;
      reg_value |= (1 << 1); // 模拟 LSERDYFLG 就绪
      break;
    case RCM_CSTS_OFFSET:
      reg_value = m_registers.CSTS;
      read_mask = RCM_CSTS_R_MASK;
      reg_value |= (1 << 1); // 模拟 LSIRDYFLG 就绪
      break;
    default:
      std::cerr << "[RCM] Read error: Invalid offset 0x" << std::hex << offset << " at 0x" <<
          address << std::dec << std::endl;
      *read_value = 0;
      return false;
  }

  // 只返回可读位的值
  *read_value = (int64_t) (reg_value & read_mask);

// qDebug("[RCM] Read: Reg 0x%llX (0x%llX) masked to 0x%llX",static_cast<unsigned long long>(offset),static_cast<unsigned long long>(reg_value),static_cast<unsigned long long>(*read_value));
  return true;
}

//   // 模拟时间流逝和状态更新
//   void Rcm::run_tick(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
//   // qDebug("run_tick\n");
//
//   // 1. 模拟 PLL 就绪延迟
//   uint32_t pllen_bit = 1 << 24;
//   uint32_t pllrdyflg_bit = 1 << 25;
//
//   if ((m_registers.CTRL & (1 << 24)) && !(m_registers.CTRL &( 1 << 25))) {
//     // PLL 已使能 (PLLEN=1) 且未就绪 (PLLRDYFLG=0)
//     if (m_pll_wait_cycles > 0) {
//       m_pll_wait_cycles--;
//       if (m_pll_wait_cycles == 0) {
//         // PLL 稳定，设置 PLLRDYFLG (Bit 25)
//         m_registers.CTRL |= pllrdyflg_bit;
// qDebug("RCM: PLL Clock Ready (PLLRDYFLG set).");
//       }
//     }
//   }
//
//   // 2. 模拟外设复位自清零
//   // 在实际应用中，这里应该调用 PeripheralFactory 来通知复位
//   if (m_registers.APB2RST != 0) {
//     m_registers.APB2RST = 0;
//   }
//   if (m_registers.APB1RST != 0) {
//     m_registers.APB1RST = 0;
//   }
//
//   // 3. 其他时钟就绪标志（如 HSE/LSE/LSI）的逻辑...
//   }
void Rcm::run_tick(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
  m_clock_timing.rcm_ticks++;
  if (m_clock_timing.rcm_ticks%1000==0) {
     qDebug() << "[RCM] Run tick at 0x" << Qt::hex << address<<"rcm_ticks:"<<Qt::dec<<m_clock_timing.rcm_ticks;
  }
  const uint32_t pllrdyflg_bit = 1 << 25;
  const uint32_t pllen_bit = 1 << 24;
  const uint32_t hserdyflg_bit = 1 << 17;
  const uint32_t hseen_bit = 1 << 16;

  // 1. PLL 就绪逻辑
  if ((m_registers.CTRL & pllen_bit) && !(m_registers.CTRL & pllrdyflg_bit)) {
    // 检查是否超时
    if ((m_clock_timing.rcm_ticks - m_clock_timing.pll_start_tick) >= m_clock_timing.PLL_DELAY_TICKS) {
      m_registers.CTRL |= pllrdyflg_bit;
      qDebug("RCM: PLL Clock Ready (PLLRDYFLG set).\n\r");
    }
  }
  // 2. HSE 就绪逻辑
  if ((m_registers.CTRL & hseen_bit) && !(m_registers.CTRL & hserdyflg_bit)) {
    // 检查是否超时
    if ((m_clock_timing.rcm_ticks - m_clock_timing.hse_start_tick) >= m_clock_timing.HSE_DELAY_TICKS) {
      m_registers.CTRL |= hserdyflg_bit;
      qDebug("RCM: HSE Clock Ready (HSERDYFLG set).\n\r");
    }
  }

  // //调试检查
  // if (m_clock_timing.rcm_ticks == 1500) {
  //   if (m_registers.CTRL & hserdyflg_bit) {
  //     qDebug("[RCM DEBUG] HSERDYFLG IS set by tick 1500. CTRL=0x%x", m_registers.CTRL);
  //   } else {
  //     qDebug("[RCM DEBUG] HSERDYFLG NOT set by tick 1500. CTRL=0x%x", m_registers.CTRL);
  //   }
  // }
}
void Rcm::runEvent() {

}
uint64_t Rcm::getSysClockFrequency() const {
  // 获取 SYSCLKSEL (RCM_CFG Bit 0-1)
  uint32_t sysclk_sel = m_registers.CFG & 0b11;

  // 1. HSI 作为系统时钟 (SYSCLKSEL = 00)
  if (sysclk_sel == 0b00) {
    return HSI_FREQ;
  }
  // 2. HSE 作为系统时钟 (SYSCLKSEL = 01)
  if (sysclk_sel == 0b01) {
    return HSE_FREQ;
  }
  // 3. PLL 作为系统时钟 (SYSCLKSEL = 10)
  if (sysclk_sel == 0b10) {
    // --- 计算 PLL 输入频率 ---
    uint64_t pll_input_freq = 0;
    uint32_t pll_src_sel = (m_registers.CFG >> 16) & 0b1;
    uint32_t pll_hse_psc = (m_registers.CFG >> 17) & 0b1;

    if (pll_src_sel == 0) {
      // HSI 作为 PLL 源
      // HSI (8MHz) -> /2 分频器
      pll_input_freq = HSI_FREQ / 2;
    } else {
      // HSE 作为 PLL 源
      pll_input_freq = HSE_FREQ;
      if (pll_hse_psc == 1) {
        // PLLHSEPSC = 1 -> /2 分频
        pll_input_freq /= 2;
      }
    }

    // --- 计算 PLL 倍频系数 N ---
    uint32_t pll_mul_cfg = (m_registers.CFG >> 18) & 0b1111; // Bit 18-21
    // APM32F103 的 PLL 倍频系数通常是 (N+2)
    // pll_mul_cfg = 0b0000 -> x2, 0b1110 -> x16
    // PLLMULCFG 0b0000 对应 x2, 0b0001 对应 x3, ..., 0b1110 对应 x16。
    uint32_t N = pll_mul_cfg + 2;

    // --- 计算 PLL 输出频率 ---
    uint64_t pll_output_freq = pll_input_freq * N;

    // 确保频率不超过芯片限制 (96MHz MAX)
    return std::min(pll_output_freq, MAX_CPU_FREQ);
  }

  // 默认或复位状态 (默认为 HSICLK)
  return HSI_FREQ;
}
