#pragma once
#include "../Register.h"
// =========================================================================
// 时钟控制寄存器（RCM_CTRL）
// 偏移地址：0x00 复位值：0x0000 XX83 (我们模拟为 0x00000083)
// =========================================================================
namespace stm32Rcm {
class CTRL_Register final: public Register {
  public:
    /**
     * @brief 构造函数，用于初始化 RCM_CTRL 寄存器。
     * @param offset 寄存器的内存偏移地址。
     */
    explicit CTRL_Register(uint32_t offset)
      // 使用 0x00000083 作为初始复位值
      : Register(offset, "RCM_CTRL (时钟控制)", 0x00000083) {

      // --- Bit 0: HSIEN (使能高速内部时钟) R/W
      m_fields.push_back({
        "HSIEN", 0, 1, BitFieldAccess::RW, // 访问权限：读写
        {{0, "关闭 HSICLK RC振荡器"}, {1, "开启 HSICLK RC振荡器"}},
        nullptr, nullptr
      });

      // --- Bit 1: HSIRDYFLG (高速内部时钟就绪标志) RO
      m_fields.push_back({
        "HSIRDYFLG", 1, 1, BitFieldAccess::RO, // 访问权限：只读
        {{0, "HSICLK 未稳定"}, {1, "HSICLK 已稳定"}},
        nullptr, nullptr
      });

      // Bit 2: 保留 - 不添加到 m_fields

      // --- Bit 7:3: HSITRM (调整高速内部时钟) R/W
      m_fields.push_back({
        "HSITRM", 3, 5, BitFieldAccess::RW, // 访问权限：读写
        {},
        nullptr, nullptr
      });

      // --- Bit 15:8: HSICAL (校准内部高速时钟) RO
      m_fields.push_back({
        "HSICAL", 8, 8, BitFieldAccess::RO, // 访问权限：只读
        {},
        nullptr, nullptr
      });

      // --- Bit 16: HSEEN (使能高速外部时钟) R/W
      m_fields.push_back({
        "HSEEN", 16, 1, BitFieldAccess::RW, // 访问权限：读写
        {{0, "HSECLK 关闭"}, {1, "HSECLK 开启"}},
        nullptr, nullptr
      });

      // --- Bit 17: HSERDYFLG (高速外部时钟就绪标志) RO
      m_fields.push_back({
        "HSERDYFLG", 17, 1, BitFieldAccess::RO, // 访问权限：只读
        {{0, "HSECLK 未稳定"}, {1, "HSECLK 已稳定"}},
        nullptr, nullptr
      });

      // --- Bit 18: HSEBCFG (配置高速外部时钟旁路模式) R/W
      m_fields.push_back({
        "HSEBCFG", 18, 1, BitFieldAccess::RW, // 访问权限：读写
        {{0, "非旁路模式"}, {1, "旁路模式"}},
        nullptr, nullptr
      });

      // --- Bit 19: CSSEN (使能时钟安全系统) R/W
      m_fields.push_back({
        "CSSEN", 19, 1, BitFieldAccess::RW, // 访问权限：读写
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });

      // Bit 23:20: 保留 - 不添加到 m_fields

      // --- Bit 24: PLLEN (使能 PLL) R/W
      m_fields.push_back({
        "PLLEN", 24, 1, BitFieldAccess::RW, // 访问权限：读写
        {{0, "PLL 关闭"}, {1, "PLL 使能"}},
        nullptr, nullptr
      });

      // --- Bit 25: PLLRDYFLG (PLL时钟就绪标志) RO
      m_fields.push_back({
        "PLLRDYFLG", 25, 1, BitFieldAccess::RO, // 访问权限：只读
        {{0, "PLL 未锁定"}, {1, "PLL 锁定"}},
        nullptr, nullptr
      });

      // Bit 31:26: 保留 - 不添加到 m_fields

      // 必须调用以构建按名称查找的映射
      build_field_map();
    }
};
// =========================================================================
// 时钟配置寄存器（RCM_CFG）
// 偏移地址：0x04 复位值：0x00000000
// =========================================================================
class CFG_Register final: public Register {
  public:
    /**
     * @brief 构造函数，用于初始化 RCM_CFG 寄存器。
     * @param offset 寄存器的内存偏移地址。
     */
    explicit CFG_Register(uint32_t offset)
      // RCM_CFG 复位值为 0x00000000
      : Register(offset, "RCM_CFG (时钟配置)", 0x00000000) {

      // --- Bit 1:0: SYSCLKSEL (选择系统时钟时钟源) R/W
      m_fields.push_back({
        "SYSCLKSEL", 0, 2, BitFieldAccess::RW,
        {{0b00, "HSICLK"}, {0b01, "HSECLK"}, {0b10, "PLLCLK"}, {0b11, "保留"}},
        nullptr, nullptr // 使用 nullptr 初始化回调
      });

      // --- Bit 3:2: SCLKSELSTS (系统时钟时钟源选择的状态) RO
      m_fields.push_back({
        "SCLKSELSTS", 2, 2, BitFieldAccess::RO, // 访问权限：只读
        {{0b00, "HSICLK"}, {0b01, "HSECLK"}, {0b10, "PLLCLK"}, {0b11, "保留"}},
        nullptr, nullptr
      });

      // --- Bit 7:4: AHBPSC (配置AHB时钟预分频) R/W
      m_fields.push_back({
        "AHBPSC", 4, 4, BitFieldAccess::RW,
        {
          {0b0000, "SYSCLK/1"},
          {0b1000, "SYSCLK/2"}, {0b1001, "SYSCLK/4"},
          {0b1010, "SYSCLK/8"}, {0b1011, "SYSCLK/16"},
          {0b1100, "SYSCLK/64"}, {0b1101, "SYSCLK/128"},
          {0b1110, "SYSCLK/256"}, {0b1111, "SYSCLK/512"}
        },
        nullptr, nullptr
      });

      // --- Bit 10:8: APB1PSC (配置APB1时钟预分频系数) R/W
      m_fields.push_back({
        "APB1PSC", 8, 3, BitFieldAccess::RW,
        {
          {0b000, "HCLK/1"},
          {0b100, "HCLK/2"}, {0b101, "HCLK/4"},
          {0b110, "HCLK/8"}, {0b111, "HCLK/16"}
        },
        nullptr, nullptr
      });

      // --- Bit 13:11: APB2PSC (配置APB2时钟预分频系数) R/W
      m_fields.push_back({
        "APB2PSC", 11, 3, BitFieldAccess::RW,
        {
          {0b000, "HCLK/1"},
          {0b100, "HCLK/2"}, {0b101, "HCLK/4"},
          {0b110, "HCLK/8"}, {0b111, "HCLK/16"}
        },
        nullptr, nullptr
      });

      // --- Bit 15:14: ADCPSC (配置ADC时钟预分频系数) R/W
      m_fields.push_back({
        "ADCPSC", 14, 2, BitFieldAccess::RW,
        {{0b00, "PCLK2/2"}, {0b01, "PCLK2/4"}, {0b10, "PCLK2/6"}, {0b11, "PCLK2/8"}},
        nullptr, nullptr
      });

      // --- Bit 16: PLLSRCSEL (选择PLL时钟源) R/W
      m_fields.push_back({
        "PLLSRCSEL", 16, 1, BitFieldAccess::RW,
        {{0, "HSICLK/2"}, {1, "HSECLK"}},
        nullptr, nullptr
      });

      // --- Bit 17: PLLHSEPSC (配置作为PLL时钟源的HSECLK分频) R/W
      m_fields.push_back({
        "PLLHSEPSC", 17, 1, BitFieldAccess::RW,
        {{0, "HSECLK/1 (不分频)"}, {1, "HSECLK/2"}},
        nullptr, nullptr
      });

      // --- Bit 21:18: PLLMULCFG (配置PLL倍频系数) R/W
      m_fields.push_back({
        "PLLMULCFG", 18, 4, BitFieldAccess::RW,
        {
          {0b0000, "x2"}, {0b0001, "x3"}, {0b0010, "x4"}, {0b0011, "x5"},
          {0b0100, "x6"}, {0b0101, "x7"}, {0b0110, "x8"}, {0b0111, "x9"},
          {0b1000, "x10"}, {0b1001, "x11"}, {0b1010, "x12"}, {0b1011, "x13"},
          {0b1100, "x14"}, {0b1101, "x15"}, {0b1110, "x16"}, {0b1111, "x16 (同1110)"}
        },
        nullptr, nullptr
      });
      // --- Bit 23:22: USBDPSC (配置USBD1/2预分频系数) R/W
      m_fields.push_back({
        "USBDPSC", 22, 2, BitFieldAccess::RW,
        {{0b00, "PLLCLK/1.5"}, {0b01, "PLLCLK/1"}, {0b10, "PLLCLK/2"}, {0b11, "保留"}},
        nullptr, nullptr
      });

      // --- Bit 26:24: MCOSEL (选择主时钟输出) R/W
      m_fields.push_back({
        "MCOSEL", 24, 3, BitFieldAccess::RW,
        {
          {0b000, "无输出"}, {0b100, "SYSCLK"}, {0b101, "HSICLK"},
          {0b110, "HSECLK"}, {0b111, "PLLCLK/2"}
        },
        nullptr, nullptr
      });

      // --- Bit 27: FPUPSC (配置FPU预分频系数) R/W
      m_fields.push_back({
        "FPUPSC", 27, 1, BitFieldAccess::RW,
        {{0, "HCLK/1"}, {1, "HCLK/2"}},
        nullptr, nullptr
      });

      // Bit 31:28 保留，不添加到 m_fields
      build_field_map();
    }
};
// =========================================================================
// 时钟中断寄存器（RCM_INT）
// 偏移地址：0x08 复位值：0x00000000
// =========================================================================
class INT_Register final: public Register {
  public:
    /**
     * @brief 构造函数，用于初始化 RCM_INT 寄存器。
     * @param offset 寄存器的内存偏移地址。
     */
    explicit INT_Register(uint32_t offset)
      // RCM_INT 复位值为 0x00000000
      : Register(offset, "RCM_INT (时钟中断)", 0x00000000) {

      // ==================================================
      // 标志位 (FLG) - 只读 (RO)
      // 这些位由硬件置位，用户不能通过写入来改变其状态（只能通过 CLR 位清除）
      // ==================================================
      m_fields.push_back({
        "LSIRDYFLG", 0, 1, BitFieldAccess::RO,
        {{0, "无中断"}, {1, "发生中断"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "LSERDYFLG", 1, 1, BitFieldAccess::RO,
        {{0, "无中断"}, {1, "发生中断"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "HSIRDYFLG", 2, 1, BitFieldAccess::RO,
        {{0, "无中断"}, {1, "发生中断"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "HSERDYFLG", 3, 1, BitFieldAccess::RO,
        {{0, "无中断"}, {1, "发生中断"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "PLLRDYFLG", 4, 1, BitFieldAccess::RO,
        {{0, "无中断"}, {1, "发生中断"}},
        nullptr, nullptr
      });
      // Bit 6:5 保留
      m_fields.push_back({
        "CSSFLG", 7, 1, BitFieldAccess::RO,
        {{0, "无HSE失效中断"}, {1, "发生HSE失效中断"}},
        nullptr, nullptr
      });

      // ==================================================
      // 使能位 (EN) - 读/写 (RW)
      // ==================================================
      m_fields.push_back({
        "LSIRDYEN", 8, 1, BitFieldAccess::RW,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "LSERDYEN", 9, 1, BitFieldAccess::RW,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "HSIRDYEN", 10, 1, BitFieldAccess::RW,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "HSERDYEN", 11, 1, BitFieldAccess::RW,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "PLLRDYEN", 12, 1, BitFieldAccess::RW,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      // Bit 15:13 保留

      // ==================================================
      // 清除位 (CLR) - 写 1 清除 (W1C)
      // 写入 1 会清零 FLG 位，但 CLR 位本身不会被置位。
      // ==================================================
      m_fields.push_back({
        "LSIRDYCLR", 16, 1, BitFieldAccess::W1C,
        {{0, "无作用"}, {1, "清除"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "LSERDYCLR", 17, 1, BitFieldAccess::W1C,
        {{0, "无作用"}, {1, "清除"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "HSIRDYCLR", 18, 1, BitFieldAccess::W1C,
        {{0, "无作用"}, {1, "清除"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "HSERDYCLR", 19, 1, BitFieldAccess::W1C,
        {{0, "无作用"}, {1, "清除"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "PLLRDYCLR", 20, 1, BitFieldAccess::W1C,
        {{0, "无作用"}, {1, "清除"}},
        nullptr, nullptr
      });
      // Bit 22:21 保留
      m_fields.push_back({
        "CSSCLR", 23, 1, BitFieldAccess::W1C,
        {{0, "无作用"}, {1, "清除"}},
        nullptr, nullptr
      });
      // Bit 31:24 保留

      // 必须调用以构建按名称查找的映射
      build_field_map();
    }
};
// =========================================================================
// APB2 外设复位寄存器（RCM_APB2RST）
// 偏移地址：0x0C 复位值：0x00000000
// =========================================================================
class APB2RST_Register final: public Register {
  public:
    /**
     * @brief 构造函数，用于初始化 RCM_APB2RST 寄存器。
     * @param offset 寄存器的内存偏移地址。
     */
    explicit APB2RST_Register(uint32_t offset)
      // RCM_APB2RST 复位值为 0x00000000
      : Register(offset, "RCM_APB2RST (APB2外设复位)", 0x00000000) {

      // 所有复位位都是写后自清零的，在模拟器中处理为 R/W + 回调
      BitFieldAccess access = BitFieldAccess::RW;

      m_fields.push_back({
        "AFIORST", 0, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr // 写入 1 后应自清零
      });
      // Bit 1 保留
      m_fields.push_back({
        "PARST", 2, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "PBRST", 3, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "PCRST", 4, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "PDRST", 5, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "PERST", 6, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      // Bit 8:7 保留
      m_fields.push_back({
        "ADC1RST", 9, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "ADC2RST", 10, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "TMR1RST", 11, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "SPI1RST", 12, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      // Bit 13 保留
      m_fields.push_back({
        "USART1RST", 14, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      // Bit 31:15 保留

      build_field_map();
    }
};
// =========================================================================
// APB1 外设复位寄存器（RCM_APB1RST）
// 偏移地址：0x10 复位值：0x00000000
// =========================================================================
class APB1RST_Register final: public Register {
  public:
    /**
     * @brief 构造函数，用于初始化 RCM_APB1RST 寄存器。
     * @param offset 寄存器的内存偏移地址。
     */
    explicit APB1RST_Register(uint32_t offset)
      // RCM_APB1RST 复位值为 0x00000000
      : Register(offset, "RCM_APB1RST (APB1外设复位)", 0x00000000) {

      // 所有复位位都是写后自清零的，在模拟器中处理为 R/W + 回调
      BitFieldAccess access = BitFieldAccess::RW;

      m_fields.push_back({
        "TMR2RST", 0, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr // 写入 1 后应自清零
      });
      m_fields.push_back({
        "TMR3RST", 1, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "TMR4RST", 2, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      // Bit 10:3 保留
      m_fields.push_back({
        "WWDTRST", 11, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      // Bit 13:12 保留
      m_fields.push_back({
        "SPI2RST", 14, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      // Bit 16:15 保留
      m_fields.push_back({
        "USART2RST", 17, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "USART3RST", 18, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      // Bit 20:19 保留
      m_fields.push_back({
        "I2C1RST", 21, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "I2C2RST", 22, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "USBDRST", 23, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      // Bit 24 保留
      m_fields.push_back({
        "CAN1RST", 25, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "CAN2RST", 26, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "BAKPRST", 27, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "PMURST", 28, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "DACRST", 29, 1, access,
        {{0, "无作用"}, {1, "复位"}},
        nullptr, nullptr
      });
      // Bit 31:30 保留

      build_field_map();
    }

    // 接下来需要为每个 RST 位域添加写回调，以实现写入 1 后的自清零逻辑。
    // 例如：
    /*
    static void handle_RST_Self_Clear(Register &reg, uint32_t new_field_value) {
        // 如果写入 1，则立即清零
        if (new_field_value == 1) {
            // 注意：这里需要知道是哪个字段被写入，所以最好使用 lambda 或单独的函数
            // 如果使用单独的函数，则需要通过 find_field_by_callback 或其他方式确定字段
            // 最简单的方式是使用 lambda 并在创建时绑定字段名
        }
    }
    */
};
// =========================================================================
// AHB外设时钟使能寄存器（RCM_AHBCLKEN）
// 偏移地址：0x14 复位值：0x00000014
// =========================================================================
class AHBCLKEN_Register: public Register {
  public:
    /**
     * @brief 构造函数，用于初始化 RCM_AHBCLKEN 寄存器。
     * @param offset 寄存器的内存偏移地址。
     */
    explicit AHBCLKEN_Register(uint32_t offset)
      // RCM_AHBCLKEN 复位值为 0x00000014
      : Register(offset, "RCM_AHBCLKEN (AHB时钟使能)", 0x00000014) {

      // 所有时钟使能位都是 R/W (读写)
      BitFieldAccess access = BitFieldAccess::RW;

      m_fields.push_back({
        "DMAEN", 0, 1, access,
        {{0, "关闭"}, {1, "开启"}},
        nullptr, nullptr
      });
      // Bit 1 保留
      m_fields.push_back({
        "SRAMEN", 2, 1, access,
        {{0, "关闭"}, {1, "开启"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "FPUEN", 3, 1, access,
        {{0, "关闭"}, {1, "开启"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "FMCEN", 4, 1, access,
        {{0, "关闭"}, {1, "开启"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "QSPIEN", 5, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "CRCEN", 6, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      // Bit 31:7 保留

      build_field_map();
    }
};
// =========================================================================
// APB2 外设时钟使能寄存器（RCM_APB2CLKEN）
// 偏移地址：0x18 复位值：0x00000000
// =========================================================================
class APB2CLKEN_Register: public Register {
  public:
    /**
     * @brief 构造函数，用于初始化 RCM_APB2CLKEN 寄存器。
     * @param offset 寄存器的内存偏移地址。
     */
    explicit APB2CLKEN_Register(uint32_t offset)
      // RCM_APB2CLKEN 复位值为 0x00000000
      : Register(offset, "RCM_APB2CLKEN (APB2时钟使能)", 0x00000000) {

      // 所有时钟使能位都是 R/W (读写)
      BitFieldAccess access = BitFieldAccess::RW;

      m_fields.push_back({
        "AFIOEN", 0, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      // Bit 1 保留
      m_fields.push_back({
        "PAEN", 2, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "PBEN", 3, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "PCEN", 4, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "PDEN", 5, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "PEEN", 6, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      // Bit 8:7 保留
      m_fields.push_back({
        "ADC1EN", 9, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "ADC2EN", 10, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "TMR1EN", 11, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "SPI1EN", 12, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      // Bit 13 保留
      m_fields.push_back({
        "USART1EN", 14, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      // Bit 31:15 保留

      build_field_map();
    }
};
// =========================================================================
// APB1 外设时钟使能寄存器（RCM_APB1CLKEN）
// 偏移地址：0x1C 复位值：0x00000000
// =========================================================================
class APB1CLKEN_Register: public Register {
  public:
    /**
     * @brief 构造函数，用于初始化 RCM_APB1CLKEN 寄存器。
     * @param offset 寄存器的内存偏移地址。
     */
    explicit APB1CLKEN_Register(uint32_t offset)
      // RCM_APB1CLKEN 复位值为 0x00000000
      : Register(offset, "RCM_APB1CLKEN (APB1时钟使能)", 0x00000000) {

      // 所有时钟使能位都是 R/W (读写)
      BitFieldAccess access = BitFieldAccess::RW;

      m_fields.push_back({
        "TMR2EN", 0, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "TMR3EN", 1, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "TMR4EN", 2, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      // Bit 10:3 保留
      m_fields.push_back({
        "WWDTEN", 11, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      // Bit 13:12 保留
      m_fields.push_back({
        "SPI2EN", 14, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      // Bit 16:15 保留
      m_fields.push_back({
        "USART2EN", 17, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "USART3EN", 18, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      // Bit 20:19 保留
      m_fields.push_back({
        "I2C1EN", 21, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "I2C2EN", 22, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "USBDEN", 23, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      // Bit 24 保留
      m_fields.push_back({
        "CAN1EN", 25, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "CAN2EN", 26, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "BAKPEN", 27, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "PMUEN", 28, 1, access,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });
      // Bit 31:29 保留

      build_field_map();
    }
};
// =========================================================================
// 备份域控制寄存器（RCM_BDCTRL）
// 偏移地址：0x20 复位值：0x00000000
// =========================================================================
class BDCTRL_Register: public Register {
  public:
    /**
     * @brief 构造函数，用于初始化 RCM_BDCTRL 寄存器。
     * @param offset 寄存器的内存偏移地址。
     */
    explicit BDCTRL_Register(uint32_t offset)
      // RCM_BDCTRL 复位值为 0x00000000
      : Register(offset, "RCM_BDCTRL (备份域控制)", 0x00000000) {

      // --- Bit 0: LSEEN (使能低速外部时钟) R/W
      m_fields.push_back({
        "LSEEN", 0, 1, BitFieldAccess::RW,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });

      // --- Bit 1: LSERDYFLG (LSE 就绪标志) RO
      m_fields.push_back({
        "LSERDYFLG", 1, 1, BitFieldAccess::RO,
        {{0, "未就绪"}, {1, "就绪"}},
        nullptr, nullptr
      });

      // --- Bit 2: LSEBCFG (LSE 旁路模式配置) R/W
      m_fields.push_back({
        "LSEBCFG", 2, 1, BitFieldAccess::RW,
        {{0, "非旁路"}, {1, "旁路模式"}},
        nullptr, nullptr
      });
      // Bit 7:3 保留

      // --- Bit 8-9: RTCSRCSEL (RTC 时钟源选择) R/W
      m_fields.push_back({
        "RTCSRCSEL", 8, 2, BitFieldAccess::RW,
        {{0b00, "无时钟"}, {0b01, "LSECLK"}, {0b10, "LSICLK"}, {0b11, "HSECLK/128"}},
        nullptr, nullptr
      });
      // Bit 14:10 保留

      // --- Bit 15: RTCCLKEN (RTC 时钟使能) R/W
      m_fields.push_back({
        "RTCCLKEN", 15, 1, BitFieldAccess::RW,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });

      // --- Bit 16: BDRST (备份域复位) R/W
      // 这是一个自清位的写触发位，但由于 Register 类中没有 W1T，我们使用 R/W 并在回调中实现自清逻辑。
      m_fields.push_back({
        "BDRST", 16, 1, BitFieldAccess::RW,
        {{0, "复位未激活"}, {1, "复位整个备份域"}},
        nullptr, nullptr // 回调函数将在之后绑定以实现复位触发和自清
      });
      // Bit 31:17 保留

      // 必须调用以构建按名称查找的映射
      build_field_map();
    }

    // 如果需要模拟硬件行为，BDRST 的写回调应包含以下逻辑：
    // static void handle_BDRST(Register &reg, uint32_t new_field_value) {
    //     if (new_field_value == 1) {
    //         // 1. 模拟备份域复位操作...
    //         // 2. 将 BDRST 位域本身清零 (自清零)
    //         reg.set_field_value_non_intrusive("BDRST", 0);
    //     }
    // }
};

// =========================================================================
// 控制/状态寄存器（RCM_CSTS）
// 偏移地址：0x24 复位值：0x0C000000
// =========================================================================
class CSTS_Register: public Register {
  public:
    /**
     * @brief 构造函数，用于初始化 RCM_CSTS 寄存器。
     * @param offset 寄存器的内存偏移地址。
     */
    explicit CSTS_Register(uint32_t offset)
      // RCM_CSTS 复位值为 0x0C000000
      : Register(offset, "RCM_CSTS (控制/状态)", 0x0C000000) {

      // --- Bit 0: LSIEN (使能低速内部时钟) R/W
      m_fields.push_back({
        "LSIEN", 0, 1, BitFieldAccess::RW,
        {{0, "禁止"}, {1, "使能"}},
        nullptr, nullptr
      });

      // --- Bit 1: LSIRDYFLG (LSI 就绪标志) RO
      m_fields.push_back({
        "LSIRDYFLG", 1, 1, BitFieldAccess::RO,
        {{0, "未就绪"}, {1, "就绪"}},
        nullptr, nullptr
      });
      // Bit 23:2 保留

      // --- Bit 24: RSTFLGCLR (复位标志清除位) W1C
      // 写入 1 清除所有复位标志,包括RSTFLGCLR。
      m_fields.push_back({
        "RSTFLGCLR", 24, 1, BitFieldAccess::W1C, // 访问权限：写 1 清除
        {{0, "无作用"}, {1, "清除复位标志"}},
        nullptr, nullptr // 回调函数将在之后绑定以实现清除逻辑
      });
      // Bit 25 保留

      // ==================================================
      // 复位标志 (RSTFLG) - 只读 (RO)
      // 这些位由硬件置位，只能通过 RSTFLGCLR 清除。
      // ==================================================
      m_fields.push_back({
        "NRSTFLG", 26, 1, BitFieldAccess::RO,
        {{0, "无"}, {1, "发生"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "PODRSTFLG", 27, 1, BitFieldAccess::RO,
        {{0, "无"}, {1, "发生"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "SWRSTFLG", 28, 1, BitFieldAccess::RO,
        {{0, "无"}, {1, "发生"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "IWDTRSTFLG", 29, 1, BitFieldAccess::RO,
        {{0, "无"}, {1, "发生"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "WWDTRSTFLG", 30, 1, BitFieldAccess::RO,
        {{0, "无"}, {1, "发生"}},
        nullptr, nullptr
      });
      m_fields.push_back({
        "LPWRRSTFLG", 31, 1, BitFieldAccess::RO,
        {{0, "无"}, {1, "发生"}},
        nullptr, nullptr
      });

      // 必须调用以构建按名称查找的映射
      build_field_map();
    }
};
}
