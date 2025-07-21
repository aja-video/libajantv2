/* SPDX-License-Identifier: MIT */
/**
	@file		commandline.cpp
	@brief		Definition of Command Line classes
	@copyright	(C) 2011-2022 AJA Video Systems, Inc.  All rights reserved.
**/
#include "ajabase/common/commandline.h"
#include "ajabase/common/common.h"
#include "ajabase/system/file_io.h"
#include <iomanip>
#include <iostream>
#include <sstream>

static const char kAssignChar = '=';
static const char kSingleDash = '-';
static const char *kDoubleDash = "--";

AJACommandLineOption::AJACommandLineOption()
: _names(),
  _desc(),
  _descExtra(),
  _values(),
  _defaultValue(),
  _isSet(false)
{}

AJACommandLineOption::AJACommandLineOption(const std::string &name,
                        const std::string &desc,
                        const std::string &defaultValue)
: _names(),
  _desc(desc),
  _descExtra(),
  _values(),
  _defaultValue(defaultValue),
  _isSet(false)
{
    AddName(name);
}

AJACommandLineOption::AJACommandLineOption(const AJAStringList &names,
                        const std::string &desc,
                        const std::string &defaultValue)
: _names(),
  _desc(desc),
  _descExtra(),
  _values(),
  _defaultValue(defaultValue),
  _isSet(false)
{
    for (const auto &name : names) {
        AddName(name);
    }
}

AJACommandLineOption::~AJACommandLineOption()
{
}

bool AJACommandLineOption::AddName(const std::string &name)
{
    bool haveName = false;
    for (size_t i = 0; i < _names.size(); i++) {
        if (_names.at(i) == name) {
            haveName = true;
            break;
        }
    }
    if (!haveName) {
        _names.push_back(name);
    }

    return haveName;
}

AJAStringList AJACommandLineOption::Names() const
{
    return _names;
}

bool AJACommandLineOption::HaveName(const std::string &name) const
{
    for (const auto &n : _names) {
        if (n == name) {
            return true;
        }
    }

    return false;
}

std::string AJACommandLineOption::Desc() const
{
    return _desc;
}

void AJACommandLineOption::SetDesc(const std::string &desc)
{
    _desc = desc;
}

std::string AJACommandLineOption::ExtraDesc() const
{
    return _descExtra;
}

void AJACommandLineOption::SetExtraDesc(const std::string &desc)
{
    _descExtra = desc;
}


void AJACommandLineOption::SetDefaultValue(const std::string &value)
{
    _defaultValue = value;
}

std::string AJACommandLineOption::DefaultValue() const
{
    return _defaultValue;
}

void AJACommandLineOption::AddValue(const std::string &value)
{
    _values.push_back(value);
}

std::string AJACommandLineOption::Value(size_t index) const
{
    if (index > _values.size() || _values.empty()) {
        return "";
    }
    return _values.at(index);
}

AJAStringList AJACommandLineOption::Values() const
{
    return _values;
}

void AJACommandLineOption::Reset()
{
    _values.clear();
    _isSet = false;
}

bool AJACommandLineOption::IsSet() const
{
    return _isSet;
}
void AJACommandLineOption::MarkSet(bool isSet)
{
    _isSet = isSet;
}

AJACommandLineParser::AJACommandLineParser(int flags)
: _flags(flags), _name(), _commandName(), _desc(), _descExtra(), _usageText(), _helpText(),
  _options(), _subParsers()
{
    init();
}

AJACommandLineParser::AJACommandLineParser(const std::string &name, int flags)
: _flags(flags), _name(name), _commandName(), _desc(), _descExtra(), _usageText(), _helpText(),
  _options(), _subParsers()
{
    init();
}

AJACommandLineParser::AJACommandLineParser(const AJACommandLineParser &other)
{
    operator=(other);
}

AJACommandLineParser::~AJACommandLineParser()
{
}

void AJACommandLineParser::operator=(const AJACommandLineParser &other)
{
	_flags = other._flags;
	_name = other._name;
	_commandName = other._commandName;
	_desc = other._desc;
	_descExtra = other._descExtra;
	_usageText = other._usageText;
	_helpText = other._helpText;
	_options = other._options;
    _subParsers.clear();
    for (SubParserMapConstIter iter = other._subParsers.begin(); iter != other._subParsers.end(); ++iter) {
        _subParsers.insert(AJASubParserPair(iter->first, iter->second));
    }
}

void AJACommandLineParser::init()
{
    if ((_flags & kNoDefaultHelpOption) == 0) {
        AddHelpOption();
    }
    if ((_flags & kNoDefaultUsageOption) == 0) {
        AddUsageOption();
    }
}

void AJACommandLineParser::Clear()
{
    _subParsers.clear();
    _options.clear();
}

void AJACommandLineParser::Reset(bool clearAll)
{
    if (clearAll) {
        Clear();
    } else {
        for (auto &sp : _subParsers) {
            if (sp.second) {
                sp.second->Reset();
            }
        }
        for (auto &opt : _options) {
            opt.Reset();
        }
    }
}

void AJACommandLineParser::Dump()
{
    if (!_commandName.empty()) {
        AJACommandLineParser *sp = _subParsers.at(_commandName);
        if (sp != NULL) {
            return sp->Dump();
        }
    } else {
        for (AJACommandLineOptionListIter iter = _options.begin();
            iter != _options.end(); ++iter) {
            const AJACommandLineOption &o = *iter;
            const AJAStringList & names = o.Names();
            std::ostringstream oss;
            oss << "[";
            std::string name;
            size_t count = 0;
            for (AJAStringListConstIter sIter = names.begin(); sIter != names.end(); ++sIter) {
                name = *sIter;
                oss << name;
                if (++count < names.size()) {
                    oss << ", ";
                }
            }
            oss << "] " << "set? " << (IsSet(name) ? "true" : "false") << " value = " << o.Value();
            std::cout << oss.str() << std::endl;
        }
    }
}

bool AJACommandLineParser::HaveOption(const std::string &name) const
{
    for (const auto &opt : _options) {
        for (const auto &n : opt.Names()) {
            if (n == name) {
                return true;
            }
        }
    }
    return false;
}

bool AJACommandLineParser::HavePositionalArg(const std::string &name) const
{
    for (const auto &arg : _positionalArgs) {
        if (arg == name) {
            return true;
        }
    }
    return false;
}

AJAStringList AJACommandLineParser::PositionalArgs() const
{
    return _positionalArgs;
}

bool AJACommandLineParser::OptionByName(const std::string &name, AJACommandLineOption &opt) const
{
    for (const auto &o : _options) {
        for (const auto &n : o.Names()) {
            if (n == name) {
                opt = o;
                return true;
            }
        }
    }
    return false;
}

bool AJACommandLineParser::AddSubParser(AJACommandLineParser *p)
{
    const std::string &name = p->Name();
    if (_subParsers.find(name) == _subParsers.end()) {
        _subParsers.insert(AJASubParserPair(name, p));
        return true;
    }

    return false;
}

bool AJACommandLineParser::Parse(const AJAStringList &args)
{
    // Must have at least 2 args (args[0] is the binary name, and args[1..N] are the user-specified args).
    if (args.size() <= 1) {
        return false;
    }

    AJAStringList::const_iterator iter = args.begin();
    ++iter; //  Skip exe path

    for (SubParserMap::iterator spIter = _subParsers.begin(); spIter != _subParsers.end(); ++spIter) {
        // Is second arg a command name which belongs to a sub-parser?
        if (*iter == spIter->first) {
            _commandName = *iter;
        }
        // Iterate all args with all sub-parsers...
        if (spIter->second != NULL) {
            if (!spIter->second->Parse(args)) {
                std::cerr << "Error parsing args with sub-parser: " << spIter->second->Name() << std::endl;
                return false;
            }
        }
    }

    // If the the parser name is set, expect the second arg to match (i.e. we are in a sub-parser).
    // For example, the following case "name" would be "theCommand".
    // If the second arg doesn't match the name, the sub-parser name is simply not present in the
    // list of args provided. This is not an error condition.
    // > MyApp.exe theCommand -d1 -n3 --verbose
    if (!_name.empty() && *iter != _name) {
        return true;
    }

    // ...otherwise just parse the args.
    bool havePositionalArgs = false;
    for (; iter != args.end(); iter++) {
        const std::string &arg = *iter;
        // Treat subsequent args as "positional"
        if (!havePositionalArgs && arg == kDoubleDash) {
            havePositionalArgs = true;
            continue;
        }

        if (havePositionalArgs) {
            _positionalArgs.push_back(arg);
            continue;
        }

        if (hasOptionPrefix(arg)) {
            AJACommandLineOption opt;
            std::string optName;
            std::string optValue;
            std::string argStr;
            bool doubleDash = aja::starts_with(arg, kDoubleDash);
            bool singleDash = aja::starts_with(arg, kSingleDash) && !doubleDash;
            if (doubleDash) {
                argStr = arg.substr(2, arg.length());
            } else if (singleDash) {
                argStr = arg.substr(1, arg.length());
            }
            if (hasAssignmentOperator(argStr)) {
                size_t assignPos = argStr.find_first_of(kAssignChar);
                optName  = argStr.substr(0, assignPos);
                optValue = argStr.substr(assignPos+1, argStr.length());
            } else {
                // Single-dash cases we need to handle:
                // example args: {a, apple}, {b, ball}, {c, cat}, {r, car}
                // -a=1
                // -a1
                // -a 1
                // -abc // SPECIAL CASE: short opts as long opts
                // Should we even allow setting a value for a single-dash arg if the arg name is longer than 1 character?
                // i.e. should -devicekona5 be legal? seems weird to me...
                if (singleDash) { /* -argname */
                    /* handle single-dash option */
                    std::string subStr;
                    if (_flags & kShortOptionsAsLong) {
                        for (size_t c = 1; c < arg.length(); c++) {
                            subStr = arg.substr(c, 1);
                            if (HaveOption(subStr))
                                setOption(subStr, true);
                        }
                        continue;
                    } else {
                        for (size_t c = arg.length(); c > 1; c--) {
                            subStr = arg.substr(1, c-1);
                            if (HaveOption(subStr)) {
                                optName = subStr;
                                size_t count = (arg.length()-1)-optName.length();
                                optValue = arg.substr(optName.length()+1, count);
                                break;
                            } else if (_flags & kErrorOnUnknownArgs) {
                                std::cerr << "Unknown arg: " << subStr << std::endl;
                                return false;
                            }
                        }
                    }
                } else { /* --argname */
                    optName = argStr;
                }

                // If we have no value, and the next arg is not another arg, use it as the value for the current arg.
                if (optValue.empty()) {
                    auto nextIter = iter;
                    nextIter++;
                    if (nextIter != args.end()) {
                        std::string nextArg = *(nextIter++);
                        if (!hasOptionPrefix(nextArg)) {
                            optValue = nextArg;
                        }
                    }
                }
            }   //  else no assignment operator

            if (HaveOption(optName)) {
                if (!optValue.empty()) {
                    setOptionValue(optName, optValue);
                }
                setOption(optName, true);
            } else if (_flags & kErrorOnUnknownArgs) {
                std::cerr << "Unknown arg: " << optName << std::endl;
                return false;
            }
        }   //  if hasOptionPrefix
    }   //  for each arg

    if (!(_flags & kNoDefaultHelpOption))
        if (IsSet("help"))
            {std::cout << HelpText();  return false;}
        
    if (!(_flags & kNoDefaultUsageOption))
        if (IsSet("usage"))
            {std::cout << UsageText();  return false;}

    return true;
}   //  Parse (AJAStringList)

bool AJACommandLineParser::Parse(int argc, const char *argv[])
{
    if (argc == 0 || argc == 1 || argv == NULL) {
        return false;
    }
    AJAStringList argList;
    for (int i = 0; i < argc; i++) {
        argList.push_back(std::string(argv[i]));
    }
    return Parse(argList);
}

bool AJACommandLineParser::Parse(int argc, char *argv[])
{
    if (argc == 0 || argc == 1 || argv == NULL) {
        return false;
    }
    AJAStringList argList;
    for (int i = 0; i < argc; i++) {
        argList.push_back(std::string(argv[i]));
    }
    return Parse(argList);
}

bool AJACommandLineParser::IsSet(const std::string &name) const
{
    if (!_commandName.empty()) {
        AJACommandLineParser *sp = _subParsers.at(_commandName);
        if (sp != NULL) {
            return sp->IsSet(name);
        }
    } else {
        AJACommandLineOption opt;
        if (OptionByName(name, opt)) {
            return opt.IsSet();
        }
    }
    return false;
}

AJAVariant AJACommandLineParser::Value(const std::string &name, size_t index) const
{
    return AJAVariant(ValueString(name, index));
}

AJAVariantList AJACommandLineParser::Values(const std::string &name) const
{
    AJAStringList values = ValueStrings(name);
    if (!values.empty()) {
        AJAVariantList variants;
        for (AJAStringListConstIter it = values.begin(); it != values.end(); ++it) {
            variants.push_back(AJAVariant(*it));
        }
        return variants;
    }
    return AJAVariantList();
}

std::string AJACommandLineParser::ValueString(const std::string &name, size_t index) const
{
    std::string val;
    if (!_commandName.empty()) {
        AJACommandLineParser *sp = _subParsers.at(_commandName);
        if (sp != NULL) {
            return sp->ValueString(name, index);
        }
    } else {
        AJACommandLineOption opt;
        if (OptionByName(name, opt)) {
            val = opt.Value(index);
            if (val.empty()) {
                val = opt.DefaultValue();
            }
        }
    }

    return val;
}

AJAStringList AJACommandLineParser::ValueStrings(const std::string &name) const
{
    if (!_commandName.empty()) {
        AJACommandLineParser *sp = _subParsers.at(_commandName);
        if (sp != NULL) {
            return sp->ValueStrings(name);
        }
    } else {
        AJACommandLineOption opt;
        if (OptionByName(name, opt)) {
            return opt.Values();
        }
    }
    return AJAStringList();
}

bool AJACommandLineParser::AddOption(const AJACommandLineOption &option)
{
    bool exists = false;
    const AJAStringList &wantNames = option.Names();
    for (AJACommandLineOptionListIter optIter = _options.begin(); optIter != _options.end(); ++optIter) {
        const AJAStringList &names = optIter->Names();
        for (AJAStringListConstIter nameIter = names.begin(); nameIter != names.end(); ++nameIter) {
            for (AJAStringListConstIter wantIter = wantNames.begin(); wantIter != wantNames.end(); ++wantIter) {
                if (*wantIter == *nameIter) {
                    exists = true;
                    goto next;
                }
            }
        }
    }

next:
    if (exists) {
        return false;
    } else {
        _options.push_back(option);
        return true;
    }
}

bool AJACommandLineParser::AddOptions(const AJACommandLineOptionList &options)
{
    size_t okCount = 0;
    for (size_t i = 0; i < options.size(); i++) {
        if (AddOption(options.at(i))) {
            ++okCount;
        }
    }
//	if (_Flags & kAutoProcessUsage)
//        AddOption(AJACommandLineOption("usage", "Show command usage"));
//	if (_Flags & (kAutoProcessHelp | kAutoProcessUsage))
//        AddHelpOption();
    return options.size() ? (okCount == options.size() ? true : false) : false;
}

bool AJACommandLineParser::AddHelpOption(bool useShortName)
{
    AJACommandLineOption helpOpt;
    helpOpt.AddName("?");
    if (useShortName) {
        helpOpt.AddName("h");
    }
    helpOpt.AddName("help");
    helpOpt.SetDesc("Print the help text");
    return AddOption(helpOpt);
}

bool AJACommandLineParser::AddUsageOption(bool useShortName)
{
    AJACommandLineOption usageOpt;
    usageOpt.AddName("usage");
    if (useShortName) {
        usageOpt.AddName("u");
    }
    usageOpt.SetDesc("Print the usage text");
    return AddOption(usageOpt);
}

std::string AJACommandLineParser::Name() const
{
    if (!_commandName.empty()) {
        AJACommandLineParser *sp = _subParsers.at(_commandName);
        if (sp != NULL) {
            return sp->Name();
        }
    }

    return _name;
}

void AJACommandLineParser::SetUsageText(const std::string &usageText)
{
    if (!_commandName.empty()) {
        AJACommandLineParser *sp = _subParsers.at(_commandName);
        if (sp != NULL) {
            sp->SetUsageText(usageText);
        }
    } else {
        _usageText = usageText;
    }
}

std::string AJACommandLineParser::UsageText()
{
    if (!_commandName.empty()) {
        AJACommandLineParser *sp = _subParsers.at(_commandName);
        if (sp != NULL) {
            return sp->UsageText();
        }
    }

    std::string usageText;
    if (!_usageText.empty()) {
        usageText = _usageText;
    } else {
        usageText = generateUsageText();
    }
    return usageText;
}

void AJACommandLineParser::SetHelpText(const std::string &helpText)
{
    if (!_commandName.empty()) {
        AJACommandLineParser *sp = _subParsers.at(_commandName);
        if (sp != NULL) {
            sp->SetHelpText(helpText);
        }
    } else {
        _helpText = helpText;
    }
}

std::string AJACommandLineParser::HelpText()
{
    if (!_commandName.empty()) {
        AJACommandLineParser *sp = _subParsers.at(_commandName);
        if (sp != NULL) {
            return sp->HelpText();
        }
    }

    std::string helpText;
    if (_helpText.empty()) {
        helpText = generateHelpText();
    } else {
        helpText = _helpText;
    }
    return helpText;
}

std::string AJACommandLineParser::generateHelpText() const
{
    std::ostringstream oss;
    std::string exePath;
    AJAFileIO::GetExecutablePath(exePath);
    oss << "Usage: " << exePath;
    if (!_name.empty()) {
        oss << " " << _name;
    }
    oss << " [OPTION...]" << std::endl;

    // Get the longest line size first...
    size_t longestSize = 0;
    for (AJACommandLineOptionListIter it = _options.begin();
        it != _options.end(); ++it) {
        const AJAStringList &names = it->Names();
        size_t namesLength = 0;
        for (AJAStringListConstIter sIter = names.begin(); sIter != names.end(); ++sIter) {
            const std::string &name = *sIter;
            namesLength += name.length();
            // add size of dashes
            if (name.length() == 1) {
                namesLength++;
            } else {
                namesLength += 2;
            }
        }
        // add size of commas/spaces (i.e. ", ")
        namesLength += ((names.size()*2)-2);
        if (namesLength > longestSize) {
            longestSize = namesLength;
        }
    }

    // ...now calculate all of the line padding.
    for (AJACommandLineOptionListIter it = _options.begin();
        it != _options.end(); ++it) {
        oss << std::setw(2) << std::right;
        const AJAStringList &names = it->Names();
        size_t nameCount = 0;
        size_t namesLength = 0;
        for (AJAStringListConstIter sIter = names.begin(); sIter != names.end(); ++sIter) {
            const std::string &name = *sIter;
            namesLength += name.length();
            if (name.length() == 1) {
                oss << "-" << name;
                namesLength++;
            } else {
                oss << "--" << name;
                namesLength += 2;
            }
            if (++nameCount < names.size()) {
                oss << ", ";
            }
        }
        namesLength += ((names.size()*2)-2);
        oss << std::setw((longestSize-namesLength) + it->Desc().length() + 8);
        oss << it->Desc() << std::endl;
    }

    return oss.str();
}

std::string AJACommandLineParser::generateUsageText() const
{
    std::ostringstream oss;
    std::string exePath;
    AJAFileIO::GetExecutablePath(exePath);
    oss << "Usage: " << exePath;
    if (!_name.empty()) {
        oss << " " << _name;
    }

    std::string abbrev;
    std::ostringstream usages;
    bool haveHelp = false;
    bool haveUsage = false;
    for (const auto &opt : _options) {
        if (opt.HaveName("?")) {
            haveHelp = true;
        } else if (opt.HaveName("usage")) {
            haveUsage = true;
        } else {
            auto names = opt.Names();
            usages << "\t[";
            for (size_t ndx = 0; ndx < names.size(); ndx++) {
                auto name = names.at(ndx);
                bool isShortName = name.length() == 1;
                if (isShortName) {
                    abbrev += name;
                    usages << '-' << name;
                } else {
                    usages << "--" << name;
                }
                if (ndx < names.size()-1) {
                     usages << '|';
                }
            }
            usages << ']' << std::endl;
        }
    }

    // Make sure help and usage options appear at the end
    if (haveHelp) {
        usages << "\t[" << "-?|--help]" << std::endl;
    }
    if (haveUsage) {
        usages << "\t[" << "--usage]" << std::endl;
    }

    if (!abbrev.empty()) {
        if (haveHelp) {
            abbrev += '?';
        }
        oss << " [-" << abbrev << "]" << std::endl;
    }
    oss << usages.str() << std::endl;

    if (!_subParsers.empty()) {
        oss << '\n' << "commands: " << std::endl;
        for (const auto &sp : _subParsers) {
            if (sp.second) {
                oss << "\t  " << sp.second->Name() << std::endl;
            }
        }
    }
    return oss.str();
}

std::string AJACommandLineParser::CommandName()
{
    return _commandName;
}

bool AJACommandLineParser::hasOptionPrefix(const std::string &name)
{
    return aja::starts_with(name, kSingleDash);
}

bool AJACommandLineParser::hasAssignmentOperator(const std::string &arg)
{
    size_t assignPos = arg.find_first_of(kAssignChar);
    return assignPos != std::string::npos;
}

bool AJACommandLineParser::setOptionValue(const std::string &name, const std::string &value)
{
    if (!value.empty()) {
        for (size_t i = 0; i < _options.size(); i++) {
            AJACommandLineOption opt = _options.at(i);
            const AJAStringList &names = opt.Names();
            for (AJAStringListConstIter iter = names.begin(); iter != names.end(); ++iter) {
                if (name == *iter) {
                    _options[i].AddValue(value);
                    return true;
                }
            }
        }
    }
    return false;
}

bool AJACommandLineParser::setOption(const std::string &name, bool isSet)
{
    for (size_t i = 0; i < _options.size(); i++) {
        AJACommandLineOption opt = _options.at(i);
        const AJAStringList &names = opt.Names();
        for (AJAStringListConstIter iter = names.begin(); iter != names.end(); ++iter) {
            if (name == *iter) {
                _options[i].MarkSet(isSet);
                return true;
            }
        }
    }
    return false;
}
