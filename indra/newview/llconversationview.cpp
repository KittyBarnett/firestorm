/** 
 * @file llconversationview.cpp
 * @brief Implementation of conversations list widgets and views
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llconversationview.h"

#include <boost/bind.hpp>
#include "llconversationmodel.h"
#include "llimconversation.h"
#include "llimfloatercontainer.h"
#include "llfloaterreg.h"
#include "lluictrlfactory.h"

//
// Implementation of conversations list session widgets
//
static LLDefaultChildRegistry::Register<LLConversationViewSession> r_conversation_view_session("conversation_view_session");

const LLColor4U DEFAULT_WHITE(255, 255, 255);

LLConversationViewSession::Params::Params() :	
	container()
{}

LLConversationViewSession::LLConversationViewSession(const LLConversationViewSession::Params& p):
	LLFolderViewFolder(p),
	mContainer(p.container),
	mItemPanel(NULL),
	mSessionTitle(NULL)
{
}

BOOL LLConversationViewSession::postBuild()
{
	LLFolderViewItem::postBuild();

	mItemPanel = LLUICtrlFactory::getInstance()->createFromFile<LLPanel>("panel_conversation_list_item.xml", NULL, LLPanel::child_registry_t::instance());

	addChild(mItemPanel);

	mSessionTitle = mItemPanel->getChild<LLTextBox>("conversation_title");

	refresh();

	return TRUE;
}

void LLConversationViewSession::draw()
{
// *TODO Seth PE: remove the code duplicated from LLFolderViewFolder::draw()
// ***** LLFolderViewFolder::draw() code begin *****
	if (mAutoOpenCountdown != 0.f)
	{
		mControlLabelRotation = mAutoOpenCountdown * -90.f;
	}
	else if (isOpen())
	{
		mControlLabelRotation = lerp(mControlLabelRotation, -90.f, LLCriticalDamp::getInterpolant(0.04f));
	}
	else
	{
		mControlLabelRotation = lerp(mControlLabelRotation, 0.f, LLCriticalDamp::getInterpolant(0.025f));
	}
// ***** LLFolderViewFolder::draw() code end *****

// *TODO Seth PE: remove the code duplicated from LLFolderViewItem::draw()
// ***** LLFolderViewItem::draw() code begin *****

	static LLUIColor sFgColor = LLUIColorTable::instance().getColor("MenuItemEnabledColor", DEFAULT_WHITE);
	static LLUIColor sHighlightBgColor = LLUIColorTable::instance().getColor("MenuItemHighlightBgColor", DEFAULT_WHITE);
	static LLUIColor sFocusOutlineColor = LLUIColorTable::instance().getColor("InventoryFocusOutlineColor", DEFAULT_WHITE);
	static LLUIColor sMouseOverColor = LLUIColorTable::instance().getColor("InventoryMouseOverColor", DEFAULT_WHITE);

	const LLFolderViewItem::Params& default_params = LLUICtrlFactory::getDefaultParams<LLFolderViewItem>();
	const S32 TOP_PAD = default_params.item_top_pad;
	const S32 FOCUS_LEFT = 1;

	getViewModelItem()->update();

	//--------------------------------------------------------------------------------//
	// Draw open folder arrow
	//
	if (hasVisibleChildren() || getViewModelItem()->hasChildren())
	{
		LLUIImage* arrow_image = default_params.folder_arrow_image;
		gl_draw_scaled_rotated_image(
			mIndentation, getRect().getHeight() - mArrowSize - mTextPad - TOP_PAD,
			mArrowSize, mArrowSize, mControlLabelRotation, arrow_image->getImage(), sFgColor);
	}


	//--------------------------------------------------------------------------------//
	// Draw highlight for selected items
	//
	const BOOL show_context = (getRoot() ? getRoot()->getShowSelectionContext() : FALSE);
	const BOOL filled = show_context || (getRoot() ? getRoot()->getParentPanel()->hasFocus() : FALSE); // If we have keyboard focus, draw selection filled
	const S32 focus_top = getRect().getHeight();
	const S32 focus_bottom = getRect().getHeight() - mItemHeight;
	const bool folder_open = (getRect().getHeight() > mItemHeight + 4);
	if (mIsSelected) // always render "current" item.  Only render other selected items if mShowSingleSelection is FALSE
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		LLColor4 bg_color = sHighlightBgColor;
		if (!mIsCurSelection)
		{
			// do time-based fade of extra objects
			F32 fade_time = (getRoot() ? getRoot()->getSelectionFadeElapsedTime() : 0.0f);
			if (getRoot() && getRoot()->getShowSingleSelection())
			{
				// fading out
				bg_color.mV[VALPHA] = clamp_rescale(fade_time, 0.f, 0.4f, bg_color.mV[VALPHA], 0.f);
			}
			else
			{
				// fading in
				bg_color.mV[VALPHA] = clamp_rescale(fade_time, 0.f, 0.4f, 0.f, bg_color.mV[VALPHA]);
			}
		}
		gl_rect_2d(FOCUS_LEFT,
				   focus_top,
				   getRect().getWidth() - 2,
				   focus_bottom,
				   bg_color, filled);
		if (mIsCurSelection)
		{
			gl_rect_2d(FOCUS_LEFT,
					   focus_top,
					   getRect().getWidth() - 2,
					   focus_bottom,
					   sFocusOutlineColor, FALSE);
		}
		if (folder_open)
		{
			gl_rect_2d(FOCUS_LEFT,
					   focus_bottom + 1, // overlap with bottom edge of above rect
					   getRect().getWidth() - 2,
					   0,
					   sFocusOutlineColor, FALSE);
			if (show_context)
			{
				gl_rect_2d(FOCUS_LEFT,
						   focus_bottom + 1,
						   getRect().getWidth() - 2,
						   0,
						   sHighlightBgColor, TRUE);
			}
		}
	}
	else if (mIsMouseOverTitle)
	{
		gl_rect_2d(FOCUS_LEFT,
			focus_top,
			getRect().getWidth() - 2,
			focus_bottom,
			sMouseOverColor, FALSE);
	}

	//--------------------------------------------------------------------------------//
	// Draw DragNDrop highlight
	//
	if (mDragAndDropTarget)
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		gl_rect_2d(FOCUS_LEFT,
				   focus_top,
				   getRect().getWidth() - 2,
				   focus_bottom,
				   sHighlightBgColor, FALSE);
		if (folder_open)
		{
			gl_rect_2d(FOCUS_LEFT,
					   focus_bottom + 1, // overlap with bottom edge of above rect
					   getRect().getWidth() - 2,
					   0,
					   sHighlightBgColor, FALSE);
		}
		mDragAndDropTarget = FALSE;
	}
// ***** LLFolderViewItem::draw() code end *****

	// draw children if root folder, or any other folder that is open or animating to closed state
	bool draw_children = getRoot() == static_cast<LLFolderViewFolder*>(this)
						 || isOpen()
						 || mCurHeight != mTargetHeight;

	for (folders_t::iterator iter = mFolders.begin();
		iter != mFolders.end();)
	{
		folders_t::iterator fit = iter++;
		(*fit)->setVisible(draw_children);
	}
	for (items_t::iterator iter = mItems.begin();
		iter != mItems.end();)
	{
		items_t::iterator iit = iter++;
		(*iit)->setVisible(draw_children);
	}

	LLView::draw();
}

// virtual
S32 LLConversationViewSession::arrange(S32* width, S32* height)
{
	LLRect rect(getIndentation() + mArrowSize,
				getLocalRect().mTop,
				getLocalRect().mRight,
				getLocalRect().mTop - getItemHeight());
	mItemPanel->setShape(rect);

	return LLFolderViewFolder::arrange(width, height);
}

void LLConversationViewSession::selectItem()
{
	
	LLConversationItem* item = dynamic_cast<LLConversationItem*>(mViewModelItem);
	LLFloater* session_floater = LLIMConversation::getConversation(item->getUUID());
	LLMultiFloater* host_floater = session_floater->getHost();
	
	if (host_floater == mContainer)
	{
		// Always expand the message pane if the panel is hosted by the container
		mContainer->collapseMessagesPane(false);
		// Switch to the conversation floater that is being selected
		mContainer->selectFloater(session_floater);
	}
	
	// Set the focus on the selected floater
	session_floater->setFocus(TRUE);

	LLFolderViewItem::selectItem();
}

void LLConversationViewSession::setVisibleIfDetached(BOOL visible)
{
	// Do this only if the conversation floater has been torn off (i.e. no multi floater host) and is not minimized
	// Note: minimized dockable floaters are brought to front hence unminimized when made visible and we don't want that here
	LLConversationItem* item = dynamic_cast<LLConversationItem*>(mViewModelItem);
	LLFloater* session_floater = LLIMConversation::getConversation(item->getUUID());
	
	if (session_floater && !session_floater->getHost() && !session_floater->isMinimized())
	{
		session_floater->setVisible(visible);
	}
}

LLConversationViewParticipant* LLConversationViewSession::findParticipant(const LLUUID& participant_id)
{
	// This is *not* a general tree parsing algorithm. We search only in the mItems list
	// assuming there is no mFolders which makes sense for sessions (sessions don't contain
	// sessions).
	LLConversationViewParticipant* participant = NULL;
	items_t::const_iterator iter;
	for (iter = getItemsBegin(); iter != getItemsEnd(); iter++)
	{
		participant = dynamic_cast<LLConversationViewParticipant*>(*iter);
		if (participant->hasSameValue(participant_id))
		{
			break;
		}
	}
	return (iter == getItemsEnd() ? NULL : participant);
}

void LLConversationViewSession::refresh()
{
	// Refresh the session view from its model data
	LLConversationItem* vmi = dynamic_cast<LLConversationItem*>(getViewModelItem());
	vmi->resetRefresh();
	
	if (mSessionTitle)
	{
		mSessionTitle->setText(vmi->getDisplayName());
	}

	// Note: for the moment, all that needs to be done is done by LLFolderViewItem::refresh()
	
	// Do the regular upstream refresh
	LLFolderViewFolder::refresh();
}

//
// Implementation of conversations list participant (avatar) widgets
//

static LLDefaultChildRegistry::Register<LLConversationViewParticipant> r("conversation_view_participant");
bool LLConversationViewParticipant::sStaticInitialized = false;
S32 LLConversationViewParticipant::sChildrenWidths[LLConversationViewParticipant::ALIC_COUNT];

LLConversationViewParticipant::Params::Params() :	
container(),
participant_id(),
avatar_icon("avatar_icon"),
info_button("info_button"),
output_monitor("output_monitor")
{}

LLConversationViewParticipant::LLConversationViewParticipant( const LLConversationViewParticipant::Params& p ):
	LLFolderViewItem(p),
    mAvatarIcon(NULL),
    mInfoBtn(NULL),
    mSpeakingIndicator(NULL),
    mUUID(p.participant_id)
{
}

void LLConversationViewParticipant::initFromParams(const LLConversationViewParticipant::Params& params)
{	
    LLAvatarIconCtrl::Params avatar_icon_params(params.avatar_icon());
    applyXUILayout(avatar_icon_params, this);
    LLAvatarIconCtrl * avatarIcon = LLUICtrlFactory::create<LLAvatarIconCtrl>(avatar_icon_params);
    addChild(avatarIcon);	
    
	LLButton::Params info_button_params(params.info_button());
    applyXUILayout(info_button_params, this);
	LLButton * button = LLUICtrlFactory::create<LLButton>(info_button_params);
	addChild(button);	

    LLOutputMonitorCtrl::Params output_monitor_params(params.output_monitor());
    applyXUILayout(output_monitor_params, this);
    LLOutputMonitorCtrl * outputMonitor = LLUICtrlFactory::create<LLOutputMonitorCtrl>(output_monitor_params);
    addChild(outputMonitor);
}

BOOL LLConversationViewParticipant::postBuild()
{
    mAvatarIcon = getChild<LLAvatarIconCtrl>("avatar_icon");

	mInfoBtn = getChild<LLButton>("info_btn");
	mInfoBtn->setClickedCallback(boost::bind(&LLConversationViewParticipant::onInfoBtnClick, this));
	mInfoBtn->setVisible(false);

	mSpeakingIndicator = getChild<LLOutputMonitorCtrl>("speaking_indicator");

    if (!sStaticInitialized)
    {
        // Remember children widths including their padding from the next sibling,
        // so that we can hide and show them again later.
        initChildrenWidths(this);
        sStaticInitialized = true;
    }

    computeLabelRightPadding();
	return LLFolderViewItem::postBuild();
}

void LLConversationViewParticipant::draw()
{
    static LLUIColor sFgColor = LLUIColorTable::instance().getColor("MenuItemEnabledColor", DEFAULT_WHITE);
    static LLUIColor sHighlightFgColor = LLUIColorTable::instance().getColor("MenuItemHighlightFgColor", DEFAULT_WHITE);
    static LLUIColor sHighlightBgColor = LLUIColorTable::instance().getColor("MenuItemHighlightBgColor", DEFAULT_WHITE);
    static LLUIColor sFocusOutlineColor = LLUIColorTable::instance().getColor("InventoryFocusOutlineColor", DEFAULT_WHITE);
    static LLUIColor sMouseOverColor = LLUIColorTable::instance().getColor("InventoryMouseOverColor", DEFAULT_WHITE);

    const BOOL show_context = (getRoot() ? getRoot()->getShowSelectionContext() : FALSE);
    const BOOL filled = show_context || (getRoot() ? getRoot()->getParentPanel()->hasFocus() : FALSE); // If we have keyboard focus, draw selection filled

    const LLFontGL* font = getLabelFontForStyle(mLabelStyle);
    F32 right_x  = 0;

    F32 y = (F32)getRect().getHeight() - font->getLineHeight() - (F32)mTextPad;
    F32 text_left = (F32)getLabelXPos();
    LLColor4 color = (mIsSelected && filled) ? sHighlightFgColor : sFgColor;

    drawHighlight(show_context, filled, sHighlightBgColor, sFocusOutlineColor, sMouseOverColor);
    drawLabel(font, text_left, y, color, right_x);

    LLView::draw();
}


void LLConversationViewParticipant::refresh()
{
	// Refresh the participant view from its model data
	LLConversationItem* vmi = dynamic_cast<LLConversationItem*>(getViewModelItem());
	vmi->resetRefresh();
	
	// Note: for the moment, all that needs to be done is done by LLFolderViewItem::refresh()
	
	// Do the regular upstream refresh
	LLFolderViewItem::refresh();
}

void LLConversationViewParticipant::addToFolder(LLFolderViewFolder* folder)
{
    //Add the item to the folder (conversation)
    LLFolderViewItem::addToFolder(folder);
	
    //Now retrieve the folder (conversation) UUID, which is the speaker session
    LLConversationItem* vmi = this->getParentFolder() ? dynamic_cast<LLConversationItem*>(this->getParentFolder()->getViewModelItem()) : NULL;
    if(vmi)
    {
        //Allows speaking icon image to be loaded based on mUUID
        mAvatarIcon->setValue(mUUID);

        //Allows the speaker indicator to be activated based on the user and conversation
        mSpeakingIndicator->setSpeakerId(mUUID, vmi->getUUID()); 
    }
}

void LLConversationViewParticipant::onInfoBtnClick()
{
	LLFloaterReg::showInstance("inspect_avatar", LLSD().with("avatar_id", mUUID));
}

void LLConversationViewParticipant::onMouseEnter(S32 x, S32 y, MASK mask)
{
    mInfoBtn->setVisible(true);
    computeLabelRightPadding();
    LLFolderViewItem::onMouseEnter(x, y, mask);
}

void LLConversationViewParticipant::onMouseLeave(S32 x, S32 y, MASK mask)
{
    mInfoBtn->setVisible(false);
    computeLabelRightPadding();
    LLFolderViewItem::onMouseEnter(x, y, mask);
}

S32 LLConversationViewParticipant::getLabelXPos()
{
    return mAvatarIcon->getRect().mRight + mIconPad;
}

// static
void LLConversationViewParticipant::initChildrenWidths(LLConversationViewParticipant* self)
{
    //speaking indicator width + padding
    S32 speaking_indicator_width = self->getRect().getWidth() - self->mSpeakingIndicator->getRect().mLeft;

    //info btn width + padding
    S32 info_btn_width = self->mSpeakingIndicator->getRect().mLeft - self->mInfoBtn->getRect().mLeft;

    S32 index = ALIC_COUNT;
    sChildrenWidths[--index] = info_btn_width;
    sChildrenWidths[--index] = speaking_indicator_width;
    llassert(index == 0);
}

void LLConversationViewParticipant::computeLabelRightPadding()
{
    mLabelPaddingRight = DEFAULT_LABEL_PADDING_RIGHT;
    LLView* control;
    S32 ctrl_width;

    for (S32 i = 0; i < ALIC_COUNT; ++i)
    {
        control = getItemChildView((EAvatarListItemChildIndex)i);

        // skip invisible views
        if (!control->getVisible()) continue;

        ctrl_width = sChildrenWidths[i]; // including space between current & left controls
        // accumulate the amount of space taken by the controls
        mLabelPaddingRight += ctrl_width;
    }
}

LLView* LLConversationViewParticipant::getItemChildView(EAvatarListItemChildIndex child_view_index)
{
    LLView* child_view = NULL;

    switch (child_view_index)
    {
        case ALIC_SPEAKER_INDICATOR:
            child_view = mSpeakingIndicator;
            break;
        case ALIC_INFO_BUTTON:
            child_view = mInfoBtn;
            break;
        default:
            LL_WARNS("AvatarItemReshape") << "Unexpected child view index is passed: " << child_view_index << LL_ENDL;
            llassert(0);
            break;
            // leave child_view untouched
    }

    return child_view;
}

// EOF

