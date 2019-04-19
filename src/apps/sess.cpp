#include "apps/sess.h"

void add_torrent(libtorrent::session& ses
	, handles_t& files
	, std::set<libtorrent::torrent_handle>& non_files
	, std::string const& torrent
	, int allocation_mode
	, std::string const& save_path
	, bool monitored_dir
	, int torrent_upload_limit
	, int torrent_download_limit)
{
	using namespace libtorrent;

	boost::intrusive_ptr<torrent_info> t;
	libtorrent::error_code ec;
	t = new torrent_info(torrent.c_str(), ec);
	if (ec)
	{
		fprintf(stderr, "%s: %s\n", torrent.c_str(), ec.message().c_str());
		return;
	}
	using namespace std;

	string str = tools::format::Utf8ToAscii(t->name());

	printf("%s\n", str.c_str());
	add_torrent_params p;

	int seed_mode = false;
	int share_mode = false;
	int disable_storage = false;
	if (seed_mode) p.flags |= add_torrent_params::flag_seed_mode;
	if (disable_storage) p.storage = disabled_storage_constructor;
	if (share_mode) p.flags |= add_torrent_params::flag_share_mode;
	lazy_entry resume_data;

	std::string filename = combine_path(save_path, combine_path(".resume", to_hex(t->info_hash().to_string()) + ".resume"));

	std::vector<char> buf;
	if (load_file(filename.c_str(), buf, ec) == 0)
		p.resume_data = &buf;

	p.ti = t;
	p.save_path = save_path;
	p.storage_mode = (storage_mode_t)allocation_mode;
	p.flags |= add_torrent_params::flag_paused;
	p.flags &= ~add_torrent_params::flag_duplicate_is_error;
	p.flags |= add_torrent_params::flag_auto_managed;
	p.userdata = (void*)strdup(torrent.c_str());
	ses.async_add_torrent(p);

}

std::string& to_string(float v, int width, int precision = 3)
{
	// this is a silly optimization
	// to avoid copying of strings
	enum { num_strings = 20 };
	static std::string buf[num_strings];
	static int round_robin = 0;
	std::string& ret = buf[round_robin];
	++round_robin;
	if (round_robin >= num_strings) round_robin = 0;
	ret.resize(20);
	int size = snprintf(&ret[0], 20, "%*.*f", width, precision, v);
	ret.resize((std::min)(size, width));
	return ret;
}

std::string add_suffix(float val, char const* suffix = 0)
{
	std::string ret;
	if (val == 0)
	{
		ret.resize(4 + 2, ' ');
		if (suffix) ret.resize(4 + 2 + strlen(suffix), ' ');
		return ret;
	}

	const char* prefix[] = { "kB", "MB", "GB", "TB" };
	const int num_prefix = sizeof(prefix) / sizeof(const char*);
	for (int i = 0; i < num_prefix; ++i)
	{
		val /= 1000.f;
		if (std::fabs(val) < 1000.f)
		{
			ret = to_string(val, 4);
			ret += prefix[i];
			if (suffix) ret += suffix;
			return ret;
		}
	}
	ret = to_string(val, 4);
	ret += "PB";
	if (suffix) ret += suffix;
	return ret;
}

bool compare_torrent(torrent_status const* lhs, torrent_status const* rhs)
{
	if (lhs->queue_position != -1 && rhs->queue_position != -1)
	{
		// both are downloading, sort by queue pos
		return lhs->queue_position < rhs->queue_position;
	}
	else if (lhs->queue_position == -1 && rhs->queue_position == -1)
	{
		// both are seeding, sort by seed-rank
		if (lhs->seed_rank != rhs->seed_rank)
			return lhs->seed_rank > rhs->seed_rank;

		return lhs->info_hash < rhs->info_hash;
	}

	return (lhs->queue_position == -1) < (rhs->queue_position == -1);
}

bool show_torrent(libtorrent::torrent_status const& st, int torrent_filter, int* counters)
{
	//add all torrent status
	++counters[torrents_all];

	if (!st.paused
		&& st.state != torrent_status::seeding
		&& st.state != torrent_status::finished)
	{
		++counters[torrents_downloading];
	}

	if (!st.paused) ++counters[torrents_not_paused];

	if (!st.paused
		&& (st.state == torrent_status::seeding
		|| st.state == torrent_status::finished))
	{
		++counters[torrents_seeding];
	}

	if (st.paused && st.auto_managed)
	{
		++counters[torrents_queued];
	}

	if (st.paused && !st.auto_managed)
	{
		++counters[torrents_stopped];
	}

	if (st.state == torrent_status::checking_files
		|| st.state == torrent_status::queued_for_checking)
	{
		++counters[torrents_checking];
	}

	switch (torrent_filter)
	{
	case torrents_all: return true;
	case torrents_downloading:
		return !st.paused
			&& st.state != torrent_status::seeding
			&& st.state != torrent_status::finished;
	case torrents_not_paused: return !st.paused;
	case torrents_seeding:
		return !st.paused
			&& (st.state == torrent_status::seeding
			|| st.state == torrent_status::finished);
	case torrents_queued: return st.paused && st.auto_managed;
	case torrents_stopped: return st.paused && !st.auto_managed;
	case torrents_checking: return st.state == torrent_status::checking_files
		|| st.state == torrent_status::queued_for_checking;
	case torrents_feeds: return false;
	}
	return true;
}

int save_file(std::string const& filename, std::vector<char>& v)
{
	using namespace libtorrent;

	file f;
	libtorrent::error_code ec;
	if (!f.open(filename, file::write_only, ec)) return -1;
	if (ec) return -1;
	file::iovec_t b = { &v[0], v.size() };
	size_type written = f.writev(0, &b, 1, ec);
	if (written != int(v.size())) return -3;
	if (ec) return -3;
	return 0;
}

void Sess::update_filtered_torrents(boost::unordered_set<torrent_status>& all_handles
	, std::vector<torrent_status const*>& filtered_handles, int* counters)
{
	filtered_handles.clear();
	memset(counters, 0, sizeof(int) * torrents_max);
	for (boost::unordered_set<torrent_status>::iterator i = all_handles.begin()
		, end(all_handles.end()); i != end; ++i)
	{
		if (!show_torrent(*i, torrent_filter, counters)) continue;
		filtered_handles.push_back(&*i);
	}
	if (active_torrent >= int(filtered_handles.size())) active_torrent = filtered_handles.size() - 1;
	else if (active_torrent == -1 && !filtered_handles.empty()) active_torrent = 0;
	std::sort(filtered_handles.begin(), filtered_handles.end(), &compare_torrent);
}

// returns true if the alert was handled (and should not be printed to the log)
// returns false if the alert was not handled
bool Sess::handle_alert(libtorrent::session& ses, libtorrent::alert* a
	, handles_t& files, std::set<libtorrent::torrent_handle>& non_files
	, int* counters, boost::unordered_set<torrent_status>& all_handles
	, std::vector<torrent_status const*>& filtered_handles
	, bool& need_resort)
{
	using namespace libtorrent;

#ifdef TORRENT_USE_OPENSSL
	if (torrent_need_cert_alert* p = alert_cast<torrent_need_cert_alert>(a))
	{
		torrent_handle h = p->handle;
		file_status st;
		std::string cert = combine_path("certificates", to_hex(h.info_hash().to_string())) + ".pem";
		std::string priv = combine_path("certificates", to_hex(h.info_hash().to_string())) + "_key.pem";
		stat_file(cert, &st, ec);
		if (ec)
		{
			char msg[256];
			snprintf(msg, sizeof(msg), "ERROR. could not load certificate %s: %s\n", cert.c_str(), ec.message().c_str());
			if (g_log_file) fprintf(g_log_file, "[%s] %s\n", time_now_string(), msg);
			return true;
		}
		stat_file(priv, &st, ec);
		if (ec)
		{
			char msg[256];
			snprintf(msg, sizeof(msg), "ERROR. could not load private key %s: %s\n", priv.c_str(), ec.message().c_str());
			if (g_log_file) fprintf(g_log_file, "[%s] %s\n", time_now_string(), msg);
			return true;
		}

		char msg[256];
		snprintf(msg, sizeof(msg), "loaded certificate %s and key %s\n", cert.c_str(), priv.c_str());
		if (g_log_file) fprintf(g_log_file, "[%s] %s\n", time_now_string(), msg);

		h.set_ssl_certificate(cert, priv, "certificates/dhparams.pem", "1234");
		h.resume();
	}
#endif

	boost::intrusive_ptr<torrent_info> ti;

	if (metadata_received_alert* p = alert_cast<metadata_received_alert>(a))
	{
		// if we have a monitor dir, save the .torrent file we just received in it
		// also, add it to the files map, and remove it from the non_files list
		// to keep the scan dir logic in sync so it's not removed, or added twice
		torrent_handle h = p->handle;
		if (h.is_valid()) {
			if (!ti) ti = h.torrent_file();
			create_torrent ct(*ti);
			entry te = ct.generate();
			std::vector<char> buffer;
			bencode(std::back_inserter(buffer), te);
			std::string filename = ti->name() + "." + to_hex(ti->info_hash().to_string()) + ".torrent";
			filename = combine_path(monitor_dir, filename);
			save_file(filename, buffer);

			files.insert(std::pair<std::string, libtorrent::torrent_handle>(filename, h));
			non_files.erase(h);
		}
	}
	else if (add_torrent_alert* p = alert_cast<add_torrent_alert>(a))
	{
		std::string filename;
		if (p->params.userdata)
		{
			filename = (char*)p->params.userdata;
			free(p->params.userdata);
		}

		if (p->error)
		{
			fprintf(stderr, "failed to add torrent: %s %s\n", filename.c_str(), p->error.message().c_str());
		}
		else
		{
			torrent_handle h = p->handle;

			if (!filename.empty())
				files.insert(std::pair<const std::string, torrent_handle>(filename, h));
			else
				non_files.insert(h);

			h.set_max_connections(max_connections_per_torrent);
			h.set_max_uploads(-1);
			h.set_upload_limit(torrent_upload_limit);
			h.set_download_limit(torrent_download_limit);
#ifndef TORRENT_NO_DEPRECATE
			h.use_interface(outgoing_interface.c_str());
#endif
#ifndef TORRENT_DISABLE_RESOLVE_COUNTRIES
			h.resolve_countries(true);
#endif

			// if we have a peer specified, connect to it
			if (!peer.empty())
			{
				char* port = (char*)strrchr((char*)peer.c_str(), ':');
				if (port > 0)
				{
					*port++ = 0;
					char const* ip = peer.c_str();
					int peer_port = atoi(port);
					if (peer_port > 0)
						h.connect_peer(tcp::endpoint(address::from_string(ip, ec), peer_port));
				}
			}

			boost::unordered_set<torrent_status>::iterator j
				= all_handles.insert(h.status()).first;
			if (show_torrent(*j, torrent_filter, counters))
			{
				filtered_handles.push_back(&*j);
				need_resort = true;
			}
		}
		update_filtered_torrents(all_handles, filtered_handles, counters);
	}
	else if (torrent_finished_alert* p = alert_cast<torrent_finished_alert>(a))
	{
		p->handle.set_max_connections(max_connections_per_torrent / 2);

		// write resume data for the finished torrent
		// the alert handler for save_resume_data_alert
		// will save it to disk
		torrent_handle h = p->handle;
		h.save_resume_data();
		++num_outstanding_resume_data;
	}
	else if (save_resume_data_alert* p = alert_cast<save_resume_data_alert>(a))
	{
		--num_outstanding_resume_data;
		torrent_handle h = p->handle;
		TORRENT_ASSERT(p->resume_data);
		if (p->resume_data)
		{
			std::vector<char> out;
			bencode(std::back_inserter(out), *p->resume_data);
			torrent_status st = h.status(torrent_handle::query_save_path);
			save_file(combine_path(st.save_path, combine_path(".resume", to_hex(st.info_hash.to_string()) + ".resume")), out);
			if (h.is_valid()
				&& non_files.find(h) == non_files.end()
				&& std::find_if(files.begin(), files.end()
				, boost::bind(&handles_t::value_type::second, _1) == h) == files.end())
				ses.remove_torrent(h);
		}
	}
	else if (save_resume_data_failed_alert* p = alert_cast<save_resume_data_failed_alert>(a))
	{
		--num_outstanding_resume_data;
		torrent_handle h = p->handle;
		if (h.is_valid()
			&& non_files.find(h) == non_files.end()
			&& std::find_if(files.begin(), files.end()
			, boost::bind(&handles_t::value_type::second, _1) == h) == files.end())
			ses.remove_torrent(h);
	}
	else if (torrent_paused_alert* p = alert_cast<torrent_paused_alert>(a))
	{
		// write resume data for the finished torrent
		// the alert handler for save_resume_data_alert
		// will save it to disk
		torrent_handle h = p->handle;
		h.save_resume_data();
		++num_outstanding_resume_data;
	}
	else if (state_update_alert* p = alert_cast<state_update_alert>(a))
	{
		bool need_filter_update = false;
		for (std::vector<torrent_status>::iterator i = p->status.begin();
			i != p->status.end(); ++i)
		{
			boost::unordered_set<torrent_status>::iterator j = all_handles.find(*i);
			// don't add new entries here, that's done in the handler
			// for add_torrent_alert
			if (j == all_handles.end()) continue;
			if (j->state != i->state
				|| j->paused != i->paused
				|| j->auto_managed != i->auto_managed)
				need_filter_update = true;
			((torrent_status&)*j) = *i;
		}
		if (need_filter_update)
			update_filtered_torrents(all_handles, filtered_handles, counters);

		return true;
	}
	return false;
}

void Sess::init()
{
	using namespace libtorrent;
	print_trackers = false;
	print_peers = false;
	print_log = false;
	print_downloads = true;
	print_piece_bar = false;
	print_file_progress = false;
	show_pad_files = true;
	show_dht_status = true;
	sequential_download = false;
	print_utp_stats = true;

	print_ip = true;
	print_as = true;
	print_timers = true;
	print_block = true;
	print_peer_rate = true;
	print_fails = true;
	print_send_bufs = true;

	num_outstanding_resume_data = 0;
	torrent_filter = torrents_all;

	g_log_file = 0;

	active_torrent = 0;

	listen_port = 6881;
	allocation_mode = libtorrent::storage_mode_sparse;
	save_path = std::string(".");
	torrent_upload_limit = 0;
	torrent_download_limit = 0;
	monitor_dir = "";
	bind_to_interface = "";
	outgoing_interface = "";
	poll_interval = 5;
	max_connections_per_torrent = 50;
	seed_mode = false;

	share_mode = false;
	disable_storage = false;
	start_dht = true;
	start_upnp = true;
	start_lsd = true;
	min_mod = false;
	high_mod = true;

	ses = new session(fingerprint("LT", LIBTORRENT_VERSION_MAJOR, LIBTORRENT_VERSION_MINOR, 0, 0)
		, session::add_default_plugins
		, alert::all_categories
		& ~(alert::dht_notification
		+ alert::progress_notification
		+ alert::debug_notification
		+ alert::stats_notification));

	std::vector<char> in;
	if (load_file(".ses_state", in, ec) == 0)
	{
		lazy_entry e;
		if (lazy_bdecode(&in[0], &in[0] + in.size(), e, ec) == 0)
			ses->load_state(e);
	}

#ifndef TORRENT_DISABLE_GEO_IP
	ses->load_asnum_db("GeoIPASNum.dat");
	ses->load_country_db("GeoIP.dat");
#endif

	create_directory(combine_path(save_path, ".resume"), ec);	//保存路径

	if (start_lsd)
		ses->start_lsd();			//启动与停止本地发现服务

	if (start_upnp)
	{
		ses->start_upnp();			//来启动与停止 UpnP 服务
		ses->start_natpmp();			//来打开或者关闭 NAT-PMP 服务
	}

	if (min_mod)
		settings = min_memory_usage();
	if (high_mod)
		settings = high_performance_seed();

	ses->set_proxy(ps);				//设置代理服务器

	ses->listen_on(std::make_pair(6881, 6881)		//监听端口的范围
		, ec, bind_to_interface.c_str());

#ifndef TORRENT_DISABLE_DHT
	if (start_dht)
	{
		settings.use_dht_as_fallback = false;

		ses->add_dht_router(std::make_pair(
			std::string("router.bittorrent.com"), 6881));
		ses->add_dht_router(std::make_pair(
			std::string("router.utorrent.com"), 6881));
		ses->add_dht_router(std::make_pair(
			std::string("router.bitcomet.com"), 6881));

		ses->start_dht();
	}
#endif
	

	settings.user_agent = "sBittorrent/" LIBTORRENT_VERSION;
	settings.choking_algorithm = session_settings::auto_expand_choker;
	settings.disk_cache_algorithm = session_settings::avoid_readback;

	settings.volatile_read_cache = false;
	ses->set_settings(settings);				//应用配置

}

AllTorrent& Sess::getItem()
{
	using namespace libtorrent;
	// loop through the alert queue to see if anything has happened.
	ses->post_torrent_updates();

	// ask for distributed copies for the selected torrent. Since this
	// is a somewhat expensive operation, don't do it by default for
	// all torrents
	/*torrent_status const& h = *filtered_handles[active_torrent];
	h.handle.status(
		torrent_handle::query_distributed_copies
		| torrent_handle::query_pieces
		| torrent_handle::query_verified_pieces);*/
	

	std::deque<alert*> alerts;
	memset(counters, 0, sizeof(counters));

	std::vector<feed_handle> feeds;
	ses->get_feeds(feeds);
	counters[torrents_feeds] = feeds.size();

	std::sort(filtered_handles.begin(), filtered_handles.end(), &compare_torrent);

	ses->pop_alerts(&alerts);
	
	for (std::deque<alert*>::iterator i = alerts.begin()
		, end(alerts.end()); i != end; ++i)
	{
		bool need_resort = false;
		TORRENT_TRY
		{
			if (!handle_alert(*ses, *i, files, non_files, counters
			, all_handles, filtered_handles, need_resort))
			{
				//printf("Didn't handle the alert\n");
			}
		} TORRENT_CATCH(std::exception& e) {}

		if (need_resort)
		{
			std::sort(filtered_handles.begin(), filtered_handles.end()
				, &compare_torrent);
		}

		delete *i;
	}
	alerts.clear();

	/*update_filtered_torrents(all_handles, filtered_handles, counters);
	std::sort(filtered_handles.begin(), filtered_handles.end(), &compare_torrent);*/

	libtorrent::session_status sess_stat = ses->status();
	char const* state_str[] =
	{ "checking (q)", "checking", "dl metadata"
	, "downloading", "finished", "seeding", "allocating", "checking (r)" };

	items.item.clear();
	items.size = 0;
	for (std::vector<torrent_status const*>::iterator i = filtered_handles.begin();
		i != filtered_handles.end(); ++i)
	{
		torrent_status const& s = **i;
		Attribute attr;
		attr.queue_pos = s.queue_position;
		if (s.name != "")
			attr.name = tools::format::Utf8ToAscii(s.name);
		attr.status = state_str[s.state];
		attr.download_rate = add_suffix(s.download_rate, "/s");
		attr.download = add_suffix(s.total_done);
		attr.upload_rate = add_suffix(s.upload_rate, "/s");
		attr.upload = add_suffix(s.total_upload);
		attr.time_download = add_suffix(s.all_time_download);
		attr.per = (float)s.total_done/(float)s.total_wanted*100;
		attr.peers = s.num_peers;
		attr.seeds = s.num_seeds;

		attr.size = add_suffix(s.total_wanted);
		/*auto pfile = s.handle.torrent_file();
		if (pfile)
			attr.size = add_suffix(pfile->total_size());*/

		items.item.push_back(attr);
		++items.size;

		/*printf("%f\n",attr.per);
		puts(attr.time_download.c_str());
		puts(attr.size.c_str());*/

	}
	items.dht_Node = sess_stat.dht_nodes;
	items.total_download = add_suffix(sess_stat.total_download);
	items.total_download_rate = add_suffix(sess_stat.download_rate, "/s");
	items.total_upload_rate = add_suffix(sess_stat.upload_rate, "/s");
	items.total_upload = add_suffix(sess_stat.total_upload);

	/*puts(items.total_download_rate.c_str());
	puts(items.total_download.c_str());*/
	return items;
}

void Sess::getStatu()
{
}

Sess::Sess()
{
	init();
	//boost::thread th(&Sess::getStatu, this);
	//th.detach();
}

void Sess::addTorrent(std::string str)
{
	// if it's a torrent file, open it as usual
	add_torrent(*ses, files, non_files, str.c_str()
		, allocation_mode, save_path, false
		, torrent_upload_limit, torrent_download_limit);
	
	this->has_task = true;
}

void Sess::addMagnet(std::string str)
{
	using namespace libtorrent;

	if (std::strstr(str.c_str(), "http://") == str.c_str()
		|| std::strstr(str.c_str(), "https://") == str.c_str()
		|| std::strstr(str.c_str(), "magnet:") == str.c_str())
	{
		add_torrent_params p;
		if (seed_mode) p.flags |= add_torrent_params::flag_seed_mode;
		if (disable_storage) p.storage = disabled_storage_constructor;
		if (share_mode) p.flags |= add_torrent_params::flag_share_mode;
		p.save_path = save_path;
		p.storage_mode = (storage_mode_t)allocation_mode;
		p.url = str;

		std::vector<char> buf;
		if (std::strstr(str.c_str(), "magnet:") == str.c_str())
		{
			add_torrent_params tmp;
			ec.clear();
			parse_magnet_uri(str, tmp, ec);

			if (ec) printf("error magnet");

			std::string filename = combine_path(save_path, combine_path(".resume"
				, to_hex(tmp.info_hash.to_string()) + ".resume"));

			if (load_file(filename.c_str(), buf, ec) == 0)
				p.resume_data = &buf;
		}

		printf("adding URL: %s\n", str.c_str());
		ses->async_add_torrent(p);
		this->has_task = true;
	}
}

Sess::~Sess()
{
	delete ses;
	printf("调用析构函数");
}

Sess* Sess::getInstance()
{
	static Sess Ses;
	return &Ses;
}
//**********************************************************************
void Sess::continueDownload(std::vector<int> rows)
{
	for each (auto h in rows)
	{
		torrent_status const& st = *filtered_handles[h];
		st.handle.auto_managed(true);
	}
}

void Sess::stopDownload(std::vector<int> rows)
{
	for each (auto h in rows)
	{
		torrent_status const& st = *filtered_handles[h];
		st.handle.auto_managed(false);
		st.handle.pause(torrent_handle::graceful_pause);
	}
}

void Sess::forceStart(std::vector<int> rows)
{
	for each (auto h in rows)
	{
		torrent_status const& st = *filtered_handles[h];
		st.handle.auto_managed(!st.auto_managed);
		if (st.auto_managed && st.paused)
			st.handle.resume();
	}
}

void Sess::restart(std::vector<int> rows)
{
	for each (auto h in rows)
	{
		torrent_status const& st = *filtered_handles[h];

	}
}

void Sess::stopAll()
{
	ses->pause();
}

void Sess::continueAll()
{
	ses->resume();
}

void Sess::rename(std::vector<int> rows, std::string const& new_name)
{
	for each (auto h in rows)
	{
		torrent_status const& st = *filtered_handles[h];
		st.handle.rename_file(0, new_name + std::to_string(h));
	}
}

//if options=1 delete document
void Sess::deleteTask(std::vector<int> rows, bool delFile/*=false*/)
{
	using namespace libtorrent;
	for each (auto h in rows)
	{
		torrent_status const& st = *filtered_handles[h];
		if (st.handle.is_valid())
		{
			if (delFile)
			{
				handles_t::iterator i = std::find_if(files.begin(), files.end()
					, boost::bind(&handles_t::value_type::second, _1) == st.handle);
				if (i != files.end())
				{
					std::string path;
					if (is_complete(i->first)) path = i->first;
					else path = combine_path(monitor_dir, i->first);
					remove(path, ec);
					if (ec) printf("failed to delete .torrent file: %s\n", ec.message().c_str());
					files.erase(i);
				}
				if (st.handle.is_valid())
				{
					ses->remove_torrent(st.handle, libtorrent::session::delete_files);
					all_handles.erase(st);
				}
					
			}
			else
			{
				if (st.handle.is_valid())
				{
					ses->remove_torrent(st.handle);
					all_handles.erase(st);
				}
					
			}

		}
	}
	update_filtered_torrents(all_handles, filtered_handles, counters);
}

//if options=1 delete document
void Sess::deleteAll(bool delFile/* = false*/)
{
	using namespace libtorrent;
	for each (auto h in filtered_handles)
	{
		auto st = *h;
		if (st.handle.is_valid())
		{
			if (delFile)
			{
				handles_t::iterator i = std::find_if(files.begin(), files.end()
					, boost::bind(&handles_t::value_type::second, _1) == st.handle);
				if (i != files.end())
				{
					std::string path;
					if (is_complete(i->first)) path = i->first;
					else path = combine_path(monitor_dir, i->first);
					remove(path, ec);
					if (ec) printf("failed to delete .torrent file: %s\n", ec.message().c_str());
					files.erase(i);
				}
				if (st.handle.is_valid())
				{
					ses->remove_torrent(st.handle, libtorrent::session::delete_files);
					all_handles.erase(st);
				}
			}
			else
			{
				if (st.handle.is_valid())
				{
					ses->remove_torrent(st.handle);
					all_handles.erase(st);
				}
			}
		}
	}
	update_filtered_torrents(all_handles, filtered_handles, counters);
}

void Sess::setDownloadRate(int bytes_per_second)
{
	ses->set_download_rate_limit(bytes_per_second);
	ses->set_upload_rate_limit(bytes_per_second/8);
}

void Sess::setUploadRate(int bytes_per_second)
{
	ses->set_upload_rate_limit(bytes_per_second);
	ses->set_download_rate_limit(bytes_per_second*8);
}

void Sess::saveResume()
{
	using namespace libtorrent;
	for each (auto h in filtered_handles)
	{
		if (h->need_save_resume)
		{
			h->handle.save_resume_data();
			++num_outstanding_resume_data;
		}
	}
}