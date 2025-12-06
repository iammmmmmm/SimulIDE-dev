#pragma once
#include "../peripheral_factory.h"
#include <unicorn/unicorn.h>

#include "Register.h"
#include "stm32RcmReg.h"

namespace stm32Rcm{
#define RCM_BASE            ((uint32_t)0x40021000)
#define RCM_END             ((uint32_t)0x40021400 )

// --- RCM 寄存器偏移地址宏定义 (Offset) ---

#define RCM_CTRL_OFFSET     ((uint32_t)0x00) // 时钟控制寄存器
#define RCM_CFG_OFFSET      ((uint32_t)0x04) // 时钟配置寄存器
#define RCM_INT_OFFSET      ((uint32_t)0x08) // 时钟中断寄存器
#define RCM_APB2RST_OFFSET  ((uint32_t)0x0C) // APB2 外设复位寄存器
#define RCM_APB1RST_OFFSET  ((uint32_t)0x10) // APB1 外设复位寄存器
#define RCM_AHBCLKEN_OFFSET ((uint32_t)0x14) // AHB 外设时钟使能寄存器
#define RCM_APB2CLKEN_OFFSET ((uint32_t)0x18) // APB2 外设时钟使能寄存器
#define RCM_APB1CLKEN_OFFSET ((uint32_t)0x1C) // APB1 外设时钟使能寄存器
#define RCM_BDCTRL_OFFSET   ((uint32_t)0x20) // 备份域控制寄存器
#define RCM_CSTS_OFFSET     ((uint32_t)0x24) // 控制/状态寄存器

// --- RCM 寄存器完整地址宏定义 (Address) ---

#define RCM_CTRL_ADDR       (RCM_BASE + RCM_CTRL_OFFSET)
#define RCM_CFG_ADDR        (RCM_BASE + RCM_CFG_OFFSET)
#define RCM_INT_ADDR        (RCM_BASE + RCM_INT_OFFSET)
#define RCM_APB2RST_ADDR    (RCM_BASE + RCM_APB2RST_OFFSET)
#define RCM_APB1RST_ADDR    (RCM_BASE + RCM_APB1RST_OFFSET)
#define RCM_AHBCLKEN_ADDR   (RCM_BASE + RCM_AHBCLKEN_OFFSET)
#define RCM_APB2CLKEN_ADDR  (RCM_BASE + RCM_APB2CLKEN_OFFSET)
#define RCM_APB1CLKEN_ADDR  (RCM_BASE + RCM_APB1CLKEN_OFFSET)
#define RCM_BDCTRL_ADDR     (RCM_BASE + RCM_BDCTRL_OFFSET)
#define RCM_CSTS_ADDR       (RCM_BASE + RCM_CSTS_OFFSET)

class Rcm : public PeripheralDevice {
  private:
    std::map<uint32_t, std::unique_ptr<Register>> m_registers;
    void initialize_registers();
    // Status: Number of wait cycles required for the analog clock to start
    struct {
      uint64_t hse_start_tick = 0; // HSE 启动时的 CPU 周期计数
      uint64_t pll_start_tick = 0; // PLL 启动时的 CPU 周期计数
      const uint64_t HSE_DELAY_TICKS = 1000; // 假设 HSE 需要
      const uint64_t PLL_DELAY_TICKS = 750;  // 假设 PLL 需要
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
    // 外设行为查询接口
    bool is_ahb_clock_enabled(uint32_t bit_mask)  {
      return (m_registers[RCM_AHBCLKEN_OFFSET]->read() & bit_mask) != 0;
    }
    bool is_apb2_clock_enabled(uint32_t bit_mask)  {
      return (m_registers[RCM_APB2CLKEN_OFFSET]->read() & bit_mask) != 0;
    }
    bool is_apb1_clock_enabled(uint32_t bit_mask)  {
      return (m_registers[RCM_APB1CLKEN_OFFSET]->read() & bit_mask) != 0;
    }
    void run_tick(uc_engine *uc, uint64_t address, uint32_t size, void *user_data);
    uint64_t getSysClockFrequency() ;
     uint64_t HSI_FREQ = 8000000; // 8MHz
     uint64_t HSE_FREQ = 8000000; // 8MHz (外部晶振)
     uint64_t MAX_CPU_FREQ = 96000000; // 96MHz
  void runEvent() override;
    bool isAHPB2ClockEnabled(string name) {
    return m_registers[RCM_APB2CLKEN_OFFSET]->get_field_value_non_intrusive(name);
    };
};
}
