// Copyright 2016 Daniel Parker
// Distributed under Boost license

#if defined(_MSC_VER)
#include "windows.h"
#endif
#include <catch/catch.hpp>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/cbor/cbor.hpp>
#include <jsoncons_ext/cbor/cbor_serializer.hpp>
#include <sstream>
#include <vector>
#include <utility>
#include <ctime>
#include <limits>

using namespace jsoncons;
using namespace jsoncons::cbor;

TEST_CASE("test_serialize_to_stream")
{
json j = json::parse(R"(
{
   "application": "hiking",
   "reputons": [
   {
       "rater": "HikingAsylum.example.com",
       "assertion": "is-good",
       "rated": "sk",
       "rating": 0.90
     }
   ]
}
)");

    std::ofstream os;
    os.open("./output/store.cbor", std::ios::binary | std::ios::out);
    encode_cbor(j,os);

    std::vector<uint8_t> v;
    std::ifstream is;
    is.open("./output/store.cbor", std::ios::binary | std::ios::in);

    json j2 = decode_cbor<json>(is);

    //std::cout << pretty_print(j2) << std::endl; 

    CHECK(j == j2);
}

TEST_CASE("serialize array to cbor")
{
    std::vector<uint8_t> v;
    cbor_buffer_serializer serializer(v);
    //serializer.begin_object(1);
    serializer.begin_array(3);
    serializer.bool_value(true);
    serializer.bool_value(false);
    serializer.null_value();
    serializer.end_array();
    //serializer.end_object();
    serializer.flush();

    try
    {
        json result = decode_cbor<json>(v);
        std::cout << result << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
} 

TEST_CASE("test_serialize_indefinite_length_array")
{
    std::vector<uint8_t> v;
    cbor_buffer_serializer serializer(v);
    serializer.begin_array();
    serializer.begin_array(4);
    serializer.bool_value(true);
    serializer.bool_value(false);
    serializer.null_value();
    serializer.string_value("Hello");
    serializer.end_array();
    serializer.end_array();
    serializer.flush();

    try
    {
        json result = decode_cbor<json>(v);
        std::cout << result << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
} 

TEST_CASE("test_serialize_bignum")
{
    std::vector<uint8_t> v;
    cbor_buffer_serializer serializer(v);
    serializer.begin_array();

    std::vector<uint8_t> bytes = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    bignum n(1, bytes.data(), bytes.size());
    std::string s;
    n.dump(s);
    serializer.bignum_value(s);
    serializer.end_array();
    serializer.flush();

    try
    {
        json result = decode_cbor<json>(v);
        CHECK(result[0].as<std::string>() == std::string("18446744073709551616"));
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
} 

TEST_CASE("test_serialize_negative_bignum1")
{
    std::vector<uint8_t> v;
    cbor_buffer_serializer serializer(v);
    serializer.begin_array();

    std::vector<uint8_t> bytes = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    bignum n(-1, bytes.data(), bytes.size());
    std::string s;
    n.dump(s);
    serializer.bignum_value(s);
    serializer.end_array();
    serializer.flush();

    try
    {
        json result = decode_cbor<json>(v);
        CHECK(result[0].as<std::string>() == std::string("-18446744073709551617"));
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
} 

TEST_CASE("test_serialize_negative_bignum2")
{
    std::vector<uint8_t> v;
    cbor_buffer_serializer serializer(v);
    serializer.begin_array();

    std::vector<uint8_t> bytes = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    bignum n(-1, bytes.data(), bytes.size());
    std::string s;
    n.dump(s);
    serializer.bignum_value(s);
    serializer.end_array();
    serializer.flush();

    try
    {
        json result = decode_cbor<json>(v);
        json_options options;
        options.bignum_format(bignum_chars_format::integer);
        std::string text;
        result.dump(text,options);
        CHECK(text == std::string("[-18446744073709551617]"));
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
} 

TEST_CASE("test_serialize_negative_bignum3")
{
    std::vector<uint8_t> v;
    cbor_buffer_serializer serializer(v);
    serializer.begin_array();

    std::vector<uint8_t> bytes = {0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

    bignum n(-1, bytes.data(), bytes.size());
    std::string s;
    n.dump(s);
    serializer.bignum_value(s);
    serializer.end_array();
    serializer.flush();

    try
    {
        json result = decode_cbor<json>(v);
        json_options options;
        options.bignum_format(bignum_chars_format::base64url);
        std::string text;
        result.dump(text,options);
        CHECK(text == std::string("[\"~AQAAAAAAAAAA\"]"));
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
    }
} 
TEST_CASE("test_serialize_decimal")
{
    SECTION("-1 184467440737095516160")
    {
        std::vector<uint8_t> v;
        cbor_buffer_serializer serializer(v);
        serializer.string_value("18446744073709551616.0", semantic_tag_type::decimal_fraction);
        serializer.flush();
        try
        {
            json result = decode_cbor<json>(v);
            CHECK(result.as<std::string>() == std::string("18446744073709551616.0"));
        }
        catch (const std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
    }
    SECTION("18446744073709551616e-5")
    {
        std::vector<uint8_t> v;
        cbor_buffer_serializer serializer(v);
        serializer.string_value("18446744073709551616e-5", semantic_tag_type::decimal_fraction);
        serializer.flush();
        try
        {
            json result = decode_cbor<json>(v);
            CHECK(result.as<std::string>() == std::string("184467440737095.51616"));
        }
        catch (const std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
    }
    SECTION("-18446744073709551616e-5")
    {
        std::vector<uint8_t> v;
        cbor_buffer_serializer serializer(v);
        serializer.string_value("-18446744073709551616e-5", semantic_tag_type::decimal_fraction);
        serializer.flush();
        try
        {
            json result = decode_cbor<json>(v);
            CHECK(result.as<std::string>() == std::string("-184467440737095.51616"));
        }
        catch (const std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
    }
    SECTION("-18446744073709551616e5")
    {
        std::vector<uint8_t> v;
        cbor_buffer_serializer serializer(v);
        serializer.string_value("-18446744073709551616e5", semantic_tag_type::decimal_fraction);
        serializer.flush();
        try
        {
            json result = decode_cbor<json>(v);
            CHECK(result.as<std::string>() == std::string("-18446744073709551616e5"));
        }
        catch (const std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
    }
} 

