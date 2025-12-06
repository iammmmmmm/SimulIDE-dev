#pragma once
#pragma once
#include "../Register.h" // 包含 Register 类的定义

// 使用 Register.h 中的命名空间
using namespace std;

// =========================================================================
// GPIO (General Purpose Input/Output) 寄存器定义
// 偏移地址以 GPIOx_CFGLOW 的地址为基准 (0x00)
// =========================================================================
namespace stm32Gpio {

/**
 * @brief 端口配置低寄存器 (GPIOx_CFGLOW)
 * 偏移地址: 0x00
 * 复位值: 0x4444 4444
 * 描述: 配置端口 x 引脚 y (y=0..7) 的模式和功能
 */
class GPIOx_CFGLOW final: public Register {
public:
    explicit GPIOx_CFGLOW(const uint32_t offset, const string &portName)
        : Register(offset, "GPIO"+portName+"_CFGLOW", 0x44444444) {

        // CFGy[1:0] 和 MODEy[1:0] 成对出现，共 8 对（y=0..7）
        // 模式位 MODEy[1:0] 在 (29:28), (25:24), ..., (1:0)
        // 功能位 CFGy[1:0] 在 (31:30), (27:26), ..., (3:2)

        for (int y = 0; y <= 7; ++y) {
            // MODEy[1:0] - Bit (1 + 4*y) : (0 + 4*y)
          const  int mode_start = 0 + 4 * y;
            // CFGy[1:0] - Bit (3 + 4*y) : (2 + 4*y)
          const  int cfg_start = 2 + 4 * y;

            // --- Bit (1+4y):(0+4y): MODEy[1:0] (配置端口x引脚y模式) R/W
            m_fields.push_back({
                "MODE" + to_string(y), mode_start, 2, BitFieldAccess::RW,
                {{0, "输入模式"},
                 {1, "输出模式, 10MHz"},
                 {2, "输出模式, 2MHz"},
                 {3, "输出模式, 50MHz"}},
                nullptr, nullptr
            });

            // --- Bit (3+4y):(2+4y): CFGy[1:0] (配置端口x引脚y功能) R/W
            m_fields.push_back({
                "CFG" + to_string(y), cfg_start, 2, BitFieldAccess::RW,
                {{0, "输入模式: 模拟输入; 输出模式: 通用推挽"},
                 {1, "输入模式: 浮空输入; 输出模式: 通用开漏"},
                 {2, "输入模式: 上拉/下拉; 输出模式: 复用功能推挽"},
                 {3, "输入模式: 保留; 输出模式: 复用功能开漏"}},
                nullptr, nullptr
            });
        }

        build_field_map();
    }
    ~GPIOx_CFGLOW() override = default;
};

// -------------------------------------------------------------------------

/**
 * @brief 端口配置高寄存器 (GPIOx_CFGHIG)
 * 偏移地址: 0x04
 * 复位值: 0x4444 4444
 * 描述: 配置端口 x 引脚 y (y=8..15) 的模式和功能
 */
class GPIOx_CFGHIG final: public Register {
public:
    explicit GPIOx_CFGHIG(const uint32_t offset, const string &portName)
        : Register(offset, "GPIO"+portName+"_CFGHIG", 0x44444444) {

        // CFGy[1:0] 和 MODEy[1:0] 成对出现，共 8 对（y=8..15）
        // 结构与 CFGLOW 相同，只是引脚索引 y=8..15

        for (int y = 8; y <= 15; ++y) {
            // 内部索引 i = y - 8, i=0..7
            const int i = y - 8;
            // 模式位 MODEy[1:0] - Bit (1 + 4*i) : (0 + 4*i)
           const int mode_start = 0 + 4 * i;
            // 功能位 CFGy[1:0] - Bit (3 + 4*i) : (2 + 4*i)
          const  int cfg_start = 2 + 4 * i;

            // --- Bit (1+4i):(0+4i): MODEy[1:0] (配置端口x引脚y模式) R/W
            m_fields.push_back({
                "MODE" + to_string(y), mode_start, 2, BitFieldAccess::RW,
                {{0, "输入模式"},
                 {1, "输出模式, 10MHz"},
                 {2, "输出模式, 2MHz"},
                 {3, "输出模式, 50MHz"}},
                nullptr, nullptr
            });

            // --- Bit (3+4i):(2+4i): CFGy[1:0] (配置端口x引脚y功能) R/W
            m_fields.push_back({
                "CFG" + to_string(y), cfg_start, 2, BitFieldAccess::RW,
                {{0, "输入模式: 模拟输入; 输出模式: 通用推挽"},
                 {1, "输入模式: 浮空输入; 输出模式: 通用开漏"},
                 {2, "输入模式: 上拉/下拉; 输出模式: 复用功能推挽"},
                 {3, "输入模式: 保留; 输出模式: 复用功能开漏"}},
                nullptr, nullptr
            });
        }

        build_field_map();
    }
    ~GPIOx_CFGHIG() override = default;
};

// -------------------------------------------------------------------------

/**
 * @brief 端口输入数据寄存器 (GPIOx_IDATA)
 * 偏移地址: 0x08
 * 复位值: 0x0000 XXXX (未定义值)
 * 描述: 端口 x 引脚 y (y=0..15) 的输入数据
 */
class GPIOx_IDATA final: public Register {
public:
    explicit GPIOx_IDATA(const uint32_t offset, const string &portName)
        : Register(offset, "GPIO"+portName+"_IDATA", 0x00000000) { // 使用 0x00000000 作为默认复位值（假设不影响读取）

        // --- Bit 15:0: IDATAy (端口x引脚y输入数据) RO
        m_fields.push_back({
            "IDATA", 0, 16, BitFieldAccess::RO,
            {}, // 数据字段，保留空
            nullptr, nullptr
        });

        // Bit 31:16: 保留

        build_field_map();
    }
    ~GPIOx_IDATA() override = default;
};

// -------------------------------------------------------------------------

/**
 * @brief 端口输出数据寄存器 (GPIOx_ODATA)
 * 偏移地址: 0x0C
 * 复位值: 0x0000 0000
 * 描述: 端口 x 引脚 y (y=0..15) 的输出数据
 */
class GPIOx_ODATA final: public Register {
public:
    explicit GPIOx_ODATA(const uint32_t offset, const string &portName)
        : Register(offset, "GPIO"+portName+"_ODATA", 0x00000000) {

        // --- Bit 15:0: ODATAy (端口x引脚y输出数据) R/W
        m_fields.push_back({
            "ODATA", 0, 16, BitFieldAccess::RW,
            {}, // 数据字段，保留空
            nullptr, nullptr
        });

        // Bit 31:16: 保留

        build_field_map();
    }
    ~GPIOx_ODATA() override = default;
};

// -------------------------------------------------------------------------

/**
 * @brief 端口位设置/清除寄存器 (GPIOx_BSC)
 * 偏移地址: 0x10
 * 复位值: 0x0000 0000
 * 描述: 设置/清除 ODATAy 位。只写，字访问。
 */
class GPIOx_BSC final: public Register {
public:
    explicit GPIOx_BSC(const uint32_t offset, const string &portName)
        : Register(offset, "GPIO"+portName+"_BSC", 0x00000000) {

        // --- Bit 15:0: BSy (置位端口x引脚y) WO
        m_fields.push_back({
            "BS", 0, 16, BitFieldAccess::WO,
            {}, // 数据字段，保留空
            nullptr, nullptr
        });

        // --- Bit 31:16: BCy (清除端口x引脚y) WO
        m_fields.push_back({
            "BC", 16, 16, BitFieldAccess::WO,
            {}, // 数据字段，保留空
            nullptr, nullptr
        });

        // 硬件特性：同时设置 BSy 和 BCy，BSy 优先（置位）。

        build_field_map();
    }
    ~GPIOx_BSC() override = default;
};

// -------------------------------------------------------------------------

/**
 * @brief 端口位清除寄存器 (GPIOx_BC)
 * 偏移地址: 0x14
 * 复位值: 0x0000 0000
 * 描述: 清除 ODATAy 位。只写，字访问。
 */
class GPIOx_BC final: public Register {
public:
    explicit GPIOx_BC(const uint32_t offset, const string &portName)
        : Register(offset, "GPIO"+portName+"_BC", 0x00000000) {

        // --- Bit 15:0: BCy (清除端口x引脚y) W (写)
        m_fields.push_back({
            "BC", 0, 16, BitFieldAccess::WO,
            {}, // 数据字段，保留空
            nullptr, nullptr
        });

        // Bit 31:16: 保留

        build_field_map();
    }
    ~GPIOx_BC() override = default;
};

// -------------------------------------------------------------------------

/**
 * @brief 端口配置锁定寄存器 (GPIOx_LOCK)
 * 偏移地址: 0x18
 * 复位值: 0x0000 0000
 * 描述: 锁定 GPIO 的配置，防止运行时误修改。
 */
class GPIOx_LOCK final: public Register {
public:
    explicit GPIOx_LOCK(const uint32_t offset, const string &portName)
        : Register(offset, "GPIO"+portName+"_LOCK", 0x00000000) {

        // --- Bit 15:0: LOCKy (配置端口x引脚y锁位) R/W
        // 只能在 LOCKKEY=0 时写入
        m_fields.push_back({
            "LOCK", 0, 16, BitFieldAccess::RW,
            {{0, "不锁定"}, {1, "锁定"}},
            nullptr, nullptr
        });

        // --- Bit 16: LOCKKEY (锁键值) R/W (特殊写入序列)
        m_fields.push_back({
            "LOCKKEY", 16, 1, BitFieldAccess::RW,
            {{0, "未激活"}, {1, "激活锁定保护"}},
            nullptr, nullptr
        });

        // Bit 31:17: 保留

        build_field_map();
    }
    ~GPIOx_LOCK() override = default;
};

}