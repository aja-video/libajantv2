/**
    @file		main.cpp
	@brief		Unittests for the Base part of AJA NTV2 Library (using doctest).
	@copyright	(C) 2019 AJA Video Systems, Inc. All rights reserved.
**/
// for doctest usage see: https://github.com/onqtam/doctest/blob/1.1.4/doc/markdown/tutorial.md

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
// need to define this so will work with compilers that don't support thread_local
// ie xcode 6, 7
#define DOCTEST_THREAD_LOCAL
#include "doctest.h"

#include "limits.h"

#include "ajabase/common/common.h"
#include "ajabase/common/guid.h"
#include "ajabase/common/performance.h"
#include "ajabase/common/timebase.h"
#include "ajabase/common/timecode.h"
#include "ajabase/common/timer.h"
#include "ajabase/persistence/persistence.h"
#include "ajabase/system/atomic.h"
#include "ajabase/system/file_io.h"
#include "ajabase/system/info.h"
#include "ajabase/system/systemtime.h"

#include <algorithm>
#include <clocale>
#include <iostream>

#ifdef AJA_WINDOWS
#include <direct.h>
#include <windows.h>
#include <mmsystem.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

/*
//template
void filename_marker() {} //this is used to easily just around in a GUI with a symbols list
TEST_SUITE("filename" * doctest::description("functions in streams/common/filename.h")) {

    TEST_CASE("constructor")
    {
    }

} //filename
*/

void types_marker() {}
TEST_SUITE("types" * doctest::description("functions in ajabase/common/types.h")) {

    TEST_CASE("AJA_FOURCC and AJA_FCC")
    {
        uint32_t ajaAsFourcc = 1634361632;

        CHECK(AJA_FOURCC('a','j','a',' ') == ajaAsFourcc);
        CHECK(AJA_FOURCC('A','J','A',' ') != ajaAsFourcc);
        CHECK(AJA_FCC("aja ") == ajaAsFourcc);
        CHECK(AJA_FCC("AJA ") != ajaAsFourcc);
    }

} //types

void common_marker() {}
TEST_SUITE("common" * doctest::description("functions in ajabase/common/common.h")) {

    TEST_CASE("aja::replace")
    {
        std::string haystack("schoolbus");
        std::string needle("bus");
        CHECK(aja::replace(haystack, needle, "") == "school");
        CHECK(haystack == "school");
        needle = "";
        CHECK(aja::replace(haystack, needle, "") == "school");
        CHECK(haystack == "school");
    }

    TEST_CASE("aja::stol")
    {
        std::size_t idx=0;
        int v = aja::stol("42",&idx,10);
        CHECK(v == 42);
        CHECK(idx == 2);
        int v2 = aja::stol("-42",&idx,10);
        CHECK(v2 == -42);
        CHECK(idx == 3);
        int v3 = aja::stol("0xF00D",&idx,16);
        CHECK(v3 == 61453);
        CHECK(idx == 6);
    }

    TEST_CASE("aja::stoul")
    {
        std::size_t idx=0;
        unsigned long v = aja::stoul("-42",&idx,10);
        CHECK(v == ULONG_MAX-41);
        CHECK(idx == 3);
        int v2 = aja::stoul("0xF00D",&idx,16);
        CHECK(v2 == 61453);
        CHECK(idx == 6);
    }

    TEST_CASE("aja::stod")
    {
        std::size_t idx=0;
        double v = aja::stod("-42",&idx);
        CHECK(v == -42);
        CHECK(idx == 3);
        double v2 = aja::stod("1e9",&idx);
        CHECK(v2 == 1000000000.000000);
        CHECK(idx == 3);
        double v3 = aja::stod("3.14",&idx);
        CHECK(v3 == 3.14);
        CHECK(idx == 4);
        double v4 = aja::stod("1e-9",&idx);
        CHECK(v4 == 0.000000001);
        CHECK(idx == 4);
    }

    TEST_CASE("aja::to_string variants")
    {
        double f1 = 23.43;
        double f2 = 1e-9;
        double f3 = 1e16;
        double f4 = 1e-40;
        double f5 = 123456789;
        double f6 = 3.1415926;
        int    f7 = 28;
        CHECK(aja::to_string(f1) == "23.430000");
        CHECK(aja::to_string(f2) == "0.000000");
        CHECK(aja::to_string(f3) == "10000000000000000.000000");
        CHECK(aja::to_string(f4) == "0.000000");
        CHECK(aja::to_string(f5) == "123456789.000000");
        CHECK(aja::to_string(f6) == "3.141593");
        CHECK(aja::to_string(f7) == "28");
        CHECK(aja::to_string(true) == "true");
        CHECK(aja::to_string(false) == "false");
    }

    TEST_CASE("aja::string_to_wstring & aja::wstring_to_string")
    {
        std::setlocale(LC_ALL, "en_US.utf8");

        std::string str, str2;
        std::wstring wstr, wstr2;

        str = "hello";
        CHECK(aja::string_to_wstring(str, wstr));
        CHECK(wstr == L"hello");

        str  = "";
        wstr = L"hello";
        CHECK(aja::wstring_to_string(wstr, str));
        CHECK(str == "hello");

#if !defined(AJA_WINDOWS)
        str  = "z\u00df\u6c34\U0001f34c";
        wstr = L"";
        CHECK(aja::string_to_wstring(str, wstr));
        CHECK(aja::wstring_to_string(wstr, str2));
        CHECK(str == str2);
#endif
        str  = "";
        wstr = L"a¥z";
        wstr2= L"";
        CHECK(aja::wstring_to_string(wstr, str));
        CHECK(aja::string_to_wstring(str, wstr2));
        CHECK(wstr == wstr2);

        str  = "";
        wstr = L"漢字";
        wstr2= L"";
        CHECK(aja::wstring_to_string(wstr, str));
        CHECK(aja::string_to_wstring(str, wstr2));
        CHECK(wstr == wstr2);
    }

    TEST_CASE("aja::string_to_cstring")
    {
        std::string str("hello");
        std::string strlong("hello there friend");
        const int c_str_len = 10;
        char c_str[c_str_len];
        CHECK_FALSE(aja::string_to_cstring(str, NULL, 100));
        CHECK_FALSE(aja::string_to_cstring(str, c_str, 0));
        CHECK(aja::string_to_cstring(str, c_str, c_str_len));
        CHECK(strcmp(c_str, "hello") == 0);
        CHECK(aja::string_to_cstring(strlong, c_str, c_str_len));
        CHECK(strcmp(c_str, "hello the") == 0);
    }

    TEST_CASE("aja::split")
    {
        std::string tosplit("Larry,Moe,Curly,Shemp");
        std::vector<std::string> results;
        SUBCASE("return style")
        {
            results = aja::split(tosplit, ',');
            CHECK(results.size() == 4);
            CHECK(results.at(0) == "Larry");
            CHECK(results.at(1) == "Moe");
            CHECK(results.at(2) == "Curly");
            CHECK(results.at(3) == "Shemp");
        }
        SUBCASE("parameter style")
        {
            aja::split(tosplit, ',', results);
            CHECK(results.size() == 4);
            CHECK(results.at(0) == "Larry");
            CHECK(results.at(1) == "Moe");
            CHECK(results.at(2) == "Curly");
            CHECK(results.at(3) == "Shemp");
        }
        SUBCASE("split not found")
        {
            aja::split(tosplit, '?', results);
            CHECK(results.size() == 1);
            CHECK(results.at(0) == tosplit);
        }
        SUBCASE("split at front")
        {
            aja::split(tosplit, 'L', results);
            CHECK(results.size() == 2);
            CHECK(results.at(0) == "");
            CHECK(results.at(1) == "arry,Moe,Curly,Shemp");
        }
        SUBCASE("split at end")
        {
            aja::split(tosplit, 'p', results);
            CHECK(results.size() == 2);
            CHECK(results.at(0) == "Larry,Moe,Curly,Shem");
            CHECK(results.at(1) == "");
        }
        SUBCASE("split all delims")
        {
            std::string alldelims = "???";
            aja::split(alldelims, '?', results);
            CHECK(results.size() == 4);
            CHECK(results.at(0) == "");
            CHECK(results.at(1) == "");
            CHECK(results.at(2) == "");
            CHECK(results.at(3) == "");
        }
    }

    TEST_CASE("aja::lower")
    {
        std::string str("This is a MiXeD case STRING!");
        CHECK(aja::lower(str) == "this is a mixed case string!");
    }

    TEST_CASE("aja::upper")
    {
        std::string str("This is a MiXeD case STRING!");
        CHECK(aja::upper(str) == "THIS IS A MIXED CASE STRING!");
    }

    TEST_CASE("aja::strip variants")
    {
        std::string test("  \tA test string\n");
        SUBCASE("lstrip")
        {
            CHECK(aja::lstrip(test) == "A test string\n");
        }
        SUBCASE("rstrip")
        {
            CHECK(aja::rstrip(test) == "  \tA test string");
        }
        SUBCASE("strip")
        {
            CHECK(aja::strip(test) == "A test string");
        }
    }

    TEST_CASE("aja::join")
    {
        std::vector<std::string> parts;
        parts.push_back("A");
        parts.push_back("B");
        parts.push_back("C");
        std::string joined = aja::join(parts, "-");
        CHECK(joined == "A-B-C");
    }

    TEST_CASE("aja::safer_strncpy")
    {
        const int maxSize = 32;
        char target[maxSize];
        const char* source = "The quick brown fox jumps over the lazy dog";

        char *retVal = aja::safer_strncpy(target, source, strlen(source), maxSize);
        CHECK(retVal == target);
        CHECK(strlen(target) == maxSize-1);
        CHECK(strcmp(target, "The quick brown fox jumps over ") == 0);

        char* target2 = NULL;
        char *retVal2 = aja::safer_strncpy(target2, source, 0, maxSize);
        CHECK(retVal2 == target2);

        char *retVal3 = aja::safer_strncpy(target2, source, 100, 0);
        CHECK(retVal3 == target2);

        const int maxSize2 = 8;
        char target3[maxSize2];
        strcpy(target3,"???????");
        CHECK(strcmp(target3, "???????") == 0);

        const char* source2 = "a dog!";
        aja::safer_strncpy(target3, source2, strlen(source2), maxSize2);
        CHECK(strcmp(target3, source2) == 0);
    }

} //common


void timebase_timecode_marker() {}
TEST_SUITE("timebase/timecode" * doctest::description("functions in ajabase/common/time[base|code].h")) {

    TEST_CASE("AJATimeBase")
    {
        std::vector<AJATimeBase> tb1;
        std::vector<AJATimeBase> tb2;

        tb1.push_back(AJATimeBase(15000, 1001));
        tb1.push_back(AJATimeBase(15000, 1000));
        tb1.push_back(AJATimeBase(18000, 1001));
        tb1.push_back(AJATimeBase(18000, 1000));
        tb1.push_back(AJATimeBase(19000, 1001));
        tb1.push_back(AJATimeBase(19000, 1000));
        tb1.push_back(AJATimeBase(24000, 1001));
        tb1.push_back(AJATimeBase(24000, 1000));
        tb1.push_back(AJATimeBase(25000, 1000));
        tb1.push_back(AJATimeBase(30000, 1001));
        tb1.push_back(AJATimeBase(30000, 1000));
        tb1.push_back(AJATimeBase(48000, 1001));
        tb1.push_back(AJATimeBase(48000, 1000));
        tb1.push_back(AJATimeBase(50000, 1000));
        tb1.push_back(AJATimeBase(60000, 1001));
        tb1.push_back(AJATimeBase(60000, 1000));
        tb1.push_back(AJATimeBase(100000, 1000));
        tb1.push_back(AJATimeBase(120000, 1001));
        tb1.push_back(AJATimeBase(120000, 1000));

        for(int i=AJA_FrameRate_Unknown+1;i<AJA_FrameRate_Size;i++)
        {
            tb2.push_back(AJATimeBase(AJA_FrameRate(i)));
        }

        REQUIRE(tb1.size() == tb2.size());

        for(size_t i=0;i<tb2.size();i++)
        {
            AJATimeBase a = tb1.at(i);
            AJATimeBase b = tb2.at(i);
            CHECK(a.GetFrameTimeScale()  == b.GetFrameTimeScale());
            CHECK(a.GetFrameDuration()   == b.GetFrameDuration());
            CHECK(a.GetAJAFrameRate()    == b.GetAJAFrameRate());
            CHECK(a.GetFramesPerSecond() == b.GetFramesPerSecond());
        }
    }

    TEST_CASE("AJATimeCode")
    {
        // frames to tc string
        uint32_t frames=150000;
        AJATimeCode tc(frames);
        std::string tmp;
        tc.QueryString(tmp, AJATimeBase(AJA_FrameRate_2398), false);
        CHECK(tmp == "01:44:10:00");
        tc.QueryString(tmp, AJATimeBase(AJA_FrameRate_2400), false);
        CHECK(tmp == "01:44:10:00");
        tc.QueryString(tmp, AJATimeBase(AJA_FrameRate_2500), false);
        CHECK(tmp == "01:40:00:00");
        tc.QueryString(tmp, AJATimeBase(AJA_FrameRate_2997), true);
        CHECK(tmp == "01:23:25;00");
        tc.QueryString(tmp, AJATimeBase(AJA_FrameRate_3000), false);
        CHECK(tmp == "01:23:20:00");
        tc.QueryString(tmp, AJATimeBase(AJA_FrameRate_5000), false);
        CHECK(tmp == "00:50:00:00");

        // test with std timecode flag set (the default)
        tc.QueryString(tmp, AJATimeBase(AJA_FrameRate_5994), true);
        CHECK(tmp == "00:41:42;14");
        // test with std timecode flag set for true 59.94 tc
        tc.SetStdTimecodeForHfr(false);
        tc.QueryString(tmp, AJATimeBase(AJA_FrameRate_5994), true);
        CHECK(tmp == "00:41:42;28");
        tc.QueryString(tmp, AJATimeBase(AJA_FrameRate_6000), false);
        CHECK(tmp == "00:41:40:00");

        // test the deprecated functionality
        char ctmp[12];
        tc.QueryString(ctmp, AJATimeBase(AJA_FrameRate_2400), false);
        CHECK(strcmp(ctmp, "01:44:10:00")==0);

        // tc string to frames
        std::vector<AJATimeCode> tcs;
        tcs.push_back(AJATimeCode("01:44:10:00", AJATimeBase(AJA_FrameRate_2398), false));
        tcs.push_back(AJATimeCode("01:44:10:00", AJATimeBase(AJA_FrameRate_2400), false));
        tcs.push_back(AJATimeCode("01:40:00:00", AJATimeBase(AJA_FrameRate_2500), false));
        tcs.push_back(AJATimeCode("01:23:25;00", AJATimeBase(AJA_FrameRate_2997), true));
        tcs.push_back(AJATimeCode("01:23:20:00", AJATimeBase(AJA_FrameRate_3000), false));
        tcs.push_back(AJATimeCode("00:50:00:00", AJATimeBase(AJA_FrameRate_5000), false));
        tcs.push_back(AJATimeCode("00:41:42;14", AJATimeBase(AJA_FrameRate_5994), true, true));
        tcs.push_back(AJATimeCode("00:41:42;28", AJATimeBase(AJA_FrameRate_5994), true, false));
        tcs.push_back(AJATimeCode("00:41:40:00", AJATimeBase(AJA_FrameRate_6000), false));

        std::vector<AJATimeCode>::iterator it = tcs.begin();
        while(it != tcs.end())
        {
            CHECK(it->QueryFrame() == frames);
            ++it;
        }

        // misc
        AJATimeCode tc2;
        tc2.SetWithCleanup("01:02:03:04 junk here", AJATimeBase(AJA_FrameRate_2400), false);
        tc2.QueryString(tmp, AJATimeBase(AJA_FrameRate_2400), false);
        CHECK(tmp == "01:02:03:04");
        tc2.SetWithCleanup("   01:02:03:04   ", AJATimeBase(AJA_FrameRate_2400), false);
        tc2.QueryString(tmp, AJATimeBase(AJA_FrameRate_2400), false);
        CHECK(tmp == "01:02:03:04");
        tc2.SetWithCleanup("   01 02 03 04   ", AJATimeBase(AJA_FrameRate_2400), false);
        tc2.QueryString(tmp, AJATimeBase(AJA_FrameRate_2400), false);
        CHECK(tmp == "01:02:03:04");
        tc2.SetWithCleanup("01-02-03-04 ", AJATimeBase(AJA_FrameRate_2400), false);
        tc2.QueryString(tmp, AJATimeBase(AJA_FrameRate_2400), false);
        CHECK(tmp == "01:02:03:04");
    }

} //timecode


void guid_marker() {}
TEST_SUITE("guid" * doctest::description("functions in ajabase/common/guid.h")) {

    TEST_CASE("CreateGuid")
    {
        std::string guid = CreateGuid();
        CHECK(guid.length() == 36);
        CHECK(guid != "00000000-0000-0000-0000-000000000000");
        std::vector<std::string> parts;
        aja::split(guid, '-', parts);
        CHECK(parts.size() == 5);
        CHECK(parts.at(0).length() == 8);
        CHECK(parts.at(1).length() == 4);
        CHECK(parts.at(2).length() == 4);
        CHECK(parts.at(3).length() == 4);
        CHECK(parts.at(4).length() == 12);
        std::string guidLowercase = guid;
        aja::lower(guidLowercase);
        CHECK(guidLowercase.find_first_not_of("0123456789abcdef-") == std::string::npos);

        std::string guid2 = CreateGuid();
        CHECK(guid != guid2);
    }

} //guid


void persistence_marker() {}
TEST_SUITE("persistence" * doctest::description("functions in ajabase/persistence/persistence.h")) {

    TEST_CASE("AJAPersistence")
    {
        //AJADebug::Open();

        std::string appID("com.aja.unittest.ajabase");
        std::string deviceType("");
        std::string deviceNumber("");
        bool        sharedPrefs = false;
        AJAPersistence p(appID, deviceType, deviceNumber, sharedPrefs);

        std::string appID2("com.aja.unittest.ajabase");
        std::string deviceType2("");
        std::string deviceNumber2("");
        bool        sharedPrefs2 = false;
        p.GetParams(appID2, deviceType2, deviceNumber2, sharedPrefs2);

        CHECK(appID == appID2);
        CHECK(deviceType == deviceType2);
        CHECK(deviceNumber == deviceNumber2);
        CHECK(sharedPrefs == sharedPrefs2);

        std::string keyName     = "";
        int			intValue    = 42;
        bool		trueValue   = true;
        bool		falseValue  = false;
        double		doubleValue = 3.14;
        std::string strValue    = "testing 1,2,3";
        char		blobValue[] = "blob test data";
        int			blobLen     = (int)strlen(blobValue);
        int			hierarchyValue1 = 17;
        int			hierarchyValue2 = 23;
        int			hierarchyValue3 = 27;

        std::string longStrValue = "some really long string to test if text stored as values are getting clipped in the persistence storage. like I said this is a long string testing the ability of sqlite to handle long strings of text. even though the column is set to 64 chars, mysql lets it grow to fit longer strings.";
        std::string orgLongStrValue = longStrValue;

        bool		isGood;

        //clear out any old values
        p.ClearPrefFile();

        // Write

        // int
        keyName = "UnitTestInt";
        p.SetValue(keyName, &intValue, AJAPersistenceTypeInt);
        intValue = 0;

        // bool
        keyName = "UnitTestBoolTrue";
        p.SetValue(keyName, &trueValue, AJAPersistenceTypeBool);
        trueValue = false;

        keyName = "UnitTestBoolFalse";
        p.SetValue(keyName, &falseValue, AJAPersistenceTypeBool);
        falseValue = true;

        // float
        keyName = "UnitTestDouble";
        p.SetValue(keyName, &doubleValue, AJAPersistenceTypeDouble);
        doubleValue = 0.0;

        // string
        keyName = "UnitTestString";
        p.SetValue(keyName, &strValue, AJAPersistenceTypeString);
        strValue = "";

        // blob
        keyName = "UnitTestBlob";
        p.SetValue(keyName, &blobValue, AJAPersistenceTypeBlob, blobLen);
        strcpy(blobValue,"");
        memset(blobValue,0,blobLen);

        // long string
        keyName = "UnitTestString (long)";
        p.SetValue(keyName, &longStrValue, AJAPersistenceTypeString);
        longStrValue = "";

        // write values to test hierarchical search
        keyName = "UnitTestHierarchyInt";
        p.SetParams(appID, "", "", sharedPrefs);
        p.SetValue(keyName, &hierarchyValue1, AJAPersistenceTypeInt);
        hierarchyValue1 = 0;

        keyName = "UnitTestHierarchyInt";
        p.SetParams(appID, "device 1", "123456", sharedPrefs);
        p.SetValue(keyName, &hierarchyValue2, AJAPersistenceTypeInt);
        hierarchyValue2 = 0;

        keyName = "UnitTestHierarchyInt";
        p.SetParams(appID, "device 2", "987654", sharedPrefs);
        p.SetValue(keyName, &hierarchyValue3, AJAPersistenceTypeInt);
        hierarchyValue3 = 0;

        // multiple
        keyName = "UnitTestMultipleInt_1";
        intValue = 10;
        p.SetValue(keyName, &intValue, AJAPersistenceTypeInt);

        keyName = "UnitTestMultipleInt_2";
        intValue = 20;
        p.SetValue(keyName, &intValue, AJAPersistenceTypeInt);

        keyName = "UnitTestMultipleInt_3";
        intValue = 30;
        p.SetValue(keyName, &intValue, AJAPersistenceTypeInt);
        intValue = 0;

        keyName = "UnitTestMultipleBool_true";
        trueValue = true;
        p.SetValue(keyName, &trueValue, AJAPersistenceTypeBool);
        trueValue = false;

        keyName = "UnitTestMultipleBool_false";
        falseValue = false;
        p.SetValue(keyName, &falseValue, AJAPersistenceTypeBool);
        falseValue = true;

        keyName = "UnitTestMultipleDouble_e";
        doubleValue = 2.71;
        p.SetValue(keyName, &doubleValue, AJAPersistenceTypeDouble);

        keyName = "UnitTestMultipleDouble_pi";
        doubleValue = 3.14;
        p.SetValue(keyName, &doubleValue, AJAPersistenceTypeDouble);
        doubleValue = 0;

        // Read
        p.SetParams(appID, "", "", sharedPrefs);

        // int
        keyName = "UnitTestInt";
        isGood = p.GetValue(keyName, &intValue, AJAPersistenceTypeInt);

        // bool
        keyName = "UnitTestBoolTrue";
        isGood = p.GetValue(keyName, &trueValue, AJAPersistenceTypeBool);

        keyName = "UnitTestBoolFalse";
        isGood = p.GetValue(keyName, &falseValue, AJAPersistenceTypeBool);

        // float
        keyName = "UnitTestDouble";
        isGood = p.GetValue(keyName, &doubleValue, AJAPersistenceTypeDouble);

        // string
        keyName = "UnitTestString";
        isGood = p.GetValue(keyName, &strValue, AJAPersistenceTypeString);

        // blob
        keyName = "UnitTestBlob";
        isGood = p.GetValue(keyName, &blobValue, AJAPersistenceTypeBlob, blobLen);

        // long string
        keyName = "UnitTestString (long)";
        isGood = p.GetValue(keyName, &longStrValue, AJAPersistenceTypeString);

        // test hierarchical search
        keyName = "UnitTestHierarchyInt";
        //both not in db
        p.SetParams(appID, "device 3", "424242", sharedPrefs);
        isGood = p.GetValue(keyName, &hierarchyValue1, AJAPersistenceTypeInt);

        // device in db, serial not in db
        p.SetParams(appID, "device 1", "171717", sharedPrefs);
        isGood = p.GetValue(keyName, &hierarchyValue2, AJAPersistenceTypeInt);

        //both in db
        p.SetParams(appID, "device 2", "987654", sharedPrefs);
        isGood = p.GetValue(keyName, &hierarchyValue3, AJAPersistenceTypeInt);

        //multiple
        //the extra '_' is used in SQL syntax as a 1 character wildcard
        keyName = "UnitTestMultipleInt__";
        std::vector<std::string> intKeys;
        std::vector<int> intValues;
        isGood = p.GetValuesInt(keyName, intKeys, intValues);

        //the extra '%' is used in SQL syntax as a character sequence wildcard
        keyName = "UnitTestMultipleBool_%";
        std::vector<std::string> boolKeys;
        std::vector<bool> boolValues;
        isGood = p.GetValuesBool(keyName, boolKeys, boolValues);

        keyName = "UnitTestMultipleDouble_%";
        std::vector<std::string> doubleKeys;
        std::vector<double> doubleValues;
        isGood = p.GetValuesDouble(keyName, doubleKeys, doubleValues);

        CHECK(isGood);
        CHECK(intValue == 42);
        CHECK(trueValue == true);
        CHECK(falseValue == false);
        CHECK(doubleValue == 3.14);
        CHECK(strValue == "testing 1,2,3");
        CHECK(strcmp(blobValue,"blob test data")==0);
        CHECK(longStrValue == orgLongStrValue);
        // as of 12/06/2017, AJAPersistence was changed to not default to
        // matching any device if exact or partial match not found
        // if it is ever changed back hierarchyValue1 should be 17
        CHECK(hierarchyValue1 == 0);
        CHECK(hierarchyValue2 == 23);
        CHECK(hierarchyValue3 == 27);

        CHECK(intKeys.size() == 3);
        CHECK(intValues.size() == 3);
        CHECK(intKeys.at(0) == "UnitTestMultipleInt_1");
        CHECK(intKeys.at(1) == "UnitTestMultipleInt_2");
        CHECK(intKeys.at(2) == "UnitTestMultipleInt_3");
        CHECK(intValues.at(0) == 10);
        CHECK(intValues.at(1) == 20);
        CHECK(intValues.at(2) == 30);

        CHECK(boolKeys.size() == 2);
        CHECK(boolValues.size() == 2);
        CHECK(boolKeys.at(0) == "UnitTestMultipleBool_true");
        CHECK(boolKeys.at(1) == "UnitTestMultipleBool_false");
        CHECK(boolValues.at(0) == true);
        CHECK(boolValues.at(1) == false);

        CHECK(doubleKeys.size() == 2);
        CHECK(doubleValues.size() == 2);
        CHECK(doubleKeys.at(0) == "UnitTestMultipleDouble_e");
        CHECK(doubleKeys.at(1) == "UnitTestMultipleDouble_pi");
        CHECK(doubleValues.at(0) == 2.71);
        CHECK(doubleValues.at(1) == 3.14);
    }

} //persistence

void atomic_marker() {}
TEST_SUITE("atomic" * doctest::description("functions in ajabase/system/atomic.h")) {

    TEST_CASE("Increment")
    {
        int32_t aInt32 = 0;
        int64_t aInt64 = 0;
        uint32_t aUInt32 = 0;
        uint64_t aUInt64 = 0;

        CHECK(AJAAtomic::Increment(&aInt32) == 1);
        CHECK(aInt32 == 1);
        CHECK(AJAAtomic::Increment(&aInt64) == 1);
        CHECK(aInt64 == 1);
        CHECK(AJAAtomic::Increment(&aUInt32) == 1);
        CHECK(aUInt32 == 1);
        CHECK(AJAAtomic::Increment(&aUInt64) == 1);
        CHECK(aUInt64 == 1);
    }

    TEST_CASE("Decrement")
    {
        int32_t aInt32 = 1;
        int64_t aInt64 = 1;
        uint32_t aUInt32 = 1;
        uint64_t aUInt64 = 1;

        CHECK(AJAAtomic::Decrement(&aInt32) == 0);
        CHECK(aInt32 == 0);
        CHECK(AJAAtomic::Decrement(&aInt64) == 0);
        CHECK(aInt64 == 0);
        CHECK(AJAAtomic::Decrement(&aUInt32) == 0);
        CHECK(aUInt32 == 0);
        CHECK(AJAAtomic::Decrement(&aUInt64) == 0);
        CHECK(aUInt64 == 0);
    }

} //atomic

void time_marker() {}
TEST_SUITE("time" * doctest::description("functions in ajabase/system/systemtime.h")) {

    TEST_CASE("AJATime")
    {
#ifdef AJA_WINDOWS
		TIMECAPS tc;
		if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) == MMSYSERR_NOERROR)
		{
			// Attempt to set the timer resolution to 1 millisecond
			uint32_t eventPeriod = std::min<uint32_t>(std::max<uint32_t>(tc.wPeriodMin, 1), tc.wPeriodMax);
			timeBeginPeriod(eventPeriod);
		}
#endif

        uint64_t startMs = AJATime::GetSystemMilliseconds();
        uint64_t startUs = AJATime::GetSystemMicroseconds();
        uint64_t startNs = AJATime::GetSystemNanoseconds();
        AJATime::SleepInMicroseconds(100 * 1000); // 1/10th of a second
        uint64_t endMs = AJATime::GetSystemMilliseconds();
        uint64_t endUs = AJATime::GetSystemMicroseconds();
        uint64_t endNs = AJATime::GetSystemNanoseconds();

        uint64_t deltaMs = endMs - startMs;
        uint64_t deltaUs = endUs - startUs;
        uint64_t deltaNs = endNs - startNs;

        // There could be variablitiy in the sleep call, so make sure in range
        // check to make sure the units are correct
        CHECK(deltaMs > 90);
        CHECK(deltaMs < 150);
        CHECK(deltaUs > 90000);
        CHECK(deltaUs < 150000);
        CHECK(deltaNs > 90000000);
        CHECK(deltaNs < 150000000);
    }

} //time

void performance_marker() {}
TEST_SUITE("performance" * doctest::description("functions in ajabase/common/performance.h")) {

    TEST_CASE("AJAPerformance")
    {
        AJAPerformance p("unit_test", AJATimerPrecisionMilliseconds);
        p.Start();
        AJATime::SleepInMicroseconds(100 * 1000);
        p.Stop();
        p.Start();
        AJATime::SleepInMicroseconds(200 * 1000);
        p.Stop();
        p.Start();
        AJATime::SleepInMicroseconds(300 * 1000);
        p.Stop();

        CHECK(p.Entries() == 3);
        CHECK(p.MinTime() != 0);
        CHECK(p.MaxTime() != 0);
        // There could be variablitiy in the sleep call, so make sure in range
        CHECK(p.StandardDeviation() > 94);
        CHECK(p.StandardDeviation() < 110);

        AJAPerformance p2("unit_test_empty", AJATimerPrecisionMilliseconds);
        CHECK(p2.Entries() == 0);
        CHECK(p2.MinTime() != 0);
        CHECK(p2.MaxTime() == 0);
        CHECK(p2.Mean() == 0.0);
        CHECK(p2.StandardDeviation() == 0.0);
    }

} //performance

void info_marker() {}
TEST_SUITE("info" * doctest::description("functions in ajabase/system/info.h")) {

    TEST_CASE("AJASystemInfo")
    {
        // just create an instance to make sure everything works at runtime
        AJASystemInfo i;
    }

} //info

void file_marker() {}
TEST_SUITE("file" * doctest::description("functions in ajabase/system/file_io.h")) {

	TEST_CASE("AJAFileIO")
	{
		std::string tempDir;
		AJAStatus status = AJAFileIO::TempDirectory(tempDir);
		WARN_MESSAGE(status == AJA_STATUS_SUCCESS, "TempDirectory() could not find standard temp dir, trying secondary location.");

		if (status != AJA_STATUS_SUCCESS)
		{
			tempDir = "ajafileio_tmp_dir";
#if defined(AJA_WINDOWS)
			_mkdir(tempDir.c_str());
#else
			if (mkdir(tempDir.c_str(), ACCESSPERMS) == 0)
			{
				chmod(tempDir.c_str(), ACCESSPERMS);
			}
#endif
		}

		status = AJAFileIO::DoesDirectoryExist(tempDir);
		CHECK_MESSAGE(status == AJA_STATUS_SUCCESS, "expected: temp dir '" + tempDir + "' to exist");

#if defined(AJA_WINDOWS)
		std::string pathsep = "\\";
#else
		std::string pathsep = "/";
#endif

		std::string tempFilePath = tempDir;

		// remove any trailing path separator on tempDir
		// then add the pathsep, simple way to sanitize the created path
		aja::rstrip(tempFilePath, pathsep);
		tempFilePath += pathsep;
		tempFilePath += "AJAFileIO_unittest_file_";
		tempFilePath += aja::to_string((unsigned long)AJATime::GetSystemMilliseconds()) + ".txt";

		AJAFileIO file;
		status = file.Open(tempFilePath, eAJAReadWrite|eAJACreateAlways, 0);
		REQUIRE_MESSAGE(status == AJA_STATUS_SUCCESS, "expected: file '" + tempFilePath + "' to be created");
		CHECK(file.IsOpen() == true);
		CHECK(file.Tell() == 0);
		// seek forward 64
		status = file.Seek(64, eAJASeekSet);
		CHECK(status == AJA_STATUS_SUCCESS);
		CHECK(file.Tell() == 64);
		// seek backward 32
		status = file.Seek(-32, eAJASeekCurrent);
		CHECK(status == AJA_STATUS_SUCCESS);
		CHECK(file.Tell() == 32);
		status = file.Truncate(16);
		CHECK(status == AJA_STATUS_SUCCESS);
		// truncate should not change offset
		CHECK(file.Tell() == 32);
		status = file.Sync();
		CHECK(status == AJA_STATUS_SUCCESS);
		int64_t createTime;
		int64_t modTime;
		int64_t size;
		status = file.FileInfo(createTime, modTime, size);
		CHECK(status == AJA_STATUS_SUCCESS);
		std::string fullFilePath;
		status = file.FileInfo(createTime, modTime, size, fullFilePath);
		CHECK(status == AJA_STATUS_SUCCESS);
		// Checking this way because on Mac it returns /private/var/folders... for fullFilePath
		// while tempFilePath is /var/folders...
		// var is a symbolic link to /private/var
		CHECK(fullFilePath.find(tempFilePath) != std::string::npos);
		// size should equal the truncate
		CHECK(size == 16);
		status = file.Seek(0, eAJASeekSet);
		CHECK(status == AJA_STATUS_SUCCESS);
		CHECK(file.Write("test") == 4);
		status = file.Truncate(4);
		CHECK(status == AJA_STATUS_SUCCESS);
		status = file.Close();
		CHECK(status == AJA_STATUS_SUCCESS);

		AJAFileIO fileRead;
		status = fileRead.Open(tempFilePath, eAJAReadOnly, 0);
		CHECK(status == AJA_STATUS_SUCCESS);
		status = fileRead.FileInfo(createTime, modTime, size);
		CHECK(status == AJA_STATUS_SUCCESS);
		std::vector<uint8_t> tmp;
		tmp.resize(size);
		CHECK(fileRead.Read((uint8_t*)&tmp[0], (uint32_t)size) == size);
		status = fileRead.Seek(0, eAJASeekSet);
		CHECK(status == AJA_STATUS_SUCCESS);
		std::string fileContents;
		CHECK(fileRead.Read(fileContents, 4) == 4);
		status = fileRead.Close();
		CHECK(status == AJA_STATUS_SUCCESS);

		// should not be empty
		status = AJAFileIO::IsDirectoryEmpty(tempDir);
		CHECK(status != AJA_STATUS_SUCCESS);

		// should be atleast one from above test
		status = AJAFileIO::DoesDirectoryContain(tempDir, "*");
		CHECK(status == AJA_STATUS_SUCCESS);

		// should not exist
		status = AJAFileIO::DoesDirectoryContain(tempDir, "*.someBonkersExt");
		CHECK(status != AJA_STATUS_SUCCESS);

		CHECK(AJAFileIO::FileExists(tempFilePath));
		status = AJAFileIO::Delete(tempFilePath);
		CHECK(status == AJA_STATUS_SUCCESS);
		CHECK(AJAFileIO::FileExists(tempFilePath) == false);
	}

} //file