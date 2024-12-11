/* SPDX-License-Identifier: MIT */
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

#include "ajabase/common/bytestream.h"
#include "ajabase/common/commandline.h"
#include "ajabase/common/common.h"
#include "ajabase/common/guid.h"
#include "ajabase/common/performance.h"
#include "ajabase/common/timebase.h"
#include "ajabase/common/timecode.h"
#include "ajabase/common/timer.h"
#include "ajabase/common/ajamovingavg.h"
#include "ajabase/persistence/persistence.h"
#include "ajabase/system/atomic.h"
#include "ajabase/system/file_io.h"
#include "ajabase/system/info.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/thread.h"

#include <algorithm>
#include <clocale>
#include <iostream>
#include <limits>
#include <string.h>

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
	TEST_CASE("aja::starts_with")
	{
		CHECK_EQ(aja::starts_with("abc", ""),  true);
		CHECK_EQ(aja::starts_with("abc", "a"), true);
		CHECK_EQ(aja::starts_with("abc", 'a'), true);
		CHECK_EQ(aja::starts_with("abc", "ab"), true);
		CHECK_EQ(aja::starts_with("abc", "bc"), false);
		CHECK_EQ(aja::starts_with(L"abc", L""),  true);
		CHECK_EQ(aja::starts_with(L"abc", L"a"), true);
		CHECK_EQ(aja::starts_with(L"abc", L'a'), true);
		CHECK_EQ(aja::starts_with(L"abc", L"ab"), true);
		CHECK_EQ(aja::starts_with(L"abc", L"bc"), false);
	}
	TEST_CASE("aja::ends_with")
	{
		CHECK_EQ(aja::ends_with("abc", ""),  true);
		CHECK_EQ(aja::ends_with("abc", "c"), true);
		CHECK_EQ(aja::ends_with("abc", 'c'), true);
		CHECK_EQ(aja::ends_with("abc", "bc"), true);
		CHECK_EQ(aja::ends_with("abc", "bcd"), false);
		CHECK_EQ(aja::ends_with(L"abc", L""),  true);
		CHECK_EQ(aja::ends_with(L"abc", L"c"), true);
		CHECK_EQ(aja::ends_with(L"abc", L'c'), true);
		CHECK_EQ(aja::ends_with(L"abc", L"bc"), true);
		CHECK_EQ(aja::ends_with(L"abc", L"bcd"), false);
	}
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
		int	   f7 = 28;
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

		str	 = "";
		wstr = L"hello";
		CHECK(aja::wstring_to_string(wstr, str));
		CHECK(str == "hello");

#if !defined(AJA_WINDOWS)
		str	 = "z\u00df\u6c34\U0001f34c";
		wstr = L"";
		CHECK(aja::string_to_wstring(str, wstr));
		CHECK(aja::wstring_to_string(wstr, str2));
		CHECK(str == str2);
#endif
		str	 = "";
		wstr = L"a¥z";
		wstr2= L"";
		CHECK(aja::wstring_to_string(wstr, str));
		CHECK(aja::string_to_wstring(str, wstr2));
		CHECK(wstr == wstr2);

		str	 = "";
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
		const std::string urlParam("foo=bar&two=2");
		std::vector<std::string> results;
		SUBCASE("return style (ch)")
		{
			results = aja::split(tosplit, ',');
			CHECK(results.size() == 4);
			CHECK(results.at(0) == "Larry");
			CHECK(results.at(1) == "Moe");
			CHECK(results.at(2) == "Curly");
			CHECK(results.at(3) == "Shemp");
		}
		SUBCASE("parameter style (ch)")
		{
			aja::split(tosplit, ',', results);
			CHECK(results.size() == 4);
			CHECK(results.at(0) == "Larry");
			CHECK(results.at(1) == "Moe");
			CHECK(results.at(2) == "Curly");
			CHECK(results.at(3) == "Shemp");
		}
		SUBCASE("split not found (ch)")
		{
			aja::split(tosplit, '?', results);
			CHECK(results.size() == 1);
			CHECK(results.at(0) == tosplit);
		}
		SUBCASE("split at front (ch)")
		{
			aja::split(tosplit, 'L', results);
			CHECK(results.size() == 2);
			CHECK(results.at(0) == "");
			CHECK(results.at(1) == "arry,Moe,Curly,Shemp");
		}
		SUBCASE("split at end (ch)")
		{
			aja::split(tosplit, 'p', results);
			CHECK(results.size() == 2);
			CHECK(results.at(0) == "Larry,Moe,Curly,Shem");
			CHECK(results.at(1) == "");
		}
		SUBCASE("split all delims (ch)")
		{
			std::string alldelims = "???";
			aja::split(alldelims, '?', results);
			CHECK(results.size() == 4);
			CHECK(results.at(0) == "");
			CHECK(results.at(1) == "");
			CHECK(results.at(2) == "");
			CHECK(results.at(3) == "");
		}

		SUBCASE("return style (str)")
		{
			results = aja::split(tosplit, ",");
			CHECK(results.size() == 4);
			CHECK(results.at(0) == "Larry");
			CHECK(results.at(1) == "Moe");
			CHECK(results.at(2) == "Curly");
			CHECK(results.at(3) == "Shemp");
		}
		SUBCASE("return style 2 (str)")
		{
			results = aja::split(tosplit, ",");
			CHECK(results.size() == 4);
			CHECK(results.at(0) == "Larry");
			CHECK(results.at(1) == "Moe");
			CHECK(results.at(2) == "Curly");
			CHECK(results.at(3) == "Shemp");
		}
		SUBCASE("split not found (str)")
		{
			results = aja::split(tosplit, "?");
			CHECK(results.size() == 1);
			CHECK(results.at(0) == tosplit);
		}
		SUBCASE("split at front (str)")
		{
			results = aja::split(tosplit, "L");
			CHECK(results.size() == 2);
			CHECK(results.at(0) == "");
			CHECK(results.at(1) == "arry,Moe,Curly,Shemp");
		}
		SUBCASE("split at end (str)")
		{
			results = aja::split(tosplit, "p");
			CHECK(results.size() == 2);
			CHECK(results.at(0) == "Larry,Moe,Curly,Shem");
			CHECK(results.at(1) == "");
		}
		SUBCASE("split all delims (str)")
		{
			std::string alldelims = "???";
			results = aja::split(alldelims, "?");
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

void commandline_marker() {}
TEST_SUITE("commandline" * doctest::description("function in ajabase/common/commandline.h")) {
	TEST_CASE("AJACommandLineParser Constructor")
	{
		AJACommandLineParser parser;
		CHECK(parser.GetName().empty() == true);
		AJACommandLineParser parser2("foobar");
		CHECK(parser2.GetName() == "foobar");
	}
	TEST_CASE("AJACommandLineParser AddOption")
	{
		AJACommandLineParser parser;
		CHECK_EQ(parser.AddOption(AJACommandLineOption(AJAStringList{"f", "foo"}, "Tweak the foo")), true);
		CHECK_EQ(parser.AddOption(AJACommandLineOption(AJAStringList{"F", "foo"}, "Tweak the foo")), false);
		CHECK_EQ(parser.AddOption(AJACommandLineOption(AJAStringList{"F", "fabulous"}, "Tweak the fabulous")), true);
		CHECK_EQ(parser.AddOption(AJACommandLineOption(AJAStringList{"f", "foobar"}, "Tweak the foobar")), false);
		CHECK_EQ(parser.AddOption(AJACommandLineOption(AJAStringList{"b", "bar"}, "Tweak the bar")), true);
		CHECK_EQ(parser.AddOption(AJACommandLineOption(AJAStringList{"z", "baz"}, "Tweak the baz")), true);
		CHECK_EQ(parser.AddOption(AJACommandLineOption(AJAStringList{"b", "boom"}, "Tweak the boom")), false);
	}
	TEST_CASE("AJACommandLineParser OptionByName")
	{
		AJACommandLineParser parser;
		parser.AddOption(AJACommandLineOption(AJAStringList{"f", "foo"}, "Tweak the foo"));
		parser.AddOption(AJACommandLineOption(AJAStringList{"F", "fabulous"}, "Tweak the fabulous"));
		parser.AddOption(AJACommandLineOption(AJAStringList{"b", "bar"}, "Tweak the bar"));
		parser.AddOption(AJACommandLineOption(AJAStringList{"z", "baz"}, "Tweak the baz"));
		AJACommandLineOption opt;
		parser.OptionByName("fabulous", opt);
		AJAStringList optNames = opt.GetNames();
		CHECK_EQ(optNames.size(), 2);
		CHECK_EQ(optNames[0], "F");
		CHECK_EQ(optNames[1], "fabulous");
	}
	TEST_CASE("AJACommandLineParser ParseArgs kShortOptionsDefault")
	{
		AJACommandLineParser parser;
		parser.AddOption(AJACommandLineOption(AJAStringList{"f", "foo"}, "Tweak the foo"));
		parser.AddOption(AJACommandLineOption(AJAStringList{"b", "bar"}, "Tweak the bar"));
		parser.AddOption(AJACommandLineOption(AJAStringList{"z", "baz"}, "Tweak the baz"));
		parser.AddOption(AJACommandLineOption(AJAStringList{"q", "qux"}, "Tweak the qux"));
		AJAStringList args;
		args.push_back("ut_ajabase"); // skipped by the parser
		args.push_back("-fnz");
		args.push_back("-b23");
		args.push_back("-q");
		args.push_back("\"AJA Video Systems\"");
		parser.ParseArgs(args);
		CHECK_EQ(parser.IsSet("foo"), true);
		CHECK_EQ(parser.IsSet("bar"), true);
		CHECK_EQ(parser.IsSet("baz"), false);
		CHECK_EQ(parser.Value("foo").AsString(), "nz");
		CHECK_EQ(parser.Value("bar").AsInt32(), 23);
		CHECK_EQ(parser.Value("qux").AsString(), "\"AJA Video Systems\"");
	}
	TEST_CASE("AJACommandLineParser ParseArgs kShortOptionsAsLong")
	{
		AJACommandLineParser parser(kShortOptionsAsLong);
		parser.AddOption(AJACommandLineOption(AJAStringList{"f", "foo"}, "Tweak the foo"));
		parser.AddOption(AJACommandLineOption(AJAStringList{"b", "bar"}, "Tweak the bar"));
		parser.AddOption(AJACommandLineOption(AJAStringList{"z", "baz"}, "Tweak the baz"));
		AJAStringList args;
		args.push_back("ut_ajabase"); // skipped by the parser
		args.push_back("-fnz");
		parser.ParseArgs(args);
		CHECK_EQ(parser.IsSet("foo"), true);
		CHECK_EQ(parser.IsSet("bar"), false);
		CHECK_EQ(parser.IsSet("baz"), true);
		parser.Reset();
		args.clear();
		args.push_back("ut_ajabase"); // skipped by the parser
		args.push_back("-fubz");
		parser.ParseArgs(args);
		CHECK_EQ(parser.IsSet("foo"), true);
		CHECK_EQ(parser.IsSet("bar"), true);
		CHECK_EQ(parser.IsSet("baz"), true);
		CHECK_EQ(parser.IsSet("u"), false);
	}
	TEST_CASE("AJACommandLineParser ParseArgs Long")
	{
		AJACommandLineParser parser;
		parser.AddOption(AJACommandLineOption(AJAStringList{"f", "foo"}, "Tweak the foo"));
		parser.AddOption(AJACommandLineOption(AJAStringList{"b", "bar"}, "Tweak the bar"));
		parser.AddOption(AJACommandLineOption(AJAStringList{"z", "baz"}, "Tweak the baz"));
		AJAStringList args;
		args.push_back("ut_ajabase"); // skipped by the parser
		args.push_back("--foo=123");
		args.push_back("--bar");
		args.push_back("42");
		parser.ParseArgs(args);
		CHECK_EQ(parser.IsSet("foo"), true);
		CHECK_EQ(parser.IsSet("bar"), true);
		CHECK_EQ(parser.IsSet("baz"), false);
		CHECK_EQ(parser.Value("foo").AsInt32(), 123);
		CHECK_EQ(parser.Value("bar").AsInt32(), 42);
	}
	TEST_CASE("AJACommandLineParser Subparser")
	{
		AJACommandLineParser subParser("sub");
		subParser.AddOption(AJACommandLineOption(AJAStringList{"q", "quack"}, "sub quack"));
		subParser.AddOption(AJACommandLineOption(AJAStringList{"u", "update"}, "sub update"));
		subParser.AddOption(AJACommandLineOption(AJAStringList{"x", "jinx"}, "sub jinx"));
		AJACommandLineParser subParser2("sub2");
		subParser2.AddOption(AJACommandLineOption(AJAStringList{"s", "stop"}, "sub2 stop"));
		subParser2.AddOption(AJACommandLineOption(AJAStringList{"g", "go"}, "sub2 go"));
		AJACommandLineParser parser;
		parser.AddSubParser(&subParser);
		parser.AddSubParser(&subParser2);
		parser.AddOption(AJACommandLineOption(AJAStringList{"f", "foo"}, "Tweak the foo"));
		parser.AddOption(AJACommandLineOption(AJAStringList{"b", "bar"}, "Tweak the bar"));
		parser.AddOption(AJACommandLineOption(AJAStringList{"z", "baz"}, "Tweak the baz"));
		AJAStringList args;
		args.push_back("ut_ajabase"); // skipped by the parser
		args.push_back("--foo=123");
		args.push_back("--bar=42");
		args.push_back("--baz=999");
		parser.ParseArgs(args);
		CHECK_EQ(parser.Value("foo").AsInt32(), 123);
		CHECK_EQ(parser.Value("bar").AsInt32(), 42);
		CHECK_EQ(parser.Value("baz").AsInt32(), 999);
		args.clear();
		args.push_back("ut_ajabase"); // skipped by the parser
		args.push_back("sub");
		args.push_back("-qa");
		args.push_back("-ub");
		args.push_back("-xc");
		parser.ParseArgs(args);
		CHECK_EQ(parser.Value("quack").AsString(), "a");
		CHECK_EQ(parser.Value("update").AsString(), "b");
		CHECK_EQ(parser.Value("jinx").AsString(), "c");
		args.clear();
		args.push_back("ut_ajabase"); // skipped by the parser
		args.push_back("sub2");
		args.push_back("--go");
		parser.ParseArgs(args);
		CHECK_EQ(parser.IsSet("go"), true);
		CHECK_EQ(parser.IsSet("stop"), false);
		CHECK_EQ(parser.IsSet("foo"), false);
		CHECK_EQ(parser.IsSet("bar"), false);
		CHECK_EQ(parser.IsSet("baz"), false);
	}
	TEST_CASE("AJACommandLineParser C++98 compliant")
	{
		AJACommandLineParser parser;
		AJACommandLineOption opt1("foo", "foo arg", "0");
		CHECK_EQ(parser.AddOption(opt1), true);
		AJACommandLineOption opt2("foo", "foo arg", "0");
		CHECK_EQ(parser.AddOption(opt2), false);
		AJACommandLineOption opt3("foobar", "foobar arg", "0");
		CHECK_EQ(parser.AddOption(opt3), true);
		AJACommandLineOption abc("abc", "abc arg", "0");
		AJACommandLineOption abd("abd", "abd arg", "0");
		AJACommandLineOption abcdef("abcdef", "abcdef arg", "0");
		CHECK_EQ(parser.AddOption(abc), true);
		CHECK_EQ(parser.AddOption(abcdef), true);
		CHECK_EQ(parser.AddOption(abd), true);
		AJAStringList args;
		args.push_back("appname.exe");
		args.push_back("--foo=42");
		args.push_back("--abc=123");
		args.push_back("--abcdef=456");
		args.push_back("--abcdefg=600");
		parser.ParseArgs(args);
		CHECK_EQ(parser.Value("foo").AsUInt32(), 42);
		CHECK_EQ(parser.Value("abc").AsUInt32(), 123);
		CHECK_EQ(parser.Value("abcdef").AsUInt32(), 456);
		CHECK_EQ(parser.Value("abcdefg").AsUInt32(), 0);
		parser.Reset(); // clear parsed args and values
		args.clear();
		args.push_back("appname.exe");
		args.push_back("-foo=42");
		args.push_back("-abc=123");
		args.push_back("-abcdef=456");
		args.push_back("-abcdefg=600");
		CHECK_EQ(parser.Value("foo").AsUInt32(), 42);
		CHECK_EQ(parser.Value("abc").AsUInt32(), 123);
		CHECK_EQ(parser.Value("abcdef").AsUInt32(), 456);
		CHECK_EQ(parser.Value("abcdefg").AsUInt32(), 0);
		parser.Reset(true); // clear everything
		args.clear();
		args.push_back("appname.exe");
		args.push_back("-f");
		args.push_back("99");
		args.push_back("-a"),
		args.push_back("hello");
		args.push_back("-r3.14159");
		opt1.AddName("f");
		opt2.AddName("o");
		opt3.AddName("r");
		abc.AddName("a");
		CHECK_EQ(parser.AddOption(opt1), true);
		CHECK_EQ(parser.AddOption(opt2), false);
		CHECK_EQ(parser.AddOption(opt3), true);
		CHECK_EQ(parser.AddOption(abc), true);
		parser.ParseArgs(args);
		CHECK_EQ(parser.Value("foo").AsUInt32(), 99);
		CHECK_EQ(parser.Value("abc").AsString(), "hello");
		CHECK_EQ(parser.Value("foobar").AsFloat(), doctest::Approx(3.14159f));
	}
}

void variant_marker() {}
TEST_SUITE("variant" * doctest::description("functions in ajabase/common/variant.h")) {
	TEST_CASE("constructors")
	{
		{ /* AJAVariantType ctor*/
			AJAVariant v1(AJA_VARIANT_NONE);
			CHECK_EQ(v1.AsInt32(), 0);
			AJAVariant v2(AJA_VARIANT_BOOL);
			CHECK_EQ(v2.AsInt32(), 0);
			AJAVariant v3(AJA_VARIANT_FLOAT);
			CHECK_EQ(v3.AsInt32(), 0);
			AJAVariant v4(AJA_VARIANT_DOUBLE);
			CHECK_EQ(v4.AsInt32(), 0);
			AJAVariant v5(AJA_VARIANT_INT8);
			CHECK_EQ(v5.AsInt32(), 0);
			AJAVariant v6(AJA_VARIANT_UINT8);
			CHECK_EQ(v6.AsInt32(), 0);
			AJAVariant v7(AJA_VARIANT_INT16);
			CHECK_EQ(v7.AsInt32(), 0);
			AJAVariant v8(AJA_VARIANT_UINT16);
			CHECK_EQ(v8.AsInt32(), 0);
			AJAVariant v9(AJA_VARIANT_INT32);
			CHECK_EQ(v9.AsInt32(), 0);
			AJAVariant v10(AJA_VARIANT_UINT32);
			CHECK_EQ(v10.AsInt32(), 0);
			AJAVariant v11(AJA_VARIANT_INT64);
			CHECK_EQ(v11.AsInt32(), 0);
			AJAVariant v12(AJA_VARIANT_UINT64);
			CHECK_EQ(v12.AsInt32(), 0);
			AJAVariant v13(AJA_VARIANT_STRING);
			CHECK_EQ(v13.AsInt32(), 0);
		}

		{ /* bool ctor */
			AJAVariant v1(true);
			CHECK_EQ(v1.AsBool(), true);
			CHECK_EQ(v1.AsFloat(), 1.0f);
			CHECK_EQ(v1.AsDouble(), 1.0);
			CHECK_EQ(v1.AsInt8(), 1);
			CHECK_EQ(v1.AsUInt8(), 1);
			CHECK_EQ(v1.AsInt16(), 1);
			CHECK_EQ(v1.AsUInt16(), 1);
			CHECK_EQ(v1.AsInt32(), 1);
			CHECK_EQ(v1.AsUInt32(), 1);
			CHECK_EQ(v1.AsInt64(), 1);
			CHECK_EQ(v1.AsUInt64(), 1);
			CHECK_EQ(v1.AsString(), "true");
		}

		{ /* float ctor */
			AJAVariant v1(3.14159f);
			CHECK_EQ(v1.AsBool(), true);
			CHECK_EQ(v1.AsFloat(), doctest::Approx(3.14159f));
			CHECK_EQ(v1.AsDouble(), doctest::Approx(3.14159));
			CHECK_EQ(v1.AsInt8(), 3);
			CHECK_EQ(v1.AsUInt8(), 3);
			CHECK_EQ(v1.AsInt16(), 3);
			CHECK_EQ(v1.AsUInt16(), 3);
			CHECK_EQ(v1.AsInt32(), 3);
			CHECK_EQ(v1.AsUInt32(), 3);
			CHECK_EQ(v1.AsInt64(), 3);
			CHECK_EQ(v1.AsUInt64(), 3);
			CHECK_EQ(v1.AsString(), std::string("3.141590"));
		}

		{ /* double ctor */
			AJAVariant v1((double)6.95334);
			CHECK_EQ(v1.AsBool(), true);
			CHECK_EQ(v1.AsFloat(), doctest::Approx(6.95334f));
			CHECK_EQ(v1.AsDouble(), doctest::Approx(6.95334));
			CHECK_EQ(v1.AsInt8(), 6);
			CHECK_EQ(v1.AsUInt8(), 6);
			CHECK_EQ(v1.AsInt16(), 6);
			CHECK_EQ(v1.AsUInt16(), 6);
			CHECK_EQ(v1.AsInt32(), 6);
			CHECK_EQ(v1.AsUInt32(), 6);
			CHECK_EQ(v1.AsInt64(), 6);
			CHECK_EQ(v1.AsUInt64(), 6);
			CHECK_EQ(v1.AsString(), std::string("6.953340"));
		}

		{ /* int8 ctor */
			AJAVariant v1((int8_t)INT8_MAX);
			CHECK_EQ(v1.AsBool(), true);
			CHECK_EQ(v1.AsFloat(), doctest::Approx(127.0f));
			CHECK_EQ(v1.AsDouble(), doctest::Approx(127.0));
			CHECK_EQ(v1.AsInt8(), 127);
			CHECK_EQ(v1.AsUInt8(), 127);
			CHECK_EQ(v1.AsInt16(), 127);
			CHECK_EQ(v1.AsUInt16(), 127);
			CHECK_EQ(v1.AsInt32(), 127);
			CHECK_EQ(v1.AsUInt32(), 127);
			CHECK_EQ(v1.AsInt64(), 127);
			CHECK_EQ(v1.AsUInt64(), 127);
			CHECK_EQ(v1.AsString(), std::string("127"));
		}

		{ /* uint8 ctor */
			AJAVariant v1((uint8_t)UINT8_MAX);
			CHECK_EQ(v1.AsBool(), true);
			CHECK_EQ(v1.AsFloat(), doctest::Approx(255.0f));
			CHECK_EQ(v1.AsDouble(), doctest::Approx(255.0));
			CHECK_EQ(v1.AsInt8(), -1);
			CHECK_EQ(v1.AsUInt8(), 255);
			CHECK_EQ(v1.AsInt16(), 255);
			CHECK_EQ(v1.AsUInt16(), 255);
			CHECK_EQ(v1.AsInt32(), 255);
			CHECK_EQ(v1.AsUInt32(), 255);
			CHECK_EQ(v1.AsInt64(), 255);
			CHECK_EQ(v1.AsUInt64(), 255);
			CHECK_EQ(v1.AsString(), std::string("255"));
		}

		{ /* int16_t ctor */
			AJAVariant v1((int16_t)INT16_MAX);
			CHECK_EQ(v1.AsBool(), true);
			CHECK_EQ(v1.AsFloat(), doctest::Approx(32767.0f));
			CHECK_EQ(v1.AsDouble(), doctest::Approx(32767.0));
			CHECK_EQ(v1.AsInt8(), -1);
			CHECK_EQ(v1.AsUInt8(), 255);
			CHECK_EQ(v1.AsInt16(), 32767);
			CHECK_EQ(v1.AsUInt16(), 32767);
			CHECK_EQ(v1.AsInt32(), 32767);
			CHECK_EQ(v1.AsUInt32(), 32767);
			CHECK_EQ(v1.AsInt64(), 32767);
			CHECK_EQ(v1.AsUInt64(), 32767);
			CHECK_EQ(v1.AsString(), std::string("32767"));
		}

		{ /* uint16_t ctor */
			AJAVariant v1((uint16_t)UINT16_MAX);
			CHECK_EQ(v1.AsBool(), true);
			CHECK_EQ(v1.AsFloat(), doctest::Approx(65535.0f));
			CHECK_EQ(v1.AsDouble(), doctest::Approx(65535.0));
			CHECK_EQ(v1.AsInt8(), -1);
			CHECK_EQ(v1.AsUInt8(), 255);
			CHECK_EQ(v1.AsInt16(), -1);
			CHECK_EQ(v1.AsUInt16(), 65535);
			CHECK_EQ(v1.AsInt32(), 65535);
			CHECK_EQ(v1.AsUInt32(), 65535);
			CHECK_EQ(v1.AsInt64(), 65535);
			CHECK_EQ(v1.AsUInt64(), 65535);
			CHECK_EQ(v1.AsString(), std::string("65535"));
		}

		{ /* int32_t ctor */
			AJAVariant v1((int32_t)INT32_MAX);
			CHECK_EQ(v1.AsBool(), true);
			CHECK_EQ(v1.AsFloat(), doctest::Approx(2147483647.0f));
			CHECK_EQ(v1.AsDouble(), doctest::Approx(2147483647.0));
			CHECK_EQ(v1.AsInt8(), -1);
			CHECK_EQ(v1.AsUInt8(), 255);
			CHECK_EQ(v1.AsInt16(), -1);
			CHECK_EQ(v1.AsUInt16(), 65535);
			CHECK_EQ(v1.AsInt32(), 2147483647);
			CHECK_EQ(v1.AsUInt32(), 2147483647);
			CHECK_EQ(v1.AsInt64(), 2147483647);
			CHECK_EQ(v1.AsUInt64(), 2147483647);
			CHECK_EQ(v1.AsString(), std::string("2147483647"));
		}

		{ /* uint32_t ctor */
			AJAVariant v1((uint32_t)UINT32_MAX);
			CHECK_EQ(v1.AsBool(), true);
			CHECK_EQ(v1.AsFloat(), doctest::Approx(4294967295.0f));
			CHECK_EQ(v1.AsDouble(), doctest::Approx(4294967295.0));
			CHECK_EQ(v1.AsInt8(), -1);
			CHECK_EQ(v1.AsUInt8(), 255);
			CHECK_EQ(v1.AsInt16(), -1);
			CHECK_EQ(v1.AsUInt16(), 65535);
			CHECK_EQ(v1.AsInt32(), -1);
			CHECK_EQ(v1.AsUInt32(), 4294967295U);
			CHECK_EQ(v1.AsInt64(), 4294967295U);
			CHECK_EQ(v1.AsUInt64(), 4294967295U);
			CHECK_EQ(v1.AsString(), std::string("4294967295"));
		}

		{ /* int64_t ctor */
			AJAVariant v1((int64_t)INT64_MAX);
			CHECK_EQ(v1.AsBool(), true);
			CHECK_EQ(v1.AsFloat(), doctest::Approx(9.22337204e+18f));
			CHECK_EQ(v1.AsDouble(), doctest::Approx(9.2233720368547758e+18));
			CHECK_EQ(v1.AsInt8(), -1);
			CHECK_EQ(v1.AsUInt8(), 255);
			CHECK_EQ(v1.AsInt16(), -1);
			CHECK_EQ(v1.AsUInt16(), 65535);
			CHECK_EQ(v1.AsInt32(), -1);
			CHECK_EQ(v1.AsUInt32(), 4294967295U);
			CHECK_EQ(v1.AsInt64(), 9223372036854775807);
			CHECK_EQ(v1.AsUInt64(), 9223372036854775807);
			CHECK_EQ(v1.AsString(), std::string("9223372036854775807"));
		}

		{ /* uint64_t ctor */
			AJAVariant v1((uint64_t)UINT64_MAX);
			CHECK_EQ(v1.AsBool(), true);
			float f = v1.AsFloat();
			double d = v1.AsDouble();
			(void)f;
			(void)d;
			CHECK_EQ(v1.AsFloat(), doctest::Approx(1.84467441e+19));
			CHECK_EQ(v1.AsDouble(), doctest::Approx(1.8446744073709552e+19));
			CHECK_EQ(v1.AsInt8(), -1);
			CHECK_EQ(v1.AsUInt8(), 255);
			CHECK_EQ(v1.AsInt16(), -1);
			CHECK_EQ(v1.AsUInt16(), 65535);
			CHECK_EQ(v1.AsInt32(), -1);
			CHECK_EQ(v1.AsUInt32(), 4294967295U);
			CHECK_EQ(v1.AsInt64(), -1);
			CHECK_EQ(v1.AsUInt64(), 18446744073709551615);
			CHECK_EQ(v1.AsString(), std::string("18446744073709551615"));
		}

		{ /* string ctor */
			AJAVariant v1((std::string)"hello");
			CHECK_EQ(v1.AsBool(), false);
			CHECK_EQ(v1.AsFloat(), doctest::Approx(0.0f));
			CHECK_EQ(v1.AsDouble(), doctest::Approx(0.0));
			CHECK_EQ(v1.AsInt8(), 0);
			CHECK_EQ(v1.AsUInt8(), 0);
			CHECK_EQ(v1.AsInt16(), 0);
			CHECK_EQ(v1.AsUInt16(), 0);
			CHECK_EQ(v1.AsInt32(), 0);
			CHECK_EQ(v1.AsUInt32(), 0);
			CHECK_EQ(v1.AsInt64(), 0);
			CHECK_EQ(v1.AsUInt64(), 0);
			CHECK_EQ(v1.AsString(), std::string("hello"));
			AJAVariant v2((std::string)"true");
			CHECK_EQ(v2.AsBool(), true);
			AJAVariant v3((std::string)"false");
			CHECK_EQ(v3.AsBool(), false);
		}

		{ /* copy ctor */
			AJAVariant v1("123");
			CHECK_EQ(v1.AsString(), "123");
			AJAVariant v2(v1);
			CHECK_EQ(v2.AsString(), "123");
		}
	}
}

void time_marker() {}
TEST_SUITE("time" * doctest::description("functions in ajabase/system/systemtime.h")) {

	TEST_CASE("AJATime")
	{
#if defined(AJA_SLEEP_USE_STL)
	std::cout << "-- C++11 std::chrono sleep impl --" << std::endl;
#else
	std::cout << "-- AJA sleep impl --" << std::endl;
#endif

#if defined(AJA_WINDOWS)
		TIMECAPS tc;
		if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) == MMSYSERR_NOERROR)
		{
			// Attempt to set the timer resolution to 1 millisecond
			uint32_t eventPeriod = std::min<uint32_t>(std::max<uint32_t>(tc.wPeriodMin, 1), tc.wPeriodMax);
			timeBeginPeriod(eventPeriod);
		}
#endif
		/* NOTE(paulh): AJATime::Sleep* methods have multiple implementations behind #defines.
		 * This code is pretty hard to test accurately on every platform. There can be a lot of
		 * variation in the amount of time each sleep takes, depending on CPU and OS.
		 * Take these results with a grain of salt.
		 *
		 * Where possible, use the AJA_SLEEP_USE_STL implementation in production code,
		 * which uses the C++11 STL std::chrono library. See `ajabase/system/systemtime.h` for details. */
		SUBCASE("AJATime::Sleep (Milliseconds)")
		{
			const int sleepDuration = 100; // 100msec (1/10th second)
			std::cout << "\nAJATime::Sleep - " << sleepDuration << "msec" << std::endl;
			uint64_t avgMs = 0;
			uint64_t avgUs = 0;
			uint64_t avgNs = 0;
			const int iterations = 10;
			for (int i = 0; i < iterations; i++) {
				uint64_t startMs = AJATime::GetSystemMilliseconds();
				uint64_t startUs = AJATime::GetSystemMicroseconds();
				uint64_t startNs = AJATime::GetSystemNanoseconds();
				AJATime::Sleep(sleepDuration);
				uint64_t endMs = AJATime::GetSystemMilliseconds();
				uint64_t endUs = AJATime::GetSystemMicroseconds();
				uint64_t endNs = AJATime::GetSystemNanoseconds();
				uint64_t deltaMs = endMs - startMs;
				uint64_t deltaUs = endUs - startUs;
				uint64_t deltaNs = endNs - startNs;
				avgMs += deltaMs;
				avgUs += deltaUs;
				avgNs += deltaNs;
			}
			uint64_t avgMsec = avgMs / iterations;
			uint64_t avgUsec = avgUs / iterations;
			uint64_t avgNsec = avgNs / iterations;
			// There could be variability in the sleep call, so make sure in range
			// check to make sure the units are correct
#if defined(AJA_SLEEP_USE_STL)
	#if defined(AJA_WINDOWS)
			CHECK(avgMsec > 90);
			CHECK(avgMsec < 150);
			CHECK(avgUsec > 90000);
			CHECK(avgUsec < 150000);
			CHECK(avgNsec > 90000000);
			CHECK(avgNsec < 150000000);
	#elif defined(AJA_MAC)
			CHECK(avgMsec > 90);
			CHECK(avgMsec < 150);
			CHECK(avgUsec > 90000);
			CHECK(avgUsec < 150000);
			CHECK(avgNsec > 90000000);
			CHECK(avgNsec < 150000000);
	#elif defined(AJA_LINUX)
			CHECK(avgMsec > 90);
			CHECK(avgMsec < 150);
			CHECK(avgUsec > 90000);
			CHECK(avgUsec < 150000);
			CHECK(avgNsec > 90000000);
			CHECK(avgNsec < 150000000);
	#endif
#else
	#if defined(AJA_WINDOWS)
			CHECK(avgMsec > 90);
			CHECK(avgMsec < 150);
			CHECK(avgUsec > 90000);
			CHECK(avgUsec < 150000);
			CHECK(avgNsec > 90000000);
			CHECK(avgNsec < 150000000);
	#elif defined(AJA_MAC)
			CHECK(avgMsec > 90);
			CHECK(avgMsec < 150);
			CHECK(avgUsec > 90000);
			CHECK(avgUsec < 150000);
			CHECK(avgNsec > 90000000);
			CHECK(avgNsec < 150000000);
	#elif defined(AJA_LINUX)
			CHECK(avgMsec > 90);
			CHECK(avgMsec < 150);
			CHECK(avgUsec > 90000);
			CHECK(avgUsec < 150000);
			CHECK(avgNsec > 90000000);
			CHECK(avgNsec < 150000000);
	#endif
#endif
			std::cout << "Milliseconds: " << avgMsec << "\n"
				<< "Microseconds: " << avgUsec << "\n"
				<< "Nanoseconds: " << avgNsec << "\n" << std::endl;
		}
		SUBCASE("AJATime::SleepInMicroseconds")
		{
			int sleepDuration = 100; // 100usec (0.0001 second)
			std::cout << "AJATime::SleepInMicroseconds - " << sleepDuration << "usec" << std::endl;
			uint64_t avgMs = 0;
			uint64_t avgUs = 0;
			uint64_t avgNs = 0;
			const int iterations = 10;
			for (int i = 0; i < iterations; i++) {
				uint64_t startMs = AJATime::GetSystemMilliseconds();
				uint64_t startUs = AJATime::GetSystemMicroseconds();
				uint64_t startNs = AJATime::GetSystemNanoseconds();
				AJATime::SleepInMicroseconds(sleepDuration);
				uint64_t endMs = AJATime::GetSystemMilliseconds();
				uint64_t endUs = AJATime::GetSystemMicroseconds();
				uint64_t endNs = AJATime::GetSystemNanoseconds();
				uint64_t deltaMs = endMs - startMs;
				uint64_t deltaUs = endUs - startUs;
				uint64_t deltaNs = endNs - startNs;
				avgMs += deltaMs;
				avgUs += deltaUs;
				avgNs += deltaNs;
			}
			uint64_t avgMsec = avgMs / iterations;
			uint64_t avgUsec = avgUs / iterations;
			uint64_t avgNsec = avgNs / iterations;
			// There could be variability in the sleep call, so make sure in range
			// check to make sure the units are correct
#if defined(AJA_SLEEP_USE_STL)
	#if defined(AJA_WINDOWS)
			CHECK(avgMsec >= 0);
			CHECK(avgMsec < 5);
			CHECK(avgUsec > 90);
			CHECK(avgUsec < 2500);
			CHECK(avgNsec > 90000);
			CHECK(avgNsec < 2500000);
	#elif defined(AJA_MAC)
			CHECK(avgMsec >= 0);
			CHECK(avgMsec < 5);
			CHECK(avgUsec > 90);
			CHECK(avgUsec < 2500);
			CHECK(avgNsec > 90000);
			CHECK(avgNsec < 2500000);
	#elif defined(AJA_LINUX)
			CHECK(avgMsec >= 0);
			CHECK(avgMsec < 5);
			CHECK(avgUsec > 90);
			CHECK(avgUsec < 2500);
			CHECK(avgNsec > 90000);
			CHECK(avgNsec < 2500000);
	#endif
#else
	#if defined(AJA_WINDOWS)
			CHECK(avgMsec >= 0);
			CHECK(avgMsec < 5);
			CHECK(avgUsec >= 0);
			CHECK(avgUsec < 2);
			CHECK(avgNsec >= 0);
			CHECK(avgNsec < 500);
	#elif defined(AJA_MAC)
			CHECK(avgMsec >= 0);
			CHECK(avgMsec < 5);
			CHECK(avgUsec > 90);
			CHECK(avgUsec < 2500);
			CHECK(avgNsec > 90000);
			CHECK(avgNsec < 2500000);
	#elif defined(AJA_LINUX)
			CHECK(avgMsec >= 0);
			CHECK(avgMsec < 5);
			CHECK(avgUsec > 90);
			CHECK(avgUsec < 2500);
			CHECK(avgNsec > 90000);
			CHECK(avgNsec < 2500000);
	#endif
#endif
			std::cout << "Milliseconds: " << avgMsec << "\n"
				<< "Microseconds: " << avgUsec << "\n"
				<< "Nanoseconds: " << avgNsec << "\n" << std::endl;
		}
// 		SUBCASE("AJATime::SleepInNanoseconds")
// 		{
// 			int sleepDuration = 100; // 100 nanoseconds
// 			std::cout << "AJATime::SleepInNanoseconds - " << sleepDuration << "nsec" << std::endl;
// 			uint64_t avgMs = 0;
// 			uint64_t avgUs = 0;
// 			uint64_t avgNs = 0;
// 			const int iterations = 10;
// 			for (int i = 0; i < iterations; i++) {
// 				uint64_t startMs = AJATime::GetSystemMilliseconds();
// 				uint64_t startUs = AJATime::GetSystemMicroseconds();
// 				uint64_t startNs = AJATime::GetSystemNanoseconds();
// 				AJATime::SleepInNanoseconds(sleepDuration);
// 				uint64_t endMs = AJATime::GetSystemMilliseconds();
// 				uint64_t endUs = AJATime::GetSystemMicroseconds();
// 				uint64_t endNs = AJATime::GetSystemNanoseconds();
// 				uint64_t deltaMs = endMs - startMs;
// 				uint64_t deltaUs = endUs - startUs;
// 				uint64_t deltaNs = endNs - startNs;
// 				avgMs += deltaMs;
// 				avgUs += deltaUs;
// 				avgNs += deltaNs;
// 			}
// 			uint64_t avgMsec = avgMs / iterations;
// 			uint64_t avgUsec = avgUs / iterations;
// 			uint64_t avgNsec = avgNs / iterations;
// 			// There could be variability in the sleep call, so make sure in range
// 			// check to make sure the units are correct
// #if defined(AJA_SLEEP_USE_STL)
// 	#if defined(AJA_WINDOWS)
// 			CHECK(avgMsec <= 1);
// 			CHECK(avgUsec < 10);
// 			CHECK(avgNsec > 50);
// 			CHECK(avgNsec < 10000);
// 	#elif defined(AJA_MAC)
// 			CHECK_EQ(avgMsec, 0);
// 			CHECK(avgUsec < 100);
// 			CHECK(avgNsec > 50);
// 			CHECK(avgNsec < 100000);
// 	#elif defined(AJA_LINUX)
// 			CHECK_EQ(avgMsec, 0);
// 			CHECK(avgUsec < 100);
// 			CHECK(avgNsec > 50);
// 			CHECK(avgNsec < 100000);
// 	#endif
// #else
// 	#if defined(AJA_WINDOWS)
// 			CHECK_EQ(avgMsec, 0);
// 			CHECK(avgUsec < 1000);
// 			CHECK(avgNsec > 0);
// 			CHECK(avgNsec < 50000);
// 	#elif defined(AJA_MAC)
// 			// NOTE(paulh): Nanosleep doesn't seem to sleep at nanosecond resolution on my M1 Mac Mini.
// 			CHECK_EQ(avgMsec, 0);
// 			CHECK(avgUsec < 1000);
// 			CHECK(avgNsec > 0);
// 			CHECK(avgNsec < 50000);
// 	#elif defined(AJA_LINUX)
// 			CHECK_EQ(avgMsec, 0);
// 			CHECK(avgUsec < 1000);
// 			CHECK(avgNsec > 0);
// 			CHECK(avgNsec < 75000);
// 	#endif
// #endif
// 			std::cout << "Milliseconds: " << avgMsec << "\n"
// 				<< "Microseconds: " << avgUsec << "\n"
// 				<< "Nanoseconds: " << avgNsec << "\n" << std::endl;
// 		}
	}

} //time


//	These macros borrowed from ntv2publicinterface.h:
#define DECN(__x__,__n__)		std::dec << std::setw(int(__n__)) << std::right << (__x__)
#define DEC0N(__x__,__n__)		std::dec << std::setw(int(__n__)) << std::setfill('0') << std::right << (__x__) << std::dec << std::setfill(' ')

static const std::vector<uint64_t> ETs = {12, 123, 1234, 12345, 123456, 1234567, 12345678, 123456789, 1234567890};
static const std::vector<std::string> mETStrs = {"12.0 msec", "123.0 msec", " 1.2 secs", "12.3 secs", " 2.1 mins", "20.6 mins", " 3.4 hrs", " 1.4 days", "14.3 days"};
static const std::vector<std::string> uETStrs = {"12.0 usec", "123.0 usec", " 1.2 msec", "12.3 msec", "123.5 msec", " 1.2 secs", "12.3 secs", " 2.1 mins", "20.6 mins"};
static const std::vector<std::string> nETStrs = {"12.0 nsec", "123.0 nsec", " 1.2 usec", "12.3 usec", "123.5 usec", " 1.2 msec", "12.3 msec", "123.5 msec", " 1.2 secs"};
static inline std::string AsStr(const AJATimer & inTimer)	{std::ostringstream oss; oss << inTimer; return oss.str();}

void timer_marker() {}
TEST_SUITE("timer" * doctest::description("functions in ajabase/common/timer.h")) {

	TEST_CASE("AJATimer")
	{
		AJATimer milli(AJATimerPrecisionMilliseconds);
		CHECK_EQ(milli.Precision(), AJATimerPrecisionMilliseconds);
		milli.Start();
		CHECK(milli.IsRunning());
		while (milli.ElapsedTime() < 1000)
			;
		milli.Stop();
		CHECK_FALSE(milli.IsRunning());
		CHECK(milli.ElapsedTime() > 0);
		std::cerr << std::endl << milli.PrecisionName(milli.Precision()) << std::endl;
		for (size_t n(0);  n < ETs.size();  n++)
		{	milli.SetStopTime(ETs.at(n));
			std::cerr << DECN(milli.ElapsedTime(),10) << ": " << milli << std::endl;
			CHECK_EQ(mETStrs.at(n), AsStr(milli));
		}

		AJATimer micro(AJATimerPrecisionMicroseconds);
		CHECK_EQ(micro.Precision(), AJATimerPrecisionMicroseconds);
		micro.Start();
		CHECK(micro.IsRunning());
		while (micro.ElapsedTime() < 1000)
			;
		micro.Stop();
		CHECK_FALSE(micro.IsRunning());
		CHECK(micro.ElapsedTime() > 0);
		std::cerr << std::endl << micro.PrecisionName(micro.Precision()) << std::endl;
		for (size_t n(0);  n < ETs.size();  n++)
		{	micro.SetStopTime(ETs.at(n));	std::cerr << DECN(micro.ElapsedTime(),10) << ": " << micro << std::endl;
			CHECK_EQ(uETStrs.at(n), AsStr(micro));
		}

		AJATimer nano(AJATimerPrecisionNanoseconds);
		CHECK_EQ(nano.Precision(), AJATimerPrecisionNanoseconds);
		nano.Start();
		CHECK(nano.IsRunning());
		while (nano.ElapsedTime() < 1000)
			;
		nano.Stop();
		CHECK_FALSE(nano.IsRunning());
		CHECK(nano.ElapsedTime() > 0);
		std::cerr << std::endl << nano.PrecisionName(nano.Precision()) << std::endl;
		for (size_t n(0);  n < ETs.size();  n++)
		{	nano.SetStopTime(ETs.at(n));	std::cerr << DECN(nano.ElapsedTime(),10) << ": " << nano << std::endl;
			CHECK_EQ(nETStrs.at(n), AsStr(nano));
		}
	}

}	//	timer


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

void thread_marker() {}
TEST_SUITE("thread" * doctest::description("functions in ajabase/system/thread.h")) {

#define THREAD_LIFETIME_MS 1000
#define THREAD_FORCE_KILL_MS 3000

	class TestThread : public AJAThread {
	public:
		AJAStatus ThreadRun(void) override {
			uint64_t startTime = AJATime::GetSystemMilliseconds();
			uint64_t now = AJATime::GetSystemMilliseconds();
			uint64_t elapsed = now - startTime;
			while (!Terminate() && elapsed < THREAD_LIFETIME_MS) {
				now = AJATime::GetSystemMilliseconds();
				elapsed = now - startTime;
				int64_t remain = (int64_t)(THREAD_LIFETIME_MS - elapsed);
				std::cout << "TestThread::ThreadRun: elapsed=" << elapsed << ", remaining=" << remain << std::endl;
				AJATime::Sleep(100);
			}
			return AJA_STATUS_SUCCESS;
		}
	};
	TEST_CASE("AJAThread::Active")
	{
		CHECK(THREAD_FORCE_KILL_MS - THREAD_LIFETIME_MS >= 1000); // run for at least 1 second
		TestThread tt;
		std::cout << "AJAThread::Active loops" << std::endl;
		tt.Start();
		uint64_t tid = tt.GetThreadId();
		bool running = true;
		uint64_t startTime = AJATime::GetSystemMilliseconds();
		while (running) {
			running = tt.Active();
			uint64_t now = AJATime::GetSystemMilliseconds();
			if (running && now - startTime >= THREAD_FORCE_KILL_MS) {
				std::cerr << "TestThread (id=" << tid
					<< ") did not exit after "
					<< THREAD_LIFETIME_MS
					<< " milliseconds! Forcing termination after " << (now - startTime) << "ms!" << std::endl;
				tt.Terminate();
				break;
			}
		}
		CHECK_FALSE(running);
		CHECK_FALSE(tt.Active());
	}
	TEST_CASE("AJAThread::Stop")
	{
		TestThread tt;
		std::cout << "AJAThread::Stop loops" << std::endl;
		tt.Start();
		//uint64_t tid = tt.GetThreadId();
		bool running = true;
		uint64_t startTime = AJATime::GetSystemMilliseconds();
		while (running) {
			uint64_t now = AJATime::GetSystemMilliseconds();
			if (now - startTime > 100) {
				AJAStatus s = tt.Stop(300);
				CHECK(s == AJA_STATUS_SUCCESS);
				running = false;
			}
		}
		tt.Terminate();
	}
}

void bytestream_marker() {}
TEST_SUITE("bytestream" * doctest::description("functions in ajabase/common/bytestream.h")) {
	TEST_CASE("Bytestream Constructor, Pos, Seek, Read/Write methods")
	{
		const size_t bufsize = 4096;
		uint8_t buf[bufsize] = { 0 };
		uint8_t txt[32] = { 0 };

		AJAByteStream b(buf);
		CHECK(b.Pos() == 0);

		b.Set(42, 13);
		CHECK(b.Pos() == 13);

		b.Write("Hello World", 12);
		CHECK(b.Pos() == 25);

		b.Write8(0xab);
		CHECK(b.Pos() == 26);

		b.Write16LE(0x1337);
		CHECK(b.Pos() == 28);

		b.Write16BE(0xabcd);
		CHECK(b.Pos() == 30);

		b.SeekRev(2);
		CHECK(b.Pos() == 28);
		CHECK(b.Read16LE() == 0xcdab);

		b.SeekRev(2);
		CHECK(b.Read16BE() == 0xabcd);

		b.SeekRev(4);
		CHECK(b.Read16LE() == 0x1337);

		b.SeekRev(2);
		CHECK(b.Read16BE() == 0x3713);

		// CHECK(b.Read)
		b.Reset();
		CHECK(b.Pos() == 0);

		CHECK(b.Read8() == 42);
		b.SeekFwd(12);
		b.Read(txt, 12);
		CHECK(!memcmp(txt, "Hello World\0", 12));

		b.Write32BE(0xdecafbad);
		b.Write32LE(0xd4c3b2a1);
		b.Write64BE(0x1337c0de1337c0de);
		b.Write64LE(0x0807060504030201);
		b.SeekRev(24);
		CHECK(b.Read32LE() == 0xadfbcade);
		b.SeekRev(4);
		CHECK(b.Read32BE() == 0xdecafbad);
		CHECK(b.Read32BE() == 0xa1b2c3d4);
		b.SeekRev(4);
		CHECK(b.Read32LE() == 0xd4c3b2a1);
		CHECK(b.Read64BE() == 0x1337c0de1337c0de);
		CHECK(b.Read64LE() == 0x0807060504030201);
		b.SeekRev(16);
		CHECK(b.Read64LE() == 0xdec03713dec03713);
		CHECK(b.Read64BE() == 0x0102030405060708);

		b.Seek(25);
		CHECK(b.Read8() == 0xde);
		CHECK(b.Read8() == 0xca);
		CHECK(b.Read8() == 0xfb);
		CHECK(b.Read8() == 0xad);

		uint8_t* get_buf = (uint8_t*)b.Buffer();
		CHECK(get_buf[0] == 42);
		CHECK(get_buf[25] == 0xde);
		CHECK(get_buf[26] == 0xca);
		CHECK(get_buf[27] == 0xfb);
		CHECK(get_buf[28] == 0xad);
	}
} //bytestream

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
			CHECK(a.GetFrameTimeScale()	 == b.GetFrameTimeScale());
			CHECK(a.GetFrameDuration()	 == b.GetFrameDuration());
			CHECK(a.GetAJAFrameRate()	 == b.GetAJAFrameRate());
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

		#if !defined(NTV2_DEPRECATE_17_5)
			// test the deprecated functionality
			char ctmp[12];
			tc.QueryString(ctmp, AJATimeBase(AJA_FrameRate_2400), false);
			CHECK(strcmp(ctmp, "01:44:10:00")==0);
		#endif	//	NTV2_DEPRECATE_17_5
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
		tc2.SetWithCleanup("   01:02:03:04	 ", AJATimeBase(AJA_FrameRate_2400), false);
		tc2.QueryString(tmp, AJATimeBase(AJA_FrameRate_2400), false);
		CHECK(tmp == "01:02:03:04");
		tc2.SetWithCleanup("   01 02 03 04	 ", AJATimeBase(AJA_FrameRate_2400), false);
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

void movingavg_marker() {}
TEST_SUITE("ajamovingavg" * doctest::description("functions in ajabase/common/ajamovingavg.h")) {

	TEST_CASE("BFT-char")
	{
		AJAMovingAvg<char> cAvg;
		CHECK_FALSE(cAvg.isValid());
		CHECK(cAvg.isEmpty());
		CHECK_EQ(cAvg.sampleCapacity(), 10);
		cAvg.addSample(-10);
		cAvg.addSample(-5);
		cAvg.addSample(0);
		cAvg.addSample(5);
		cAvg.addSample(10);
		CHECK_EQ(cAvg.numStoredSamples(),5);
		CHECK_EQ(cAvg.totalSamples(),5);
		CHECK_EQ(cAvg.average(), 0);
		CHECK_EQ(cAvg.averageF(), 0.0);
		CHECK_EQ(cAvg.minimum(), -10);
		CHECK_EQ(cAvg.maximum(), 10);
		cAvg.addSample(7);
		cAvg.addSample(8);
		cAvg.addSample(9);
		cAvg.addSample(10);
		cAvg.addSample(11);
		CHECK_EQ(cAvg.numStoredSamples(),10);
		CHECK_EQ(cAvg.totalSamples(),10);
		CHECK_EQ(cAvg.average(), 4);
		CHECK_EQ(cAvg.minimum(), -10);
		CHECK_EQ(cAvg.maximum(), 11);
		cAvg.addSample(8);
		cAvg.addSample(10);
		cAvg.addSample(12);
		cAvg.addSample(6);
		cAvg.addSample(5);
		CHECK_EQ(cAvg.numStoredSamples(),10);
		CHECK_EQ(cAvg.totalSamples(),15);
		CHECK_EQ(cAvg.average(), 8);
		CHECK_EQ(cAvg.averageF(), 8.6);
		CHECK_EQ(cAvg.minimum(), -10);
		CHECK_EQ(cAvg.maximum(), 12);
		CHECK_EQ(cAvg.recentMaximum(), 12);
		CHECK_EQ(cAvg.recentMinimum(), 5);
	}

	TEST_CASE("BFT-int")
	{
		AJAMovingAvg<int> iAvg;
		CHECK_FALSE(iAvg.isValid());
		CHECK(iAvg.isEmpty());
		CHECK_EQ(iAvg.sampleCapacity(), 10);
		iAvg.addSample(-10);
		iAvg.addSample(-5);
		iAvg.addSample(0);
		iAvg.addSample(5);
		iAvg.addSample(10);
		CHECK_EQ(iAvg.numStoredSamples(),5);
		CHECK_EQ(iAvg.totalSamples(),5);
		CHECK_EQ(iAvg.average(), 0);
		CHECK_EQ(iAvg.averageF(), 0.0);
		CHECK_EQ(iAvg.minimum(), -10);
		CHECK_EQ(iAvg.maximum(), 10);
		iAvg.addSample(7);
		iAvg.addSample(8);
		iAvg.addSample(9);
		iAvg.addSample(10);
		iAvg.addSample(11);
		CHECK_EQ(iAvg.numStoredSamples(),10);
		CHECK_EQ(iAvg.totalSamples(),10);
		CHECK_EQ(iAvg.average(), 4);
		CHECK_EQ(iAvg.minimum(), -10);
		CHECK_EQ(iAvg.maximum(), 11);
		iAvg.addSample(8);
		iAvg.addSample(10);
		iAvg.addSample(12);
		iAvg.addSample(6);
		iAvg.addSample(5);
		CHECK_EQ(iAvg.numStoredSamples(),10);
		CHECK_EQ(iAvg.totalSamples(),15);
		CHECK_EQ(iAvg.average(), 8);
		CHECK_EQ(iAvg.averageF(), 8.6);
		CHECK_EQ(iAvg.minimum(), -10);
		CHECK_EQ(iAvg.maximum(), 12);
		CHECK_EQ(iAvg.recentMaximum(), 12);
		CHECK_EQ(iAvg.recentMinimum(), 5);
	}

	TEST_CASE("BFT-int8")
	{
		AJAMovingAvg<int8_t> i8Avg;
		CHECK_FALSE(i8Avg.isValid());
		CHECK(i8Avg.isEmpty());
		CHECK_EQ(i8Avg.sampleCapacity(), 10);
		i8Avg.addSample(-10);
		i8Avg.addSample(-5);
		i8Avg.addSample(0);
		i8Avg.addSample(5);
		i8Avg.addSample(10);
		CHECK_EQ(i8Avg.numStoredSamples(),5);
		CHECK_EQ(i8Avg.totalSamples(),5);
		CHECK_EQ(i8Avg.average(), 0);
		CHECK_EQ(i8Avg.averageF(), 0.0);
		CHECK_EQ(i8Avg.minimum(), -10);
		CHECK_EQ(i8Avg.maximum(), 10);
		i8Avg.addSample(7);
		i8Avg.addSample(8);
		i8Avg.addSample(9);
		i8Avg.addSample(10);
		i8Avg.addSample(11);
		CHECK_EQ(i8Avg.numStoredSamples(),10);
		CHECK_EQ(i8Avg.totalSamples(),10);
		CHECK_EQ(i8Avg.average(), 4);
		CHECK_EQ(i8Avg.minimum(), -10);
		CHECK_EQ(i8Avg.maximum(), 11);
		i8Avg.addSample(8);
		i8Avg.addSample(10);
		i8Avg.addSample(12);
		i8Avg.addSample(6);
		i8Avg.addSample(5);
		CHECK_EQ(i8Avg.numStoredSamples(),10);
		CHECK_EQ(i8Avg.totalSamples(),15);
		CHECK_EQ(i8Avg.average(), 8);
		CHECK_EQ(i8Avg.averageF(), 8.6);
		CHECK_EQ(i8Avg.minimum(), -10);
		CHECK_EQ(i8Avg.maximum(), 12);
		CHECK_EQ(i8Avg.recentMaximum(), 12);
		CHECK_EQ(i8Avg.recentMinimum(), 5);
	}

	TEST_CASE("BFT-uint32")
	{
		AJAMovingAvg<uint32_t> u32Avg;
		u32Avg.addSample(2);	u32Avg.addSample(4);	u32Avg.addSample(6);	u32Avg.addSample(8);
		u32Avg.addSample(12);	u32Avg.addSample(10);	u32Avg.addSample(10);	u32Avg.addSample(10);
		u32Avg.addSample(10);	u32Avg.addSample(10);
		CHECK_EQ(u32Avg.average(), 8);
		u32Avg.addSample(10);	u32Avg.addSample(10);	u32Avg.addSample(10);	u32Avg.addSample(10);
		u32Avg.addSample(10);
		CHECK_EQ(u32Avg.average(), 10);
		CHECK_EQ(u32Avg.minimum(), 2);
		CHECK_EQ(u32Avg.maximum(), 12);
		CHECK_EQ(u32Avg.recentMinimum(), 10);
		CHECK_EQ(u32Avg.recentMaximum(), 10);
	}

} //movingavg


void persistence_marker() {}
TEST_SUITE("persistence" * doctest::description("functions in ajabase/persistence/persistence.h")) {

	TEST_CASE("AJAPersistence")
	{
		//AJADebug::Open();

		std::string appID("com.aja.unittest.ajabase");
		std::string deviceType("");
		std::string deviceNumber("");
		bool		sharedPrefs = false;
		AJAPersistence p(appID, deviceType, deviceNumber, sharedPrefs);

		std::string appID2("com.aja.unittest.ajabase");
		std::string deviceType2("");
		std::string deviceNumber2("");
		bool		sharedPrefs2 = false;
		p.GetParams(appID2, deviceType2, deviceNumber2, sharedPrefs2);

		CHECK(appID == appID2);
		CHECK(deviceType == deviceType2);
		CHECK(deviceNumber == deviceNumber2);
		CHECK(sharedPrefs == sharedPrefs2);

		std::string keyName		= "";
		int			intValue	= 42;
		bool		trueValue	= true;
		bool		falseValue	= false;
		double		doubleValue = 3.14;
		std::string strValue	= "testing 1,2,3";
		char		blobValue[] = "blob test data";
		int			blobLen		= (int)strlen(blobValue);
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

void info_marker() {}
TEST_SUITE("info" * doctest::description("functions in ajabase/system/info.h")) {

	TEST_CASE("AJASystemInfo")
	{
		// just create an instance to make sure everything works at runtime
		AJASystemInfo i;
		std::string cpuType;
		CHECK(i.GetValue(AJA_SystemInfoTag_CPU_Type, cpuType) == AJA_STATUS_SUCCESS);
		std::cout << "CPU Type: " << cpuType << std::endl;
		CHECK(!cpuType.empty());
	}

} //info

void file_marker() {}
TEST_SUITE("file" * doctest::description("functions in ajabase/system/file_io.h")) {

	// Shared amongst sub-cases
	AJAStatus status = AJA_STATUS_FAIL;
	std::string tempDir;
	std::wstring tempDirWStr;
	std::string tempFileName;
	std::string tempFilePath;
	const std::string pathSepStr = std::string(1, AJA_PATHSEP);
	const std::wstring pathSepWStr = std::wstring(1, AJA_PATHSEP_WIDE);

	TEST_CASE("AJAFileIO")
	{
		SUBCASE("::TempDirectory")
		{
			status = AJAFileIO::TempDirectory(tempDir);
			WARN_MESSAGE(status == AJA_STATUS_SUCCESS, "AJAFileIO::TempDirectory() could not find platform temp dir, trying secondary location.");

			if (status != AJA_STATUS_SUCCESS)
			{
				tempDir = "ajafileio_tmp_dir";
				char cwdBuf[AJA_MAX_PATH+1] = "";
	#if defined(AJA_WINDOWS)
				_mkdir(tempDir.c_str());
				_getcwd(cwdBuf, AJA_MAX_PATH);
	#else
				if (mkdir(tempDir.c_str(), ACCESSPERMS) == 0)
				{
					chmod(tempDir.c_str(), ACCESSPERMS);
				}
				char* result = getcwd(cwdBuf, AJA_MAX_PATH);
				AJA_UNUSED(result);
	#endif
				// prepend full working directory path so rest of tests complete
				std::string cwdStr = cwdBuf;
				aja::rstrip(cwdStr, pathSepStr);
				tempDir = std::string(cwdBuf) + pathSepStr + tempDir;
			}
		}

		SUBCASE("::DoesDirectoryExist")
		{
			CHECK_GT(tempDir.size(), 0);
			status = AJAFileIO::DoesDirectoryExist(tempDir);
			const std::string checkMsg = "Expected temp directory '" +
				tempDir + "' not found! Check result of ::TempDirectory sub-case.";
			CHECK_MESSAGE(status == AJA_STATUS_SUCCESS, checkMsg);
			CHECK_EQ(AJAFileIO::DirectoryExists(tempDir), true);
		}

		SUBCASE("::GetWorkingDirectory")
		{
			CHECK_GT(tempDir.size(), 0);

			// GetWorkingDirectory std::string
			std::string cwdStr;
			std::string cwdWStr;
			status = AJAFileIO::GetWorkingDirectory(cwdStr);
			CHECK_MESSAGE(status == AJA_STATUS_SUCCESS, "GetWorkingDirectory(std::string&) failed!");

			status = AJAFileIO::GetWorkingDirectory(cwdWStr);
			CHECK_MESSAGE(status == AJA_STATUS_SUCCESS, "GetWorkingDirectory(std::wstring&) failed!");

			// Check GetWorkingDirectory path values
			std::string tempDirCwd;
			std::wstring tempDirCwdWStr;
			int result = chdir(tempDir.c_str());
			AJA_UNUSED(result);

			status = AJAFileIO::GetWorkingDirectory(tempDirCwd);
			aja::rstrip(tempDir, pathSepStr);
			aja::rstrip(tempDirCwd, pathSepStr);
#if defined (AJA_MAC)
			CHECK_EQ(tempDir, aja::replace(tempDirCwd, "/private", ""));
#else
			CHECK_EQ(tempDir, tempDirCwd);
#endif
			result = chdir(cwdStr.c_str());
			AJA_UNUSED(result);
		}
		SUBCASE("::GetExecutablePath")
		{
			std::string pathStr, dirNameStr, fileNameStr;
			std::wstring pathWide, dirNameWide, fileNameWide;
			status = AJAFileIO::GetExecutablePath(pathStr);
			CHECK_EQ(status, AJA_STATUS_SUCCESS);
			status = AJAFileIO::GetExecutablePath(pathWide);
			CHECK_EQ(status, AJA_STATUS_SUCCESS);
			AJAFileIO::GetDirectoryName(pathStr, dirNameStr);
			CHECK_EQ(status, AJA_STATUS_SUCCESS);
			AJAFileIO::GetFileName(pathStr, fileNameStr);
			CHECK_EQ(status, AJA_STATUS_SUCCESS);
			AJAFileIO::GetDirectoryName(pathWide, dirNameWide);
			CHECK_EQ(status, AJA_STATUS_SUCCESS);
			AJAFileIO::GetFileName(pathWide, fileNameWide);
			CHECK_EQ(status, AJA_STATUS_SUCCESS);
			if (status == AJA_STATUS_SUCCESS) {
#if defined(AJA_WINDOWS)
				CHECK_EQ(fileNameStr, "ut_ajabase.exe");
				CHECK_EQ(fileNameWide, L"ut_ajabase.exe");
#elif defined(AJA_LINUX) || defined(AJA_MAC)
				CHECK_EQ(fileNameStr, "ut_ajabase");
				CHECK_EQ(fileNameWide, L"ut_ajabase");
#endif
			}
		}

		SUBCASE("FileInfo")
		{

			tempFilePath = tempDir;

			// remove any trailing path separator on tempDir
			// then add the pathSepStr, simple way to sanitize the created path
			aja::rstrip(tempFilePath, pathSepStr);
			tempFilePath += pathSepStr;
			tempFileName = "AJAFileIO_unittest_file_" +
				aja::to_string((unsigned long)AJATime::GetSystemMilliseconds()) + ".txt";
			tempFilePath += tempFileName;

			AJAFileIO file;
			status = file.Open(tempFilePath, eAJAReadWrite|eAJACreateAlways, 0);
			std::string reqMsg = "Error creating temp file: '" + tempFilePath + "'";
			REQUIRE_MESSAGE(status == AJA_STATUS_SUCCESS, reqMsg);
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
#if !defined(AJA_WINDOWS)
			CHECK(fullFilePath.find(tempFilePath) != std::string::npos);
#endif
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
		}

		SUBCASE("::IsDirectoryEmpty")
		{
			// temp dir should not be empty if previous sub-case passed
			status = AJAFileIO::IsDirectoryEmpty(tempDir);
			CHECK(status != AJA_STATUS_SUCCESS);
		}

		SUBCASE("::DoesDirectoryContain")
		{
			// should be at least one file in temp dir from FileInfo test
			status = AJAFileIO::DoesDirectoryContain(tempDir, "*");
			CHECK(status == AJA_STATUS_SUCCESS);

			// should not exist
			status = AJAFileIO::DoesDirectoryContain(tempDir, "*.someBonkersExt");
			CHECK(status != AJA_STATUS_SUCCESS);
		}

		SUBCASE("::FileExists")
		{
			CHECK(AJAFileIO::FileExists(tempFilePath));
			status = AJAFileIO::Delete(tempFilePath);
			CHECK(status == AJA_STATUS_SUCCESS);
			CHECK(AJAFileIO::FileExists(tempFilePath) == false);
		}

		SUBCASE("::GetFileName")
		{
			std::string gotFileName;
			std::wstring tempFilePathWStr;
			std::wstring gotFileNameWStr;
			std::wstring tempFileNameWStr;

			// GetFileName std::string
			status = AJAFileIO::GetFileName(tempFilePath, gotFileName);
			CHECK_MESSAGE(status == AJA_STATUS_SUCCESS, "GetFileName(const std::string&, std::string&) failed!");
			CHECK_EQ(tempFileName, gotFileName);

			// GetFileName std::wstring
			CHECK_EQ(aja::string_to_wstring(tempFilePath, tempFilePathWStr), true);
			CHECK_EQ(aja::string_to_wstring(tempFileName, tempFileNameWStr), true);
			status = AJAFileIO::GetFileName(tempFilePathWStr, gotFileNameWStr);
			CHECK_MESSAGE(status == AJA_STATUS_SUCCESS, "GetFileName(const std::wstring&, std::wstring&) failed!");
			CHECK_EQ(tempFileNameWStr, gotFileNameWStr);
		}

		SUBCASE("::GetDirectoryName")
		{
			// GetDirectoryName std::string
			std::string dirName;
			std::wstring dirNameWide;
			status = AJAFileIO::GetDirectoryName(tempFilePath, dirName);
			CHECK_MESSAGE(status == AJA_STATUS_SUCCESS, "GetDirectoryName(const std::string&, std::string&) failed");
			aja::rstrip(tempDir, pathSepStr);
			aja::rstrip(dirName, pathSepStr);
			CHECK_EQ(tempDir, dirName);

			status = AJAFileIO::GetDirectoryName(tempDir + pathSepStr + "foo" + pathSepStr + "bar.txt", dirName);
			CHECK_MESSAGE(status == AJA_STATUS_SUCCESS, "GetDirectoryName(const std::string&, std::string&) failed");
			CHECK_EQ(tempDir + pathSepStr + "foo", dirName);
		}

		SUBCASE("::GetHandle")
		{
			AJAFileIO fio;
			AJAStatus status = fio.Open("foobar.dat", eAJAWriteOnly|eAJACreateAlways, 0);
			CHECK_EQ(status, AJA_STATUS_SUCCESS);
			if (status == AJA_STATUS_SUCCESS) {
				std::string buf("Hello, AJAFileIO!");
				CHECK_EQ(fio.Write(buf), buf.size());
#if defined(AJA_WINDOWS)
				HANDLE handle = (HANDLE)fio.GetHandle();
				CHECK(handle != NULL);
				if (handle != NULL) {
					CHECK_EQ(::GetFileType(handle), FILE_TYPE_DISK);
					LARGE_INTEGER sizeInfo;
					if(::GetFileSizeEx(handle,&sizeInfo)) {
						CHECK_EQ(sizeInfo.QuadPart, buf.size());
					}
				}
#else
				FILE* handle = (FILE*)fio.GetHandle();
				CHECK(handle != NULL);
				if(handle != NULL) {
					fseek(handle, 0 , SEEK_END);
					long fileSize = ftell(handle);
					fseek(handle, 0 , SEEK_SET);// needed for next read from beginning of file
					CHECK_EQ(fileSize, buf.size());
				}
#endif
				if (fio.IsOpen())
					CHECK_EQ(fio.Close(), AJA_STATUS_SUCCESS);
			}

		}
	}

} //file
