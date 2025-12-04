#include "stm32Rcm.h"
#include <iomanip>
#include <qlogging.h>
#include <qtextstream.h>
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
  m_registers[RCM_CTRL_OFFSET] = std::make_unique<stm32RCM_CTRL_Register>(RCM_CTRL_OFFSET);
  // 0x04: 时钟配置寄存器 (RCM_CFG)
  m_registers[RCM_CFG_OFFSET] = std::make_unique<stm32RCM_CFG_Register>(RCM_CFG_OFFSET);
  // 0x08: 时钟中断寄存器 (RCM_INT)
  m_registers[RCM_INT_OFFSET] = std::make_unique<stm32RCM_INT_Register>(RCM_INT_OFFSET);
  // 0x0C: APB2 外设复位寄存器 (RCM_APB2RST)
  m_registers[RCM_APB2RST_OFFSET] = std::make_unique<stm32RCM_APB2RST_Register>(RCM_APB2RST_OFFSET);
  // 0x10: APB1 外设复位寄存器 (RCM_APB1RST)
  m_registers[RCM_APB1RST_OFFSET] = std::make_unique<stm32RCM_APB1RST_Register>(RCM_APB1RST_OFFSET);
  // 0x14: AHB 外设时钟使能寄存器 (RCM_AHBCLKEN)
  m_registers[RCM_AHBCLKEN_OFFSET] = std::make_unique<stm32RCM_AHBCLKEN_Register>(RCM_AHBCLKEN_OFFSET);
  // 0x18: APB2 外设时钟使能寄存器 (RCM_APB2CLKEN)
  m_registers[RCM_APB2CLKEN_OFFSET] = std::make_unique<stm32RCM_APB2CLKEN_Register>(RCM_APB2CLKEN_OFFSET);
  // 0x1C: APB1 外设时钟使能寄存器 (RCM_APB1CLKEN)
  m_registers[RCM_APB1CLKEN_OFFSET] = std::make_unique<stm32RCM_APB1CLKEN_Register>(RCM_APB1CLKEN_OFFSET);
  // 0x20: 备份域控制寄存器 (RCM_BDCTRL)
  m_registers[RCM_BDCTRL_OFFSET] = std::make_unique<stm32RCM_BDCTRL_Register>(RCM_BDCTRL_OFFSET);
  // 0x24: 控制/状态寄存器 (RCM_CSTS)
  m_registers[RCM_CSTS_OFFSET] = std::make_unique<stm32RCM_CSTS_Register>(RCM_CSTS_OFFSET);
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
  const auto new_value = static_cast<uint32_t>(value);
  m_registers[offset]->write(new_value);
  return true;
}

// 读取处理
bool Rcm::handle_read(uc_engine *uc,uint64_t address,int size,int64_t *read_value,void *user_data) {

  if (size != 4 || !read_value) {
    qWarning("[RCM] Read error:  Invalid access size or null pointer at%x",
             static_cast<unsigned>(address));
    return false;
  }
  auto offset = static_cast<uint32_t>(address - RCM_BASE);
  uint32_t stored_value = 0;
  if (m_registers.count(offset)) {
    stored_value = m_registers[offset]->read();
  } else {
    qWarning().noquote() << QString("   [CMU R: 0x%1] 警告: 访问未注册寄存器!").arg(offset, 0, 16);
    *read_value = 0;
    return false;
  }
  *read_value = static_cast<int64_t>(stored_value);
// qDebug("[RCM] Read: Reg 0x%llX (0x%llX) masked to 0x%llX",static_cast<unsigned long long>(offset),static_cast<unsigned long long>(reg_value),static_cast<unsigned long long>(*read_value));
  return true;
}

void Rcm::run_tick(uc_engine *uc, uint64_t address, uint32_t size, void *user_data) {
  m_clock_timing.rcm_ticks++;
  // if (m_clock_timing.rcm_ticks%1000==0) {
  //    qDebug("[RCM] Run tick at 0x%x rcm_ticks:%llu" , static_cast<unsigned>(address),m_clock_timing.rcm_ticks);
  // }

}
void Rcm::runEvent() {

}
uint64_t Rcm::getSysClockFrequency()  {
  uint32_t cfg_value = m_registers[RCM_CFG_OFFSET]->read();
  // 获取 SYSCLKSEL (RCM_CFG Bit 0-1)
  uint32_t sysclk_sel =cfg_value & 0b11;

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
    uint32_t pll_src_sel = (cfg_value >> 16) & 0b1;
    uint32_t pll_hse_psc = (cfg_value >> 17) & 0b1;

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
    uint32_t pll_mul_cfg = (cfg_value >> 18) & 0b1111; // Bit 18-21
    // APM32F103 的 PLL 倍频系数通常是 (N+2)
    // pll_mul_cfg = 0b0000 -> x2, 0b1110 -> x16
    // PLLMULCFG 0b0000 对应 x2, 0b0001 对应 x3, ..., 0b1110 对应 x16。
    uint32_t N = pll_mul_cfg + 2;

    // --- 计算 PLL 输出频率 ---
    uint64_t pll_output_freq = pll_input_freq * N;
    // 确保频率不超过芯片限制 (eg: 96MHz MAX)
    return std::min(pll_output_freq, MAX_CPU_FREQ);
  }
  // 默认或复位状态 (默认为 HSICLK)
  return HSI_FREQ;
}
