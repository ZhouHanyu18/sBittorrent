#ifndef SESS_H
#define SESS_H
#include <iterator>
#include "libtorrent/config.hpp"

#ifdef _MSC_VER
#pragma warning(push, 1)
#endif

#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/unordered_set.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "libtorrent/extensions/metadata_transfer.hpp"
#include "libtorrent/extensions/ut_metadata.hpp"
#include "libtorrent/extensions/ut_pex.hpp"
#include "libtorrent/extensions/smart_ban.hpp"

#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/identify_client.hpp"
#include "libtorrent/alert_types.hpp"
#include "libtorrent/ip_filter.hpp"
#include "libtorrent/magnet_uri.hpp"
#include "libtorrent/bitfield.hpp"
#include "libtorrent/file.hpp"
#include "libtorrent/peer_info.hpp"
#include "libtorrent/socket_io.hpp" // print_address
#include "libtorrent/time.hpp"
#include "libtorrent/create_torrent.hpp"


#include "tools/format.h"
#include "apps/information.h"

using boost::bind;
using libtorrent::torrent_status;

typedef std::multimap<std::string, libtorrent::torrent_handle> handles_t;

class Sess
{
private:
	Sess();
	~Sess();
	void init();
	void getStatu();
	bool handle_alert(libtorrent::session& ses, libtorrent::alert* a
		, handles_t& files, std::set<libtorrent::torrent_handle>& non_files
		, int* counters, boost::unordered_set<torrent_status>& all_handles
		, std::vector<torrent_status const*>& filtered_handles
		, bool& need_resort);
	void update_filtered_torrents(boost::unordered_set<torrent_status>& all_handles
		, std::vector<torrent_status const*>& filtered_handles, int* counters);
private:
	bool print_trackers;
	bool print_peers;
	bool print_log;
	bool print_downloads;
	bool print_piece_bar;
	bool print_file_progress;
	bool show_pad_files;
	bool show_dht_status;
	bool sequential_download;
	bool print_utp_stats;

	bool print_ip;
	bool print_as;
	bool print_timers;
	bool print_block;
	bool print_peer_rate;
	bool print_fails;
	bool print_send_bufs;

	// the number of times we've asked to save resume data
	// without having received a response (successful or failure)
	int num_outstanding_resume_data;

	int torrent_filter;

	FILE* g_log_file;

	int active_torrent;

	int listen_port;
	int allocation_mode;
	std::string save_path;
	int torrent_upload_limit;
	int torrent_download_limit;
	std::string monitor_dir;
	std::string bind_to_interface;
	std::string outgoing_interface;
	int poll_interval;
	int max_connections_per_torrent;
	bool seed_mode;

	bool share_mode;
	bool disable_storage;
	bool start_dht ;
	bool start_upnp ;
	bool start_lsd ;
	bool min_mod;
	bool high_mod;
	// if non-empty, a peer that will be added to all torrents
	std::string peer;

	static char const* state_str[];

	struct torrent_entry
	{
		torrent_entry(libtorrent::torrent_handle h) : handle(h) {}
		libtorrent::torrent_handle handle;
		libtorrent::torrent_status status;
	};
	//****************************************************************************//
	libtorrent::session_settings settings;
	libtorrent::proxy_settings ps;
	handles_t files;
	std::set<libtorrent::torrent_handle> non_files;
	libtorrent::session *ses;
	//std::vector<std::string> torrents;
	boost::unordered_set<torrent_status> all_handles;
	std::vector<torrent_status const*> filtered_handles;
	libtorrent::error_code ec;
private:
	AllTorrent items;
public:
	static Sess* getInstance();//内部静态变量的懒汉实现
	AllTorrent& getItem();
	void addTorrent(std::string str);
	void addMagnet(std::string str);
	bool has_task = false;
public:
	void continueDownload();
	void stopDownload();
	void restart();
	void stopAll();
	void continueAll();
	void deleteTask();
	void deleteAll();

};
#endif //SESS_H