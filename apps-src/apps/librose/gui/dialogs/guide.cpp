/* $Id: campaign_difficulty.cpp 49602 2011-05-22 17:56:13Z mordante $ */
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

#define GETTEXT_DOMAIN "rose-lib"

#include "gui/dialogs/guide.hpp"

#include "gettext.hpp"

#include "display.hpp"
#include "gui/dialogs/helper.hpp"
#include "gui/widgets/listbox.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"
#include "gui/widgets/track.hpp"

#include <boost/bind.hpp>

namespace gui2 {

tguide_::tguide_(display& disp, const std::vector<std::string>& items, int retval)
	: disp_(disp)
	, items_(items)
	, retval_(retval)
	, cursel_(0)
	, can_exit_(false)
{
	VALIDATE(!items.empty(), "Must set items!");
}

void tguide_::pre_show(CVideo& /*video*/, twindow& window)
{
	track_ = find_widget<ttrack>(&window, "image", false, true);
	track_->set_canvas_variable("background_image", variant(null_str));
	track_->set_did_timer(boost::bind(&tguide::callback_timer, this, _1, _2, _3, 0));
	track_->set_did_control_drag_detect(boost::bind(&tguide_::callback_control_drag_detect, this, _1, _2, _3));
	track_->set_did_drag_coordinate(boost::bind(&tguide_::callback_set_drag_coordinate, this, _1, _2, _3));
	track_->set_enable_drag_draw_coordinate(false);

	if (retval_ != twindow::NONE) {
		connect_signal_mouse_left_click(
			*track_
			, boost::bind(
				&tguide_::set_retval
				, this
				, boost::ref(window)
				, retval_));
	}
}

void tguide_::callback_timer(ttrack& widget, const tpoint& offset, const bool blit_bg, int drag_offset_x)
{
	SDL_Renderer* renderer = get_renderer();
	const SDL_Rect widget_rect = widget.get_rect();

	if (blit_bg) {
		SDL_RenderCopy(renderer, widget.background_texture().get(), NULL, &widget_rect);
	}

	const int xsrc = offset.x + widget_rect.x;
	const int ysrc = offset.y + widget_rect.y;

	int left_sel = twidget::npos, right_sel = twidget::npos;
	if (drag_offset_x < 0) {
		left_sel = cursel_;
		if (cursel_ + 1 < (int)items_.size()) {
			right_sel = cursel_ + 1;
		}

	} else if (drag_offset_x > 0) {
		if (cursel_ > 0) {
			left_sel = cursel_ - 1;
		}
		right_sel = cursel_;

	} else {
		left_sel = cursel_;
	}
	
	if (surfs_.empty()) {
		// in order to smooth, preload all image
		std::stringstream err;
		for (std::vector<std::string>::const_iterator it = items_.begin(); it != items_.end(); ++ it) {
			surface surf = image::get_image(*it);
			
			err.str("");
			err << *it << " isn't existed!";
			VALIDATE(surf, err.str());

			surfs_.push_back(SDL_CreateTextureFromSurface(renderer, surf));
		}
	}

	std::string img;
	texture left_surf, right_surf;
	int left_surf_w = 0, left_surf_h = 0, right_surf_w = 0, right_surf_h = 0;
	if (left_sel != twidget::npos) {
		left_surf = surfs_[left_sel];
		SDL_QueryTexture(left_surf.get(), NULL, NULL, &left_surf_w, &left_surf_h);
	}

	if (right_sel != twidget::npos) {
		right_surf = surfs_[right_sel];
		SDL_QueryTexture(right_surf.get(), NULL, NULL, &right_surf_w, &right_surf_h);
	}

	SDL_Rect left_src, left_dst, right_src, right_dst;

	if (!drag_offset_x) {
		VALIDATE(left_sel != twidget::npos && right_sel == twidget::npos, null_str);
		left_src = ::create_rect(0, 0, left_surf_w, left_surf_h);
		left_dst = ::create_rect(xsrc, ysrc, widget_rect.w, widget_rect.h);
		SDL_RenderCopy(renderer, left_surf.get(), &left_src, &left_dst);

		post_blit(widget_rect, left_sel, left_src, left_dst, right_sel, right_src, right_dst);
		return;
	}

	const int abs_drag_offset_x = abs(drag_offset_x);
	if (left_surf) {
		double wratio = 1.0 * left_surf_w / widget_rect.w;
		if (drag_offset_x < 0) {
			left_src = ::create_rect(abs_drag_offset_x * wratio, 0, 0, left_surf_h);
			left_src.w = left_surf_w - left_src.x;
			left_dst = ::create_rect(xsrc, ysrc, widget_rect.w - abs_drag_offset_x, widget_rect.h);
		} else {
			left_src = ::create_rect(0, 0, abs_drag_offset_x * wratio, left_surf_h);
			left_src.x = left_surf_w - left_src.w;
			left_dst = ::create_rect(xsrc, ysrc, abs_drag_offset_x, widget_rect.h);
		}

		SDL_RenderCopy(renderer, left_surf.get(), &left_src, &left_dst);
	}
	if (right_surf) {
		double wratio = 1.0 * right_surf_w / widget_rect.w;
		if (drag_offset_x < 0) {
			right_src = ::create_rect(0, 0, abs_drag_offset_x * wratio, right_surf_h);
			right_dst = ::create_rect(xsrc + widget_rect.w - abs_drag_offset_x, ysrc, abs_drag_offset_x, widget_rect.h);
		} else {
			right_src = ::create_rect(0, 0, right_surf_w - abs_drag_offset_x * wratio, right_surf_h);
			right_dst = ::create_rect(xsrc + abs_drag_offset_x, ysrc, widget_rect.w - abs_drag_offset_x, widget_rect.h);
		}

		SDL_RenderCopy(renderer, right_surf.get(), &right_src, &right_dst);
	}

	post_blit(widget_rect, left_sel, left_src, left_dst, right_sel, right_src, right_dst);
}

bool tguide_::callback_control_drag_detect(tcontrol* control, bool start, const twidget::tdrag_direction type)
{
	if (!start) {
		int drag_offset_x = control->last_drag_coordinate().x - control->first_drag_coordinate().x;

		if (abs(drag_offset_x) >= (int)control->get_width() / 3) {
			if (drag_offset_x < 0) {
				if (cursel_ + 1 < (int)items_.size()) {
					cursel_ ++;
				} else {
					can_exit_ = true;
				}
			} else if (drag_offset_x > 0 && cursel_ > 0) {
				cursel_ --;
				if (cursel_ < 0) {
					cursel_ = 0;
				}
			}
		}
		callback_set_drag_coordinate(control, control->last_drag_coordinate(), control->last_drag_coordinate());
	}

	return false;
}

bool tguide_::callback_set_drag_coordinate(tcontrol* control, const tpoint& first, const tpoint& last)
{
	int drag_offset_x = last.x - first.x;
	ttrack& widget = find_widget<ttrack>(control->get_window(), "image", false);
	callback_timer(widget, widget.get_frame_offset(), true, drag_offset_x);

	return false;
}

void tguide_::set_retval(twindow& window, int retval)
{
	if (!can_exit_) {
		return;
	}
	window.set_retval(retval);
}

REGISTER_DIALOG(rose, guide)

tguide::tguide(display& disp, const std::vector<std::string>& items, const std::string& startup_img, const int percent)
	: tguide_(disp, items, twindow::OK)
	, startup_img_(startup_img)
	, percent_(percent)
	, startup_rect_(empty_rect)
{}

void tguide::pre_show(CVideo& video, twindow& window)
{
	window.canvas()[0].set_variable("background_image",	variant("dialogs/white-background.png"));

	tguide_::pre_show(video, window);
}

void tguide::post_blit(const SDL_Rect& widget_rect, int left_sel, const SDL_Rect& left_src, const SDL_Rect& left_dst, int right_sel, const SDL_Rect& right_src, const SDL_Rect& right_dst)
{
	SDL_Renderer* renderer = get_renderer();
	const int last_at = surfs_.size() - 1;
	if (left_sel != last_at && right_sel != last_at) {
		return;
	}
	if (startup_img_.empty()) {
		return;
	}
	surface surf2 = image::get_image(startup_img_);
	if (!surf2) {
		return;
	}
	if (startup_tex_.get() == NULL) {
		startup_tex_ = SDL_CreateTextureFromSurface(renderer, surf2);
	}
	int startup_tex_w, startup_tex_h;
	SDL_QueryTexture(startup_tex_.get(), NULL, NULL, &startup_tex_w, &startup_tex_h);

	texture last_surf = surfs_.back();
	SDL_Rect dst = empty_rect;

	if (left_sel == last_at) {
		VALIDATE(right_sel == twidget::npos, null_str);
		dst.x = left_dst.x + (widget_rect.w - startup_tex_w) / 2;
		dst.y = left_dst.y + widget_rect.h * percent_ / 100;

		dst.x -= widget_rect.w - left_dst.w;

	} else {
		dst.x = right_dst.x + (widget_rect.w - startup_tex_w) / 2;
		dst.y = right_dst.y + widget_rect.h * percent_ / 100;
	}

	dst.w = startup_tex_w;
	dst.h = startup_tex_h;

	SDL_RenderCopy(renderer, startup_tex_.get(), NULL, &dst);

	if (left_sel == last_at && !left_src.x) {
		startup_rect_ = ::create_rect(dst.x, dst.y, startup_tex_w, startup_tex_h);
	} else {
		startup_rect_ = empty_rect;
	}
}

void tguide::set_retval(twindow& window, int retval)
{
	if (startup_img_.empty()) {
		tguide_::set_retval(window, retval);
		return;
	}

	const tpoint& first = track_->first_drag_coordinate();
	const tpoint& last = track_->last_drag_coordinate();
	if (first != last || !point_in_rect(first.x, first.y, startup_rect_) || !point_in_rect(last.x, last.y, startup_rect_)) {
		return;
	}

	window.set_retval(retval);
}

}
