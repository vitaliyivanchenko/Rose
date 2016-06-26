#define GETTEXT_DOMAIN "studio-lib"

#include "gui/dialogs/build.hpp"

#include "display.hpp"
#include "game_config.hpp"
#include "preferences.hpp"
#include "gettext.hpp"
#include "gui/auxiliary/timer.hpp"
#include "gui/dialogs/message.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/label.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/track.hpp"
#include "gui/widgets/window.hpp"
#include "preferences_display.hpp"
#include "help.hpp"
#include "filesystem.hpp"
#include "loadscreen.hpp"
#include <time.h>

#include <boost/bind.hpp>

#include <algorithm>

tbuild::tbuild()
	: editor_(game_config::path)
	, build_ctx_(*this)
	, exit_task_(false)
	, require_set_task_bar_(true)
{
}

tbuild::~tbuild()
{
	exit_task_ = true;
}

void tbuild::pre_show(gui2::ttrack& track)
{
	track.set_canvas_variable("background_image", variant(null_str));
	track.set_did_timer(boost::bind(&tbuild::task_status_callback, this, _1, _2, _3));
	track.set_enable_drag_draw_coordinate(false);
	track.set_timer_interval(100);
	task_status_ = &track;
}

static void increment_progress_cb2(std::string const &name, uint32_t param1, void* param2)
{
	tbuild::tbuild_ctx* ctx = (tbuild::tbuild_ctx*)param2;
	ctx->name = name;
	ctx->nfiles ++;
}

void tbuild::do_build2()
{
	thread_->Start();
}

void tbuild::DoWork()
{
	// this is in thread. don't call any operator aboult dialog.
	const std::vector<std::pair<editor::BIN_TYPE, editor::wml2bin_desc> >& descs = editor_.wml2bin_descs();
	set_increment_progress progress(increment_progress_cb2, &build_ctx_);

	int count = (int)descs.size();
	for (int at = 0; at < count && !exit_task_; at ++) {
		const std::pair<editor::BIN_TYPE, editor::wml2bin_desc>& desc = descs[at];
		if (!desc.second.require_build) {
			continue;
		}
		main_->Invoke<void>(RTC_FROM_HERE, rtc::Bind(&tbuild::handle_desc, this, desc, true, at, true));

		bool ret = false;
		try {
			ret = editor_.cfgs_2_cfg(desc.first, desc.second.bin_name, desc.second.app, true, desc.second.wml_nfiles, desc.second.wml_sum_size, (uint32_t)desc.second.wml_modified, tdomains);
		} catch (twml_exception& e) {
			e.show();
		}
		main_->Invoke<void>(RTC_FROM_HERE, rtc::Bind(&tbuild::handle_desc, this, desc, false, at, ret));
	}
}

void tbuild::OnWorkStart()
{
	app_work_start();
}

void tbuild::OnWorkDone()
{ 
	build_ctx_.reset(gui2::twidget::npos);

	app_work_done();

	task_status_->set_dirty();
}

void tbuild::handle_desc(const std::pair<editor::BIN_TYPE, editor::wml2bin_desc>& desc, const bool started, const int at, const bool ret)
{
	if (started) {
		build_ctx_.reset(at);
	}
	app_handle_desc(started, at, ret);
}

void tbuild::task_status_callback(gui2::ttrack& widget, const gui2::tpoint& offset, const bool blit_bg)
{
	const SDL_Rect widget_rect = widget.get_rect();

	const int xsrc = offset.x + widget_rect.x;
	const int ysrc = offset.y + widget_rect.y;

	SDL_Renderer* renderer = get_renderer();
	SDL_Rect dst = widget_rect;

	if (build_ctx_.desc_at == gui2::twidget::npos) {
		if (!blit_bg || require_set_task_bar_) {
			require_set_task_bar_ = false;

			draw_rect2(renderer, dst, 0xfffefefe);

			surface text_surf = font::get_rendered_text2(editor_.working_dir(), INT32_MAX, 12 * gui2::twidget::hdpi_scale, font::BLACK_COLOR);
			dst = ::create_rect(xsrc + 4 * gui2::twidget::hdpi_scale, ysrc + (widget_rect.h - text_surf->h) / 2, text_surf->w, text_surf->h);
			blit_from_surface(renderer, text_surf, NULL, &dst);
		}
		return;
	}


	if (blit_bg) {
		SDL_RenderCopy(renderer, widget.background_texture().get(), NULL, &widget_rect);
	}

	const std::pair<editor::BIN_TYPE, editor::wml2bin_desc>& desc = editor_.wml2bin_descs()[build_ctx_.desc_at];

	dst.w = build_ctx_.nfiles * widget_rect.w / desc.second.wml_nfiles;
	draw_rect2(renderer, dst, 0xff00ff00);

	std::stringstream ss;
	ss << build_ctx_.nfiles << "/" << desc.second.wml_nfiles;
	if (!build_ctx_.name.empty()) {
		ss << "    " << build_ctx_.name.substr(editor_.working_dir().size());
	}
	surface text_surf = font::get_rendered_text2(ss.str(), INT32_MAX, 12 * gui2::twidget::hdpi_scale, font::BLACK_COLOR);
	dst = ::create_rect(xsrc + 4 * gui2::twidget::hdpi_scale, ysrc + (widget_rect.h - text_surf->h) / 2, text_surf->w, text_surf->h);
	blit_from_surface(renderer, text_surf, NULL, &dst);
}