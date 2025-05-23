/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2routingexpert.h
	@brief		Declares RoutingExpert class used by CNTV2SignalRouter.
	@copyright	(C) 2014-2022 AJA Video Systems, Inc.
**/

#ifndef NTV2ROUTINGEXPERT_H
#define NTV2ROUTINGEXPERT_H

#include "ntv2signalrouter.h"

#include "ajabase/system/lock.h"
#include "ajabase/common/ajarefptr.h"

class RoutingExpert;
typedef AJARefPtr<RoutingExpert>	RoutingExpertPtr;

class AJAExport RoutingExpert
{
	public:
		friend class CNTV2SignalRouter;

		static RoutingExpertPtr		GetInstance(const bool inCreateIfNecessary = true);
		static bool					DisposeInstance(void);
		static uint32_t				NumInstances(void);

	private:
		RoutingExpert();
	public:
		~RoutingExpert();

	protected:
		std::string			InputXptToString (const NTV2InputXptID inInputXpt) const;
		std::string			OutputXptToString (const NTV2OutputXptID inOutputXpt) const;
		NTV2InputXptID		StringToInputXpt (const std::string & inStr) const;
		NTV2OutputXptID		StringToOutputXpt (const std::string & inStr) const;
		NTV2WidgetType		WidgetIDToType (const NTV2WidgetID inWidgetID);
		NTV2Channel			WidgetIDToChannel(const NTV2WidgetID inWidgetID);
		NTV2WidgetID		WidgetIDFromTypeAndChannel(const NTV2WidgetType inWidgetType, const NTV2Channel inChannel);
		bool				GetWidgetsForOutput (const NTV2OutputXptID inOutputXpt, NTV2WidgetIDSet & outWidgetIDs) const;
		bool				GetWidgetsForInput (const NTV2InputXptID inInputXpt, NTV2WidgetIDSet & outWidgetIDs) const;
		bool				GetWidgetInputs (const NTV2WidgetID inWidgetID, NTV2InputXptIDSet & outInputs) const;
		bool				GetWidgetOutputs (const NTV2WidgetID inWidgetID, NTV2OutputXptIDSet & outOutputs) const;
		bool				IsOutputXptValid (const NTV2OutputXptID inOutputXpt) const;
		bool				IsRGBOnlyInputXpt (const NTV2InputXptID inInputXpt) const;
		bool				IsYUVOnlyInputXpt (const NTV2InputXptID inInputXpt) const;
		bool				IsKeyInputXpt (const NTV2InputXptID inInputXpt) const;
		bool				IsSDIWidget(const NTV2WidgetType inWidgetType) const;
		bool				IsSDIInWidget(const NTV2WidgetType inWidgetType) const;
		bool				IsSDIOutWidget(const NTV2WidgetType inWidgetType) const;
		bool				Is3GSDIWidget(const NTV2WidgetType inWidgetType) const;
		bool				Is12GSDIWidget(const NTV2WidgetType inWidgetType) const;
		bool				IsDualLinkWidget(const NTV2WidgetType inWidgetType) const;
		bool				IsDualLinkInWidget(const NTV2WidgetType inWidgetType) const;
		bool				IsDualLinkOutWidget(const NTV2WidgetType inWidgetType) const;
		bool				IsHDMIWidget(const NTV2WidgetType inWidgetType) const;
		bool				IsHDMIInWidget(const NTV2WidgetType inWidgetType) const;
		bool				IsHDMIOutWidget(const NTV2WidgetType inWidgetType) const;

	protected:
		typedef std::map <std::string, NTV2InputXptID>			String2InputXpt;
		typedef String2InputXpt::const_iterator					String2InputXptConstIter;

		typedef std::map <NTV2InputXptID, std::string>			InputXpt2String;
		typedef InputXpt2String::const_iterator					InputXpt2StringConstIter;

		typedef std::map <std::string, NTV2OutputXptID>			String2OutputXpt;
		typedef String2OutputXpt::const_iterator				String2OutputXptConstIter;

		typedef std::map <NTV2OutputXptID, std::string>			OutputXpt2String;
		typedef OutputXpt2String::const_iterator				OutputXpt2StringConstIter;

		typedef std::set<NTV2WidgetType>						NTV2WidgetTypeSet;
		typedef NTV2WidgetTypeSet::const_iterator				NTV2WidgetTypeSetConstIter;

		typedef std::pair <NTV2WidgetID, NTV2WidgetType>		Widget2TypePair;
		typedef std::multimap <NTV2WidgetID, NTV2WidgetType>	Widget2Types;
		typedef Widget2Types::const_iterator					Widget2TypesConstIter;

		typedef std::pair <NTV2WidgetID, NTV2Channel>			Widget2ChannelPair;
		typedef std::multimap <NTV2WidgetID, NTV2Channel>		Widget2Channels;
		typedef Widget2Channels::const_iterator					Widget2ChannelsConstIter;

		typedef std::pair <NTV2WidgetID, NTV2InputXptID>		Widget2InputXptPair;
		typedef std::multimap <NTV2WidgetID, NTV2InputXptID>	Widget2InputXpts;
		typedef Widget2InputXpts::const_iterator				Widget2InputXptsConstIter;

		typedef std::pair <NTV2WidgetID, NTV2OutputXptID>		Widget2OutputXptPair;
		typedef std::multimap <NTV2WidgetID, NTV2OutputXptID>	Widget2OutputXpts;
		typedef Widget2OutputXpts::const_iterator				Widget2OutputXptsConstIter;

		typedef std::multimap <NTV2InputXptID, NTV2WidgetID>	InputXpt2WidgetIDs;
		typedef std::pair <NTV2InputXptID, NTV2WidgetID>		InputXpt2WidgetIDPair;
		typedef InputXpt2WidgetIDs::const_iterator				InputXpt2WidgetIDsConstIter;

		typedef std::multimap <NTV2OutputXptID, NTV2WidgetID>	OutputXpt2WidgetIDs;
		typedef std::pair <NTV2OutputXptID, NTV2WidgetID>		OutputXpt2WidgetIDPair;
		typedef OutputXpt2WidgetIDs::const_iterator				OutputXpt2WidgetIDsConstIter;

		private:
			void InitInputXpt2String(void);
			void InitOutputXpt2String(void);
			void InitInputXpt2WidgetIDs(void);
			void InitOutputXpt2WidgetIDs(void);
			void InitWidgetIDToChannels(void);
			void InitWidgetIDToWidgetTypes(void);

			mutable AJALock			gLock;
			String2InputXpt			gString2InputXpt;
			InputXpt2String			gInputXpt2String;
			InputXpt2WidgetIDs		gInputXpt2WidgetIDs;
			String2OutputXpt		gString2OutputXpt;
			OutputXpt2String		gOutputXpt2String;
			OutputXpt2WidgetIDs		gOutputXpt2WidgetIDs;
			Widget2OutputXpts		gWidget2OutputXpts;
			Widget2InputXpts		gWidget2InputXpts;
			Widget2Channels			gWidget2Channels;
			Widget2Types			gWidget2Types;
			// NTV2InputXptID Helpers
			NTV2InputXptIDSet		gRGBOnlyInputXpts;
			NTV2InputXptIDSet		gYUVOnlyInputXpts;
			NTV2InputXptIDSet		gKeyInputXpts;
			// NTV2WidgetType Helpers
			NTV2WidgetTypeSet		gSDIWidgetTypes;
			NTV2WidgetTypeSet		gSDI3GWidgetTypes;
			NTV2WidgetTypeSet		gSDI12GWidgetTypes;
			NTV2WidgetTypeSet		gSDIInWidgetTypes;
			NTV2WidgetTypeSet		gSDIOutWidgetTypes;
			NTV2WidgetTypeSet		gDualLinkWidgetTypes;
			NTV2WidgetTypeSet		gDualLinkInWidgetTypes;
			NTV2WidgetTypeSet		gDualLinkOutWidgetTypes;
			NTV2WidgetTypeSet		gHDMIWidgetTypes;
			NTV2WidgetTypeSet		gHDMIInWidgetTypes;
			NTV2WidgetTypeSet		gHDMIOutWidgetTypes;
			NTV2WidgetTypeSet		gAnalogWidgetTypes;

};	//	RoutingExpert

static RoutingExpertPtr gpRoutingExpert;	//	RoutingExpert singleton
static AJALock			gRoutingExpertLock; //	Singleton guard mutex

#endif
