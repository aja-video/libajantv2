/* SPDX-License-Identifier: MIT */
/**
	@file		commandline.h
	@brief		Declaration of Command Line classes
	@copyright	(C) 2011-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_COMMANDLINE_H
#define AJA_COMMANDLINE_H

#include "ajabase/common/types.h"
#include "ajabase/common/export.h"
#include "ajabase/common/variant.h"

#include <string>
#include <map>
#include <vector>

typedef std::vector<std::string> AJAStringList;
typedef AJAStringList::const_iterator AJAStringListConstIter;

typedef enum {
	kOptionSingleDash = 1 << 0,
	kOptionDoubleDash = 1 << 1,
} AJACommandLineOptionFlags;

/**
 *	Class that represents a command line option.
 */
class AJA_EXPORT AJACommandLineOption
{
public:
	AJACommandLineOption();
	AJACommandLineOption(const std::string &name);
	AJACommandLineOption(const std::string &name,
						 const std::string &desc);
	AJACommandLineOption(const std::string &name,
						 const std::string &desc,
						 const std::string &defaultValue);
	AJACommandLineOption(const AJAStringList &names);
	AJACommandLineOption(const AJAStringList &names,
						 const std::string &desc);
	AJACommandLineOption(const AJAStringList &names,
						 const std::string &desc,
						 const std::string &defaultValue);
	virtual ~AJACommandLineOption();

	/**
	 * Add an arg name to this command line option.
	 *
	 * @param[in] name	The arg name to add.
	 */
	bool AddName(const std::string &name);

	/**
	 * Get list of arg names attached to this command line option.
	 *
	 * @return	The list of names.
	 */
	AJAStringList GetNames() const;

	/**
	 * Get the description string for this command line option.
	 *
	 * @return	The description string.
	 */
	std::string GetDesc() const;

	/**
	 * Set the description string for this command line option.
	 *
	 * @param[in] desc	The description string.
	 */
	void SetDesc(const std::string &desc);

	/**
	 * Get the extra description string for this command line option.
	 *
	 * @return	The extra description string.
	 */
	std::string GetExtraDesc() const;

	/**
	 * Set the extra description string for this command line option.
	 *
	 * @param[in] desc	The extra description string.
	 */
	void SetExtraDesc(const std::string &desc);

	/**
	 * Set the default value string for this command line option.
	 *
	 * @param[in] desc	The default value string.
	 */
	void SetDefaultValue(const std::string &value);

	/**
	 * Get the default value string for this command line option.
	 *
	 * @return	The default value string.
	 */
	std::string GetDefaultValue() const;

	void AddValue(const std::string &value);
	std::string GetValue(size_t index = 0) const;
	AJAStringList GetValues() const;

private:
	AJAStringList mNames;
	std::string mDesc;
	std::string mDescExtra;
	AJAStringList mValues;
	std::string mDefaultValue;
};

typedef std::vector<AJACommandLineOption> AJACommandLineOptionList;
typedef AJACommandLineOptionList::const_iterator AJACommandLineOptionListIter;

class AJACommandLineParser;
typedef std::map<std::string, AJACommandLineParser *> SubParserMap;
typedef SubParserMap::const_iterator SubParserMapConstIter;
typedef std::pair<std::string, AJACommandLineParser*> AJASubParserPair;

struct OptionEntry {
	int pos;
	std::string value;
};
typedef std::map<std::string, OptionEntry> OptionEntryMap;

typedef enum {
	kShortOptionsAsLong = 1 << 0,
} AJACommandLineParserFlags;

/**
 *	Class that represents a command line args parser.
 */
class AJA_EXPORT AJACommandLineParser
{
public:
	explicit AJACommandLineParser(int flags=0);
	explicit AJACommandLineParser(const std::string &name, int flags=0);
	AJACommandLineParser(const AJACommandLineParser &other);
	~AJACommandLineParser();
	void operator=(const AJACommandLineParser &other);

	void Reset(bool clearAll=false);
	void Dump();

	bool OptionByName(const std::string &name, AJACommandLineOption &opt) const;
	/**
	 * Register another AJACommandLineParser instance with this parser.
	 *
	 * @param[in] sp	The subparser to add.
	 *
	 * @return	Returns `true` if the subparser was added successfully, otherwise `false`.
	 */
	bool AddSubParser(AJACommandLineParser *sp);
	/**
	 * Parse a list of strings containing command line args.
	 *
	 * @param[in] desc	The list of arg strings.
	 *
	 * @return	Returns `true` if the command line args were parsed successfully, otherwise `false`.
	 */
	void ParseArgs(const AJAStringList &args);
	/**
	 * Parse a list of strings containing command line args.
	 *
	 * @param[in] argc	The arg count.
	 * @param[in] argv	The list of arg strings.
	 */
	void ParseArgs(int argc, char *argv[]);
	/**
	 * Tests if the specified arg was set on the command line.
	 *
	 * @param[in] name	The arg name.
	 *
	 * @return	Returns `true` if the arg was set, otherwise `false`.
	 */
	bool IsSet(const std::string &name) const;
	/**
	 * Get a variant representing the value of the specified arg name.
	 *
	 * @param[in] name	The arg name.
	 * @param[in] index The index of the value to get.
	 *
	 * @return	AJAVariant representing the arg value, if the arg was set, otherwise 0.
	 */
	AJAVariant Value(const std::string &name, size_t index = 0) const;
	/**
	 * Get a list of variants representing the values of the specified arg name.
	 *
	 * @param[in] name	The arg name.
	 *
	 * @return	AJAVariantList of values, empty if no values set.
	 */
	AJAVariantList Values(const std::string &name) const;
	/**
	 * Get the string value for the specified arg name.
	 *
	 * @param[in] name	The arg name.
	 * @param[in] index The index of value to get.
	 *
	 * @return	std::string value of the arg, if set, otherwise empty std::string.
	 */
	std::string ValueString(const std::string &name, size_t index = 0) const;
	/**
	 * Get a list of string values for the specified arg name.
	 *
	 * @param[in] name	The arg name.
	 *
	 * @return	AJAStringList of string values, empty if no values set.
	 */
	AJAStringList ValueStrings(const std::string &name) const;
	/**
	 * Add a command line option to this args parser.
	 * This method will fail if another option with the
	 * same name has already been added to this parser.
	 *
	 * @param[in] option	The command line option to add.
	 *
	 * @return	`true` if option was added successfully, otherwise `false`.
	 */
	bool AddOption(const AJACommandLineOption &option);
	/**
	 * Add a list of command line options to this args parser.
	 * This method will fail if the list contains an option whose
	 * name has already been added to the args parser.
	 *
	 * @param[in] options	The list of command line options to add.
	 *
	 * @return	`true` if options are added successfully, otherwise `false`.
	 */
	bool AddOptions(const std::vector<AJACommandLineOption> &options);
	/**
	 * Add help option to this args parser.
	 * If -h/--help is specified from the command line, will print the help string.
	 */
	bool AddHelpOption();
	/**
	 * Get the name of this command line args parser.
	 *
	 * @return	The name of the parser, if any has been set, otherwise empty string.
	 */
	std::string GetName() const;
	/**
	 * Set the usage text string to print if help is invoked by the args parser.
	 *
	 * @param[in] name	The usage text string.
	 */
	void SetUsageText(const std::string &usageText);
	/**
	 * Get the usage text string for this args parser.
	 *
	 * @return	The usage text string, if set, otherwise empty string.
	 */
	std::string GetUsageText() const;
	/**
	 * Set the help text string to print if help is invoked by the args parser.
	 *
	 * @param[in] name	The help text string.
	 */
	void SetHelpText(const std::string &helpText);
	/**
	 * Get the help text string for this args parser.
	 *
	 * @return	The help text string, if set, otherwise empty string.
	 */
	std::string GetHelpText() const;
	/**
	 * Get the name of the command (if-any) represented by this args parser.
	 * 
	 * @return The command name string, if set, otherwise empty string.
	 */
	std::string GetCommandName();
	/**
	 * Get the list of known options parsed by this args parser from the command line.
	 * 
	 * @return A list of strings parsed from the command line recognized by this parser.
	 */
	AJAStringList GetKnownOptions();
	/**
	 * Get the list of unknown options parsed by this args parser from the command line.
	 * 
	 * @return A list of strings parsed from the command line not recognized by this parser.
	 */
	AJAStringList GetUnknownOptions();

private:
	static bool hasOptionPrefix(const std::string &name);
	bool reverseOptionSearch(AJAStringListConstIter *iter, const AJAStringList &args, const std::string &arg, int prefixSize, AJACommandLineOption &opt);
	bool parseValue(const std::string &arg, std::string &value);
	bool parseOptionValue(const std::string &name, const std::string &arg, AJAStringList::const_iterator *iter, AJAStringList::const_iterator end);
	bool setOptionValue(const std::string &name, const std::string &value);
	std::string removePrefix(const std::string &name, int &count);

	int mFlags;
	std::string mName;
	std::string mCommandName;
	std::string mDesc;
	std::string mDescExtra;
	std::string mUsageText;
	std::string mHelpText;
	AJACommandLineOptionList mOptions;
	AJAStringList mKnownOptions;
	AJAStringList mUnknownOptions;
	SubParserMap mSubParsers;
};

#endif // AJA_COMMANDLINE_H
