#ifndef AJA_VARIANT_H
#define AJA_VARIANT_H

#include "ajabase/common/public.h"

#include <memory>
#include <type_traits>

namespace aja {

/*
 *  Variant - A simple Variant class for C++11
 */
class AJA_EXPORT Variant {
public:
    enum class Type : uint8_t {
        None = 0,
        Boolean,
        Float,
        Double,
        Int8,
        UInt8,
        Int16,
        UInt16,
        Int32,
        UInt32,
        Int64,
        UInt64,
        String,
        Blob,
        Map,
        Vector,
    };

    // Binary blob storage
    using BlobStorage = std::vector<uint8_t>;
    using VectorStorage = std::vector<Variant>;
    using MapStorage = std::map<std::string, std::unique_ptr<Variant>>;

    Variant() noexcept {} // a null Variant

    explicit Variant(Type type);
    explicit Variant(const bool value);
    explicit Variant(const float value);
    explicit Variant(const double value);
    explicit Variant(const int8_t value);
    explicit Variant(const uint8_t value);
    explicit Variant(const int16_t value);
    explicit Variant(const uint16_t value);
    explicit Variant(const int32_t value);
    explicit Variant(const uint32_t value);
    explicit Variant(const int64_t value);
    explicit Variant(const uint64_t value);
    explicit Variant(const char* value);
    explicit Variant(const char* value, std::size_t length);
    explicit Variant(const std::string& value);
    // explicit Variant(const BlobStorage& value);
    // explicit Variant(const MapStorage& value);
    // explicit Variant(const VectorStorage& value);

    Variant(const Variant& other);
    Variant(Variant&& other) noexcept;
    ~Variant();

    Type GetType() const { return m_type; }

    bool GetBool() const;
    float GetFloat() const;
    double GetDouble() const;
    int8_t GetInt8() const;
    uint8_t GetUInt8() const;
    int16_t GetInt16() const;
    uint16_t GetUInt16() const;
    int32_t GetInt32() const;
    uint32_t GetUInt32() const;
    int64_t GetInt64() const;
    uint64_t GetUInt64() const;
    const std::string& GetString() const;
    // const BlobStorage& AsBlob() const;
    // const MapStorage& AsMap() const;
    // const VectorStorage& AsVector() const;

    void SetBool(const bool value);
    void SetFloat(const float value);
    void SetDouble(const double value);
    void SetInt8(const int8_t value);
    void SetUInt8(const uint8_t value);
    void SetInt16(const int16_t value);
    void SetUInt16(const uint16_t value);
    void SetInt32(const int32_t value);
    void SetUInt32(const uint32_t value);
    void SetInt64(const int64_t value);
    void SetUInt64(const uint64_t value);
    void SetString(const char* value);
    void SetString(const char* value, std::size_t length);
    void SetString(const std::string& value);
    void SetString(std::string&& value);
    // void SetBlob(const BlobStorage& value);
    // void SetMap(const MapStorage& value);
    // void SetVector(const VectorStorage& value);

    // Type conversion methods
    bool AsBool() const;
    float AsFloat() const;
    double AsDouble() const;
    int8_t AsInt8() const;
    uint8_t AsUInt8() const;
    int16_t AsInt16() const;
    uint16_t AsUInt16() const;
    int32_t AsInt32() const;
    uint32_t AsUInt32() const;
    int64_t AsInt64() const;
    uint64_t AsUInt64() const;
    std::string AsString() const;

protected:
    Type m_type = Type::None;

    union {
        bool m_boolean_value;
        float m_float_value;
        double m_double_value;
        int8_t m_int8_value;
        uint8_t m_uint8_value;
        int16_t m_int16_value;
        uint16_t m_uint16_value;
        int32_t m_int32_value;
        uint32_t m_uint32_value;
        int64_t m_int64_value;
        uint64_t m_uint64_value;
        std::string m_string_value;
        BlobStorage m_blob_value;
        // MapStorage m_map_value;
        // VectorStorage m_vector_value;
    };
};

using VariantPtr = std::unique_ptr<Variant>;

// class AJA_EXPORT VariantSerializer {
// public:
//     virtual ~VariantSerializer();
//     virtual bool Serialize(const Variant& src) = 0;
// };

// class AJA_EXPORT VariantDeserializer {
// public:
//     virtual ~VariantDeserializer();
//     virtual VariantPtr Deserialize(std::string& var_str) = 0;
// };

};

#endif
