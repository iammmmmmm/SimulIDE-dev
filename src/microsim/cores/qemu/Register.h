#pragma once
#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <iomanip>
#include <memory>
#include <functional>
#include <QDebug>
struct BitField;
// =========================================================================
// 0. 回调函数定义
// =========================================================================
class Register;
//FIXME 我们应该在这个玩意成为屎山之前或写了太多太多Reg之前尽量让这个问题少一点，不然下场会很惨
//TODO 任何对于这个玩意的想法必须尽早实现，尽早测试完善，这里将会是地府大门
/**
 * @brief 位域级别的写入回调函数签名。
 * * 此回调函数在特定位域的值被写入（修改）后执行，用于处理写入操作带来的副作用或进行特殊逻辑。
该方法会在位域级别读写回调后调用
 * * @param reg 寄存器自身的引用。
 * @param field 被写入的位域结构体的引用。
 * @param new_field_value 写入该位域的新的值（即位域的实际新值）。**回调函数在此处执行该位域变化引起的副作用。**
 * @param user_data 用户自定义数据指针，用于传递额外的上下文信息。
 */
using BitFieldWriteCallback = std::function<void(Register &reg,
                                                 const BitField &field,
                                                 uint32_t new_field_value,
                                                 void *user_data)>;

/**
 * @brief 位域级别的读取回调函数签名。
 * * 此回调函数在特定位域的值被读取前（或在读取值准备好后）执行，用于处理读取操作的副作用或修改即将返回的位域读取值。
该方法会在位域级别读写回调后调用
 * * @param reg 寄存器自身的引用。
 * @param field 被读取的位域结构体的引用。
 * @param field_value_to_read 对即将返回的该位域的值的引用。回调函数可以在这里修改返回值，
 * 例如清除状态位（如果该位域是写清零型状态位）或返回特殊的只读值。
 * @param user_data 用户自定义数据指针，用于传递额外的上下文信息。
 */
using BitFieldReadCallback = std::function<void(Register &reg,
                                                const BitField &field,
                                                uint32_t &field_value_to_read,
                                                void *user_data)>;
/**
 * @brief 寄存器级别的写入回调函数签名。
 * * 此回调函数在整个寄存器的值被写入（修改）后执行，用于处理整个寄存器写入操作带来的副作用或进行特殊逻辑。
该方法会先于寄存器级别读写回调前调用
 * * @param reg 寄存器自身的引用。
 * @param new_register_value 写入该寄存器的新的完整值（即寄存器的实际新值）。**回调函数在此处执行该寄存器变化引起的副作用。**
 * @param user_data 用户自定义数据指针，用于传递额外的上下文信息。
 */
using RegisterWriteCallback = std::function<void(Register &reg,
                                                 uint32_t new_register_value,
                                                 void *user_data)>;


/**
 * @brief 寄存器级别的读取回调函数签名。
 * * 此回调函数在整个寄存器的值被读取前（或在读取值准备好后）执行，用于处理读取操作的副作用（如清除状态位）或修改即将返回的读取值。
该方法会先于寄存器级别读写回调前调用
 * * @param reg 寄存器自身的引用。
 * @param register_value_to_read 对即将返回的该寄存器值的引用。回调函数可以在这里修改返回值，
 * 例如，可以实现读取清除（RC）类型的状态位逻辑。
 * @param user_data 用户自定义数据指针，用于传递额外的上下文信息。
 */
using RegisterReadCallback = std::function<void(Register &reg,
                                                uint32_t &register_value_to_read,
                                                void *user_data)>;


// =========================================================================
// 1. 位域定义结构
// =========================================================================
enum class BitFieldAccess {
  RW, // 读写 (Read/Write)
  RO, // 只读 (Read Only) - 写入时被忽略
  WO, // 只写 (Write Only) - 写入有效，读取返回0 (需要在 read() 中处理)
  W1C, // 写1清零 (Write 1 to Clear) - 写入 1 清除该位，写入 0 无影响
  W0C // 写0清零 (Write 0 to Clear) - 写入 0 清除该位，写入 1 无影响 (较少见)
};
struct BitField {
  std::string name; // 位域名称，如 "HIRCEN"
  int start_bit; // 起始位 (从 0 开始)
  int width; // 宽度，对于单个位为 1
  BitFieldAccess access = BitFieldAccess::RW;
  // 存储位域值到描述的映射，例如 0->"关闭", 1->"打开"
  std::map<uint32_t, std::string> value_descriptions;
  //当此位域被修改时执行的回调函数
  BitFieldWriteCallback write_callback = nullptr;
  BitFieldReadCallback read_callback = nullptr;
};

// =========================================================================
// 2. 寄存器抽象基类
// =========================================================================

class Register {
  protected:
    uint32_t m_offset;
    uint32_t m_current_value = 0x00000000;
    std::string m_name;
    std::map<std::string, BitField *> m_field_map;

  public:
    RegisterWriteCallback write_callback = nullptr;
    RegisterReadCallback read_callback = nullptr;
    // 存储该寄存器所有位域的定义
    std::vector<BitField> m_fields;
    static bool s_enable_debug_output;
    Register(uint32_t offset, std::string name) : m_offset(offset), m_name(std::move(name)) {
    }
    /**
     * @brief 构建位域查找映射。必须在所有 BitField 都被添加到 m_fields 后调用。
     */
    void build_field_map() {
      m_field_map.clear();
      for (auto &field: m_fields) {
        m_field_map[field.name] = &field; // 映射名称到 BitField 对象的地址
      }
    }
    /**
     * @brief 按名称查找位域 (O(log N) 性能)。
     * @param field_name 要查找的位域名称。
     * @return 找到的 BitField 引用指针，如果未找到则返回 nullptr。
     */
    //TODO 再加一个map，建立name和指针的表，以空间换速度和开发人员的头发
    BitField *find_field(const std::string &field_name) {
      auto it = m_field_map.find(field_name);
      if (it != m_field_map.end()) {
        return it->second; // 返回映射中存储的 BitField 指针
      }
      return nullptr; // 未找到
    }
    /**
 * @brief 三参数构造函数，用于设置初始复位值
 */
    Register(uint32_t offset, std::string name, uint32_t reset_value)
      : m_offset(offset), m_current_value(reset_value), m_name(std::move(name)) {
    }
    virtual ~Register();
    // 设置全局调试开关
    static void set_debug_output(bool enable) {
      s_enable_debug_output = enable;
    }
    // // 设置寄存器级别的读取回调
    // void set_read_callback(RegisterReadCallback callback) {
    //   m_read_callback = std::move(callback);
    // }
    // 获取寄存器偏移地址
    uint32_t get_offset() const { return m_offset; }

    // 模拟读取操作
    uint32_t read(void *user_data) {
      uint32_t simulated_value = m_current_value; // 默认返回当前内部值（可能已被位域回调修改）

      //位域级别读取回调
      for (const auto &field: m_fields) {
        if (field.read_callback) {
          // 提取当前位域值
          const uint32_t mask = ((1U << field.width) - 1) << field.start_bit;
          uint32_t field_value = (m_current_value & mask) >> field.start_bit;

          // 执行回调，回调可能会修改 field_value
          field.read_callback(*this, field, field_value, user_data);

          // 如果回调修改了值，则更新 m_current_value
          // 注意：有些位域读取后会清除标志位，所以需要更新 m_current_value
          const uint32_t new_field_shifted = field_value << field.start_bit;
          m_current_value = (m_current_value & ~mask) | (new_field_shifted & mask);
        }
      }
      //寄存器级别回调
      if (read_callback != nullptr) {
        read_callback(*this, simulated_value, user_data);
      }
      // 处理只写 (WO) 属性：确保 WO 位的返回值是 0。
      for (const auto &field: m_fields) {
        if (field.access == BitFieldAccess::WO) {
          const uint32_t mask = ((1U << field.width) - 1) << field.start_bit;
          // 清除模拟值中 WO 位域的值，使其返回 0
          simulated_value &= ~mask;
        }
      }

      // 读取调试输出
      if (s_enable_debug_output) {
        debug_interpret_read(simulated_value);
      }
      return simulated_value;
    }

    /**
    * @brief 读取并返回指定位域的当前值（侵入式）。
    * * 注意：此方法会调用 Register::read()，因此会触发所有读回调和副作用。
    * * 模拟CPU/总线读取。
    * * @param field_name 要读取的位域名称。
    * @return uint32_t 位域的值。如果位域不存在，返回 0 并输出警告。
    */
    uint32_t get_field_value(const std::string &field_name, void *user_data) {
      // 1. 调用 read() 获取当前模拟的寄存器值，确保执行了所有回调。
      uint32_t current_reg_value = read(user_data);

      // 2. 查找位域定义
      const BitField *field = find_field(field_name);
      if (!field) {
        qDebug() << "Error: Attempted to read value from non-existent field" << field_name.c_str()
            << "in register" << m_name.c_str();
        return 0; // 返回 0 作为安全值
      }

      // 3. 计算掩码
      const uint32_t mask = ((1U << field->width) - 1) << field->start_bit;

      // 4. 提取位域值并返回
      return (current_reg_value & mask) >> field->start_bit;
    }

    /**
     * @brief 内部使用：读取指定位域的当前存储值，不触发任何读回调和副作用（非侵入式）。
     * 适用于模拟器内部逻辑（如时钟模块、状态检查）查询寄存器状态。
     * @param field_name 要读取的位域名称。
     * @return uint32_t 位域的值。如果位域不存在，返回 0 并输出警告。
     */
    uint32_t get_field_value_non_intrusive(const std::string &field_name) {
      // 1. 查找位域定义 (使用 const 版本的 find_field)
      const BitField *field = find_field(field_name);
      if (!field) {
        qDebug() << "Error: Attempted to read non-existent field" << field_name.c_str()
            << "in register" << m_name.c_str() << " (Non-intrusive read)";
        return 0; // 返回 0 作为安全值
      }

      // 2. 直接从 m_current_value 中提取值，不调用 read()
      const uint32_t mask = ((1U << field->width) - 1) << field->start_bit;

      return (m_current_value & mask) >> field->start_bit;
    }
    std::string getName() {
      return m_name;
    }

    // 模拟写入操作
    void write(uint32_t new_value, int size, void *user_data) {
      const uint32_t register_write_mask = (size >= 32) ? 0xFFFFFFFFU : ((1U << size) - 1);

      const uint32_t masked_new_value = new_value & register_write_mask;

      const uint32_t old_value = m_current_value;

      uint32_t effective_value = old_value & (~register_write_mask);

      for (const auto &field: m_fields) {
        const uint32_t field_mask = ((1U << field.width) - 1) << field.start_bit;
        const uint32_t effective_mask = field_mask & register_write_mask;
        if (effective_mask == 0) {
          continue;
        }
        switch (field.access) {
          case BitFieldAccess::RO:
          case BitFieldAccess::W1C: {
            effective_value = (effective_value & ~effective_mask) | (old_value & effective_mask);
            break;
          }
          case BitFieldAccess::RW:
          case BitFieldAccess::WO:
            effective_value = (effective_value & ~effective_mask) | (masked_new_value &
              effective_mask);
            break;

          case BitFieldAccess::W0C: {
            const uint32_t new_field_value = (masked_new_value & field_mask) >> field.start_bit;

            effective_value = (effective_value & ~effective_mask) | (old_value & effective_mask);

            if (new_field_value == 0) {
              effective_value &= ~effective_mask;
            } else {
            }
          }
          break;
        }
      }
      m_current_value = effective_value;

      for (auto &field: m_fields) {
        const uint32_t field_mask = ((1U << field.width) - 1) << field.start_bit;
        const uint32_t effective_mask = field_mask & register_write_mask;
        uint32_t callback_value = 0;
        if (effective_mask == 0) {
          continue;
        }

        bool should_call = false;
        switch (field.access) {
          case BitFieldAccess::RW:
          case BitFieldAccess::WO:
            callback_value = (effective_value & field_mask) >> field.start_bit;
            if (((old_value & effective_mask) >> field.start_bit) != ((effective_value &
              effective_mask) >> field.start_bit)) {
              should_call = true;
            }
            break;
          case BitFieldAccess::W1C:
          case BitFieldAccess::W0C:
            callback_value = (masked_new_value & field_mask) >> field.start_bit;
            if (field.access == BitFieldAccess::W1C && callback_value == 1) {
              should_call = true;
            } else if (field.access == BitFieldAccess::W0C && callback_value == 0) {
              should_call = true;
            }
            break;

          case BitFieldAccess::RO:
            break;
        }
        if (should_call && field.write_callback) {
          field.write_callback(*this, field, callback_value, user_data);
        }
      }
      if (write_callback != nullptr) {
        const uint32_t register_callback_value = effective_value & register_write_mask;
        write_callback(*this, register_callback_value, user_data);
      }

      if (s_enable_debug_output) {
        debug_interpret_write(masked_new_value);
      }
    }
    /**
    * @brief 修改特定位域的值（侵入式）,用于模拟硬件行为.
    * * 注意：此方法会调用 Register::write()，会触发所有读回调和副作用。
    * * 模拟CPU/总线读取。
   * @param field_name 要修改的位域名称。
   * @param size 数据大小
   * @param new_field_value 要设置的位域值。
   */
    void set_field_value(const std::string &field_name,
                         int size,
                         uint32_t new_field_value,
                         void *user_data) {
      BitField *field = find_field(field_name);
      if (!field) {
        qDebug() << "Error: Field" << field_name.c_str() << "not found in register" << m_name.
            c_str();
        return;
      }
      // 1. 获取当前寄存器值 (作为构造写入值的基准)
      const uint32_t old_value = m_current_value;

      // 2. 计算位域掩码
      const uint32_t mask = ((1U << field->width) - 1) << field->start_bit;

      // 3. 检查新值是否在位域范围内 (可选的边界检查)
      if (new_field_value >= (1U << field->width)) {
        qDebug() << "Warning: Value" << new_field_value << "exceeds field" << field_name.c_str() <<
            "width" << field->width << "bits. Truncating.";
        new_field_value = (1U << field->width) - 1;
      }

      // 4. 构造软件期望写入的完整 32 位值(value_to_write)
      const uint32_t new_field_shifted = new_field_value << field->start_bit;

      // value_to_write = (旧值 & ~目标位域掩码) | (新值 & 目标位域掩码)
      // 这就是原子写入的构造方式，确保只修改目标位域。
      const uint32_t value_to_write = (old_value & ~mask) | (new_field_shifted & mask);

      // 5. 调用完整的 write 方法来处理所有回调和读写限制。
      // write() 方法将负责处理 RO/WO/W1C/W0C 访问权限。
      this->write(value_to_write, size, user_data);

      if (s_enable_debug_output) {
        qDebug().noquote() << QString("--- (原子写入) 字段 %1: %2 更新完成 ---")
            .arg(m_name.c_str(), field_name.c_str());
      }
    }
    /**
        * @brief 专门用于模拟器内部逻辑，修改特定位域的值，不触发调试输出和回调。
        * 适用于模拟硬件的副作用，例如在启用时钟后设置就绪标志。
        * @param field_name 要修改的位域名称。
        * @param value_to_set 要设置的位域值。
        */
    void set_field_value_non_intrusive(const std::string &field_name, uint32_t value_to_set) {
      BitField *field = find_field(field_name);
      if (!field) {
        qDebug() << "Error: Field" << field_name.c_str() << "not found in register" << m_name.
            c_str();
        return;
      }

      // 1. 计算掩码
      const uint32_t mask = ((1U << field->width) - 1) << field->start_bit;

      // 检查新值是否在位域范围内
      if (value_to_set >= (1U << field->width)) {
        qDebug() << "Warning: Value" << value_to_set << "exceeds field" << field_name.c_str() <<
            "width" << field->width << "bits.";
        value_to_set = (1U << field->width) - 1; // 截断到最大值
      }

      const uint32_t new_field_shifted = value_to_set << field->start_bit;

      // 2. 清除旧值并设置新值
      m_current_value = (m_current_value & ~mask) | (new_field_shifted & mask);
      if (s_enable_debug_output) {
        qDebug().noquote() << "模拟器内部写入字段：" << field_name.c_str() << "，值：0x" << Qt::hex <<
            new_field_shifted;
      }
      // 注意：这里不执行 write 回调，也不执行 debug 输出，因为这是模拟的副作用，而不是一次真实地写操作。
    }

  private:
    // 位域解析和显示逻辑 (仅用于调试)
    void debug_interpret_write(uint32_t value) const {
      // 显示写入头信息
      qDebug().noquote() << "\n--- 写入寄存器: "
          << m_name.c_str() // 将 std::string 转换为 C 字符串
          << QString(" (Offset: 0x%1) ---")
          .arg(m_offset, 2, 16, QChar('0')); // Qt 格式化: 2位, 十六进制, 填充'0'

      qDebug().noquote() << QString("  实际值: 0x%1") // 更改为 实际值
          .arg(value, 8, 16, QChar('0')); // Qt 格式化: 8位, 十六进制, 填充'0'
      // 获取寄存器内部的当前值 (在 write() 逻辑计算后, 但在 debug 输出前)
      // 因为 debug_interpret_write 是在 m_current_value 被更新后调用的，所以此时 m_current_value 是最终结果。
      const uint32_t internal_value = m_current_value;
      for (const auto &field: m_fields) {
        // 计算掩码 (Mask)
        uint32_t mask = ((1U << field.width) - 1) << field.start_bit;

        // 提取该位域的值
        uint32_t field_value;
        if (field.access == BitFieldAccess::RO || field.access == BitFieldAccess::W1C) {
          // 对于 RO 位域（只读，写入被忽略）和 W1C 位域（写入不改变状态），
          // 应该显示其在寄存器中实际保留的值 (internal_value)。
          field_value = (internal_value & mask) >> field.start_bit;
        } else {
          // 对于 RW, WO, W0C 位域，显示固件尝试写入的值 (value)，以体现固件意图。
          field_value = (value & mask) >> field.start_bit;
        }
        // 构建位域范围字符串
        QString bit_range = QString::number(field.start_bit);
        if (field.width > 1) {
          bit_range += "-" + QString::number(field.start_bit + field.width - 1);
        }
        // 标记访问权限
        QString access_str;
        switch (field.access) {
          case BitFieldAccess::RO: access_str = "(RO)";
            break;
          case BitFieldAccess::WO: access_str = "(WO)";
            break;
          case BitFieldAccess::W1C: access_str = "(W1C)";
            break;
          case BitFieldAccess::RW: default: access_str = "(RW)";
            break;
        }

        // 基础输出
        QString output_line = QString("  > **位域 %1: %2 %3** ").arg(
          bit_range,
          field.name.c_str(),
          access_str);

        // 查找描述信息
        if (field.value_descriptions.count(field_value)) {
          output_line += QString(" (值: %1) -> %2").arg(field_value)
              .arg(field.value_descriptions.at(field_value).c_str());
        } else {
          // 如果没有特定描述，直接显示值
          output_line += QString(" (值: %1)").arg(field_value);
        }

        qDebug().noquote() << output_line;
      }
    }
    // 位域解析和显示逻辑
    void debug_interpret_read(uint32_t value) const {
      // 显示读取头信息
      qDebug().noquote() << "\n--- 读取寄存器: "
          << m_name.c_str()
          << QString(" (Offset: 0x%1) ---")
          .arg(m_offset, 2, 16, QChar('0'));

      qDebug().noquote() << QString("  返回值: 0x%1")
          .arg(value, 8, 16, QChar('0'));

      for (const auto &field: m_fields) {
        // 计算掩码 (Mask)
        uint32_t mask = ((1U << field.width) - 1) << field.start_bit;

        // 提取该位域的值
        uint32_t field_value = (value & mask) >> field.start_bit;

        // 构建位域范围字符串
        QString bit_range = QString::number(field.start_bit);
        if (field.width > 1) {
          bit_range += "-" + QString::number(field.start_bit + field.width - 1);
        }
        // 标记访问权限
        QString access_str;
        switch (field.access) {
          case BitFieldAccess::RO: access_str = "(RO)";
            break;
          case BitFieldAccess::WO: access_str = "(WO)";
            break;
          case BitFieldAccess::W1C: access_str = "(W1C)";
            break;
          case BitFieldAccess::RW: default: access_str = "(RW)";
            break;
        }
        // 基础输出
        QString output_line = QString("  > **位域 %1: %2 %3** ").arg(
          bit_range,
          field.name.c_str(),
          access_str);
        // 查找描述信息
        if (field.value_descriptions.count(field_value)) {
          output_line += QString(" (当前值: %1) -> %2").arg(field_value)
              .arg(field.value_descriptions.at(field_value).c_str());
        } else {
          // 如果没有特定描述，直接显示值
          output_line += QString(" (当前值: %1)").arg(field_value);
        }
        qDebug().noquote() << output_line;
      }
    }
};
