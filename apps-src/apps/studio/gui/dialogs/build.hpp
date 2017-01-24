#ifndef GUI_DIALOGS_BUILD_HPP_INCLUDED
#define GUI_DIALOGS_BUILD_HPP_INCLUDED

#include "gui/dialogs/dialog.hpp"
#include "editor.hpp"
#include "thread.hpp"

class display;

namespace gui2 {
class ttrack;
}

class tbuild: public tworker
{
public:
	tbuild();
	virtual ~tbuild();

	struct tbuild_ctx {
		tbuild_ctx(tbuild& owner)
			: owner(owner)
			, nfiles(0)
			, desc_at(gui2::twidget::npos)
		{}
		void reset(int _desc_at)
		{
			nfiles = 0;
			name.clear();
			desc_at = _desc_at;
		}

		size_t nfiles;
		std::string name;
		int desc_at;
		tbuild& owner;
	};

protected:
	void pre_show(gui2::ttrack& track);
	bool is_building() const { return build_ctx_.desc_at != gui2::twidget::npos; }
	void do_build2();

	void DoWork() override;
	void OnWorkStart() override;
	void OnWorkDone() override;

private:
	void handle_desc(const std::pair<editor::BIN_TYPE, editor::wml2bin_desc>& desc, const bool started, const int at, const bool ret);
	void task_status_callback(gui2::ttrack& widget, const gui2::tpoint& offset, const bool from_timer);

	virtual void app_work_start() = 0;
	virtual void app_work_done() = 0;
	virtual void app_handle_desc(const bool started, const int at, const bool ret) = 0;

protected:
	tbuild_ctx build_ctx_;
	bool exit_task_;

	bool require_set_task_bar_;
	gui2::ttrack* task_status_;
	editor editor_;

	std::map<std::string, std::string> tdomains;
};

#endif
