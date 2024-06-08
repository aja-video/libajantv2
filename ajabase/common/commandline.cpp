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
: mNames(), mDesc(), mDescExtra(), mValues(), mDefaultValue(), mIsSet(false)
{}

AJACommandLineOption::AJACommandLineOption(const std::string &name)
: mNames(), mDesc(), mDescExtra(), mValues(), mDefaultValue(), mIsSet(false)
{
    AddName(name);
}

AJACommandLineOption::AJACommandLineOption(const std::string &name,
                                           const std::string &desc)
: mNames(), mDesc(desc), mDescExtra(), mValues(), mDefaultValue(), mIsSet(false)
{
    AddName(name);
}

AJACommandLineOption::AJACommandLineOption(const std::string &name,
                        const std::string &desc,
                        const std::string &defaultValue)
: mNames(), mDesc(desc), mDescExtra(), mValues(), mDefaultValue(defaultValue), mIsSet(false)
{
    AddName(name);
}

AJACommandLineOption::AJACommandLineOption(const AJAStringList &names)
: mNames(), mDesc(), mDescExtra(), mValues(), mDefaultValue(), mIsSet(false)
{
    for (size_t i = 0; i < names.size(); i++) {
        AddName(names.at(i));
    }
}

AJACommandLineOption::AJACommandLineOption(const AJAStringList &names,
                                           const std::string &desc)
: mNames(), mDesc(desc), mDescExtra(), mValues(), mDefaultValue(), mIsSet(false)
{
    for (size_t i = 0; i < names.size(); i++) {
        AddName(names.at(i));
    }
}

AJACommandLineOption::AJACommandLineOption(const AJAStringList &names,
                                           const std::string &desc,
                                           const std::string &defaultValue)
: mNames(), mDesc(desc), mDescExtra(), mValues(), mDefaultValue(), mIsSet(false)
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

bool AJACommandLineOption::IsSet() const
{
    return mIsSet;
}
void AJACommandLineOption::MarkSet(bool isSet)
{
    mIsSet = isSet;
}

AJACommandLineParser::AJACommandLineParser(int flags)
: mFlags(flags), mName(), mCommandName(), mDesc(), mDescExtra(), mUsageText(), mHelpText(),
  mOptions(), mSubParsers()
{
    init();
}

AJACommandLineParser::AJACommandLineParser(const std::string &name, int flags)
: mFlags(flags), mName(name), mCommandName(), mDesc(), mDescExtra(), mUsageText(), mHelpText(),
  mOptions(), mSubParsers()
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
	mFlags = other.mFlags;
	mName = other.mName;
	mCommandName = other.mCommandName;
	mDesc = other.mDesc;
	mDescExtra = other.mDescExtra;
	mUsageText = other.mUsageText;
	mHelpText = other.mHelpText;
	mOptions = other.mOptions;
    mSubParsers.clear();
    for (SubParserMapConstIter iter = other.mSubParsers.begin(); iter != other.mSubParsers.end(); iter++) {
        mSubParsers.insert(AJASubParserPair(iter->first, iter->second));
    }
}

void AJACommandLineParser::init()
{
    if ((mFlags & kNoDefaultHelpOption) == 0) {
        AddHelpOption();
    }
    if ((mFlags & kNoDefaultUsageOption) == 0) {
        AddUsageOption();
    }
}

void AJACommandLineParser::Reset(bool clearAll)
{
    if (clearAll) {
        mSubParsers.clear();
        mOptions.clear();
    }
}

void AJACommandLineParser::Dump()
{
    if (!mCommandName.empty()) {
        AJACommandLineParser *sp = mSubParsers.at(mCommandName);
        if (sp != NULL)
            return sp->Dump();
    } else {
        for (AJACommandLineOptionListIter iter = mOptions.begin();
            iter != mOptions.end(); iter++) {
            const AJACommandLineOption &o = *iter;
            const AJAStringList & names = o.GetNames();
            std::ostringstream oss;
            oss << "[";
            std::string name;
            size_t count = 0;
            for (AJAStringListConstIter sIter = names.begin(); sIter != names.end(); sIter++) {
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

bool AJACommandLineParser::HaveOption(const std::string &name) const
{
    for (const auto &opt : mOptions) {
        for (const auto &n : opt.GetNames()) {
            if (n == name)
                return true;
        }
    }
    return false;
}

bool AJACommandLineParser::OptionByName(const std::string &name, AJACommandLineOption &opt) const
{
    for (const auto &o : mOptions) {
        for (const auto &n : o.GetNames()) {
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
    const std::string &name = p->GetName();
    if (mSubParsers.find(name) == mSubParsers.end()) {
        mSubParsers.insert(AJASubParserPair(name, p));
        return true;
    }

    return false;
}

void AJACommandLineParser::ParseArgs(const AJAStringList &args)
{
    // Must have at least 2 args (args[0] is the binary name, and args[1..N] are the user-specified args).
    if (args.size() <= 1)
        return;

    AJAStringList::const_iterator iter = args.begin();
    ++iter;

    for (SubParserMap::iterator spIter = mSubParsers.begin(); spIter != mSubParsers.end(); spIter++) {
        // Is second arg a command name which belongs to a sub-parser?
        if (*iter == spIter->first)
            mCommandName = *iter;
        // Iterate all args with all sub-parsers...
        if (spIter->second != NULL)
            spIter->second->ParseArgs(args);
    }

    // If the parser name is specified, expect 2nd arg to match.
    // The parser name is treated as a "sub-command name", eg.
    // > MyApp.exe theCommand -d1 -n3 --verbose
    if ((*iter != mName) && !mName.empty())
        return;

    // ...otherwise just parse the args.
    for (; iter != args.end(); iter++) {
        const std::string &arg = *iter;
        if (hasOptionPrefix(arg)) {
            AJACommandLineOption opt;
            std::string optName;
            std::string optValue;
            std::string argStr;
            bool doubleDash = aja::starts_with(arg, kDoubleDash);
            bool singleDash = aja::starts_with(arg, kSingleDash) && !doubleDash;
            if (doubleDash)
                argStr = arg.substr(2, arg.length());
            else if (singleDash)
                argStr = arg.substr(1, arg.length());

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
                    if (mFlags & kShortOptionsAsLong) {
                        for (size_t c = 1; c < arg.length(); c++) {
                            subStr = arg.substr(c, 1);
                            if (HaveOption(subStr))
                                setOption(subStr, true);
                        }
                        return;
                    } else {
                        for (size_t c = arg.length(); c > 1; c--) {
                            subStr = arg.substr(1, c-1);
                            if (HaveOption(subStr)) {
                                optName = subStr;
                                optValue = arg.substr(
                                    arg.find_last_of(optName)+1,
                                    arg.length()-optName.length()
                                );
                                break;
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
                        if (!hasOptionPrefix(nextArg))
                            optValue = nextArg;
                    }
                }
            }

            if (HaveOption(optName)) {
                if (!optValue.empty())
                    setOptionValue(optName, optValue);
                setOption(optName, true);
            }
        }
    }
}

void AJACommandLineParser::ParseArgs(int argc, const char *argv[])
{
    if (argc == 0 || argc == 1 || argv == NULL)
        return;

    AJAStringList argList;
    for (int i = 0; i < argc; i++)
        argList.push_back(std::string(argv[i]));

    ParseArgs(argList);
}

void AJACommandLineParser::ParseArgs(int argc, char *argv[])
{
    if (argc == 0 || argc == 1 || argv == NULL)
        return;

    AJAStringList argList;
    for (int i = 0; i < argc; i++)
        argList.push_back(std::string(argv[i]));

    ParseArgs(argList);
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
        for (AJAStringListConstIter it = values.begin(); it != values.end(); it++) {
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
    for (AJACommandLineOptionListIter optIter = mOptions.begin(); optIter != mOptions.end(); optIter++) {
        const AJAStringList &names = optIter->GetNames();
        for (AJAStringListConstIter nameIter = names.begin(); nameIter != names.end(); nameIter++) {
            for (AJAStringListConstIter wantIter = wantNames.begin(); wantIter != wantNames.end(); wantIter++) {
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
    uint32_t okCount = 0;
    for (size_t i = 0; i < options.size(); i++) {
        if (AddOption(options.at(i)))
            ++okCount;
    }
    return options.size() > 0 ? (okCount == (uint32_t)options.size() ? true : false) : false;
}

bool AJACommandLineParser::AddHelpOption()
{
    AJACommandLineOption helpOpt;
    helpOpt.AddName("?");
    helpOpt.AddName("h");
    helpOpt.AddName("help");
    helpOpt.SetDesc("Print the help text");
    return AddOption(helpOpt);
}

bool AJACommandLineParser::AddUsageOption()
{
    AJACommandLineOption usageOpt;
    usageOpt.AddName("usage");
    usageOpt.SetDesc("Print the usage text");
    return AddOption(usageOpt);
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

std::string AJACommandLineParser::GetUsageText()
{
    if (!mCommandName.empty()) {
        AJACommandLineParser *sp = mSubParsers.at(mCommandName);
        if (sp != NULL) {
            return sp->GetUsageText();
        }
    }

    std::string usageText;
    if (!mUsageText.empty()) {
        usageText = mUsageText;
    } else {
        usageText = generateUsageText();
    }
    return usageText;
}

void AJACommandLineParser::SetHelpText(const std::string &helpText)
{
    if (!mCommandName.empty()) {
        AJACommandLineParser *sp = mSubParsers.at(mCommandName);
        if (sp != NULL) {
            sp->SetHelpText(helpText);
        }
    } else {
        mHelpText = helpText;
    }
}

std::string AJACommandLineParser::GetHelpText()
{
    if (!mCommandName.empty()) {
        AJACommandLineParser *sp = mSubParsers.at(mCommandName);
        if (sp != NULL) {
            return sp->GetHelpText();
        }
    }

    std::string helpText;
    if (mHelpText.empty()) {
        helpText = generateHelpText();
    } else {
        helpText = mHelpText;
    }
    return helpText;
}

std::string AJACommandLineParser::generateHelpText() const
{
    std::ostringstream oss;
    std::string exePath;
    AJAFileIO::GetExecutablePath(exePath);
    oss << "Usage: " << exePath;
    if (!mName.empty())
        oss << " " << mName;
    oss << " [OPTION...]" << std::endl;

    // Get the longest line size first...
    size_t longestSize = 0;
    for (AJACommandLineOptionListIter it = mOptions.begin();
        it != mOptions.end(); it++) {
        const AJAStringList &names = it->GetNames();
        size_t namesLength = 0;
        for (AJAStringListConstIter sIter = names.begin(); sIter != names.end(); sIter++) {
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

    // ...now calculate all of the line padding.
    for (AJACommandLineOptionListIter it = mOptions.begin();
        it != mOptions.end(); it++) {
        oss << std::setw(2) << std::right;
        const AJAStringList &names = it->GetNames();
        size_t nameCount = 0;
        size_t namesLength = 0;
        for (AJAStringListConstIter sIter = names.begin(); sIter != names.end(); sIter++) {
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

    return oss.str();
}

std::string AJACommandLineParser::generateUsageText() const
{
    std::ostringstream oss;
    std::string exePath;
    AJAFileIO::GetExecutablePath(exePath);
    oss << "Usage: " << exePath;
    if (!mName.empty())
        oss << " " << mName;
    oss << "\nTODO" << std::endl;
    return oss.str();
}

std::string AJACommandLineParser::GetCommandName()
{
    return mCommandName;
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
        for (size_t i = 0; i < mOptions.size(); i++) {
            AJACommandLineOption opt = mOptions.at(i);
            const AJAStringList &names = opt.GetNames();
            for (AJAStringListConstIter iter = names.begin(); iter != names.end(); iter++) {
                if (name == *iter) {
                    mOptions[i].AddValue(value);
                    return true;
                }
            }
        }
    }

    return false;
}

bool AJACommandLineParser::setOption(const std::string &name, bool isSet)
{
    for (size_t i = 0; i < mOptions.size(); i++) {
        AJACommandLineOption opt = mOptions.at(i);
        const AJAStringList &names = opt.GetNames();
        for (AJAStringListConstIter iter = names.begin(); iter != names.end(); iter++) {
            if (name == *iter) {
                mOptions[i].MarkSet(isSet);
                return true;
            }
        }
    }

    return false;
}
