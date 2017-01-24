#ifndef LIBROSE_TASK_HPP_INCLUDED
#define LIBROSE_TASK_HPP_INCLUDED

#include "config_cache.hpp"
#include "config.hpp"
#include "version.hpp"
#include <set>

#include <boost/bind.hpp>
#include <boost/function.hpp>

class display;

class tcallback_lock
{
public:
	tcallback_lock(bool result, const boost::function<void (const bool)>& callback)
		: result_(result)
		, did_destruction_(callback)
	{}

	~tcallback_lock()
	{
		if (did_destruction_) {
			did_destruction_(result_);
		}
	}
	void set_result(bool val) { result_ = val; }

private:
	bool result_;
	boost::function<void (const bool)> did_destruction_;
};

class ttask
{
public:
	template<class T>
	static std::unique_ptr<T> create_task(const config& cfg, const std::string& key, void* context)
	{
		T* task = new T(cfg, context);
		task->parse_cfg(cfg, key);
		return std::unique_ptr<T>(task);
	}

	static const std::string res_alias; // ex: c:/ddksample/apps-res
	static const std::string src_alias; // ex: c:/ddksample/apps-src
	static const std::string src2_alias; // ex: c:/ddksample/apps-res/apps
	static const std::string user_alias; // ex: user data directory

	static const std::string app_res_alias; // ex: c:/ddksample/blesmart-res
	static const std::string app_src_alias; // ex: c:/ddksample/blesmart-src
	static const std::string app_src2_alias; // ex: c:/ddksample/blesmart-res/blesmart


	enum res_type {res_none, res_file, res_dir, res_files};

	class tsubtask {
	public:
		static std::string type_2_printable_name(int type);

		virtual bool handle(display& disp) = 0;
		virtual bool fail_rollback(display& disp) const = 0;

		const std::string& id() const { return subtask_id_; }

	protected:
		explicit tsubtask(ttask& task, const std::string& id, const config& cfg, const std::map<std::string, std::string>& base_paths)
			: task_(task)
			, subtask_id_(id)
			, paths_(base_paths)
		{}

	protected:
		void replace_str(std::string& name, const std::pair<std::string, std::string>& replace) const;
		bool do_delete_path(display& disp, const std::set<std::string>& paths) const;
		bool make_path(display& disp, const std::string& tag, bool del) const;
		const std::string& alias_2_path(const std::string& alias) const;

	protected:
		ttask& task_;
		const std::string subtask_id_;
		std::map<std::string, std::string> paths_;
	};

	class tsubtask_copy: public tsubtask
	{
	public:
		explicit tsubtask_copy(ttask& task, const std::string& id, const config& cfg, const std::map<std::string, std::string>& base_paths);

		bool handle(display& disp) override;
		bool fail_rollback(display& disp) const override;

	private:
		void did_fail_rollback(display& disp, const bool result) const;
		bool do_copy(display& disp, const std::string& src_alias, const std::string& dst_alias);

		struct tres {
			tres(res_type type, const std::string& name, const std::string& custom, const bool overwrite)
				: type(type)
				, name(name)
				, custom(custom)
				, overwrite(overwrite)
			{}

			res_type type;
			std::string name;
			std::string custom;
			bool overwrite;
		};
		void resolve_res_2_rollback(const res_type type, const std::string& name, const std::set<std::string>* files = NULL);

	private:
		std::string src_alias_;
		std::string dst_alias_;

		std::vector<tres> copy_res_;
		std::set<std::string> require_delete_;

		struct trollback {
			trollback(const res_type type, const std::string& name)
				: type(type)
				, name(name)
			{}

			res_type type;
			std::string name;
		};

		std::map<int, trollback> rollbacks_;
		int current_number_;
		std::set<std::string> existed_paths_;
		std::set<std::string> created_paths_;
	};

	class tsubtask_remove: public tsubtask
	{
	public:
		explicit tsubtask_remove(ttask& task, const std::string& id, const config& cfg, const std::map<std::string, std::string>& base_paths);

		bool handle(display& disp) override;
		bool fail_rollback(display& disp) const override { return true; }

	private:
		bool do_remove(display& disp, const std::string& dst_alias) const;

	private:
		struct tres {
			tres(res_type type, const std::string& name)
				: type(type)
				, name(name)
			{}

			res_type type;
			std::string name;
		};
		std::string obj_alias_;
		std::vector<tres> remove_res_;
	};

	class tsubtask_rename: public tsubtask
	{
	public:
		explicit tsubtask_rename(ttask& task, const std::string& id, const config& cfg, const std::map<std::string, std::string>& base_paths);

		bool handle(display& disp) override;
		bool fail_rollback(display& disp) const override { return true; }

	private:
		bool do_rename(display& disp, const std::string& dst_alias) const;

	private:
		struct tres {
			tres(res_type type, const std::string& name, const std::string& new_name)
				: type(type)
				, name(name)
				, new_name(new_name)
			{}

			res_type type;
			std::string name;
			std::string new_name;
		};
		std::string base_path_alias_;
		std::vector<tres> replace_res_;
	};

	ttask(const config&);

	void parse_cfg(const config& cfg, const std::string& key);
	bool handle(display& disp);
	const std::string& alias_2_path(const std::string& alias) const;

	const std::string& name() const { return name_; }
	virtual bool valid() const;
	bool make_path(display& disp, const std::string& tag, bool del) const;

protected:
	void complete_paths(const config& cfg);

private:
	std::pair<std::string, std::string> get_replace(const tsubtask& subtask)
	{
		return app_get_replace(subtask);
	}

	virtual void app_complete_paths() {}

	// @last indicate it is last subtask.
	// if return false, will not execute this subtask. but don't fail. 
	virtual bool app_can_execute(const tsubtask& subtask, const bool last) { return true; }
	// @last indicate it is last subtask.
	// if return false, think this task fail, will execute fail rollback. 
	virtual bool app_post_handle(display& disp, const tsubtask& subtask, const bool last) { return true; }

	virtual std::pair<std::string, std::string> app_get_replace(const tsubtask& subtask) { return std::make_pair(null_str, null_str); }

protected:
	std::vector<std::unique_ptr<tsubtask> > subtasks_;

	std::string name_;
	std::map<std::string, std::string> paths_;
};

const std::string& alias_2_path(const std::map<std::string, std::string>& paths, const std::string& alias);

#endif