/* SPDX-License-Identifier: MIT */
/**
	@file		commandline.h
	@brief		Declaration of Command Line classes
	@copyright	(C) 2011-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_COMMANDLINE_H
#define AJA_COMMANDLINE_H

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
 *	Class that represents a command-line option.
 */
class AJA_EXPORT AJACommandLineOption
{
public:
	AJACommandLineOption();
	AJACommandLineOption(const std::string &name,
						 const std::string &desc = "",
						 const std::string &defaultValue = "");
	AJACommandLineOption(const AJAStringList &names,
						 const std::string &desc = "",
						 const std::string &defaultValue = "");
	virtual ~AJACommandLineOption();

	/**
	 * Add an arg name to this AJACommandLineOption.
	 *
	 * @param[in] name	The arg name to add.
	 */
	bool AddName(const std::string &name);

	/**
	 * Get list of arg names attached to this AJACommandLineOption.
	 *
	 * @return	The list of names.
	 */
	AJAStringList Names() const;

	/**
	 * Determine if this AJACommandLineOption contains the specified name.
	 *
	 * @param[in] name	The option name to check.
	 *
	 * @return	Returns true if the option name is found, otherwise false.
	 */
	bool HaveName(const std::string &name) const;

	/**
	 * Get the description string for this AJACommandLineOption.
	 *
	 * @return	The description string.
	 */
	std::string Desc() const;

	/**
	 * Set the description string for this AJACommandLineOption.
	 *
	 * @param[in] desc	The description string.
	 */
	void SetDesc(const std::string &desc);

	/**
	 * Get the extra description string for this AJACommandLineOption.
	 *
	 * @return	The extra description string.
	 */
	std::string ExtraDesc() const;

	/**
	 * Set the extra description string for this AJACommandLineOption.
	 *
	 * @param[in] desc	The extra description string.
	 */
	void SetExtraDesc(const std::string &desc);

	/**
	 * Set the default value string for this AJACommandLineOption.
	 *
	 * @param[in] desc	The default value string.
	 */
	void SetDefaultValue(const std::string &value);

	/**
	 * Get the default value string for this AJACommandLineOption.
	 *
	 * @return	The default value string.
	 */
	std::string DefaultValue() const;

	/**
	 * Add a value string to this AJACommandLineOption.
	 *
	 * @param[in] value	The value string.
	 */
	void AddValue(const std::string &value);

	/**
	 * Get value string at the specified index from this AJACommandLineOption.
	 *
	 * @param[in] index	The index of the value to retrieve.
	 *
	 * @return	The default value string.
	 */
	std::string Value(size_t index = 0) const;

	/**
	 * Get all value strings from this AJACommandLineOption.
	 *
	 * @return	The AJAStringList of all value strings.
	 */
	AJAStringList Values() const;

	/**
	 * Reset values and "set" state of this AJACommandLineOption.
	 */
	void Reset();

	/**
	 * Get all value strings from this AJACommandLineOption.
	 *
	 * @return	Returns true if the option is set, otherwise return false.
	 */
	bool IsSet() const;

	/**
	 * Mark this AJACommandLineOption as "set", or enabled.
	 * Typically used by an AJACommandLineParser instance to flag specific options
	 * as having been set at the command-line, at the time the arguments are parsed.
	 *
	 * @param[in] isSet	A boolean representing the enabled state of the option.
	 */
	void MarkSet(bool isSet = true);

private:
	AJAStringList _names;
	std::string _desc;
	std::string _descExtra;
	AJAStringList _values;
	std::string _defaultValue;
	bool _isSet;
};

typedef AJACommandLineOption	AJACmdLineOption;

typedef std::vector<AJACommandLineOption> AJACommandLineOptionList;
typedef AJACommandLineOptionList::const_iterator AJACommandLineOptionListIter;

class AJACommandLineParser;
typedef std::map<std::string, AJACommandLineParser *> SubParserMap;
typedef SubParserMap::const_iterator SubParserMapConstIter;
typedef std::pair<std::string, AJACommandLineParser*> AJASubParserPair;

typedef enum {
	kShortOptionsAsLong = 1 << 0,
	kNoDefaultHelpOption = 1 << 1,
	kNoDefaultUsageOption = 1 << 2,
} AJACommandLineParserFlags;

/**
 *	Class that represents a command-line args parser.
 */
class AJA_EXPORT AJACommandLineParser
{
public:
	explicit AJACommandLineParser(int flags=0);
	explicit AJACommandLineParser(const std::string &name, int flags=0);
	AJACommandLineParser(const AJACommandLineParser &other);
	~AJACommandLineParser();
	void operator=(const AJACommandLineParser &other);

	/**
	 * Delete all sub-parsers and options from this AJACommandLineParser.
	 */
	void Clear();

	/**
	 * Reset the values and "set" state of all sub-parsers and options in this AJACommandLineParser.
	 *
	 * @param[in] clearAll	Delete all sub-parsers and options instead of resetting them?
	 */
	void Reset(bool clearAll=false);

	/**
	 * Dump the contents of this AJACommandLineParser to stdout.
	 */
	void Dump();

	/**
	 * Determine this AJACommandLineParser has an option with the specified name.
	 *
	 * @param[in] name	The name of the option to look up.
	 *
	 * @return	Returns `true` if the option is found, otherwise `false`.
	 */
	bool HaveOption(const std::string &name) const;

	/**
	 * Determine this AJACommandLineParser has a positional arg.
	 *
	 * @param[in] name	The positional arg string to look up.
	 *
	 * @return	Returns `true` if the positional arg is found, otherwise `false`.
	 */
	bool HavePositionalArg(const std::string &name) const;

	/**
	 * Get a vector containing all positional args parsed from the command line args.
	 *
	 * @return	Returns an AJAStringList of all positional args found.
	 */
	AJAStringList PositionalArgs() const;

	/**
	 * Look up AJACommandLineOption by name in this AJACommandLineParser.
	 *
	 * @param[in] name	The name of the option to look up.
	 * @param[out] opt	The AJACommandLineOption to retrieve.
	 *
	 * @return	Returns `true` if the option is found, otherwise `false`.
	 */
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
	 * Parse a list of strings containing command-line args.
	 *
	 * @param[in] desc	The list of arg strings.
	 *
	 * @return	Returns `true` if the command-line args were parsed successfully, otherwise `false`.
	 */
	bool ParseArgs(const AJAStringList &args, bool errUnknown=false);

	/**
	 * Parse a list of strings containing command-line args.
	 *
	 * @param[in] argc	The arg count.
	 * @param[in] argv	The list of arg strings.
	 */
	bool ParseArgs(int argc, const char *argv[], bool errUnknown=false);

	/**
	 * Parse a list of strings containing command-line args.
	 *
	 * @param[in] argc	The arg count.
	 * @param[in] argv	The list of arg strings.
	 */
	bool ParseArgs(int argc, char *argv[], bool errUnknown=false);

	/**
	 * Tests if the specified arg was set on the command-line.
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
	 * Add a AJACommandLineOption to this args parser.
	 * This method will fail if another option with the
	 * same name has already been added to this parser.
	 *
	 * @param[in] option	The AJACommandLineOption to add.
	 *
	 * @return	`true` if option was added successfully, otherwise `false`.
	 */
	bool AddOption(const AJACommandLineOption &option);

	/**
	 * Add a list of AJACommandLineOptions to this args parser.
	 * This method will fail if the list contains an option whose
	 * name has already been added to the args parser.
	 *
	 * @param[in] options	The list of AJACommandLineOptions to add.
	 *
	 * @return	`true` if options are added successfully, otherwise `false`.
	 */
	bool AddOptions(const std::vector<AJACommandLineOption> &options);

	/**
	 * Add default -?/--help option to this args parser.
	 * The caller must check if IsSet("help"), and then call HelpText,
	 * in order to retrieve the help text string for printing to the console.
	 *
	 * @param[in] useShortName  Specify whether or not the help option should consume the '-h' shortname.
	 */
	bool AddHelpOption(bool useShortName=false);

	/**
	 * Add default --usage option to this args parser.
	 * The caller must check if IsSet("usage"), and then call GetUsageText,
	 * in order to retrieve the usage text string for printing to the console.
	 *
	 * @param[in] useShortName	Specify whether or not the usage option should consume the '-u' shortname.
	 */
	bool AddUsageOption(bool useShortName=false);

	/**
	 * Get the name of this command-line args parser.
	 *
	 * @return	The name of the parser, if any has been set, otherwise empty string.
	 */
	std::string Name() const;

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
	std::string UsageText();

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
	std::string HelpText();

	/**
	 * Get the name of the command (if-any) represented by this args parser.
	 *
	 * @return The command name string, if set, otherwise empty string.
	 */
	std::string CommandName();

private:
	static bool hasOptionPrefix(const std::string &name);
	static bool hasAssignmentOperator(const std::string &arg);

	void init();

	std::string generateHelpText() const;
	std::string generateUsageText() const;

	bool setOptionValue(const std::string &name, const std::string &value);
	bool setOption(const std::string &name, bool isSet = true);

	int _flags;
	std::string _name;
	std::string _commandName;
	std::string _desc;
	std::string _descExtra;
	std::string _usageText;
	std::string _helpText;
	AJACommandLineOptionList _options;
	AJAStringList _positionalArgs;
	SubParserMap _subParsers;
};

typedef AJACommandLineParser	AJACmdLineParser;

#endif // AJA_COMMANDLINE_H
