#include "Rcm.h"
#include <iomanip>
#include <iostream>
#include <qlogging.h>
#include <qtextstream.h>
#include <unicorn/unicorn.h>
// 实现 CMU 构造函数
Rcm::Rcm() {
  // 调用初始化函数设置复位值
  initialize_registers();
  qDebug("RCM peripheral initialized at base address %x" , RCM_BASE );
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
  m_registers.CTRL = (uint32_t)0x00000083; // 假设 HSICAL=0, HSITRM=0 (实际硬件值由硬件决定)

  // RCM_CFG (0x04) - 复位值: 0x0000 0000
  m_registers.CFG = (uint32_t)0x00000000;

  // RCM_INT (0x08) - 复位值: 0x0000 0000
  m_registers.INT = (uint32_t)0x00000000;

  // RCM_APB2RST (0x0C) - 复位值: 0x0000 0000
  m_registers.APB2RST = (uint32_t)0x00000000;

  // RCM_APB1RST (0x10) - 复位值: 0x0000 0000
  m_registers.APB1RST = (uint32_t)0x00000000;

  // RCM_AHBCLKEN (0x14) - 复位值: 0x0000 0014 (SRAMEN=1, FMCEN=1)
  m_registers.AHBCLKEN = (uint32_t)0x00000014;

  // RCM_APB2CLKEN (0x18) - 复位值: 0x0000 0000
  m_registers.APB2CLKEN = (uint32_t)0x00000000;

  // RCM_APB1CLKEN (0x1C) - 复位值: 0x0000 0000
  m_registers.APB1CLKEN = (uint32_t)0x00000000;

  // RCM_BDCTRL (0x20) - 复位值: 0x0000 0000
  // 注意：BDCTRL只能由备份域复位有效复位
  m_registers.BDCTRL = (uint32_t)0x00000000;

  // RCM_CSTS (0x24) - 复位值: 0x0C00 0000
  // PODRSTFLG=1, SWRSTFLG=1, 假设其他复位标志在系统启动时为0。
  // 0x0C00 0000 = (PODRSTFLG | SWRSTFLG) = 1<<27 | 1<<28 (根据描述 0x0C000000)
  // 0x0C00 0000 对应位 26, 27。 26: NRSTFLG, 27: PODRSTFLG。
  m_registers.CSTS = (uint32_t)0x0C000000;

  // 假设 PLL/HSE 默认是关闭和未就绪的
  m_registers.CTRL &= ~((1 << 17) | (1 << 25));
}

// 处理写入操作
bool Rcm::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) {
  if (size != 4) {
    // 仅支持 32 位写入
    qWarning("   [CMU W] WARing: Non-32-bit write operations are ignored. Address: %x", static_cast<unsigned>(address) );
    return true;
  }
  uint32_t offset = address - RCM_BASE;
  uint32_t data = static_cast<uint32_t>(value);
  volatile unsigned *reg = nullptr;
  uint32_t w_mask = 0;
  uint32_t rc_mask = 0; // 写 1 清除标志位的掩码
  switch (offset) {
    case RCM_CTRL_OFFSET:
      reg = &m_registers.CTRL;
      w_mask = RCM_CTRL_W_MASK;
      break;
    case RCM_CFG_OFFSET:
      reg = &m_registers.CFG;
      w_mask = RCM_CFG_W_MASK;
      break;
    case RCM_INT_OFFSET:
      reg = &m_registers.INT;
      w_mask = RCM_INT_W_MASK;
      rc_mask = RCM_INT_RC_MASK;
      break;
    case RCM_APB2RST_OFFSET:
    case RCM_APB1RST_OFFSET:
      // 复位寄存器特殊处理：写 1 置位复位，触发外设复位行为
      reg = (offset == RCM_APB2RST_OFFSET) ? &m_registers.APB2RST : &m_registers.APB1RST;
      w_mask = (offset == RCM_APB2RST_OFFSET) ? RCM_APB2RST_W_MASK : RCM_APB1RST_W_MASK;

      // 仅写入 data 中被掩码允许的位
      *reg |= (data & w_mask);
      // 注意：此时需要通知对应的外设执行 reset() 方法！

      qDebug( "RCM: Triggered reset for peripheral(s) at %x with mask %x" , offset , *reg );
      return true; // 已处理，返回
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
     qWarning( "RCM Write error: Invalid offset %x%s%x", offset , " at 0x" , static_cast<unsigned>(address) );
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
      // 清除寄存器中可写位的值
      *reg &= ~w_mask;
      // 写入新值中可写位的值
      *reg |= (data & w_mask);
    }
    return true;
  }
  return false;
}

// 读取处理
bool Rcm::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) {
    uint32_t offset = (uint32_t)(address - RCM_BASE);
    uint32_t reg_value = 0;
    uint32_t read_mask = 0;

    if (size != 4 || !read_value) {
        qWarning("RCM Read error:  Invalid access size or null pointer at%x",static_cast<unsigned>(address));
        return false;
    }

    switch (offset) {
        case RCM_CTRL_OFFSET:
            reg_value = m_registers.CTRL;
            read_mask = RCM_CTRL_R_MASK;
            // 模拟时钟稳定，设置 HSIRDYFLG/HSERDYFLG/PLLRDYFLG
            reg_value |= ((1 << 1) | (1 << 17) | (1 << 25)); // 假设时钟稳定
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
            std::cerr << "RCM Read error: Invalid offset 0x" << std::hex << offset << " at 0x" << address << std::dec << std::endl;
            *read_value = 0;
            return false;
    }

    // 只返回可读位的值
    *read_value = (int64_t)(reg_value & read_mask);

    std::cout << "RCM Read: Reg 0x" << std::hex << offset << " (0x" << reg_value << ") masked to 0x" << *read_value << std::dec << std::endl;
    return true;
}

  // 模拟时间流逝和状态更新
  void Rcm::run_tick(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
  // qDebug("run_tick\n");

  // 1. 模拟 PLL 就绪延迟
  uint32_t pllen_bit = 1 << 24;
  uint32_t pllrdyflg_bit = 1 << 25;

  if ((m_registers.CTRL & pllen_bit) && !(m_registers.CTRL & pllrdyflg_bit)) {
    // PLL 已使能 (PLLEN=1) 且未就绪 (PLLRDYFLG=0)
    if (m_pll_wait_cycles > 0) {
      m_pll_wait_cycles--;
      if (m_pll_wait_cycles == 0) {
        // PLL 稳定，设置 PLLRDYFLG (Bit 25)
        m_registers.CTRL |= pllrdyflg_bit;
        std::cout << "RCM: PLL Clock Ready (PLLRDYFLG set)." << std::endl;
      }
    }
  }

  // 2. 模拟外设复位自清零
  // 在实际应用中，这里应该调用 PeripheralFactory 来通知复位
  if (m_registers.APB2RST != 0) {
    m_registers.APB2RST = 0;
  }
  if (m_registers.APB1RST != 0) {
    m_registers.APB1RST = 0;
  }

  // 3. 其他时钟就绪标志（如 HSE/LSE/LSI）的逻辑...
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

    if (pll_src_sel == 0) { // HSI 作为 PLL 源
      // HSI (8MHz) -> /2 分频器
      pll_input_freq = HSI_FREQ / 2;
    } else { // HSE 作为 PLL 源
      pll_input_freq = HSE_FREQ;
      if (pll_hse_psc == 1) { // PLLHSEPSC = 1 -> /2 分频
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

