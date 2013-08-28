/**
 * @file llfloaterconversationpreview.cpp
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llconversationlog.h"
#include "llfloaterconversationpreview.h"
#include "llimview.h"
#include "lllineeditor.h"
// <FS:CR> [FS communication UI]
//#include "llfloaterimnearbychat.h"
#include "fsfloaternearbychat.h"
// </FS:CR> [FS communication UI]
#include "llspinctrl.h"
#include "lltrans.h"
// <FS:CR>
#include "llviewercontrol.h"
#include "llavataractions.h"
#include "llviewerwindow.h"
#include "llwindow.h"
#include "llconversationlog.h"
// </FS:CR>

const std::string LL_FCP_COMPLETE_NAME("complete_name");
const std::string LL_FCP_ACCOUNT_NAME("user_name");

LLFloaterConversationPreview::LLFloaterConversationPreview(const LLSD& session_id)
:	LLFloater(session_id),
	mChatHistory(NULL),
	mSessionID(session_id.asUUID()),
	mCurrentPage(0),
	mPageSize(gSavedSettings.getS32("ConversationHistoryPageSize")),
	mAccountName(session_id[LL_FCP_ACCOUNT_NAME]),
	mCompleteName(session_id[LL_FCP_COMPLETE_NAME])
{
}

BOOL LLFloaterConversationPreview::postBuild()
{
	// <FS:CR> [FS communication UI]
	//mChatHistory = getChild<LLChatHistory>("chat_history"); 
	//LLLoadHistoryThread::setLoadEndSignal(boost::bind(&LLFloaterConversationPreview::SetPages, this, _1, _2)); <FS:TM> 3.6.4 check this, LL added
	mChatHistory = getChild<FSChatHistory>("chat_history");
	// <FS:CR> [FS communication UI]
	
	childSetAction("open_external_btn", boost::bind(&LLFloaterConversationPreview::onBtnOpenExternal, this));	//<FS:CR> Open chat history externally

	const LLConversation* conv = LLConversationLog::instance().getConversation(mSessionID);
	std::string name;
	std::string file;

	if (mAccountName != "")
	{
		name = mCompleteName;
		file = mAccountName;
	}
	else if (mSessionID != LLUUID::null && conv)
	{
		name = conv->getConversationName();
		file = conv->getHistoryFileName();
	}
	else
	{
		name = LLTrans::getString("NearbyChatTitle");
		file = "chat";
	}
	// <FS:Ansariel> Remember used log file name
	mChatHistoryFileName = file;
	LLStringUtil::format_map_t args;
	args["[NAME]"] = name;
	std::string title = getString("Title", args);
	setTitle(title);

	LLSD load_params;
	load_params["load_all_history"] = true;
	load_params["cut_off_todays_date"] = false;


	LLSD loading;
	loading[LL_IM_TEXT] = LLTrans::getString("loading_chat_logs");
	mMessages.push_back(loading);
	mPageSpinner = getChild<LLSpinCtrl>("history_page_spin");
	mPageSpinner->setCommitCallback(boost::bind(&LLFloaterConversationPreview::onMoreHistoryBtnClick, this));
	mPageSpinner->setMinValue(1);
	mPageSpinner->set(1);
	mPageSpinner->setEnabled(false);
	mChatHistoryLoaded = false;
	LLLogChat::startChatHistoryThread(file, load_params);
	return LLFloater::postBuild();
}

void LLFloaterConversationPreview::SetPages(std::list<LLSD>& messages, const std::string& file_name)
{
	if(file_name == mChatHistoryFileName)
	{
		mMessages = messages;


		mCurrentPage = mMessages.size() / mPageSize;
		mPageSpinner->setEnabled(true);
		mPageSpinner->setMaxValue(mCurrentPage+1);
		mPageSpinner->set(mCurrentPage+1);

		std::string total_page_num = llformat("/ %d", mCurrentPage+1);
		getChild<LLTextBox>("page_num_label")->setValue(total_page_num);
		mChatHistoryLoaded = true;

	}

}
void LLFloaterConversationPreview::draw()
{
	if(mChatHistoryLoaded)
	{
		showHistory();
		mChatHistoryLoaded = false;
	}
	LLFloater::draw();
}

void LLFloaterConversationPreview::onOpen(const LLSD& key)
{
	showHistory();
}

void LLFloaterConversationPreview::showHistory()
{
	if (!mMessages.size())
	{
		return;
	}

	mChatHistory->clear();

	std::ostringstream message;
	std::list<LLSD>::const_iterator iter = mMessages.begin();

	int delta = 0;
	if (mCurrentPage)
	{
		int remainder = mMessages.size() % mPageSize;
		delta = (remainder == 0) ? 0 : (mPageSize - remainder);
	}

	std::advance(iter, (mCurrentPage * mPageSize) - delta);

	for (int msg_num = 0; (iter != mMessages.end() && msg_num < mPageSize); ++iter, ++msg_num)
	{
		if (iter->size() == 0)
		{
			continue;
		}

		LLSD msg = *iter;

		LLUUID from_id 		= LLUUID::null;
		std::string time	= msg["time"].asString();
		std::string from	= msg["from"].asString();
		std::string message	= msg["message"].asString();

		if (msg["from_id"].isDefined())
		{
			from_id = msg["from_id"].asUUID();
		}
		else
 		{
			std::string legacy_name = gCacheName->buildLegacyName(from);
 			gCacheName->getUUID(legacy_name, from_id);
 		}

		LLChat chat;
		chat.mFromID = from_id;
		chat.mSessionID = mSessionID;
		chat.mFromName = from;
		chat.mTimeStr = time;
		chat.mChatStyle = CHAT_STYLE_HISTORY;
		chat.mText = message;

		if (from_id.isNull() && SYSTEM_FROM == from)
		{
			chat.mSourceType = CHAT_SOURCE_SYSTEM;

		}
		else if (from_id.isNull())
		{
			// <FS:CR> [FS communication UI]
			//chat.mSourceType = LLFloaterIMNearbyChat::isWordsName(from) ? CHAT_SOURCE_UNKNOWN : CHAT_SOURCE_OBJECT;
			chat.mSourceType = FSFloaterNearbyChat::isWordsName(from) ? CHAT_SOURCE_UNKNOWN : CHAT_SOURCE_OBJECT;
			// </FS:CR> [FS communication UI]
		}

		LLSD chat_args;
		chat_args["use_plain_text_chat_history"] =
						gSavedSettings.getBOOL("PlainTextChatHistory");
		// <FS:CR>
		//chat_args["show_time"] = gSavedSettings.getBOOL("IMShowTime");
		chat_args["show_time"] = gSavedSettings.getBOOL("FSShowTimestampsTranscripts");
		// </FS:CR>
		chat_args["show_names_for_p2p_conv"] = gSavedSettings.getBOOL("IMShowNamesForP2PConv");
		chat_args["conversation_log"] = true;	// <FS:CR> Don't dim the history in conversation log

		mChatHistory->appendMessage(chat,chat_args);
	}

}

void LLFloaterConversationPreview::onMoreHistoryBtnClick()
{
	mCurrentPage = (int)(mPageSpinner->getValueF32());
	if (--mCurrentPage < 0)
	{
		return;
	}

	showHistory();
}

// <FS:CR> Open chat history externally
void (LLFloaterConversationPreview::onBtnOpenExternal())
{
	gViewerWindow->getWindow()->openFile(LLLogChat::makeLogFileName(mChatHistoryFileName));
}
