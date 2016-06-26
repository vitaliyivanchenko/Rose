/* $Id: title_screen.cpp 48740 2011-03-05 10:01:34Z mordante $ */
/*
   Copyright (C) 2008 - 2011 by Mark de Wever <koraq@xs4all.nl>
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

#include "gui/widgets/effect.hpp"
#include "gui/widgets/track.hpp"

#include "display.hpp"
#include "font.hpp"
#include "gettext.hpp"

#include <boost/bind.hpp>
#include <algorithm>

namespace gui2 {

namespace effect {

void trefresh::start_mascot_anim(int x, int y)
{
	if (anim_type_ == anim2::NONE) {
		return;
	}
	if (is_null_rect(mascot_anim_rect_)) {
		surface surf = image::get_image("misc/mascot.png");
		mascot_anim_rect_ = ::create_rect(x, y, surf->w, surf->h);
	}

	if (mascot_anim_id_ == INVALID_ANIM_ID) {
		float_animation* anim = start_float_anim_th(disp_, anim_type_, &mascot_anim_id_);
		anim->set_scale(0, 0, true, false);
		anim->set_special_rect(mascot_anim_rect_);

		start_float_anim_bh(*anim, true);

	} else {
		float_animation* anim = dynamic_cast<float_animation*>(&disp_.area_anim(mascot_anim_id_));
		mascot_anim_rect_.x = x;
		mascot_anim_rect_.y = y;
		anim->set_special_rect(mascot_anim_rect_);
	}
}

void trefresh::erase_mascot_anim()
{
	if (mascot_anim_id_ != INVALID_ANIM_ID) {
		disp_.erase_area_anim(mascot_anim_id_);
		mascot_anim_id_ = INVALID_ANIM_ID;
	}
}

bool trefresh::did_control_drag_detect(tcontrol* control, bool start, const twidget::tdrag_direction type)
{
	if (start) {
		if (did_can_drag_ && !did_can_drag_()) {
			return false;
		}
		springback_granularity_ = twidget::npos;
		track_status_ = track_drag;
		// maybe start drag when springback.
		yoffset_at_start_ = springback_yoffset_;

	} else if (track_status_ == track_drag) {
		springback_yoffset_ = control->last_drag_coordinate().y - control->first_drag_coordinate().y + yoffset_at_start_;
		track_status_ = track_springback;
	}
	return false;
}

int trefresh::draw(const int _set_yoffset)
{
	int set_yoffset = _set_yoffset;
	set_yoffset += yoffset_at_start_;
	if (_set_yoffset == 0) {
		if (track_status_ == track_drag) {
			set_yoffset = widget_->last_drag_coordinate().y - widget_->first_drag_coordinate().y;
			set_yoffset += yoffset_at_start_;

		} else if (track_status_ == track_springback) {
			// recalculate springback_yoffset_
			if (springback_yoffset_ > 0) {
				if (springback_granularity_ == twidget::npos) {
					springback_granularity_ = springback_yoffset_ / 6;
				}
				springback_yoffset_ -= springback_granularity_;
			}
			if (refresh_ == refreshing) {
				if (springback_yoffset_ < max_egg_diameter_ + egg_gap_) {
					springback_yoffset_ = max_egg_diameter_ + egg_gap_;
				}
			} else {
				if (refresh_ == refreshed) {
					// linger some time if refreshed.
					uint32_t now = SDL_GetTicks();
					if (refreshed_linger_ticks_ == 0 && springback_yoffset_ <= max_egg_diameter_ + egg_gap_) {
						refreshed_linger_ticks_ = now + 500; // 500 msecond
						springback_granularity_ = max_egg_diameter_ / 4;
					}
					if (now < refreshed_linger_ticks_) {
						springback_yoffset_ = max_egg_diameter_ + egg_gap_;
					}
				}
				if (springback_yoffset_ <= 0) {
					track_status_ = track_normal;
					refresh_ = norefresh;
					springback_yoffset_ = 0;
					yoffset_at_start_ = 0;
					refreshed_linger_ticks_ = 0;
				}
			}
			set_yoffset = springback_yoffset_;
		}
	}

	const SDL_Rect widget_rect = widget_->get_rect();
	const int xsrc = widget_rect.x;
	const int ysrc = widget_rect.y;
	SDL_Rect dst;

	SDL_Renderer* renderer = get_renderer();
	if (refresh_ == norefresh) {
		if (set_yoffset > egg_gap_) {
			surface surf = image::get_image(image_);
			if (surf) {
				int width = set_yoffset - egg_gap_, height = set_yoffset - egg_gap_;
				if (set_yoffset >= shape_egg_diameter_ + egg_gap_) {
					width = shape_egg_diameter_;
				}
				dst = ::create_rect(xsrc + (widget_rect.w - width) / 2, ysrc + egg_gap_, width, height);

				blit_from_surface(renderer, surf, NULL, &dst);
			}
		}
		if (set_yoffset >= max_egg_diameter_ + egg_gap_) {
			refresh_ = refreshing;
			if (did_refreshing_) {
				did_refreshing_();
			}
		}

		if (mascot_anim_id_ != INVALID_ANIM_ID && set_yoffset == 0) {
			erase_mascot_anim();
		}

	} else if (refresh_ == refreshing) {
		// VALIDATE(set_yoffset >= egg_gap_ + max_egg_diameter_, null_str);
		start_mascot_anim(xsrc + (widget_rect.w - mascot_anim_rect_.w) / 2, ysrc + set_yoffset - max_egg_diameter_);

	} else {
		if (mascot_anim_id_ != INVALID_ANIM_ID) {
			erase_mascot_anim();
		}

		// refreshed
		if (refreshed_text_.empty()) {
			refreshed_text_ = _("Refresh successfully");
		}
		surface text_surf = font::get_rendered_text2(refreshed_text_, -1, 32, font::NORMAL_COLOR);
		dst = ::create_rect(xsrc + (widget_rect.w - text_surf->w) / 2, ysrc + set_yoffset - ((max_egg_diameter_ - text_surf->h) / 2 + text_surf->h), text_surf->w, text_surf->h);
		blit_from_surface(renderer, text_surf, NULL, &dst);
	}

	return set_yoffset;
}

void tfold::set_widget(tstacked_widget* widget)
{
	VALIDATE(widget && !widget_, null_str);
	widget_ = widget;
	top_grid_ = dynamic_cast<tstacked_widget::tgrid3*>(widget->layer(0));
	bottom_grid_ = dynamic_cast<tstacked_widget::tgrid3*>(widget->layer(1));
}

bool tfold::did_control_drag_detect(tcontrol* control, bool start, const twidget::tdrag_direction type)
{
	if (start) {
		VALIDATE(!draging_, null_str);

		std_top_rect_ = null_rect;
		diff_ = tpoint(0, 0);

		if (!did_can_drag_ || did_can_drag_(control->first_drag_coordinate().x, control->first_drag_coordinate().y)) {
			draging_ = true;
			// drag_line_callback_timer(*drag_line_, drag_line_->get_frame_offset(), false);
		}

	} else {
		if (!is_null_rect(std_top_rect_)) {
			if (upward_) {
				if (diff_.y < 0) {
					if (diff_.y < -1 * std_top_rect_.h / 5) {
						move(-1 * (std_top_rect_.h - min_reserve_height_));
						upward_ = false;

					} else {
						move(0);
					}
				} else {
					// diff_.y isn't increase/decrease 1, exist segment when drag.
					// diff_.y of standard-line maybe in segment, and result in not triger by system.
					// so should reset standard-line again.
					move(0);
				}
			} else {
				if (diff_.y > 0) {
					if (diff_.y > std_top_rect_.h / 4) {
						move(std_top_rect_.h - min_reserve_height_);
						upward_ = true;

					} else {
						move(0);
					}
				} else {
					// diff_.y isn't increase/decrease 1, exist segment when drag.
					// diff_.y of standard-line maybe in segment, and result in not triger by system.
					// so should reset standard-line again.
					move(0);
				}
			}
		}

		if (draging_) {
			draging_ = false;
			// drag_line_callback_timer(*drag_line_, drag_line_->get_frame_offset(), false);
		}
	}

	return false;
}

bool tfold::did_drag_coordinate(tcontrol* control, const tpoint& first, const tpoint& last)
{
	diff_ = tpoint(last.x - first.x, last.y - first.y);

	if (!draging_) {
		return false;
	}

	if (is_null_rect(std_top_rect_)) {
		std_top_rect_ = top_grid_->get_rect();
		std_bottom_rect_ = bottom_grid_->get_rect();

		if (did_calculate_reserve_) {
			int reserve_y;
			did_calculate_reserve_(reserve_y, min_reserve_height_);
			VALIDATE(reserve_y >= std_top_rect_.y, null_str);
			VALIDATE(min_reserve_height_ >= 0, null_str);

			max_top_movable_ = reserve_y - std_top_rect_.y;
		} else {
			max_top_movable_ = 0;
			min_reserve_height_ = 0;
		}
	}

	move(diff_.y);

	return false;
}

void tfold::move(const int diff_y) const
{
	if (diff_y < 0) {
		if (!upward_) {
			return;
		}
		SDL_Rect rect0 = std_top_rect_;
		SDL_Rect rect1 = std_bottom_rect_;

		int abs_diff_y = abs(diff_y);
		//
		// move calendar
		//
		int move_top_y = abs_diff_y;
		if (move_top_y > max_top_movable_) {
			move_top_y = max_top_movable_;
		}

		rect0.h = std_top_rect_.h - abs_diff_y;
		if (rect0.h >= min_reserve_height_) {
			top_grid_->set_origin(tpoint(std_top_rect_.x, std_top_rect_.y - move_top_y));
			top_grid_->set_visible_area(rect0);
			top_grid_->set_dirty(true);

			//
			// move data
			//
			twidget::tsimple_place_lock lock;
			bottom_grid_->place(tpoint(std_bottom_rect_.x, std_bottom_rect_.y + diff_y), tpoint(std_bottom_rect_.w, std_bottom_rect_.h + abs_diff_y));
			bottom_grid_->set_dirty(true);
		}

	} else if (diff_y > 0) {
		if (upward_) {
			return;
		}
		SDL_Rect rect0 = std_top_rect_;
		SDL_Rect rect1 = std_bottom_rect_;

		const int max_bottom_height = std_top_rect_.h - max_top_movable_ - min_reserve_height_;

		//
		// move calendar
		//
		int move_top_y;
		if (diff_y < max_bottom_height) {
			move_top_y = 0;
			rect0.y += max_top_movable_;
		} else {
			move_top_y = diff_y - max_bottom_height;
			// rect0.y += max_top_movable_ - move_top_y;
			rect0.y += max_top_movable_;
		}

		rect0.h = min_reserve_height_ + diff_y;
		if (rect0.h <= std_top_rect_.h) {
			top_grid_->set_origin(tpoint(std_top_rect_.x, std_top_rect_.y + move_top_y));
			top_grid_->set_visible_area(rect0);
			top_grid_->set_dirty(true);

			//
			// move data
			//
			twidget::tsimple_place_lock lock;
			bottom_grid_->place(tpoint(std_bottom_rect_.x, std_bottom_rect_.y + diff_y), tpoint(std_bottom_rect_.w, std_bottom_rect_.h - diff_y));
			bottom_grid_->set_dirty(true);
		}

	} else {
		if (top_grid_->get_rect() != std_top_rect_) {
			SDL_Rect clip_rect = std_top_rect_;
			if (!upward_) {
				clip_rect.y += max_top_movable_;
				clip_rect.h = min_reserve_height_;
			}
			top_grid_->set_origin(tpoint(std_top_rect_.x, std_top_rect_.y));
			top_grid_->set_visible_area(clip_rect);
			top_grid_->set_dirty(true);
		}

		if (bottom_grid_->get_rect() != std_bottom_rect_) {
			twidget::tsimple_place_lock lock;
			bottom_grid_->place(tpoint(std_bottom_rect_.x, std_bottom_rect_.y), tpoint(std_bottom_rect_.w, std_bottom_rect_.h));
			bottom_grid_->set_dirty(true);
		}
	}
}

} // namespace effect

} // namespace gui2

