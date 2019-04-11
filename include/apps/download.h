#ifndef DOWNLOAD_H
#define DOWNLOAD_H
#include <iterator>
#include "libtorrent/config.hpp"

#ifdef _MSC_VER
#pragma warning(push, 1)
#endif

#include <boost/bind.hpp>
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

using boost::bind;

typedef std::multimap<std::string, libtorrent::torrent_handle> handles_t;

using libtorrent::torrent_status;

class Download
{
public:
	Download();
	~Download();
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

	// if non-empty, a peer that will be added to all torrents
	std::string peer;

	static char const* state_str[];

	struct torrent_entry
	{
		torrent_entry(libtorrent::torrent_handle h) : handle(h) {}
		libtorrent::torrent_handle handle;
		libtorrent::torrent_status status;
	};
public:
	bool show_torrent(libtorrent::torrent_status const& st, int torrent_filter, int* counters);
	static bool yes(libtorrent::torrent_status const&);
	static bool compare_torrent(torrent_status const* lhs, torrent_status const* rhs);
	void update_filtered_torrents(boost::unordered_set<torrent_status>& all_handles
		, std::vector<torrent_status const*>& filtered_handles, int* counters);
	char const* esc(char const* code);
	std::string to_string(int v, int width);
	std::string& to_string(float v, int width, int precision);
	std::string add_suffix(float val, char const* suffix);
	std::string const& piece_bar(libtorrent::bitfield const& p, int width);
	std::string const& progress_bar(int progress, int width, char const* code);
	int peer_index(libtorrent::tcp::endpoint addr, std::vector<libtorrent::peer_info> const& peers);
	void print_peer_info(std::string& out, std::vector<libtorrent::peer_info> const& peers);
	void add_torrent(libtorrent::session& ses
		, handles_t& files
		, std::set<libtorrent::torrent_handle>& non_files
		, std::string const& torrent
		, int allocation_mode
		, std::string const& save_path
		, bool monitored_dir
		, int torrent_upload_limit
		, int torrent_download_limit);
	void scan_dir(std::string const& dir_path
		, libtorrent::session& ses
		, handles_t& files
		, std::set<libtorrent::torrent_handle>& non_files
		, int allocation_mode
		, std::string const& save_path
		, int torrent_upload_limit
		, int torrent_download_limit);
	torrent_status const& get_active_torrent(std::vector<torrent_status const*> const& filtered_handles);
	void print_alert(libtorrent::alert const* a, std::string& str);
	int save_file(std::string const& filename, std::vector<char>& v);
	bool handle_alert(libtorrent::session& ses, libtorrent::alert* a
		, handles_t& files, std::set<libtorrent::torrent_handle>& non_files
		, int* counters, boost::unordered_set<torrent_status>& all_handles
		, std::vector<torrent_status const*>& filtered_handles
		, bool& need_resort);
	void print_piece(libtorrent::partial_piece_info* pp
		, libtorrent::cached_piece_info* cs
		, std::vector<libtorrent::peer_info> const& peers
		, std::string& out);
	int main(int argc, char argv[][100]);
};
#endif //DOWNLOAD_H