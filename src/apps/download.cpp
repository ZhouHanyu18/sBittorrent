#include "apps/download.h"

enum {
	torrents_all,
	torrents_downloading,
	torrents_not_paused,
	torrents_seeding,
	torrents_queued,
	torrents_stopped,
	torrents_checking,
	torrents_feeds,

	torrents_max
};

char const* Download::state_str[] =
{ "checking (q)", "checking", "dl metadata"
, "downloading", "finished", "seeding", "allocating", "checking (r)" };

Download::Download()
{
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
	torrent_filter = torrents_not_paused;

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
}

Download::~Download()
{

}


bool Download::yes(libtorrent::torrent_status const&)
{
	return true;
}



bool Download::compare_torrent(torrent_status const* lhs, torrent_status const* rhs)
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


std::string Download::to_string(int v, int width)
{
	char buf[100];
	snprintf(buf, sizeof(buf), "%*d", width, v);
	return buf;
}

std::string& Download::to_string(float v, int width, int precision = 3)
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

std::string Download::add_suffix(float val, char const* suffix = 0)
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



// monitored_dir is true if this torrent is added because
// it was found in the directory that is monitored. If it
// is, it should be remembered so that it can be removed
// if it's no longer in that directory.
void Download::add_torrent(libtorrent::session& ses
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
	error_code ec;
	t = new torrent_info(torrent.c_str(), ec);
	if (ec)
	{
		fprintf(stderr, "%s: %s\n", torrent.c_str(), ec.message().c_str());
		return;
	}

	printf("%s\n", t->name().c_str());

	add_torrent_params p;
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

void Download::scan_dir(std::string const& dir_path
	, libtorrent::session& ses
	, handles_t& files
	, std::set<libtorrent::torrent_handle>& non_files
	, int allocation_mode
	, std::string const& save_path
	, int torrent_upload_limit
	, int torrent_download_limit)
{
	std::set<std::string> valid;

	using namespace libtorrent;

	error_code ec;
	for (directory i(dir_path, ec); !i.done(); i.next(ec))
	{
		std::string file = combine_path(dir_path, i.file());
		if (extension(file) != ".torrent") continue;

		handles_t::iterator k = files.find(file);
		if (k != files.end())
		{
			valid.insert(file);
			continue;
		}

		// the file has been added to the dir, start
		// downloading it.
		add_torrent(ses, files, non_files, file, allocation_mode
			, save_path, true, torrent_upload_limit, torrent_download_limit);
		valid.insert(file);
	}

	// remove the torrents that are no longer in the directory

	for (handles_t::iterator i = files.begin(); !files.empty() && i != files.end();)
	{
		if (i->first.empty() || valid.find(i->first) != valid.end())
		{
			++i;
			continue;
		}

		torrent_handle& h = i->second;
		if (!h.is_valid())
		{
			files.erase(i++);
			continue;
		}

		h.auto_managed(false);
		h.pause();
		// the alert handler for save_resume_data_alert
		// will save it to disk
		if (h.need_save_resume_data())
		{
			h.save_resume_data();
			++num_outstanding_resume_data;
		}

		files.erase(i++);
	}
}



int Download::save_file(std::string const& filename, std::vector<char>& v)
{
	using namespace libtorrent;

	file f;
	error_code ec;
	if (!f.open(filename, file::write_only, ec)) return -1;
	if (ec) return -1;
	file::iovec_t b = { &v[0], v.size() };
	size_type written = f.writev(0, &b, 1, ec);
	if (written != int(v.size())) return -3;
	if (ec) return -3;
	return 0;
}


int Download::main(int argc, char argv[][100])
{
	using namespace libtorrent;
	session_settings settings;

	proxy_settings ps;

	int refresh_delay = 1000;
	bool start_dht = true;
	bool start_upnp = true;
	bool start_lsd = true;
	int loop_limit = 0;

	std::deque<std::string> events;

	ptime next_dir_scan = time_now();

	// the string is the filename of the .torrent file, but only if
	// it was added through the directory monitor. It is used to
	// be able to remove torrents that were added via the directory
	// monitor when they're not in the directory anymore.
	boost::unordered_set<torrent_status> all_handles;
	std::vector<torrent_status const*> filtered_handles;

	handles_t files;
	// torrents that were not added via the monitor dir
	std::set<torrent_handle> non_files;

	int counters[torrents_max];
	memset(counters, 0, sizeof(counters));

	session ses(fingerprint("LT", LIBTORRENT_VERSION_MAJOR, LIBTORRENT_VERSION_MINOR, 0, 0)
		, session::add_default_plugins
		, alert::all_categories
		& ~(alert::dht_notification
		+ alert::progress_notification
		+ alert::debug_notification
		+ alert::stats_notification));

	std::vector<char> in;
	error_code ec;
	if (load_file(".ses_state", in, ec) == 0)
	{
		lazy_entry e;
		if (lazy_bdecode(&in[0], &in[0] + in.size(), e, ec) == 0)
			ses.load_state(e);
	}

#ifndef TORRENT_DISABLE_GEO_IP
	ses.load_asnum_db("GeoIPASNum.dat");
	ses.load_country_db("GeoIP.dat");
#endif

	// load the torrents given on the commandline

	std::vector<add_torrent_params> magnet_links;
	std::vector<std::string> torrents;

	//change by zhy
	{
		int i = 1;
		// match it against the <hash>@<tracker> format
		if (strlen(argv[i]) > 45
			&& is_hex(argv[i], 40)
			&& (strncmp(argv[i] + 40, "@http://", 8) == 0
			|| strncmp(argv[i] + 40, "@udp://", 7) == 0))
		{
			sha1_hash info_hash;
			from_hex(argv[i], 40, (char*)&info_hash[0]);

			add_torrent_params p;
			if (seed_mode) p.flags |= add_torrent_params::flag_seed_mode;
			if (disable_storage) p.storage = disabled_storage_constructor;
			if (share_mode) p.flags |= add_torrent_params::flag_share_mode;
			p.trackers.push_back(argv[i] + 41);
			p.info_hash = info_hash;
			p.save_path = save_path;
			p.storage_mode = (storage_mode_t)allocation_mode;
			p.flags |= add_torrent_params::flag_paused;
			p.flags &= ~add_torrent_params::flag_duplicate_is_error;
			p.flags |= add_torrent_params::flag_auto_managed;
			magnet_links.push_back(p);
		}

		torrents.push_back(argv[i]);
	}



	// create directory for resume files
	create_directory(combine_path(save_path, ".resume"), ec);
	if (ec)
		fprintf(stderr, "failed to create resume file directory: %s\n", ec.message().c_str());

	if (start_lsd)
		ses.start_lsd();			//启动与停止本地发现服务

	if (start_upnp)
	{
		ses.start_upnp();			//来启动与停止 UpnP 服务
		ses.start_natpmp();			//来打开或者关闭 NAT-PMP 服务
	}

	ses.set_proxy(ps);				//设置代理服务器

	ses.listen_on(std::make_pair(listen_port, listen_port)		//监听端口的范围
		, ec, bind_to_interface.c_str());
	if (ec)
	{
		fprintf(stderr, "failed to listen on %s on ports %d-%d: %s\n"
			, bind_to_interface.c_str(), listen_port, listen_port + 1, ec.message().c_str());
	}

#ifndef TORRENT_DISABLE_DHT
	if (start_dht)
	{
		settings.use_dht_as_fallback = false;

		ses.add_dht_router(std::make_pair(
			std::string("router.bittorrent.com"), 6881));
		ses.add_dht_router(std::make_pair(
			std::string("router.utorrent.com"), 6881));
		ses.add_dht_router(std::make_pair(
			std::string("router.bitcomet.com"), 6881));

		ses.start_dht();
	}
#endif

	settings.user_agent = "sBittorrent/" LIBTORRENT_VERSION;
	settings.choking_algorithm = session_settings::auto_expand_choker;
	settings.disk_cache_algorithm = session_settings::avoid_readback;
	settings.volatile_read_cache = false;

	ses.set_settings(settings);				//应用配置

	for (std::vector<add_torrent_params>::iterator i = magnet_links.begin()
		, end(magnet_links.end()); i != end; ++i)
	{
		ses.async_add_torrent(*i);
	}

	for (std::vector<std::string>::iterator i = torrents.begin()
		, end(torrents.end()); i != end; ++i)
	{
		if (std::strstr(i->c_str(), "http://") == i->c_str()
			|| std::strstr(i->c_str(), "https://") == i->c_str()
			|| std::strstr(i->c_str(), "magnet:") == i->c_str())
		{
			add_torrent_params p;
			if (seed_mode) p.flags |= add_torrent_params::flag_seed_mode;
			if (disable_storage) p.storage = disabled_storage_constructor;
			if (share_mode) p.flags |= add_torrent_params::flag_share_mode;
			p.save_path = save_path;
			p.storage_mode = (storage_mode_t)allocation_mode;
			p.url = *i;

			std::vector<char> buf;
			if (std::strstr(i->c_str(), "magnet:") == i->c_str())
			{
				add_torrent_params tmp;
				ec.clear();
				parse_magnet_uri(*i, tmp, ec);

				if (ec) continue;

				std::string filename = combine_path(save_path, combine_path(".resume"
					, to_hex(tmp.info_hash.to_string()) + ".resume"));

				if (load_file(filename.c_str(), buf, ec) == 0)
					p.resume_data = &buf;
			}

			printf("adding URL: %s\n", i->c_str());
			ses.async_add_torrent(p);
			continue;
		}

		// if it's a torrent file, open it as usual
		add_torrent(ses, files, non_files, i->c_str()
			, allocation_mode, save_path, false
			, torrent_upload_limit, torrent_download_limit);
	}

	// main loop
	std::vector<peer_info> peers;
	std::vector<partial_piece_info> queue;

	int tick = 0;

	while (loop_limit > 1 || loop_limit == 0)
	{
		++tick;
		ses.post_torrent_updates();
		if (active_torrent >= int(filtered_handles.size())) active_torrent = filtered_handles.size() - 1;
		if (active_torrent >= 0)
		{
			// ask for distributed copies for the selected torrent. Since this
			// is a somewhat expensive operation, don't do it by default for
			// all torrents
			torrent_status const& h = *filtered_handles[active_torrent];
			h.handle.status(
				torrent_handle::query_distributed_copies
				| torrent_handle::query_pieces
				| torrent_handle::query_verified_pieces);
		}

		std::vector<feed_handle> feeds;
		ses.get_feeds(feeds);

		counters[torrents_feeds] = feeds.size();

		std::sort(filtered_handles.begin(), filtered_handles.end(), &compare_torrent);

		if (loop_limit > 1) --loop_limit;
		int c = 0;

		int terminal_width = 80;
		int terminal_height = 50;
		int max_lines = terminal_height - 15;

		session_status sess_stat = ses.status();

		// in test mode, also quit when we loose the last peer
		if (loop_limit > 1 && sess_stat.num_peers == 0 && tick > 30) break;

		std::string out;
		out = "";


		cache_status cs = ses.get_cache_status();
		if (cs.blocks_read < 1) cs.blocks_read = 1;
		if (cs.blocks_written < 1) cs.blocks_written = 1;

		char str[500];
		snprintf(str, sizeof(str), "down: %s  (%s) up: %s  (%s) "
			, add_suffix(sess_stat.download_rate, "/s").c_str()
			, add_suffix(sess_stat.total_download).c_str()
			, add_suffix(sess_stat.upload_rate, "/s").c_str()
			, add_suffix(sess_stat.total_upload).c_str());
		out += str;

		out += "\n***********************************************************\n";

		puts(out.c_str());

		if (!monitor_dir.empty()
			&& next_dir_scan < time_now())
		{
			scan_dir(monitor_dir, ses, files, non_files
				, allocation_mode, save_path, torrent_upload_limit
				, torrent_download_limit);
			next_dir_scan = time_now() + seconds(poll_interval);
		}
	}

	// keep track of the number of resume data
	// alerts to wait for
	int num_paused = 0;
	int num_failed = 0;

	ses.pause();
	printf("saving resume data\n");
	std::vector<torrent_status> temp;
	ses.get_torrent_status(&temp, &yes, 0);
	for (std::vector<torrent_status>::iterator i = temp.begin();
		i != temp.end(); ++i)
	{
		torrent_status& st = *i;
		if (!st.handle.is_valid())
		{
			printf("  skipping, invalid handle\n");
			continue;
		}
		if (!st.has_metadata)
		{
			printf("  skipping %s, no metadata\n", st.name.c_str());
			continue;
		}
		if (!st.need_save_resume)
		{
			printf("  skipping %s, resume file up-to-date\n", st.name.c_str());
			continue;
		}

		// save_resume_data will generate an alert when it's done
		st.handle.save_resume_data();
		++num_outstanding_resume_data;
		printf("\r%d  ", num_outstanding_resume_data);
	}
	printf("\nwaiting for resume data [%d]\n", num_outstanding_resume_data);

	while (num_outstanding_resume_data > 0)
	{
		alert const* a = ses.wait_for_alert(seconds(10));
		if (a == 0) continue;

		std::deque<alert*> alerts;
		ses.pop_alerts(&alerts);
		std::string now = time_now_string();
		for (std::deque<alert*>::iterator i = alerts.begin()
			, end(alerts.end()); i != end; ++i)
		{
			// make sure to delete each alert
			std::auto_ptr<alert> a(*i);

			torrent_paused_alert const* tp = alert_cast<torrent_paused_alert>(*i);
			if (tp)
			{
				++num_paused;
				printf("\rleft: %d failed: %d pause: %d "
					, num_outstanding_resume_data, num_failed, num_paused);
				continue;
			}

			if (alert_cast<save_resume_data_failed_alert>(*i))
			{
				++num_failed;
				--num_outstanding_resume_data;
				printf("\rleft: %d failed: %d pause: %d "
					, num_outstanding_resume_data, num_failed, num_paused);
				continue;
			}

			save_resume_data_alert const* rd = alert_cast<save_resume_data_alert>(*i);
			if (!rd) continue;
			--num_outstanding_resume_data;
			printf("\rleft: %d failed: %d pause: %d "
				, num_outstanding_resume_data, num_failed, num_paused);

			if (!rd->resume_data) continue;

			torrent_handle h = rd->handle;
			torrent_status st = h.status(torrent_handle::query_save_path);
			std::vector<char> out;
			bencode(std::back_inserter(out), *rd->resume_data);
			save_file(combine_path(st.save_path, combine_path(".resume", to_hex(st.info_hash.to_string()) + ".resume")), out);
		}
	}

	if (g_log_file) fclose(g_log_file);
	printf("\nsaving session state\n");
	{
		entry session_state;
		ses.save_state(session_state);

		std::vector<char> out;
		bencode(std::back_inserter(out), session_state);
		save_file(".ses_state", out);
	}

	printf("closing session");

	return 0;
}

