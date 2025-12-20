#pragma once
#include "../peripheral_factory.h"
#include <unicorn/unicorn.h>

#include "Register.h"
#include "stm32RcmReg.h"

namespace stm32Rcm{
#define RCM_BASE            ((uint32_t)0x40021000)
#define RCM_END             ((uint32_t)0x40021400 )

// --- RCM 寄存器偏移地址宏定义 (Offset) ---

#define CTRL_OFFSET     ((uint32_t)0x00) // 时钟控制寄存器
#define CFG_OFFSET      ((uint32_t)0x04) // 时钟配置寄存器
#define INT_OFFSET      ((uint32_t)0x08) // 时钟中断寄存器
#define APB2RST_OFFSET  ((uint32_t)0x0C) // APB2 外设复位寄存器
#define APB1RST_OFFSET  ((uint32_t)0x10) // APB1 外设复位寄存器
#define AHBCLKEN_OFFSET ((uint32_t)0x14) // AHB 外设时钟使能寄存器
#define APB2CLKEN_OFFSET ((uint32_t)0x18) // APB2 外设时钟使能寄存器
#define APB1CLKEN_OFFSET ((uint32_t)0x1C) // APB1 外设时钟使能寄存器
#define BDCTRL_OFFSET   ((uint32_t)0x20) // 备份域控制寄存器
#define CSTS_OFFSET     ((uint32_t)0x24) // 控制/状态寄存器

// --- RCM 寄存器完整地址宏定义 (Address) ---

#define CTRL_ADDR       (RCM_BASE + CTRL_OFFSET)
#define CFG_ADDR        (RCM_BASE + CFG_OFFSET)
#define INT_ADDR        (RCM_BASE + INT_OFFSET)
#define APB2RST_ADDR    (RCM_BASE + APB2RST_OFFSET)
#define APB1RST_ADDR    (RCM_BASE + APB1RST_OFFSET)
#define AHBCLKEN_ADDR   (RCM_BASE + AHBCLKEN_OFFSET)
#define APB2CLKEN_ADDR  (RCM_BASE + APB2CLKEN_OFFSET)
#define APB1CLKEN_ADDR  (RCM_BASE + APB1CLKEN_OFFSET)
#define BDCTRL_ADDR     (RCM_BASE + BDCTRL_OFFSET)
#define CSTS_ADDR       (RCM_BASE + CSTS_OFFSET)

class Rcm : public PeripheralDevice {
  private:
    std::map<uint32_t, std::unique_ptr<Register>> m_registers;
    void initialize_registers();
    // Status: Number of wait cycles required for the analog clock to start
    struct {
      uint64_t hse_start_tick = 0; // HSE 启动时的 CPU 周期计数
      uint64_t pll_start_tick = 0; // PLL 启动时的 CPU 周期计数
      const uint64_t HSE_DELAY_TICKS = 1000; // 假设 HSE 稳定需要这么久
      const uint64_t PLL_DELAY_TICKS = 750;  // 假设 PLL 稳定需要这么久
      uint64_t rcm_ticks = 0;
    } m_clock_timing;
  public:
    Rcm(uint64_t hsi_freq,uint64_t hse_freq,uint64_t max_cpu_freq);
    Rcm();
    bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value, void *user_data) override;
    bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value, void *user_data) override;
    uint64_t getBaseAddress() override { return RCM_BASE; }
    std::string getName() override { return "CMU (rcm)"; }
    uint64_t getSize() override {return RCM_END-RCM_BASE;}
    void run_tick(uc_engine *uc, uint64_t address, uint32_t size, void *user_data);
    uint64_t getSysClockFrequency() ;
     uint64_t HSI_FREQ = 8000000; // 8MHz
     uint64_t HSE_FREQ = 8000000; // 8MHz (外部晶振)
     uint64_t MAX_CPU_FREQ = 96000000; // 96MHz
  void runEvent() override;
    bool isAHPB2ClockEnabled(std::string name);
    bool isAHPBClockEnabled(std::string name);
    bool isAHPB1ClockEnabled(const std::string& name);
    uint64_t getMcuTime() {
      const uint64_t freq = getSysClockFrequency();
      const uint64_t ticks = m_clock_timing.rcm_ticks;
      const uint64_t seconds = ticks / freq;
      const uint64_t remainder = ticks % freq;
      const unsigned __int128 wide_remainder = static_cast<unsigned __int128>(remainder) * 1000000000000ULL;
      const auto remainder_ps = static_cast<uint64_t>(wide_remainder / freq);
      //to see Stm32::runToTime,二者含义一样，都是来自经验的魔法值
      return ((seconds * 1000000000000ULL) + remainder_ps)*2;
     // return (m_clock_timing.rcm_ticks * 1000000000000ULL) / getSysClockFrequency();

    }
    uint64_t getRcmTicks() const {
      return m_clock_timing.rcm_ticks;
    }
};
}
