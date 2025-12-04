#pragma once


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

// RCM 寄存器读写掩码宏定义

// --- RCM_CTRL (0x00) ---
// 可写掩码: HSIEN, HSITRM, HSEEN, HSEBCFG, CSSEN, PLLEN
#define RCM_CTRL_W_MASK     ((uint32_t)0x011C00BD)
// 可读掩码: HSIEN, HSIRDYFLG, HSITRM, HSICAL, HSEEN, HSERDYFLG, HSEBCFG, CSSEN, PLLEN, PLLRDYFLG
#define RCM_CTRL_R_MASK     ((uint32_t)0x031CFFBF)
// 写 1 清除标志位掩码 (无)
#define RCM_CTRL_RC_MASK    ((uint32_t)0x00000000)

// --- RCM_CFG (0x04) ---
// 可写掩码: SYSCLKSEL, AHBPSC, APB1PSC, APB2PSC, ADCPSC, PLLSRCSEL, PLLHSEPSC, PLLMULCFG, USBDPSC, MCOSEL, FPUPSC
#define RCM_CFG_W_MASK      ((uint32_t)0x0FEF7FFF)
// 可读掩码: ALL Writable bits + SCLKSELSTS
#define RCM_CFG_R_MASK      ((uint32_t)0x0FEF7FFF)
// 写 1 清除标志位掩码 (无)
#define RCM_CFG_RC_MASK     ((uint32_t)0x00000000)

// --- RCM_INT (0x08) ---
// 可写掩码: LSIRDYEN - PLLRDYEN, LSIRDYCLR - CSSCLR
#define RCM_INT_W_MASK      ((uint32_t)0x00FF1F00)
// 可读掩码: LSIRDYFLG - PLLRDYFLG, CSSFLG, LSIRDYEN - PLLRDYEN
#define RCM_INT_R_MASK      ((uint32_t)0x00001F8F)
// 写 1 清除标志位掩码: LSIRDYCLR - CSSCLR (位 16-23)
#define RCM_INT_RC_MASK     ((uint32_t)0x00FF0000)

// --- RCM_APB2RST (0x0C) ---
// 可写掩码: AFIORST, PARST - PERST, ADC1RST, ADC2RST, TMR1RST, SPI1RST, USART1RST
#define RCM_APB2RST_W_MASK  ((uint32_t)0x00004FDE)
// 可读掩码: ALL Writable bits
#define RCM_APB2RST_R_MASK  ((uint32_t)0x00004FDE)
// 写 1 清除标志位掩码 (无)
#define RCM_APB2RST_RC_MASK ((uint32_t)0x00000000)

// --- RCM_APB1RST (0x10) ---
// 可写掩码: TMR2RST - TMR4RST, WWDTRST, SPI2RST, USART2RST, USART3RST, I2C1RST, I2C2RST, USBDRST, CAN1RST, CAN2RST, BAKPRST, PMURST, DACRST
#define RCM_APB1RST_W_MASK  ((uint32_t)0x37E4C807)
// 可读掩码: ALL Writable bits
#define RCM_APB1RST_R_MASK  ((uint32_t)0x37E4C807)
// 写 1 清除标志位掩码 (无)
#define RCM_APB1RST_RC_MASK ((uint32_t)0x00000000)

// --- RCM_AHBCLKEN (0x14) ---
// 可写掩码: DMAEN, SRAMEN, FPUEN, FMCEN, QSPIEN, CRCEN
#define RCM_AHBCLKEN_W_MASK ((uint32_t)0x0000007D)
// 可读掩码: ALL Writable bits
#define RCM_AHBCLKEN_R_MASK ((uint32_t)0x0000007D)
// 写 1 清除标志位掩码 (无)
#define RCM_AHBCLKEN_RC_MASK ((uint32_t)0x00000000)

// --- RCM_APB2CLKEN (0x18) ---
// 可写掩码: AFIOEN, PAEN - PEEN, ADC1EN, ADC2EN, TMR1EN, SPI1EN, USART1EN
#define RCM_APB2CLKEN_W_MASK ((uint32_t)0x00004FDE)
// 可读掩码: ALL Writable bits
#define RCM_APB2CLKEN_R_MASK ((uint32_t)0x00004FDE)
// 写 1 清除标志位掩码 (无)
#define RCM_APB2CLKEN_RC_MASK ((uint32_t)0x00000000)

// --- RCM_APB1CLKEN (0x1C) ---
// 可写掩码: TMR2EN - TMR4EN, WWDTEN, SPI2EN, USART2EN, USART3EN, I2C1EN, I2C2EN, USBDEN, CAN1EN, CAN2EN, BAKPEN, PMUEN
#define RCM_APB1CLKEN_W_MASK ((uint32_t)0x17E4C807)
// 可读掩码: ALL Writable bits
#define RCM_APB1CLKEN_R_MASK ((uint32_t)0x17E4C807)
// 写 1 清除标志位掩码 (无)
#define RCM_APB1CLKEN_RC_MASK ((uint32_t)0x00000000)

// --- RCM_BDCTRL (0x20) ---
// 可写掩码: LSEEN, LSEBCFG, RTCSRCSEL, RTCCLKEN, BDRST
#define RCM_BDCTRL_W_MASK   ((uint32_t)0x00018305)
// 可读掩码: LSEEN, LSERDYFLG, LSEBCFG, RTCSRCSEL, RTCCLKEN, BDRST
#define RCM_BDCTRL_R_MASK   ((uint32_t)0x00018307)
// 写 1 清除标志位掩码 (无)
#define RCM_BDCTRL_RC_MASK  ((uint32_t)0x00000000)

// --- RCM_CSTS (0x24) ---
// 可写掩码: LSIEN, RSTFLGCLR
#define RCM_CSTS_W_MASK     ((uint32_t)0x01000001)
// 可读掩码: LSIEN, LSIRDYFLG, RSTFLGCLR, NRSTFLG - LPWRRSTFLG
#define RCM_CSTS_R_MASK     ((uint32_t)0xFF000003)
// 写 1 清除标志位掩码: RSTFLGCLR (位 24)
#define RCM_CSTS_RC_MASK    ((uint32_t)0x01000000)


#include "../peripheral_factory.h"
#include <unicorn/unicorn.h>
typedef struct {
  volatile uint32_t CTRL;         /**< 0x00: 时钟控制寄存器 (RCM_CTRL) */
  volatile uint32_t CFG;          /**< 0x04: 时钟配置寄存器 (RCM_CFG) */
  volatile uint32_t INT;          /**< 0x08: 时钟中断寄存器 (RCM_INT) */
  volatile uint32_t APB2RST;      /**< 0x0C: APB2 外设复位寄存器 (RCM_APB2RST) */
  volatile uint32_t APB1RST;      /**< 0x10: APB1 外设复位寄存器 (RCM_APB1RST) */
  volatile uint32_t AHBCLKEN;     /**< 0x14: AHB 外设时钟使能寄存器 (RCM_AHBCLKEN) */
  volatile uint32_t APB2CLKEN;    /**< 0x18: APB2 外设时钟使能寄存器 (RCM_APB2CLKEN) */
  volatile uint32_t APB1CLKEN;    /**< 0x1C: APB1 外设时钟使能寄存器 (RCM_APB1CLKEN) */
  volatile uint32_t BDCTRL;       /**< 0x20: 备份域控制寄存器 (RCM_BDCTRL) */
  volatile uint32_t CSTS;         /**< 0x24: 控制/状态寄存器 (RCM_CSTS) */
} RCM_TypeDef;
class Rcm : public PeripheralDevice {
  private:
    RCM_TypeDef m_registers{};
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
    bool is_ahb_clock_enabled(uint32_t bit_mask) const {
      return (m_registers.AHBCLKEN & bit_mask) != 0;
    }
    bool is_apb2_clock_enabled(uint32_t bit_mask) const {
      return (m_registers.APB2CLKEN & bit_mask) != 0;
    }
    bool is_apb1_clock_enabled(uint32_t bit_mask) const {
      return (m_registers.APB1CLKEN & bit_mask) != 0;
    }
    void run_tick(uc_engine *uc, uint64_t address, uint32_t size, void *user_data);
    uint64_t getSysClockFrequency() const;

     uint64_t HSI_FREQ = 8000000; // 8MHz
     uint64_t HSE_FREQ = 8000000; // 8MHz (外部晶振)
     uint64_t MAX_CPU_FREQ = 96000000; // 96MHz
  void runEvent() override;
};
