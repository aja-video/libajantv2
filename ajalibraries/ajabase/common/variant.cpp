#include "variant.h"

#include <new>

namespace aja {

Variant::Variant(Type type)
:
m_type(type)
{
    switch(m_type) {
        case Type::None:
            return;
        case Type::Boolean:
            m_boolean_value = false;
            return;
        case Type::Float:
            m_float_value = 0.f;
            return;
        case Type::Double:
            m_double_value = 0.0;
            return;
        case Type::Int8:
            m_int8_value = 0;
            return;
        case Type::UInt8:
            m_uint8_value = 0;
            return;
        case Type::Int16:
            m_int16_value = 0;
            return;
        case Type::UInt16:
            m_uint16_value = 0;
            return;
        case Type::Int32:
            m_int32_value = 0;
            return;
        case Type::UInt32:
            m_uint32_value = 0;
            return;
        case Type::Int64:
            m_int64_value = 0;
            return;
        case Type::UInt64:
            m_uint64_value = 0;
            return;
        case Type::String:
            new (&m_string_value) std::string();
            return;
        case Type::Blob:
            new (&m_blob_value) std::vector<uint8_t>;
        // case Type::Map:
        //     new (&m_map_value) std::map<std::string, Variant>;
        // case Type::Vector:
        //     new (&m_vector_value) std::vector<Variant>;
    }
}

Variant::Variant(const bool value)
    : m_type(Type::Boolean), m_boolean_value(value) {}
Variant::Variant(const float value)
    : m_type(Type::Float), m_float_value(value) {}
Variant::Variant(const double value)
    : m_type(Type::Double), m_double_value(value) {}
Variant::Variant(const int8_t value)
    : m_type(Type::Int8), m_int8_value(value) {}
Variant::Variant(const uint8_t value)
    : m_type(Type::UInt8), m_uint8_value(value) {}
Variant::Variant(const int16_t value)
    : m_type(Type::Int16), m_int16_value(value) {}
Variant::Variant(const uint16_t value)
    : m_type(Type::UInt16), m_uint16_value(value) {}
Variant::Variant(const int32_t value)
    : m_type(Type::Int32), m_int32_value(value) {}
Variant::Variant(const uint32_t value)
    : m_type(Type::UInt32), m_uint32_value(value) {}
Variant::Variant(const int64_t value)
    : m_type(Type::Int64), m_int64_value(value) {}
Variant::Variant(const uint64_t value)
    : m_type(Type::UInt64), m_uint64_value(value) {}
Variant::Variant(const char* value)
    : m_type(Type::String), m_string_value(std::string(value)) {}
Variant::Variant(const char* value, std::size_t length)
    : m_type(Type::String), m_string_value(std::string(value, length)) {}
Variant::Variant(const std::string& value)
    : m_type(Type::String), m_string_value(value) {}

Variant::Variant(const Variant& other) : Variant(other.m_type) {
    m_type = other.m_type;
    switch (m_type) {
        case Type::None:
            return;
        case Type::Boolean:
            m_boolean_value = other.m_boolean_value;
            return;
        case Type::Float:
            m_float_value = other.m_float_value;
            return;
        case Type::Double:
            m_double_value = other.m_double_value;
            return;
        case Type::Int8:
            m_int8_value = other.m_int8_value;
            return;
        case Type::UInt8:
            m_uint8_value = other.m_uint8_value;
            return;
        case Type::Int16:
            m_int16_value = other.m_int16_value;
            return;
        case Type::UInt16:
            m_uint16_value = other.m_uint16_value;
            return;
        case Type::Int32:
            m_int32_value = other.m_int32_value;
            return;
        case Type::UInt32:
            m_uint32_value = other.m_uint32_value;
            return;
        case Type::Int64:
            m_int64_value = other.m_int64_value;
            return;
        case Type::UInt64:
            m_uint64_value = other.m_uint64_value;
            return;
        case Type::String:
            m_string_value = std::string(other.m_string_value);
            return;
    }
}

Variant::Variant(Variant&& other) noexcept : Variant(other.m_type) {
    m_type = other.m_type;
    switch (m_type) {
        case Type::None:
            return;
        case Type::Boolean:
            m_boolean_value = other.m_boolean_value;
            return;
        case Type::Float:
            m_float_value = other.m_float_value;
            return;
        case Type::Double:
            m_double_value = other.m_double_value;
            return;
        case Type::Int8:
            m_int8_value = other.m_int8_value;
            return;
        case Type::UInt8:
            m_uint8_value = other.m_uint8_value;
            return;
        case Type::Int16:
            m_int16_value = other.m_int16_value;
            return;
        case Type::UInt16:
            m_uint16_value = other.m_uint16_value;
            return;
        case Type::Int32:
            m_int32_value = other.m_int32_value;
            return;
        case Type::UInt32:
            m_uint32_value = other.m_uint32_value;
            return;
        case Type::Int64:
            m_int64_value = other.m_int64_value;
            return;
        case Type::UInt64:
            m_uint64_value = other.m_uint64_value;
            return;
        case Type::String:
            m_string_value = other.m_string_value;
            return;
    }
}

Variant::~Variant() {
    switch(m_type) {
        case Type::None:
            return;
        case Type::Boolean:
        case Type::Float:
        case Type::Double:
        case Type::Int8:
        case Type::UInt8:
        case Type::Int16:
        case Type::UInt16:
        case Type::Int32:
        case Type::UInt32:
        case Type::Int64:
        case Type::UInt64:
            // Nothing to do!
            return;
        case Type::String:
            m_string_value.~basic_string();
            return;
        case Type::Blob:
            m_blob_value.~BlobStorage();
            return;
        // case Type::Map:
        //     m_map_value.~MapStorage();
        //     return;
        // case Type::Vector:
        //     m_vector_value.~VectorStorage();
        //     return;
    }

    m_type = Type::None;
}

// Getters
bool Variant::GetBool() const {
    return m_boolean_value;
}
float Variant::GetFloat() const {
    return m_float_value;
}
double Variant::GetDouble() const {
    return m_double_value;
}
int8_t Variant::GetInt8() const {
    return m_int8_value;
}
uint8_t Variant::GetUInt8() const {
    return m_uint8_value;
}
int16_t Variant::GetInt16() const {
    return m_int16_value;
}
uint16_t Variant::GetUInt16() const {
    return m_uint16_value;
}
int32_t Variant::GetInt32() const {
    return m_int32_value;
}
uint32_t Variant::GetUInt32() const {
    return m_uint32_value;
}
int64_t Variant::GetInt64() const {
    return m_int64_value;
}
uint64_t Variant::GetUInt64() const {
    return m_uint64_value;
}
const std::string& Variant::GetString() const {
    return m_string_value;
}

// Setters
void Variant::SetBool(const bool value) {
    m_boolean_value = value;
}
void Variant::SetFloat(const float value) {
    m_float_value = value;
}
void Variant::SetDouble(const double value) {
    m_double_value = value;
}
void Variant::SetInt8(const int8_t value) {
    m_int8_value = value;
}
void Variant::SetUInt8(const uint8_t value) {
    m_uint8_value = value;
}
void Variant::SetInt16(const int16_t value) {
    m_int16_value = value;
}
void Variant::SetUInt16(const uint16_t value) {
    m_uint16_value = value;
}
void Variant::SetInt32(const int32_t value) {
    m_int32_value = value;
}
void Variant::SetUInt32(const uint32_t value) {
    m_uint32_value = value;
}
void Variant::SetInt64(const int64_t value) {
    m_int64_value = value;
}
void Variant::SetUInt64(const uint64_t value) {
    m_uint64_value = value;
}
void Variant::SetString(const char* value) {
    m_string_value = std::string(value);
}
void Variant::SetString(const char* value, std::size_t length) {
    m_string_value = std::string(value, length);
}
void Variant::SetString(const std::string& value) {
    m_string_value = value;
}
void Variant::SetString(std::string&& value) {
    m_string_value = std::move(value);
}

// Type conversion methods
bool Variant::AsBool() const {
    switch(m_type) {
        case Type::None:
            return false;
        case Type::Boolean:
            return m_boolean_value;
        case Type::Float:
            return m_float_value ? true : false;
        case Type::Double:
            return m_double_value ? true : false;
        case Type::Int8:
            return m_int8_value ? true : false;
        case Type::UInt8:
            return m_uint8_value ? true : false;
        case Type::Int16:
            return m_int16_value ? true : false;
        case Type::UInt16:
            return m_uint16_value ? true : false;
        case Type::Int32:
            return m_int32_value ? true : false;
        case Type::UInt32:
            return m_uint32_value ? true : false;
        case Type::Int64:
            return m_int64_value ? true : false;
        case Type::UInt64:
            return m_uint64_value ? true : false;
        case Type::String:
        {
            if (!m_string_value.empty())
                return true;
            else
                return false;
        }
    }

    return false;
}
float Variant::AsFloat() const {
    switch(m_type) {
        case Type::None:
            return 0.f;
        case Type::Boolean:
            return m_boolean_value ? 1.f : 0.f;
        case Type::Float:
            return m_float_value;
        case Type::Double:
            return static_cast<float>(m_double_value);
        case Type::Int8:
            return static_cast<float>(m_int8_value);
        case Type::UInt8:
            return static_cast<float>(m_uint8_value);
        case Type::Int16:
            return static_cast<float>(m_int16_value);
        case Type::UInt16:
            return static_cast<float>(m_uint16_value);
        case Type::Int32:
            return static_cast<float>(m_int32_value);
        case Type::UInt32:
            return static_cast<float>(m_uint32_value);
        case Type::Int64:
            return static_cast<float>(m_int64_value);
        case Type::UInt64:
            return static_cast<float>(m_uint64_value);
        case Type::String:
        {
            if (m_string_value.empty())
                return 0.f;
            else
                return std::stof(m_string_value);
        }
    }

    return 0.f;
}
double Variant::AsDouble() const {
    switch(m_type) {
        case Type::None:
            return 0.0;
        case Type::Boolean:
            return m_boolean_value ? 1.0 : 0.0;
        case Type::Float:
            return m_float_value;
        case Type::Double:
            return m_double_value;
        case Type::Int8:
            return static_cast<double>(m_int8_value);
        case Type::UInt8:
            return static_cast<double>(m_uint8_value);
        case Type::Int16:
            return static_cast<double>(m_int16_value);
        case Type::UInt16:
            return static_cast<double>(m_uint16_value);
        case Type::Int32:
            return static_cast<double>(m_int32_value);
        case Type::UInt32:
            return static_cast<double>(m_uint32_value);
        case Type::Int64:
            return static_cast<double>(m_int64_value);
        case Type::UInt64:
            return static_cast<double>(m_uint64_value);
        case Type::String:
        {
            if (m_string_value.empty())
                return 0.0;
            else
                return std::stod(m_string_value);
        }
    }

    return 0.0;
}
int8_t Variant::AsInt8() const {
    switch(m_type) {
        case Type::None:
            return 0;
        case Type::Boolean:
            return m_boolean_value ? 1 : 0;
        case Type::Float:
            return static_cast<int8_t>(m_float_value);
        case Type::Double:
            return static_cast<int8_t>(m_double_value);
        case Type::Int8:
            return m_int8_value;
        case Type::UInt8:
            return static_cast<int8_t>(m_uint8_value);
        case Type::Int16:
            return static_cast<int8_t>(m_int16_value);
        case Type::UInt16:
            return static_cast<int8_t>(m_uint16_value);
        case Type::Int32:
            return static_cast<int8_t>(m_int32_value);
        case Type::UInt32:
            return static_cast<int8_t>(m_uint32_value);
        case Type::Int64:
            return static_cast<int8_t>(m_int64_value);
        case Type::UInt64:
            return static_cast<int8_t>(m_uint64_value);
        case Type::String:
        {
            if (m_string_value.empty())
                return 0;
            else
                return static_cast<int8_t>(std::stoi(m_string_value));
        }
    }

    return 0;
}
uint8_t Variant::AsUInt8() const {
    switch(m_type) {
        case Type::None:
            return 0;
        case Type::Boolean:
            return m_boolean_value ? 1 : 0;
        case Type::Float:
            return static_cast<uint8_t>(m_float_value);
        case Type::Double:
            return static_cast<uint8_t>(m_double_value);
        case Type::Int8:
            return static_cast<uint8_t>(m_int8_value);
        case Type::UInt8:
            return m_uint8_value;
        case Type::Int16:
            return static_cast<uint8_t>(m_int16_value);
        case Type::UInt16:
            return static_cast<uint8_t>(m_uint16_value);
        case Type::Int32:
            return static_cast<uint8_t>(m_int32_value);
        case Type::UInt32:
            return static_cast<uint8_t>(m_uint32_value);
        case Type::Int64:
            return static_cast<uint8_t>(m_int64_value);
        case Type::UInt64:
            return static_cast<uint8_t>(m_uint64_value);
        case Type::String:
        {
            if (m_string_value.empty())
                return 0;
            else
                return static_cast<uint8_t>(std::stoi(m_string_value));
        }
    }

    return 0;
}
int16_t Variant::AsInt16() const {
    switch(m_type) {
        case Type::None:
            return 0;
        case Type::Boolean:
            return m_boolean_value ? 1 : 0;
        case Type::Float:
            return static_cast<int16_t>(m_float_value);
        case Type::Double:
            return static_cast<int16_t>(m_double_value);
        case Type::Int8:
            return static_cast<int16_t>(m_int8_value);
        case Type::UInt8:
            return static_cast<int16_t>(m_uint8_value);
        case Type::Int16:
            return m_int16_value;
        case Type::UInt16:
            return static_cast<int16_t>(m_uint16_value);
        case Type::Int32:
            return static_cast<int16_t>(m_int32_value);
        case Type::UInt32:
            return static_cast<int16_t>(m_uint32_value);
        case Type::Int64:
            return static_cast<int16_t>(m_int64_value);
        case Type::UInt64:
            return static_cast<int16_t>(m_uint64_value);
        case Type::String:
        {
            if (m_string_value.empty())
                return 0;
            else
                return static_cast<int16_t>(std::stoi(m_string_value));
        }
    }

    return 0;
}
uint16_t Variant::AsUInt16() const {
    switch(m_type) {
        case Type::None:
            return 0;
        case Type::Boolean:
            return m_boolean_value ? 1 : 0;
        case Type::Float:
            return static_cast<uint16_t>(m_float_value);
        case Type::Double:
            return static_cast<uint16_t>(m_double_value);
        case Type::Int8:
            return static_cast<uint16_t>(m_int8_value);
        case Type::UInt8:
            return static_cast<uint16_t>(m_uint8_value);
        case Type::Int16:
            return static_cast<uint16_t>(m_int16_value);
        case Type::UInt16:
            return m_uint16_value;
        case Type::Int32:
            return static_cast<uint16_t>(m_int32_value);
        case Type::UInt32:
            return static_cast<uint16_t>(m_uint32_value);
        case Type::Int64:
            return static_cast<uint16_t>(m_int64_value);
        case Type::UInt64:
            return static_cast<uint16_t>(m_uint64_value);
        case Type::String:
        {
            if (m_string_value.empty())
                return 0;
            else
                return static_cast<uint16_t>(std::stoi(m_string_value));
        }
    }

    return 0;
}
int32_t Variant::AsInt32() const {
    switch(m_type) {
        case Type::None:
            return 0;
        case Type::Boolean:
            return m_boolean_value ? 1 : 0;
        case Type::Float:
            return static_cast<int32_t>(m_float_value);
        case Type::Double:
            return static_cast<int32_t>(m_double_value);
        case Type::Int8:
            return static_cast<int32_t>(m_int8_value);
        case Type::UInt8:
            return static_cast<int32_t>(m_uint8_value);
        case Type::Int16:
            return static_cast<int32_t>(m_int16_value);
        case Type::UInt16:
            return static_cast<int32_t>(m_uint16_value);
        case Type::Int32:
            return m_int32_value;
        case Type::UInt32:
            return static_cast<int32_t>(m_uint32_value);
        case Type::Int64:
            return static_cast<int32_t>(m_int64_value);
        case Type::UInt64:
            return static_cast<int32_t>(m_uint64_value);
        case Type::String:
        {
            if (m_string_value.empty())
                return 0;
            else {
#if __LP64__
                return std::stol(m_string_value);
#else
                return std::stoi(m_string_value);
#endif
            }
        }
    }

    return 0;
}
uint32_t Variant::AsUInt32() const {
    switch(m_type) {
        case Type::None:
            return 0;
        case Type::Boolean:
            return m_boolean_value ? 1 : 0;
        case Type::Float:
            return static_cast<uint32_t>(m_float_value);
        case Type::Double:
            return static_cast<uint32_t>(m_double_value);
        case Type::Int8:
            return static_cast<uint32_t>(m_int8_value);
        case Type::UInt8:
            return static_cast<uint32_t>(m_uint8_value);
        case Type::Int16:
            return static_cast<uint32_t>(m_int16_value);
        case Type::UInt16:
            return static_cast<uint32_t>(m_uint16_value);
        case Type::Int32:
            return static_cast<uint32_t>(m_int32_value);
        case Type::UInt32:
            return m_uint32_value;
        case Type::Int64:
            return static_cast<uint32_t>(m_int64_value);
        case Type::UInt64:
            return static_cast<uint32_t>(m_uint64_value);
        case Type::String:
        {
            if (m_string_value.empty())
                return 0;
            else {
#if defined(__LP64__)
                return std::stoul(m_string_value);
#else
                return static_cast<uint32_t>(std::stoi(m_string_value));
#endif
            }
        }
    }

    return 0;
}
int64_t Variant::AsInt64() const {
    switch(m_type) {
        case Type::None:
            return 0;
        case Type::Boolean:
            return m_boolean_value ? 1 : 0;
        case Type::Float:
            return static_cast<int64_t>(m_float_value);
        case Type::Double:
            return static_cast<int64_t>(m_double_value);
        case Type::Int8:
            return static_cast<int64_t>(m_int8_value);
        case Type::UInt8:
            return static_cast<int64_t>(m_uint8_value);
        case Type::Int16:
            return static_cast<int64_t>(m_int16_value);
        case Type::UInt16:
            return static_cast<int64_t>(m_uint16_value);
        case Type::Int32:
            return static_cast<int64_t>(m_int32_value);
        case Type::UInt32:
            return static_cast<int64_t>(m_uint32_value);
        case Type::Int64:
            return m_int64_value;
        case Type::UInt64:
            return static_cast<int64_t>(m_uint64_value);
        case Type::String:
        {
            if (m_string_value.empty())
                return 0;
            else
                return std::stoll(m_string_value);
        }
    }

    return 0;
}
uint64_t Variant::AsUInt64() const {
    switch(m_type) {
        case Type::None:
            return 0;
        case Type::Boolean:
            return m_boolean_value ? 1 : 0;
        case Type::Float:
            return static_cast<uint64_t>(m_float_value);
        case Type::Double:
            return static_cast<uint64_t>(m_double_value);
        case Type::Int8:
            return static_cast<uint64_t>(m_int8_value);
        case Type::UInt8:
            return static_cast<uint64_t>(m_uint8_value);
        case Type::Int16:
            return static_cast<uint64_t>(m_int16_value);
        case Type::UInt16:
            return static_cast<uint64_t>(m_uint16_value);
        case Type::Int32:
            return static_cast<uint64_t>(m_int32_value);
        case Type::UInt32:
            return static_cast<uint64_t>(m_uint32_value);
        case Type::Int64:
            return static_cast<uint64_t>(m_int64_value);
        case Type::UInt64:
            return m_uint64_value;
        case Type::String:
        {
            if (m_string_value.empty())
                return 0;
            else
                return std::stoull(m_string_value);
        }
    }

    return 0;
}
std::string Variant::AsString() const {
    switch(m_type) {
        case Type::None:
            return std::string();
        case Type::Boolean:
        {
            if (m_boolean_value)
                return std::string("true");
            else
                return std::string("false");
        }
        case Type::Float:
            return std::to_string(m_float_value);
        case Type::Double:
            return std::to_string(m_double_value);
        case Type::Int8:
            return std::to_string(m_int8_value);
        case Type::UInt8:
            return std::to_string(m_uint8_value);
        case Type::Int16:
            return std::to_string(m_int16_value);
        case Type::UInt16:
            return std::to_string(m_uint16_value);
        case Type::Int32:
            return std::to_string(m_int32_value);
        case Type::UInt32:
            return std::to_string(m_uint32_value);
        case Type::Int64:
            return std::to_string(m_int64_value);
        case Type::UInt64:
            return std::to_string(m_uint64_value);
        case Type::String:
            return m_string_value;
    }

    return std::string();
}

} // namespace aja
