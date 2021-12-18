#pragma once

namespace Hunter::Compiler {

    enum class IntType {
        i8 = 8,
        i16 = 16,
        i32 = 32,
        i64 = 64
    };

    enum class DataTypeId {
        Void = 0,
        String  = 1,
        Memory  = 2,
        Struct  = 3,
        List  = 4,
        i8      = static_cast<int>(IntType::i8),
        i16     = static_cast<int>(IntType::i16),
        i32     = static_cast<int>(IntType::i32),
        i64     = static_cast<int>(IntType::i64),

        Custom,

        Unknown = 999,
    };

    class DataType {
    public:
        explicit DataType(const DataTypeId id) : m_TypeId(id) {}

        [[nodiscard]] DataTypeId GetId() const {
            return m_TypeId;
        }

    private:
        DataTypeId m_TypeId;
    };

}
