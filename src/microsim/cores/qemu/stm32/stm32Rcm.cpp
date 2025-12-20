#include "stm32Rcm.h"
#include <iomanip>
#include <qlogging.h>
#include <unicorn/unicorn.h>
#include "stm32.h"
#include "Register.h"
#include "stm32RcmReg.h"
using namespace stm32Rcm;
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

  // 0x00: 时钟控制寄存器 (RCM_CTRL)
  m_registers[CTRL_OFFSET] = std::make_unique<CTRL_Register>(CTRL_OFFSET);
  // 0x04: 时钟配置寄存器 (RCM_CFG)
  m_registers[CFG_OFFSET] = std::make_unique<CFG_Register>(CFG_OFFSET);
  // 0x08: 时钟中断寄存器 (RCM_INT)
  m_registers[INT_OFFSET] = std::make_unique<INT_Register>(INT_OFFSET);
  // 0x0C: APB2 外设复位寄存器 (RCM_APB2RST)
  m_registers[APB2RST_OFFSET] = std::make_unique<APB2RST_Register>(APB2RST_OFFSET);
  // 0x10: APB1 外设复位寄存器 (RCM_APB1RST)
  m_registers[APB1RST_OFFSET] = std::make_unique<APB1RST_Register>(APB1RST_OFFSET);
  // 0x14: AHB 外设时钟使能寄存器 (RCM_AHBCLKEN)
  m_registers[AHBCLKEN_OFFSET] = std::make_unique<AHBCLKEN_Register>(AHBCLKEN_OFFSET);
  // 0x18: APB2 外设时钟使能寄存器 (RCM_APB2CLKEN)
  m_registers[APB2CLKEN_OFFSET] = std::make_unique<APB2CLKEN_Register>(APB2CLKEN_OFFSET);
  // 0x1C: APB1 外设时钟使能寄存器 (RCM_APB1CLKEN)
  m_registers[APB1CLKEN_OFFSET] = std::make_unique<APB1CLKEN_Register>(APB1CLKEN_OFFSET);
  // 0x20: 备份域控制寄存器 (RCM_BDCTRL)
  m_registers[BDCTRL_OFFSET] = std::make_unique<BDCTRL_Register>(BDCTRL_OFFSET);
  // 0x24: 控制/状态寄存器 (RCM_CSTS)
  m_registers[CSTS_OFFSET] = std::make_unique<CSTS_Register>(CSTS_OFFSET);

  m_registers[CTRL_OFFSET]->find_field("HSEEN")->write_callback= {
    [this](Register &reg,const BitField &field,  uint32_t new_field_value,void *user_data) {
      if (new_field_value == 1) {
      //open hse
        m_clock_timing.hse_start_tick=m_clock_timing.rcm_ticks;
        qDebug()<<"[RCM] mcu固件请求打开 HSE 时钟,在内部ticks："<<m_clock_timing.rcm_ticks<<Qt::endl;
      }
    }
  };
  m_registers[CTRL_OFFSET]->find_field("PLLEN")->write_callback= {
    [this](Register &reg,const BitField &field,  uint32_t new_field_value,void *user_data) {
      if (new_field_value == 1) {
        //open pll
        m_clock_timing.pll_start_tick=m_clock_timing.rcm_ticks;
        qDebug()<<"[RCM] mcu固件请求打开 PLL ,在内部ticks："<<m_clock_timing.rcm_ticks<<Qt::endl;
      }
    }
  };
  m_registers[CFG_OFFSET]->find_field("SYSCLKSEL")->write_callback= {
    [](Register &reg,const BitField &field,  uint32_t new_field_value,void *user_data) {
    reg.set_field_value_non_intrusive("SCLKSELSTS",new_field_value);
    }
  };
m_registers[CFG_OFFSET]->write_callback= {
  [this](Register &reg,uint32_t new_register_value,void *user_data) {
    qDebug()<<"[RCM] mcu固件写RCM CFG寄存器："<<Qt::hex<<new_register_value;
  }
};
}

// 处理写入操作
bool Rcm::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value, void *user_data) {
  if (size != 4) {
    // 仅支持 32 位写入
    qWarning("   [CMU W] WARing: Non-32-bit write operations are ignored. Address: %x",
             static_cast<unsigned>(address));
    return false;
  }
  const auto offset = static_cast<uint32_t>(address - RCM_BASE);
  if (m_registers.count(offset)) {
    const auto new_value = static_cast<uint32_t>(value);
  m_registers[offset]->write(new_value,size, user_data);
    // 强制打印 RCM_CTRL (0x00) 的所有写入
    if (offset == 0x00) {
      qDebug() << "[RCM RAW] CTRL Write: 0x" << Qt::hex << new_value
               << " | PLL1EN bit:" << ((new_value >> 24) & 1)
               << " | PLL2EN bit:" << ((new_value >> 26) & 1);
    }
  }else {
    qWarning().noquote() << QString("   [CMU W: 0x%1] 警告: 访问未注册寄存器!").arg(offset, 0, 16);
    return false;
  }

  return true;
}

// 读取处理
bool Rcm::handle_read(uc_engine *uc,uint64_t address,int size,int64_t *read_value,void *user_data) {

  if (size != 4 || !read_value) {
    qWarning("[RCM R] Read error:  Invalid access size or null pointer at%x",
             static_cast<unsigned>(address));
    return false;
  }
  const auto offset = static_cast<uint32_t>(address - RCM_BASE);
  uint32_t stored_value = 0;
  if (m_registers.count(offset)) {
    stored_value = m_registers[offset]->read(user_data);
  } else {
    qWarning().noquote() << QString("   [CMU R: 0x%1] 警告: 访问未注册寄存器!").arg(offset, 0, 16);
    *read_value = 0;
    return false;
  }
  *read_value = static_cast<int64_t>(stored_value);
// qDebug("[RCM] Read: Reg 0x%llX (0x%llX) masked to 0x%llX",static_cast<unsigned long long>(offset),static_cast<unsigned long long>(reg_value),static_cast<unsigned long long>(*read_value));
  return true;
}
/**
 * @brief 在 CPU 模拟器每个指令周期结束后运行，模拟 RCM 模块的时钟滴答。
 *
 * 此函数主要用于模拟 RCM 模块中涉及时间延迟的操作，特别是外部高速时钟 (HSE) 的启动过程。
 * 它通过追踪滴答计数来模拟真实的硬件延迟。
 *
 * @param uc uC 引擎实例指针 。
 * @param address 当前指令的地址。
 * @param size 当前指令的大小（字节数）。
 * @param user_data 传递给钩子函数的自定义用户数据。
 */
void Rcm::run_tick(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
  m_clock_timing.rcm_ticks++;
  // if (m_clock_timing.rcm_ticks%10000==0) {
  //   qDebug("[RCM] Run tick at 0x%x rcm_ticks:%llu" , static_cast<unsigned>(address),m_clock_timing.rcm_ticks);
  // }

  //  HSE 启动等待实现
  if (m_clock_timing.hse_start_tick != 0) {
    const uint64_t elapsed_ticks = m_clock_timing.rcm_ticks - m_clock_timing.hse_start_tick;
    // 时钟就绪标志应在延迟时间结束后设置
    if (elapsed_ticks >= m_clock_timing.HSE_DELAY_TICKS) {
      // 1. 设置 HSE Ready 标志位 (HSERDYFLG)
      m_registers[CTRL_OFFSET]->set_field_value_non_intrusive("HSERDYFLG", 1);
      // 2. 清除启动追踪标记
      m_clock_timing.hse_start_tick = 0;
      qDebug() << "[RCM] HSE 启动完成。HSERDYFLG 已置位 1。 在ticks："<<getRcmTicks();

      // // 3. 检查 HSI Ready 中断是否启用 (HSERDYFLG 在 RCM_INT 寄存器中)
      // if (m_registers[RCM_INT_OFFSET]->get_field_value_non_intrusive("HSERDYFLG") == 1) {
      //   // 设置 HSI Ready 中断标志 (HSERDYFLG)
      //   m_registers[RCM_INT_OFFSET]->set_field_value_non_intrusive("HSERDYFLG", 1);
      //   qDebug() << "[RCM] HSI Ready 中断标志 (HSERDYFLG) 已置位。";
      // }
    }
  }
  //  PLL 启动等待实现
  if (m_clock_timing.pll_start_tick!= 0) {
    const uint64_t elapsed_ticks = m_clock_timing.rcm_ticks - m_clock_timing.pll_start_tick;
    if (elapsed_ticks>=m_clock_timing.PLL_DELAY_TICKS) {
      m_registers[CTRL_OFFSET]->set_field_value_non_intrusive("PLLRDYFLG", 1);
      m_clock_timing.pll_start_tick = 0;
      qDebug()<<"[RCM] PLL 启动完成。PLLRDYFLG 已置位1.";
    }
  }
}
void Rcm::runEvent() {

}
/**
 * @brief 检查指定的外设（挂载在APB2总线上）的时钟是否已使能。
 *
 * 此函数通过非侵入式地读取 RCM_APB2CLKEN 寄存器中对应的位域（即时钟使能位）的值来判断。
 *
 * @param name 要查询的时钟使能位域的名称，例如 "PAEN"（对应GPIO A时钟）。
 * @return bool 如果对应的时钟使能位为 1，则返回 true (时钟已使能)；
 * 如果为 0，则返回 false (时钟未使能)。
 *
 */
bool Rcm::isAHPB2ClockEnabled(std::string name) {
    return m_registers[APB2CLKEN_OFFSET]->get_field_value_non_intrusive(name);
}
/**
 * @brief 检查指定的外设（挂载在APB1总线上）的时钟是否已使能。
 *
 * 此函数通过非侵入式地读取 RCM_AHBCLKEN 寄存器中对应的位域（即时钟使能位）的值来判断。
 *
 * @param name 要查询的时钟使能位域的名称，例如 "DMAEN"（对应DMAEN时钟）。
 * @return bool 如果对应的时钟使能位为 1，则返回 true (时钟已使能)；
 * 如果为 0，则返回 false (时钟未使能)。
 *
*/
bool Rcm::isAHPBClockEnabled(std::string name) {
  return m_registers[AHBCLKEN_OFFSET]->get_field_value_non_intrusive(name);
}
/**
 * @brief 检查指定的外设（挂载在APB1总线上）的时钟是否已使能。
 *
 * 此函数通过非侵入式地读取 RCM_APB1CLKEN 寄存器中对应的位域（即时钟使能位）的值来判断。
 *
 * @param name 要查询的时钟使能位域的名称，例如 "TMR2EN"（对应定时器2时钟）。
 * @return bool 如果对应的时钟使能位为 1，则返回 true (时钟已使能)；
 * 如果为 0，则返回 false (时钟未使能)。
 *。
 */
bool Rcm::isAHPB1ClockEnabled(const std::string& name) {
  return m_registers[APB1CLKEN_OFFSET]->get_field_value_non_intrusive(name);
}
/**
 * @brief 获取当前配置的系统时钟 (SYSCLK) 频率。
 *
 * 此函数根据 RCM_CFG 寄存器中的 SYSCLKSEL、PLLSRCSEL、PLLHSEPSC 和 PLLMULCFG
 * 位域的当前值，计算并返回 CPU 的核心时钟频率。
 *
 * @return uint64_t 当前系统时钟的频率，单位为 Hz。
 * 返回值会被限制在 MAX_CPU_FREQ 范围内，以模拟芯片的最高运行频率限制。
 *
 */
uint64_t Rcm::getSysClockFrequency() {
  // 获取 SYSCLKSEL (通常为 2 bits)
  uint32_t sysclk_sel = m_registers[CFG_OFFSET]->get_field_value_non_intrusive("SYSCLKSEL");

  // 1. HSI 作为系统时钟 (SYSCLKSEL = 0)
  if (sysclk_sel == 0) {
    return HSI_FREQ;
  }

  // 2. HSE 作为系统时钟 (SYSCLKSEL = 1)
  if (sysclk_sel == 1) {
    return HSE_FREQ;
  }

  // 3. PLL 作为系统时钟 (SYSCLKSEL = 2, 二进制 10)
  if (sysclk_sel == 2) {
    uint64_t pll_input_freq = 0;
    const uint32_t pll_src_sel = m_registers[CFG_OFFSET]->get_field_value_non_intrusive("PLLSRCSEL");
    const uint32_t pll_hse_psc = m_registers[CFG_OFFSET]->get_field_value_non_intrusive("PLLHSEPSC");

    if (pll_src_sel == 0) {
      // HSI / 2 作为 PLL 源
      pll_input_freq = HSI_FREQ / 2;
    } else {
      // HSE 作为 PLL 源
      pll_input_freq = HSE_FREQ;
      if (pll_hse_psc == 1) {
        pll_input_freq /= 2;
      }
    }

    // 计算 PLL 倍频系数
    const uint32_t pll_mul_cfg = m_registers[CFG_OFFSET]->get_field_value_non_intrusive("PLLMULCFG");

    uint32_t multiplier = pll_mul_cfg + 2;
    if (multiplier > 16) multiplier = 16; // 限制最大倍频

    const uint64_t pll_output_freq = pll_input_freq * multiplier;

    // 针对某些芯片（如 APM32），可能还有 PLL 后的再分频，如果此处就是 SYSCLK，直接返回
    return std::min(pll_output_freq, MAX_CPU_FREQ);
  }

  // 4. 这里的处理非常重要：如果 sysclk_sel 为 3 (二进制 11)，通常是不可用的或特定的时钟
  return HSI_FREQ;
}
