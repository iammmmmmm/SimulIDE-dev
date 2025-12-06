#pragma once
#include "../Register.h" // 包含 Register 类的定义

// 使用 Register.h 中的命名空间
using namespace std;

// =========================================================================
// FMC (Flash Memory Controller) 寄存器定义
// =========================================================================
namespace stm32Fmc {
/**
 * @brief FMC 控制寄存器1 (FMC_CTRL1)
 * 偏移地址: 0x00
 * 复位值: 0x0000 0030
 */
class FMC_CTRL1 final: public Register {
public:
    explicit FMC_CTRL1(uint32_t offset)
        : Register(offset, "FMC_CTRL1", 0x00000030) {

        // --- Bit 2:0: WS (配置等待周期) R/W
        m_fields.push_back({
            "WS", 0, 3, BitFieldAccess::RW,
            {{0, "0个等待周期, 0<系统时钟<=24MHz"},
             {1, "1个等待周期, 24MHz<系统时钟<=48MHz"},
             {2, "2个等待周期, 48MHz<系统时钟<=72MHz"},
             {3, "3个等待周期, 72MHz<系统时钟<=96MHz"}},
            nullptr, nullptr
        });

        // --- Bit 3: HCAEN (使能Flash半周期访问) R/W
        m_fields.push_back({
            "HCAEN", 3, 1, BitFieldAccess::RW,
            {{0, "禁止"},
             {1, "允许"}},
            nullptr, nullptr
        });

        // --- Bit 4: PBEN (使能预取缓存区) R/W
        m_fields.push_back({
            "PBEN", 4, 1, BitFieldAccess::RW,
            {{0, "禁用"},
             {1, "使能"}},
            nullptr, nullptr
        });

        // --- Bit 5: PBSF (预取缓存区状态标志) RO
        m_fields.push_back({
            "PBSF", 5, 1, BitFieldAccess::RO,
            {{0, "处于关闭状态"},
             {1, "处于打开状态"}},
            nullptr, nullptr
        });

        // Bit 31:6: 保留

        // 必须调用以构建按名称查找的映射
        build_field_map();
    }
    // 析构函数可以保持默认
    ~FMC_CTRL1() override = default;
};

// -------------------------------------------------------------------------

/**
 * @brief FMC 关键字寄存器 (FMC_KEY)
 * 偏移地址: 0x04
 * 复位值: 0x0000 0000
 */
class FMC_KEY final: public Register {
public:
    explicit FMC_KEY(uint32_t offset)
        : Register(offset, "FMC_KEY", 0x00000000) {

        // --- Bit 31:0: KEY (FMC关键字) WO (只写)
        // 写入关键字可解锁FMC，读操作返回0。
        m_fields.push_back({
            "KEY", 0, 32, BitFieldAccess::WO,
            {},
            nullptr, nullptr
        });

        // 必须调用以构建按名称查找的映射
        build_field_map();
    }
    ~FMC_KEY() override = default;
};

// -------------------------------------------------------------------------

/**
 * @brief 选项字节关键字寄存器 (FMC_OBKEY)
 * 偏移地址: 0x08
 * 复位值: 0x0000 0000
 */
class FMC_OBKEY final: public Register {
public:
    explicit FMC_OBKEY(uint32_t offset)
        : Register(offset, "FMC_OBKEY", 0x00000000) {

        // --- Bit 31:0: OBKEY (选项字节关键字) WO (只写)
        // 写入关键字可解除选项字节写锁定，读操作返回0。
        m_fields.push_back({
            "OBKEY", 0, 32, BitFieldAccess::WO,
            {},
            nullptr, nullptr
        });

        // 必须调用以构建按名称查找的映射
        build_field_map();
    }
    ~FMC_OBKEY() override = default;
};

/**
 * @brief 状态寄存器 (FMC_STS)
 * 偏移地址: 0x0C
 * 复位值: 0x0000 0000
 */
class FMC_STS final: public Register {
public:
    explicit FMC_STS(uint32_t offset)
        : Register(offset, "FMC_STS", 0x00000000) {

        // --- Bit 0: BUSYF (忙碌标志) RO
        m_fields.push_back({
            "BUSYF", 0, 1, BitFieldAccess::RO,
            {{0, "空闲"},
             {1, "忙碌"}},
            nullptr, nullptr
        });

        // Bit 1: 保留

        // --- Bit 2: PEF (编程错误标志) R/W (通常为 W1C)
        m_fields.push_back({
            "PEF", 2, 1, BitFieldAccess::RW,
            {{0, "无错误"},
             {1, "编程错误"}},
            nullptr, nullptr
        });

        // Bit 3: 保留

        // --- Bit 4: WPEF (写保护错误标志) R/W (通常为 W1C)
        m_fields.push_back({
            "WPEF", 4, 1, BitFieldAccess::RW,
            {{0, "无错误"},
             {1, "写保护错误"}},
            nullptr, nullptr
        });

        // --- Bit 5: OCF (操作完成标志) R/W (通常为 W1C)
        m_fields.push_back({
            "OCF", 5, 1, BitFieldAccess::RW,
            {{0, "操作未完成"},
             {1, "操作完成"}},
            nullptr, nullptr
        });

        // Bit 31:6: 保留

        // 必须调用以构建按名称查找的映射
        build_field_map();
        // 实际的 BUSYF、PEF、WPEF、OCF 位在硬件中通常需要特殊的读写处理（如 W1C 或读清零）。
        // 如果需要模拟这些复杂行为，应在 Register 基类中或此处使用 Read/Write Callback。
    }
    ~FMC_STS() override = default;
};

// -------------------------------------------------------------------------

/**
 * @brief FMC 控制寄存器2 (FMC_CTRL2)
 * 偏移地址: 0x10
 * 复位值: 0x0000 0080 (LOCK=1)
 */
class FMC_CTRL2 final: public Register {
public:
    explicit FMC_CTRL2(uint32_t offset)
        : Register(offset, "FMC_CTRL2", 0x00000080) {

        // --- Bit 0: PG (编程) R/W
        m_fields.push_back({"PG", 0, 1, BitFieldAccess::RW,
            {{0, "Flash编程操作禁止"}, {1, "启动Flash编程操作"}},
            nullptr, nullptr});

        // --- Bit 1: PAGEERA (页擦除) R/W
        m_fields.push_back({"PAGEERA", 1, 1, BitFieldAccess::RW,
            {{0, "页擦除禁止"}, {1, "启动页擦除"}},
            nullptr, nullptr});

        // --- Bit 2: MASSERA (整片擦除) R/W
        m_fields.push_back({"MASSERA", 2, 1, BitFieldAccess::RW,
            {{0, "整片擦除禁止"}, {1, "启动整片擦除"}},
            nullptr, nullptr});

        // Bit 3: 保留

        // --- Bit 4: OBP (编程选项字节) R/W
        m_fields.push_back({"OBP", 4, 1, BitFieldAccess::RW,
            {{0, "禁止选项字节编程"}, {1, "允许选项字节编程"}},
            nullptr, nullptr});

        // --- Bit 5: OBE (擦除选项字节) R/W
        m_fields.push_back({"OBE", 5, 1, BitFieldAccess::RW,
            {{0, "禁止选项字节擦除"}, {1, "允许选项字节擦除"}},
            nullptr, nullptr});

        // --- Bit 6: STA (开始进行擦除操作) R/W (W1T)
        m_fields.push_back({"STA", 6, 1, BitFieldAccess::RW,
            {{0, "操作未触发"}, {1, "触发FLASH操作"}},
            nullptr, nullptr});

        // --- Bit 7: LOCK (锁定) R/W
        m_fields.push_back({
            "LOCK", 7, 1, BitFieldAccess::RW,
            {{0, "未锁定"},
             {1, "锁定"}},
            nullptr, nullptr
        });

        // Bit 8: 保留

        // --- Bit 9: OBWEN (使能选项字节写操作) R/W
        m_fields.push_back({"OBWEN", 9, 1, BitFieldAccess::RW,
            {{0, "选项字节写禁止"}, {1, "选项字节写使能"}},
            nullptr, nullptr});

        // --- Bit 10: ERRIE (使能错误中断) R/W
        m_fields.push_back({"ERRIE", 10, 1, BitFieldAccess::RW,
            {{0, "错误中断禁止"}, {1, "错误中断使能"}},
            nullptr, nullptr});

        // Bit 11: 保留

        // --- Bit 12: OCIE (使能操作完成中断) R/W
        m_fields.push_back({"OCIE", 12, 1, BitFieldAccess::RW,
            {{0, "操作完成中断禁止"}, {1, "操作完成中断使能"}},
            nullptr, nullptr});

        // Bit 31:13: 保留

        build_field_map();
    }
    ~FMC_CTRL2() override = default;
};

// -------------------------------------------------------------------------

/**
 * @brief 地址寄存器 (FMC_ADDR)
 * 偏移地址: 0x14
 * 复位值: 0x0000 0000
 */
class FMC_ADDR final: public Register {
public:
    explicit FMC_ADDR(uint32_t offset)
        : Register(offset, "FMC_ADDR", 0x00000000) {

        // --- Bit 31:0: ADDR (Flash地址) WO (只写)
        // 写入要编程或擦除的地址。
        m_fields.push_back({
            "ADDR", 0, 32, BitFieldAccess::WO,
            {},
            nullptr, nullptr
        });

        // 必须调用以构建按名称查找的映射
        build_field_map();
    }
    ~FMC_ADDR() override = default;
};

// -------------------------------------------------------------------------

/**
 * @brief 选项字节控制/状态寄存器 (FMC_OBCS)
 * 偏移地址: 0x1C
 * 复位值: 0x03FF FFFC
 */
class FMC_OBCS final: public Register {
    public:
        explicit FMC_OBCS(uint32_t offset)
            : Register(offset, "FMC_OBCS", 0x03FFFFFC) {

            // --- Bit 0: OBE (选项字节错误) RO
            m_fields.push_back({"OBE", 0, 1, BitFieldAccess::RO,
                {{0, "无选项字节错误"}, {1, "选项字节错误发生"}},
                nullptr, nullptr});

            // --- Bit 1: READPROT (读保护) RO
            m_fields.push_back({"READPROT", 1, 1, BitFieldAccess::RO,
                {{0, "读保护禁用"}, {1, "读保护使能"}},
                nullptr, nullptr});

            // --- Bit 9:2: UOB (用户选项字节) RO
            m_fields.push_back({"UOB", 2, 8, BitFieldAccess::RO, {}, nullptr, nullptr}); // 数据字段，保留空

            // --- Bit 17:10: DATA0 (Data0) RO
            m_fields.push_back({"DATA0", 10, 8, BitFieldAccess::RO, {}, nullptr, nullptr}); // 数据字段，保留空

            // --- Bit 25:18: DATA1 (Data1) RO
            m_fields.push_back({"DATA1", 18, 8, BitFieldAccess::RO, {}, nullptr, nullptr}); // 数据字段，保留空

            // Bit 31:26: 保留

            build_field_map();
        }
        ~FMC_OBCS() override = default;
};

// -------------------------------------------------------------------------

/**
 * @brief 写保护寄存器 (FMC_WRTPROT)
 * 偏移地址: 0x20
 * 复位值: 0xFFFF FFFF
 */
class FMC_WRTPROT final: public Register {
public:
    explicit FMC_WRTPROT(uint32_t offset)
        : Register(offset, "FMC_WRTPROT", 0xFFFFFFFF) {

        // --- Bit 31:0: WRTPROT (写保护) RO
        m_fields.push_back({
            "WRTPROT", 0, 32, BitFieldAccess::RO,
            {{0x00000000, "写保护全部有效"},
             {0xFFFFFFFF, "写保护全部无效"}},
            nullptr, nullptr
        });

        // 必须调用以构建按名称查找的映射
        build_field_map();
    }
    ~FMC_WRTPROT() override = default;
};
}
