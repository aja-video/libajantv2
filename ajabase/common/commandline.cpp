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
: mNames(), mDesc(), mDescExtra(), mValues(), mDefaultValue()
{}

AJACommandLineOption::AJACommandLineOption(const std::string &name)
: mNames(), mDesc(), mDescExtra(), mValues(), mDefaultValue()
{
    AddName(name);
}

AJACommandLineOption::AJACommandLineOption(const std::string &name,
                                           const std::string &desc)
: mNames(), mDesc(desc), mDescExtra(), mValues(), mDefaultValue()
{
    AddName(name);
}

AJACommandLineOption::AJACommandLineOption(const std::string &name,
                        const std::string &desc,
                        const std::string &defaultValue)
: mNames(), mDesc(desc), mDescExtra(), mValues(), mDefaultValue(defaultValue)
{
    AddName(name);
}

AJACommandLineOption::AJACommandLineOption(const AJAStringList &names)
: mNames(), mDesc(), mDescExtra(), mValues(), mDefaultValue()
{
    for (size_t i = 0; i < names.size(); i++) {
        AddName(names.at(i));
    }
}

AJACommandLineOption::AJACommandLineOption(const AJAStringList &names,
                                           const std::string &desc)
: mNames(), mDesc(desc), mDescExtra(), mValues(), mDefaultValue()
{
    for (size_t i = 0; i < names.size(); i++) {
        AddName(names.at(i));
    }
}

AJACommandLineOption::AJACommandLineOption(const AJAStringList &names,
                                           const std::string &desc,
                                           const std::string &defaultValue)
: mNames(), mDesc(desc), mDescExtra(), mValues(), mDefaultValue()
{
    for (size_t i = 0; i < names.size(); i++) {
        AddName(names.at(i));
    }
    mDefaultValue = defaultValue;
}

AJACommandLineOption::~AJACommandLineOption()
{
}

bool AJACommandLineOption::AddName(const std::string &name)
{
    bool haveName = false;
    for (size_t i = 0; i < mNames.size(); i++) {
        if (mNames.at(i) == name) {
            haveName = true;
            break;
        }
    }
    if (!haveName) {
        mNames.push_back(name);
    }

    return haveName;
}

AJAStringList AJACommandLineOption::GetNames() const
{
    return mNames;
}

std::string AJACommandLineOption::GetDesc() const
{
    return mDesc;
}

void AJACommandLineOption::SetDesc(const std::string &desc)
{
    mDesc = desc;
}

std::string AJACommandLineOption::GetExtraDesc() const
{
    return mDescExtra;
}

void AJACommandLineOption::SetExtraDesc(const std::string &desc)
{
    mDescExtra = desc;
}


void AJACommandLineOption::SetDefaultValue(const std::string &value)
{
    mDefaultValue = value;
}

std::string AJACommandLineOption::GetDefaultValue() const
{
    return mDefaultValue;
}

void AJACommandLineOption::AddValue(const std::string &value)
{
    mValues.push_back(value);
}

std::string AJACommandLineOption::GetValue(size_t index) const
{
    if (index > mValues.size() || mValues.empty())
        return "";
    return mValues.at(index);
}

AJAStringList AJACommandLineOption::GetValues() const
{
    return mValues;
}

AJACommandLineParser::AJACommandLineParser(int flags)
: mFlags(flags), mName(), mCommandName(), mDesc(), mDescExtra(), mUsageText(), mHelpText(),
  mOptions(), mKnownOptions(), mUnknownOptions(), mSubParsers()
{
}

AJACommandLineParser::AJACommandLineParser(const std::string &name, int flags)
: mFlags(flags), mName(name), mCommandName(), mDesc(), mDescExtra(), mUsageText(), mHelpText(),
  mOptions(), mKnownOptions(), mUnknownOptions(), mSubParsers()
{
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
	mFlags = other.mFlags;
	mName = other.mName;
	mCommandName = other.mCommandName;
	mDesc = other.mDesc;
	mDescExtra = other.mDescExtra;
	mUsageText = other.mUsageText;
	mHelpText = other.mHelpText;
	mOptions = other.mOptions;
    mKnownOptions.clear();
	mKnownOptions = other.mKnownOptions;
    mUnknownOptions.clear();
	mUnknownOptions = other.mUnknownOptions;
    mSubParsers.clear();
    for (SubParserMapConstIter iter = other.mSubParsers.begin(); iter != other.mSubParsers.end(); ++iter) {
        mSubParsers.insert(AJASubParserPair(iter->first, iter->second));
    }
}

void AJACommandLineParser::Reset(bool clearAll)
{
    mKnownOptions.clear();
    mUnknownOptions.clear();
    if (clearAll) {
        mSubParsers.clear();
        mOptions.clear();
    }
}

void AJACommandLineParser::Dump()
{
    if (!mCommandName.empty()) {
        AJACommandLineParser *sp = mSubParsers.at(mCommandName);
        if (sp != NULL) {
            return sp->Dump();
        }
    } else {
        for (AJACommandLineOptionListIter iter = mOptions.begin();
            iter != mOptions.end(); ++iter) {
            const AJACommandLineOption &o = *iter;
            const AJAStringList & names = o.GetNames();
            std::ostringstream oss;
            oss << "[";
            std::string name;
            size_t count = 0;
            for (AJAStringListConstIter sIter = names.begin(); sIter != names.end(); ++sIter) {
                name = *sIter;
                oss << name;
                if (++count < names.size())
                    oss << ", ";
            }
            oss << "] " << "set? " << (IsSet(name) ? "true" : "false") << " value = " << o.GetValue();
            std::cout << oss.str() << std::endl;
        }
    }
}

bool AJACommandLineParser::OptionByName(const std::string &name, AJACommandLineOption &opt) const
{
    for (AJACommandLineOptionListIter iter = mOptions.begin();
        iter != mOptions.end(); ++iter) {
        const AJACommandLineOption &o = *iter;
        const AJAStringList &names = o.GetNames();
        for (AJAStringListConstIter nameIter = names.begin(); nameIter != names.end(); ++nameIter) {
            if (name == *nameIter) {
                opt = o;
                return true;
            }
        }
    }

    return false;
}

bool AJACommandLineParser::AddSubParser(AJACommandLineParser *p)
{
    const std::string &name = p->GetName();
    if (mSubParsers.find(name) == mSubParsers.end()) {
        mSubParsers.insert(AJASubParserPair(name, p));
        return true;
    }

    return false;
}

bool AJACommandLineParser::reverseOptionSearch(AJAStringListConstIter *iter,
    const AJAStringList &args, const std::string &arg,
    int prefixSize, AJACommandLineOption &opt)
{
    if (static_cast<int>(arg.length()) > prefixSize) {
        std::string subStr;
        for (size_t c = arg.length(); static_cast<int>(c) > prefixSize; c--) {
            subStr = arg.substr(prefixSize, c-prefixSize);
            if (OptionByName(subStr, opt)) {
                parseOptionValue(subStr, arg, iter, args.end());
                mKnownOptions.push_back(subStr);
                return true;
            } else {
                mUnknownOptions.push_back(subStr);
            }
        }
    }
    return false;
}

int AJACommandLineParser::ParseArgs(const AJAStringList &args)
{
    // Must have at least 2 args (args[0] is the binary name, and args[1..N] are the user-specified args).
    if (args.size() <= 1)
        return 2;

    AJAStringList::const_iterator iter = args.begin();
    ++iter; //  Skip exe path

    for (SubParserMap::iterator spIter = mSubParsers.begin(); spIter != mSubParsers.end(); ++spIter) {
        // Is second arg a command name which belongs to a sub-parser?
        if (*iter == spIter->first)
            mCommandName = *iter;
        // Iterate all args with all sub-parsers...
        if (spIter->second != NULL) {
            spIter->second->ParseArgs(args);
        }
    }

    // If the parser name is specified, expect 2nd arg to match.
    // The parser name is treated as a "sub-command name", eg.
    // > MyApp.exe theCommand -d1 -n3 --verbose
    if ((*iter != mName) && !mName.empty())
        return 3;

    // ...otherwise just parse the args.
    for (; iter != args.end(); ++iter) {
        const std::string &arg = *iter;
        AJACommandLineOption opt;
        std::string optValue;
        if (aja::starts_with(arg, kDoubleDash)) {
            reverseOptionSearch(&iter, args, arg, 2, opt);
            // if (arg.length() > 2) {
            //     std::string subStr;
            //     // for (size_t c = 2; c < arg.length(); c++) {
            //     for (size_t c = arg.length(); c > 2; c--) {
            //         // subStr += arg.substr(c, 1);
            //         subStr = arg.substr(2, c-2);
            //         if (OptionByName(subStr, opt)) {
            //             parseOptionValue(subStr, arg, &iter, args.end());
            //             mKnownOptions.push_back(subStr);
            //             break;
            //         } else {
            //             mUnknownOptions.push_back(subStr);
            //         }
            //     }
            // }
        } else if (aja::starts_with(arg, kSingleDash)) {
            if (arg.length() > 1) {
                std::string subStr;
                if (mFlags & kShortOptionsAsLong) {
                    for (size_t c = 1; c < arg.length(); c++) {
                        subStr = arg.substr(c, 1);
                        if (OptionByName(subStr, opt)) {
                            mKnownOptions.push_back(subStr);
                        } else {
                            mUnknownOptions.push_back(subStr);
                        }
                    }
                } else {
                    reverseOptionSearch(&iter, args, arg, 1, opt);
                    // for (size_t c = 1; c < arg.length(); c++) {
                    //     subStr += arg.substr(c, 1);
                    //     if (OptionByName(subStr, opt)) {
                    //         parseOptionValue(subStr, arg, &iter, args.end());
                    //         mKnownOptions.push_back(subStr);
                    //         break;
                    //     } else {
                    //         mUnknownOptions.push_back(subStr);
                    //     }
                    // }
                }
            }
        } else {
            // positional args?
        }
    }
    if (mFlags & kAutoProcessHelp)
        if (!GetHelpText().empty() && IsSet("help"))
            {std::cout << GetHelpText() << std::endl;  return 1;}
    if (mFlags & kAutoProcessUsage)
        if (!GetHelpText().empty() && IsSet("usage"))
            {std::cout << GetUsageText() << std::endl;  return 1;}
    return 0;
}

int AJACommandLineParser::ParseArgs(int argc, const char *argv[])
{
    if (argc == 0 || argc == 1 || argv == NULL)
        return 2;

    AJAStringList argList;
    for (int i = 0; i < argc; i++)
        argList.push_back(std::string(argv[i]));

    return ParseArgs(argList);
}

int AJACommandLineParser::ParseArgs(int argc, char *argv[])
{
    if (argc == 0 || argc == 1 || argv == NULL)
        return 2;

    AJAStringList argList;
    for (int i = 0; i < argc; i++)
        argList.push_back(std::string(argv[i]));

    return ParseArgs(argList);
}

bool AJACommandLineParser::IsSet(const std::string &name) const
{
    if (!mCommandName.empty()) {
        AJACommandLineParser *sp = mSubParsers.at(mCommandName);
        if (sp != NULL) {
            return sp->IsSet(name);
        }
    } else {
        AJACommandLineOption opt;
        if (OptionByName(name, opt)) {
            const AJAStringList &names = opt.GetNames();
            for (AJAStringListConstIter nameIt = names.begin(); nameIt != names.end(); ++nameIt) {
                for (AJAStringListConstIter it = mKnownOptions.begin(); it != mKnownOptions.end(); ++it) {
                    if (*nameIt == *it)
                        return true;
                }
            }
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
    if (!mCommandName.empty()) {
        AJACommandLineParser *sp = mSubParsers.at(mCommandName);
        if (sp != NULL) {
            return sp->ValueString(name, index);
        }
    } else {
        AJACommandLineOption opt;
        if (OptionByName(name, opt)) {
            val = opt.GetValue(index);
            if (val.empty()) {
                val = opt.GetDefaultValue();
            }
        }
    }

    return val;
}

AJAStringList AJACommandLineParser::ValueStrings(const std::string &name) const
{
    if (!mCommandName.empty()) {
        AJACommandLineParser *sp = mSubParsers.at(mCommandName);
        if (sp != NULL) {
            return sp->ValueStrings(name);
        }
    } else {
        AJACommandLineOption opt;
        if (OptionByName(name, opt)) {
            return opt.GetValues();
        }
    }
    return AJAStringList();
}

bool AJACommandLineParser::AddOption(const AJACommandLineOption &option)
{
    bool exists = false;
    const AJAStringList &wantNames = option.GetNames();
    for (AJACommandLineOptionListIter optIter = mOptions.begin(); optIter != mOptions.end(); ++optIter) {
        const AJAStringList &names = optIter->GetNames();
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
        mOptions.push_back(option);
        return true;
    }
}

bool AJACommandLineParser::AddOptions(const AJACommandLineOptionList &options)
{
    size_t okCount = 0;
    for (size_t i = 0; i < options.size(); i++) {
        if (AddOption(options.at(i)))
            ++okCount;
    }
	if (mFlags & kAutoProcessUsage)
        AddOption(AJACommandLineOption("usage", "Show command usage"));
	if (mFlags & (kAutoProcessHelp | kAutoProcessUsage))
        AddHelpOption();
    return options.size() ? (okCount == options.size() ? true : false) : false;
}

bool AJACommandLineParser::AddHelpOption()
{
    AJACommandLineOption helpOpt;
    helpOpt.AddName("?");
    helpOpt.AddName("h");
    helpOpt.AddName("help");
    helpOpt.SetDesc("Print the help text");
    if (AddOption(helpOpt)) {
        std::ostringstream oss, usg;
        std::string exePath, exeFileName;
        AJAFileIO::GetExecutablePath(exePath);
        AJAFileIO::GetFileName(exePath, exeFileName);
        oss << "usage: " << exePath;
        if (!mName.empty())
            oss << " " << mName;
        oss << " [OPTION...]" << std::endl;

        // Get the longest line size first...
        size_t longestSize = 0;
        for (AJACommandLineOptionListIter it = mOptions.begin();
            it != mOptions.end(); ++it) {
            const AJAStringList &names = it->GetNames();
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
            if (namesLength > longestSize)
                longestSize = namesLength;
        }

        mHelpText.clear();

        // ...now calculate all of the line padding.
        for (AJACommandLineOptionListIter it = mOptions.begin();
            it != mOptions.end(); ++it) {
            oss << std::setw(2) << std::right;
            const AJAStringList names = it->GetNames();
            size_t nameCount(0), namesLength(0);
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
            oss << std::setw((longestSize-namesLength) + it->GetDesc().length() + 8);
            oss << it->GetDesc() << std::endl;
        }

        mHelpText = oss.str();

        if (mFlags & kAutoProcessUsage)
        {
            mUsageText = mHelpText;

            //  Transform help text into usage text...
            aja::replace(mUsageText, "usage:", "Usage:");
            aja::replace(mUsageText, " [OPTION...]", "");
            aja::replace(mUsageText, exePath, exeFileName);
            aja::replace(mUsageText, ", ", "|");
            AJAStringList before(aja::split(mUsageText, "\n")), after;
            for (size_t n(0); n < before.size(); n++) {
                std::string line(before.at(n));
                size_t pos(line.find("        "));
                if (pos == std::string::npos)
                    {after.push_back(line);  continue;}
                line.erase(pos, line.length() - pos);
                line += "]";
                if (line.at(0) == ' ')
                    line[0] = '[';
                else
                    line = '[' + line;
                after.push_back(line);
            }
            mUsageText.clear();
            std::ostringstream usg;
            size_t len(0);
            for (size_t n(0);  n < after.size();  ) {
                if (len + 1 + after.at(n).length() < 70) {
                    len += 1 + after.at(n).length();
                    usg << std::string(n ? " " : "") << after.at(n);
                }
                else
                {
                    usg << std::endl << std::string(after.at(0).length() + 1, ' ') << after.at(n);
                    len = after.at(0).length() + 1 + after.at(n).length();
                }
                ++n;
            }
            mUsageText = usg.str();
        }
        return true;
    }

    return false;
}

std::string AJACommandLineParser::GetName() const
{
    if (!mCommandName.empty()) {
        AJACommandLineParser *sp = mSubParsers.at(mCommandName);
        if (sp != NULL) {
            return sp->GetName();
        }
    }

    return mName;
}

void AJACommandLineParser::SetUsageText(const std::string &usageText)
{
    if (!mCommandName.empty()) {
        AJACommandLineParser *sp = mSubParsers.at(mCommandName);
        if (sp != NULL) {
            sp->SetUsageText(usageText);
        }
    } else {
        mUsageText = usageText;
    }
}

std::string AJACommandLineParser::GetUsageText() const
{
    if (!mCommandName.empty()) {
        AJACommandLineParser *sp = mSubParsers.at(mCommandName);
        if (sp != NULL) {
            return sp->GetUsageText();
        }
    }

    return mUsageText;
}

void AJACommandLineParser::SetHelpText(const std::string &helpText)
{
    if (helpText.empty())
        return;
    if (!mCommandName.empty()) {
        AJACommandLineParser *sp = mSubParsers.at(mCommandName);
        if (sp != NULL) {
            sp->SetHelpText(helpText);
        }
    } else {
        mHelpText = helpText;
    }
}

std::string AJACommandLineParser::GetHelpText() const
{
    if (!mCommandName.empty()) {
        AJACommandLineParser *sp = mSubParsers.at(mCommandName);
        if (sp != NULL) {
            return sp->GetHelpText();
        }
    }

    return mHelpText;
}

AJAStringList AJACommandLineParser::GetKnownOptions()
{
    if (!mCommandName.empty()) {
        AJACommandLineParser *sp = mSubParsers.at(mCommandName);
        if (sp != NULL) {
            return sp->GetKnownOptions();
        }
    }

    return mKnownOptions;
}

std::string AJACommandLineParser::GetCommandName()
{
    return mCommandName;
}

AJAStringList AJACommandLineParser::GetUnknownOptions()
{
    if (!mCommandName.empty()) {
        AJACommandLineParser *sp = mSubParsers.at(mCommandName);
        if (sp != NULL) {
            return sp->GetUnknownOptions();
        }
    }

    return mUnknownOptions;
}

bool AJACommandLineParser::hasOptionPrefix(const std::string &name)
{
    return aja::starts_with(name, kSingleDash);
}

bool AJACommandLineParser::parseOptionValue(const std::string &name,
                                            const std::string &arg,
                                            AJAStringList::const_iterator *iter,
                                            AJAStringList::const_iterator end)
{
    std::string optName;
    std::string optValue;
    size_t assignPos = arg.find(kAssignChar, 0);
    int prefixSize = aja::starts_with(arg, kDoubleDash) ?
        2 : (aja::starts_with(arg, "-") ? 1 : 0);
    if (assignPos != std::string::npos) {
        // Get value after assignment operator.
        optValue = arg.substr(assignPos+1, arg.length()-assignPos);
    } else {
        // Get value from next arg after this one if no assignment operator found.
        optValue = arg.substr(prefixSize+name.length(), arg.length());
        if (optValue.empty()) {
            if (iter != NULL && *iter != end) {
                AJAStringList::const_iterator tmp = *iter;
                if (++tmp != end) {
                    /* TODO(paulh): fix special case
                    *  parser options: [foo], [b, bar]
                    *  > program.exe --foo -b
                    *  Will be parsed as:
                    *     --foo=-b
                    *     bar IsSet() == true
                    *
                    * The current workaround is to always use the assignment operator:
                    * > program.exe --foo=-b
                    * > program.exe --foo="-b"
                    *
                    * The fix is to iterate through all known options and check if the
                    * arg specified here as "tmp" is one of the option names.
                    * If the option is an option name, do not treat it as a value.
                    * We might want to have an AJACommandLineParser flag to allow
                    * handling either case.
                    * CASE A:
                    * > program.exe --foo -b
                    * [foo] is treated as "set", with no value.
                    * [b, bar] is treated as "set", with no value.
                    * CASE B:
                    * > program.exe --foo -b
                    * [foo] is treated as "set", with a value of "-b"
                    * [b, bar] is treated as "not set".
                    */
                    optValue = *(tmp);
                }
            }
        }

        if (optValue.empty())
            return false;
    }

    return setOptionValue(name, optValue);
}

bool AJACommandLineParser::setOptionValue(const std::string &name, const std::string &value)
{
    if (!value.empty()) {
        for (size_t i = 0; i < mOptions.size(); i++) {
            AJACommandLineOption opt = mOptions.at(i);
            const AJAStringList &names = opt.GetNames();
            for (AJAStringListConstIter iter = names.begin(); iter != names.end(); ++iter) {
                if (name == *iter) {
                    mOptions[i].AddValue(value);
                    return true;
                }
            }
        }
    }

    return false;
}

std::string AJACommandLineParser::removePrefix(const std::string &name, int &count)
{
    AJAStringList prefixes;
    prefixes.push_back("--");
    prefixes.push_back("-");
    for (AJAStringListConstIter iter = prefixes.begin(); iter != prefixes.end(); ++iter) {
        const std::string &prefix = *iter;
        if (aja::starts_with(name, prefix)) {
            size_t prefixSize = prefix.length();
            count = (int)prefixSize;
            return name.substr(prefixSize, name.length()-prefixSize);
        }
    }

    return name;
}