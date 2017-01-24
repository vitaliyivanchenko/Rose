/* $Id: campaign_difficulty.hpp 49603 2011-05-22 17:56:17Z mordante $ */
/*
   Copyright (C) 2010 - 2011 by Ignacio Riquelme Morelle <shadowm2006@gmail.com>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef GUI_DIALOGS_MENU_HPP_INCLUDED
#define GUI_DIALOGS_MENU_HPP_INCLUDED

#include "gui/dialogs/dialog.hpp"

#include <vector>

namespace gui2 {

class ttoggle_panel;
class tlistbox;

class tmenu : public tdialog
{
public:
	struct titem {
		titem(int val, const std::string& str, const std::vector<titem>& submenu = std::vector<tmenu::titem>())
			: val(val)
			, str(str)
			, submenu(submenu)
		{}

		int val;
		std::string str;
		std::vector<titem> submenu;
	};
	explicit tmenu(display& disp, const std::vector<titem>& items, int val, const tmenu* parent = NULL);

	/**
	 * Returns the selected item index after displaying.
	 * @return -1 if the dialog was cancelled.
	 */
	int selected_index() const { return index_; }
	int selected_val() const { return selected_val_ != twidget::npos? selected_val_: items_[index_].val; }

	const std::vector<titem>& items() const { return items_; }
	void request_close(int retval, int selected_val);

private:
	/** Inherited from tdialog, implemented by REGISTER_DIALOG. */
	virtual const std::string& window_id() const;

	/** Inherited from tdialog. */
	void pre_show(CVideo& video, twindow& window);

	/** Inherited from tdialog. */
	void post_show(twindow& window);

	void item_selected(twindow& window, tlistbox& list, twidget& widget, const int type);
	void item_focus_changed(twindow& window, tlistbox& list, twidget& panel, const bool enter);
	bool did_leave_dismiss(twindow& window, const int x, const int y);
	bool did_click_dismiss_except(twindow& window, const int x, const int y);
	const ttoggle_panel& focused_panel() const;

private:
	twindow* window_;
	display& disp_;
	int index_;
	std::vector<titem> items_;
	const tmenu* parent_;

	bool mouse_motioned_;
	ttoggle_panel* focused_panel_;
	int selected_val_;
};


}


#endif /* ! GUI_DIALOGS_MENU_HPP_INCLUDED */
