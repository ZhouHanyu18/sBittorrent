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
	 print_downloads = false;
	 print_piece_bar = false;
	 print_file_progress = false;
	 show_pad_files = false;
	 show_dht_status = false;
	 sequential_download = false;
	 print_utp_stats = false;

	 print_ip = true;
	 print_as = false;
	 print_timers = false;
	 print_block = false;
	 print_peer_rate = false;
	 print_fails = false;
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







#ifdef _WIN32

#if defined(_MSC_VER)
#	define for if (false) {} else for
#endif

#include <windows.h>
#include <conio.h>

bool sleep_and_input(int* c, int sleep)
{
	for (int i = 0; i < 2; ++i)
	{
		if (_kbhit())
		{
			*c = _getch();
			return true;
		}
		Sleep(sleep / 2);
	}
	return false;
};

void clear_home()
{
	CONSOLE_SCREEN_BUFFER_INFO si;
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(h, &si);
	COORD c = { 0, 0 };
	DWORD n;
	FillConsoleOutputCharacter(h, ' ', si.dwSize.X * si.dwSize.Y, c, &n);
	SetConsoleCursorPosition(h, c);
}

#else

#include <stdlib.h>
#include <stdio.h>

#include <termios.h>
#include <string.h>
#include <sys/ioctl.h>

#define ANSI_TERMINAL_COLORS

struct set_keypress
{
	set_keypress()
	{
		termios new_settings;
		tcgetattr(0, &stored_settings);
		new_settings = stored_settings;
		// Disable canonical mode, and set buffer size to 1 byte
		new_settings.c_lflag &= (~ICANON);
		new_settings.c_cc[VTIME] = 0;
		new_settings.c_cc[VMIN] = 1;
		tcsetattr(0, TCSANOW, &new_settings);
	}
	~set_keypress() { tcsetattr(0, TCSANOW, &stored_settings); }
	termios stored_settings;
};

bool sleep_and_input(int* c, int sleep)
{
	// sets the terminal to single-character mode
	// and resets when destructed
	set_keypress s;
	libtorrent::ptime start = libtorrent::time_now_hires();
	int ret = 0;
retry:
	fd_set set;
	FD_ZERO(&set);
	FD_SET(0, &set);
	timeval tv = { sleep / 1000, (sleep % 1000) * 1000 };
	ret = select(1, &set, 0, 0, &tv);
	if (ret > 0)
	{
		*c = getc(stdin);
		return true;
	}
	if (errno == EINTR)
	{
		if (total_milliseconds(libtorrent::time_now_hires() - start) < sleep)
			goto retry;
		return false;
	}

	if (ret < 0 && errno != 0 && errno != ETIMEDOUT)
	{
		fprintf(stderr, "select failed: %s\n", strerror(errno));
		libtorrent::sleep(500);
	}

	return false;
}

void clear_home()
{
	puts("\033[2J\033[0;0H");
}

#endif













// maps filenames to torrent_handles


bool Download::show_torrent(libtorrent::torrent_status const& st, int torrent_filter, int* counters)
{
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

void Download::update_filtered_torrents(boost::unordered_set<torrent_status>& all_handles
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

char const* Download::esc(char const* code)
{
#ifdef ANSI_TERMINAL_COLORS
	// this is a silly optimization
	// to avoid copying of strings
	enum { num_strings = 200 };
	static char buf[num_strings][20];
	static int round_robin = 0;
	char* ret = buf[round_robin];
	++round_robin;
	if (round_robin >= num_strings) round_robin = 0;
	ret[0] = '\033';
	ret[1] = '[';
	int i = 2;
	int j = 0;
	while (code[j]) ret[i++] = code[j++];
	ret[i++] = 'm';
	ret[i++] = 0;
	return ret;
#else
	return "";
#endif
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

std::string const& Download::piece_bar(libtorrent::bitfield const& p, int width)
{
#ifdef ANSI_TERMINAL_COLORS
	static const char* lookup[] =
	{
		// black, blue, cyan, white
		"40", "44", "46", "47"
	};

	const int table_size = sizeof(lookup) / sizeof(lookup[0]);
#else
	static const char char_lookup[] =
	{ ' ', '.', ':', '-', '+', '*', '#' };

	const int table_size = sizeof(char_lookup) / sizeof(char_lookup[0]);
#endif

	double piece_per_char = p.size() / double(width);
	static std::string bar;
	bar.clear();
	bar.reserve(width * 6);
	bar += "[";
	if (p.size() == 0)
	{
		for (int i = 0; i < width; ++i) bar += ' ';
		bar += "]";
		return bar;
	}

	// the [piece, piece + pieces_per_char) range is the pieces that are represented by each character
	double piece = 0;
	for (int i = 0; i < width; ++i, piece += piece_per_char)
	{
		int num_pieces = 0;
		int num_have = 0;
		int end = (std::max)(int(piece + piece_per_char), int(piece) + 1);
		for (int k = int(piece); k < end; ++k, ++num_pieces)
			if (p[k]) ++num_have;
		int color = int(std::ceil(num_have / float(num_pieces) * (table_size - 1)));
#ifdef ANSI_TERMINAL_COLORS
		bar += esc(lookup[color]);
		bar += " ";
#else
		bar += char_lookup[color];
#endif
	}
#ifdef ANSI_TERMINAL_COLORS
	bar += esc("0");
#endif
	bar += "]";
	return bar;
}

std::string const& Download::progress_bar(int progress, int width, char const* code = "33")
{
	static std::string bar;
	bar.clear();
	bar.reserve(width + 10);

	int progress_chars = (progress * width + 500) / 1000;
	bar = esc(code);
	std::fill_n(std::back_inserter(bar), progress_chars, '#');
	std::fill_n(std::back_inserter(bar), width - progress_chars, '-');
	bar += esc("0");
	return bar;
}

int Download::peer_index(libtorrent::tcp::endpoint addr, std::vector<libtorrent::peer_info> const& peers)
{
	using namespace libtorrent;
	std::vector<peer_info>::const_iterator i = std::find_if(peers.begin()
		, peers.end(), boost::bind(&peer_info::ip, _1) == addr);
	if (i == peers.end()) return -1;

	return i - peers.begin();
}

void Download::print_peer_info(std::string& out, std::vector<libtorrent::peer_info> const& peers)
{
	using namespace libtorrent;
	if (print_ip) out += "IP                                                   ";
#ifndef TORRENT_DISABLE_GEO_IP
	if (print_as) out += "AS                                         ";
#endif
	out += "down     (total | peak   )  up      (total | peak   ) sent-req tmo bsy rcv flags            source  ";
	if (print_fails) out += "fail hshf ";
	if (print_send_bufs) out += "rq sndb            quota rcvb            q-bytes ";
	if (print_timers) out += "inactive wait timeout q-time ";
	out += "disk   rtt ";
	if (print_block) out += "block-progress ";
#ifndef TORRENT_DISABLE_RESOLVE_COUNTRIES
	out += "country ";
#endif
	if (print_peer_rate) out += "peer-rate est.rec.rate ";
	out += "client \n";

	char str[500];
	for (std::vector<peer_info>::const_iterator i = peers.begin();
		i != peers.end(); ++i)
	{
		if (i->flags & (peer_info::handshake | peer_info::connecting | peer_info::queued))
			continue;

		if (print_ip)
		{
			snprintf(str, sizeof(str), "%-30s %-22s", (print_endpoint(i->ip) +
				(i->connection_type == peer_info::bittorrent_utp ? " [uTP]" : "")).c_str()
				, print_endpoint(i->local_endpoint).c_str());
			out += str;
		}

#ifndef TORRENT_DISABLE_GEO_IP
		if (print_as)
		{
			error_code ec;
			snprintf(str, sizeof(str), "%-42s ", i->inet_as_name.c_str());
			out += str;
		}
#endif

		char temp[10];
		snprintf(temp, sizeof(temp), "%d/%d"
			, i->download_queue_length
			, i->target_dl_queue_length);
		temp[7] = 0;

		snprintf(str, sizeof(str)
			, "%s%s (%s|%s) %s%s (%s|%s) %s%7s %4d%4d%4d %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c %c%c%c%c%c%c "
			, esc("32"), add_suffix(i->down_speed, "/s").c_str()
			, add_suffix(i->total_download).c_str(), add_suffix(i->download_rate_peak, "/s").c_str()
			, esc("31"), add_suffix(i->up_speed, "/s").c_str(), add_suffix(i->total_upload).c_str()
			, add_suffix(i->upload_rate_peak, "/s").c_str(), esc("0")

			, temp // sent requests and target number of outstanding reqs.
			, i->timed_out_requests
			, i->busy_requests
			, i->upload_queue_length

			, (i->flags & peer_info::interesting) ? 'I' : '.'
			, (i->flags & peer_info::choked) ? 'C' : '.'
			, (i->flags & peer_info::remote_interested) ? 'i' : '.'
			, (i->flags & peer_info::remote_choked) ? 'c' : '.'
			, (i->flags & peer_info::supports_extensions) ? 'e' : '.'
			, (i->flags & peer_info::local_connection) ? 'l' : 'r'
			, (i->flags & peer_info::seed) ? 's' : '.'
			, (i->flags & peer_info::on_parole) ? 'p' : '.'
			, (i->flags & peer_info::optimistic_unchoke) ? 'O' : '.'
			, (i->read_state == peer_info::bw_limit) ? 'r' :
			(i->read_state == peer_info::bw_network) ? 'R' :
			(i->read_state == peer_info::bw_disk) ? 'D' : '.'
			, (i->write_state == peer_info::bw_limit) ? 'w' :
			(i->write_state == peer_info::bw_network) ? 'W' :
			(i->write_state == peer_info::bw_disk) ? 'D' : '.'
			, (i->flags & peer_info::snubbed) ? 'S' : '.'
			, (i->flags & peer_info::upload_only) ? 'U' : 'D'
			, (i->flags & peer_info::endgame_mode) ? '-' : '.'
#ifndef TORRENT_DISABLE_ENCRYPTION
			, (i->flags & peer_info::rc4_encrypted) ? 'E' :
			(i->flags & peer_info::plaintext_encrypted) ? 'e' : '.'
#else
			, '.'
#endif
			, (i->flags & peer_info::holepunched) ? 'h' : '.'

			, (i->source & peer_info::tracker) ? 'T' : '_'
			, (i->source & peer_info::pex) ? 'P' : '_'
			, (i->source & peer_info::dht) ? 'D' : '_'
			, (i->source & peer_info::lsd) ? 'L' : '_'
			, (i->source & peer_info::resume_data) ? 'R' : '_'
			, (i->source & peer_info::incoming) ? 'I' : '_');
		out += str;

		if (print_fails)
		{
			snprintf(str, sizeof(str), "%3d %3d "
				, i->failcount, i->num_hashfails);
			out += str;
		}
		if (print_send_bufs)
		{
			snprintf(str, sizeof(str), "%2d %6d (%s) %5d %6d (%s) %6d "
				, i->requests_in_buffer, i->used_send_buffer, add_suffix(i->send_buffer_size).c_str()
				, i->send_quota, i->used_receive_buffer, add_suffix(i->receive_buffer_size).c_str()
				, i->queue_bytes);
			out += str;
		}
		if (print_timers)
		{
			snprintf(str, sizeof(str), "%8d %4d %7d %6d "
				, total_seconds(i->last_active)
				, total_seconds(i->last_request)
				, i->request_timeout
				, total_seconds(i->download_queue_time));
			out += str;
		}
		snprintf(str, sizeof(str), "%s %4d "
			, add_suffix(i->pending_disk_bytes).c_str()
			, i->rtt);
		out += str;

		if (print_block)
		{
			if (i->downloading_piece_index >= 0)
			{
				out += progress_bar(
					i->downloading_progress * 1000 / i->downloading_total, 14);
			}
			else
			{
				out += progress_bar(0, 14);
			}
		}

#ifndef TORRENT_DISABLE_RESOLVE_COUNTRIES
		if (i->country[0] == 0)
		{
			out += " ..";
		}
		else
		{
			snprintf(str, sizeof(str), " %c%c", i->country[0], i->country[1]);
			out += str;
		}
#endif
		if (print_peer_rate)
		{
			bool unchoked = (i->flags & peer_info::choked) == 0;

			snprintf(str, sizeof(str), " %s %s"
				, add_suffix(i->remote_dl_rate, "/s").c_str()
				, unchoked ? add_suffix(i->estimated_reciprocation_rate, "/s").c_str() : "      ");
			out += str;
		}
		out += " ";

		if (i->flags & peer_info::handshake)
		{
			out += esc("31");
			out += " waiting for handshake";
			out += esc("0");
			out += "\n";
		}
		else if (i->flags & peer_info::connecting)
		{
			out += esc("31");
			out += " connecting to peer";
			out += esc("0");
			out += "\n";
		}
		else if (i->flags & peer_info::queued)
		{
			out += esc("33");
			out += " queued";
			out += esc("0");
			out += "\n";
		}
		else
		{
			out += " ";
			out += i->client;
			out += "\n";
		}
	}
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

torrent_status const& Download::get_active_torrent(std::vector<torrent_status const*> const& filtered_handles)
{
	if (active_torrent >= int(filtered_handles.size())
		|| active_torrent < 0) active_torrent = 0;
	return *filtered_handles[active_torrent];
}

void Download::print_alert(libtorrent::alert const* a, std::string& str)
{
	using namespace libtorrent;

#ifdef ANSI_TERMINAL_COLORS
	if (a->category() & alert::error_notification)
	{
		str += esc("31");
	}
	else if (a->category() & (alert::peer_notification | alert::storage_notification))
	{
		str += esc("33");
	}
#endif
	str += "[";
	str += time_now_string();
	str += "] ";
	str += a->message();
#ifdef ANSI_TERMINAL_COLORS
	str += esc("0");
#endif

	if (g_log_file)
		fprintf(g_log_file, "[%s] %s\n", time_now_string(), a->message().c_str());
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

// returns true if the alert was handled (and should not be printed to the log)
// returns false if the alert was not handled
bool Download::handle_alert(libtorrent::session& ses, libtorrent::alert* a
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
		error_code ec;
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
					error_code ec;
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

void Download::print_piece(libtorrent::partial_piece_info* pp
	, libtorrent::cached_piece_info* cs
	, std::vector<libtorrent::peer_info> const& peers
	, std::string& out)
{
	using namespace libtorrent;

	char str[1024];
	assert(pp == 0 || cs == 0 || cs->piece == pp->piece_index);
	int piece = pp ? pp->piece_index : cs->piece;
	int num_blocks = pp ? pp->blocks_in_piece : cs->blocks.size();

	snprintf(str, sizeof(str), "%5d: [", piece);
	out += str;
	char const* last_color = 0;
	for (int j = 0; j < num_blocks; ++j)
	{
		int index = pp ? peer_index(pp->blocks[j].peer(), peers) % 36 : -1;
		char chr = '+';
		if (index >= 0)
			chr = (index < 10) ? '0' + index : 'A' + index - 10;

		char const* color = "";

		if (pp == 0)
		{
			color = cs->blocks[j] ? esc("34;7") : esc("0");
			chr = ' ';
		}
		else
		{
#ifdef ANSI_TERMINAL_COLORS
			if (cs && cs->blocks[j]) color = esc("36;7");
			else if (pp->blocks[j].bytes_progress > 0
				&& pp->blocks[j].state == block_info::requested)
			{
				if (pp->blocks[j].num_peers > 1) color = esc("1;7");
				else color = esc("33;7");
				chr = '0' + (pp->blocks[j].bytes_progress * 10 / pp->blocks[j].block_size);
			}
			else if (pp->blocks[j].state == block_info::finished) color = esc("32;7");
			else if (pp->blocks[j].state == block_info::writing) color = esc("36;7");
			else if (pp->blocks[j].state == block_info::requested) color = esc("0");
			else { color = esc("0"); chr = ' '; }
#else
			if (cs && cs->blocks[j]) chr = 'c';
			else if (pp->blocks[j].state == block_info::finished) chr = '#';
			else if (pp->blocks[j].state == block_info::writing) chr = '+';
			else if (pp->blocks[j].state == block_info::requested) chr = '-';
			else chr = ' ';
#endif
		}
		if (last_color == 0 || strcmp(last_color, color) != 0)
			snprintf(str, sizeof(str), "%s%c", color, chr);
		else
			out += chr;

		out += str;
	}
#ifdef ANSI_TERMINAL_COLORS
	out += esc("0");
#endif
	char const* piece_state[4] = { "", " slow", " medium", " fast" };
	snprintf(str, sizeof(str), "] %3d cache age: %-.1f %s\n"
		, cs ? cs->next_to_hash : 0
		, cs ? (total_milliseconds(time_now() - cs->last_use) / 1000.f) : 0.f
		, pp ? piece_state[pp->piece_state] : "");
	out += str;
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
		ses.start_lsd();

	if (start_upnp)
	{
		ses.start_upnp();
		ses.start_natpmp();
	}

	ses.set_proxy(ps);

	ses.listen_on(std::make_pair(listen_port, listen_port)
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

	ses.set_settings(settings);

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

		// loop through the alert queue to see if anything has happened.
		std::deque<alert*> alerts;
		ses.pop_alerts(&alerts);
		std::string now = time_now_string();
		for (std::deque<alert*>::iterator i = alerts.begin()
			, end(alerts.end()); i != end; ++i)
		{
			bool need_resort = false;
			TORRENT_TRY
			{
				if (!handle_alert(ses, *i, files, non_files, counters
				, all_handles, filtered_handles, need_resort))
				{
					// if we didn't handle the alert, print it to the log
					std::string event_string;
					print_alert(*i, event_string);
					events.push_back(event_string);
					if (events.size() >= 20) events.pop_front();
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

		session_status sess_stat = ses.status();

		// in test mode, also quit when we loose the last peer
		if (loop_limit > 1 && sess_stat.num_peers == 0 && tick > 30) break;

		std::string out;
		out = "[h] show key mappings\n";

		char const* filter_names[] = { "all", "downloading", "non-paused", "seeding", "queued", "stopped", "checking", "RSS" };
		for (int i = 0; i < int(sizeof(filter_names) / sizeof(filter_names[0])); ++i)
		{
			char filter[200];
			snprintf(filter, sizeof(filter), "%s[%s (%d)]%s", torrent_filter == i ? esc("7") : ""
				, filter_names[i], counters[i], torrent_filter == i ? esc("0") : "");
			out += filter;
		}
		out += '\n';

		char str[500];
		int torrent_index = 0;
		int lines_printed = 3;

		if (torrent_filter == torrents_feeds)
		{
			for (std::vector<feed_handle>::iterator i = feeds.begin()
				, end(feeds.end()); i != end; ++i)
			{
				if (lines_printed >= max_lines)
				{
					out += "...\n";
					break;
				}

				feed_status st = i->get_feed_status();
				if (st.url.size() > 70) st.url.resize(70);

				char update_timer[20];
				snprintf(update_timer, sizeof(update_timer), "%d", st.next_update);
				snprintf(str, sizeof(str), "%-70s %s (%2d) %s\n", st.url.c_str()
					, st.updating ? "updating" : update_timer
					, int(st.items.size())
					, st.error ? st.error.message().c_str() : "");
				out += str;
				++lines_printed;
			}
		}

		// handle scrolling down when moving the cursor
		// below the fold
		static int start_offset = 0;
		if (active_torrent >= max_lines - lines_printed - start_offset)
			start_offset = active_torrent - max_lines + lines_printed + 1;
		if (active_torrent < start_offset) start_offset = active_torrent;

		for (std::vector<torrent_status const*>::iterator i = filtered_handles.begin();
			i != filtered_handles.end(); ++torrent_index)
		{
			if (torrent_index < start_offset)
			{
				++i;
				continue;
			}
			if (lines_printed >= max_lines)
			{
				out += "...\n";
				break;
			}

			torrent_status const& s = **i;
			if (!s.handle.is_valid())
			{
				i = filtered_handles.erase(i);
				continue;
			}
			else
			{
				++i;
			}

#ifdef ANSI_TERMINAL_COLORS
			char const* term = "\x1b[0m";
#else
			char const* term = "";
#endif
			if (active_torrent == torrent_index)
			{
				term = "\x1b[0m\x1b[7m";
				out += esc("7");
				out += "*";
			}
			else
			{
				out += " ";
			}

			int queue_pos = s.queue_position;
			if (queue_pos == -1) out += "-  ";
			else
			{
				snprintf(str, sizeof(str), "%-3d", queue_pos);
				out += str;
			}

			if (s.paused) out += esc("34");
			else out += esc("37");

			std::string name = s.name;
			if (name.size() > 40) name.resize(40);
			snprintf(str, sizeof(str), "%-40s %s ", name.c_str(), term);
			out += str;

			if (!s.error.empty())
			{
				out += esc("31");
				out += "error ";
				out += s.error;
				out += esc("0");
				out += "\n";
				++lines_printed;
				continue;
			}

			int seeds = 0;
			int downloaders = 0;

			if (s.num_complete >= 0) seeds = s.num_complete;
			else seeds = s.list_seeds;

			if (s.num_incomplete >= 0) downloaders = s.num_incomplete;
			else downloaders = s.list_peers - s.list_seeds;

			snprintf(str, sizeof(str), "%s%-13s down: (%s%s%s) up: %s%s%s (%s%s%s) swarm: %4d:%4d"
				"  bw queue: (%d|%d) all-time (Rx: %s%s%s Tx: %s%s%s) seed rank: %x %c%s\n"
				, (!s.paused && !s.auto_managed) ? "[F] " : ""
				, (s.paused && !s.auto_managed) ? "paused" :
				(s.paused && s.auto_managed) ? "queued" :
				Download::state_str[s.state]
				, esc("32"), add_suffix(s.total_download).c_str(), term
				, esc("31"), add_suffix(s.upload_rate, "/s").c_str(), term
				, esc("31"), add_suffix(s.total_upload).c_str(), term
				, downloaders, seeds
				, s.up_bandwidth_queue, s.down_bandwidth_queue
				, esc("32"), add_suffix(s.all_time_download).c_str(), term
				, esc("31"), add_suffix(s.all_time_upload).c_str(), term
				, s.seed_rank, s.need_save_resume ? 'S' : ' ', esc("0"));
			out += str;
			++lines_printed;

			if (torrent_index != active_torrent && s.state == torrent_status::seeding) continue;
			char const* progress_bar_color = "33"; // yellow
			if (s.state == torrent_status::downloading_metadata)
			{
				progress_bar_color = "35"; // magenta
			}
			else if (s.current_tracker.empty())
			{
				progress_bar_color = "31"; // red
			}
			else if (sess_stat.has_incoming_connections)
			{
				progress_bar_color = "32"; // green
			}

			snprintf(str, sizeof(str), "     %-10s: %s%-11"PRId64"%s Bytes %6.2f%% %s\n"
				, s.sequential_download ? "sequential" : "progress"
				, esc("32"), s.total_done, esc("0")
				, s.progress_ppm / 10000.f
				, progress_bar(s.progress_ppm / 1000, terminal_width - 43, progress_bar_color).c_str());
			out += str;
			++lines_printed;

			if (print_piece_bar && (s.state != torrent_status::seeding || s.seed_mode))
			{
				out += "     ";
				out += piece_bar(s.pieces, terminal_width - 7);
				out += "\n";
				++lines_printed;
				if (s.seed_mode)
				{
					out += "     ";
					out += piece_bar(s.verified_pieces, terminal_width - 7);
					out += "\n";
					++lines_printed;
				}
			}

			if (s.state != torrent_status::queued_for_checking && s.state != torrent_status::checking_files)
			{
				boost::posix_time::time_duration t = s.next_announce;
				snprintf(str, sizeof(str)
					, "     peers: %s%d%s (%s%d%s) seeds: %s%d%s distributed copies: %s%4.2f%s "
					"sparse regions: %d download: %s%s%s next announce: %s%02d:%02d:%02d%s "
					"tracker: %s%s%s\n"
					, esc("37"), s.num_peers, esc("0")
					, esc("37"), s.connect_candidates, esc("0")
					, esc("37"), s.num_seeds, esc("0")
					, esc("37"), s.distributed_copies, esc("0")
					, s.sparse_regions
					, esc("32"), add_suffix(s.download_rate, "/s").c_str(), esc("0")
					, esc("37"), int(t.hours()), int(t.minutes()), int(t.seconds()), esc("0")
					, esc("36"), s.current_tracker.c_str(), esc("0"));
				out += str;
				++lines_printed;
			}
		}

		cache_status cs = ses.get_cache_status();
		if (cs.blocks_read < 1) cs.blocks_read = 1;
		if (cs.blocks_written < 1) cs.blocks_written = 1;

		snprintf(str, sizeof(str), "==== conns: %d down: %s%s%s (%s%s%s) up: %s%s%s (%s%s%s) "
			"tcp/ip: %s%s%s %s%s%s DHT: %s%s%s %s%s%s tracker: %s%s%s %s%s%s ====\n"
			, sess_stat.num_peers
			, esc("32"), add_suffix(sess_stat.download_rate, "/s").c_str(), esc("0")
			, esc("32"), add_suffix(sess_stat.total_download).c_str(), esc("0")
			, esc("31"), add_suffix(sess_stat.upload_rate, "/s").c_str(), esc("0")
			, esc("31"), add_suffix(sess_stat.total_upload).c_str(), esc("0")
			, esc("32"), add_suffix(sess_stat.ip_overhead_download_rate, "/s").c_str(), esc("0")
			, esc("31"), add_suffix(sess_stat.ip_overhead_upload_rate, "/s").c_str(), esc("0")
			, esc("32"), add_suffix(sess_stat.dht_download_rate, "/s").c_str(), esc("0")
			, esc("31"), add_suffix(sess_stat.dht_upload_rate, "/s").c_str(), esc("0")
			, esc("32"), add_suffix(sess_stat.tracker_download_rate, "/s").c_str(), esc("0")
			, esc("31"), add_suffix(sess_stat.tracker_upload_rate, "/s").c_str(), esc("0"));
		out += str;

		snprintf(str, sizeof(str), "==== waste: %s fail: %s unchoked: %d / %d "
			"bw queues: %8d (%d) | %8d (%d) disk queues: %d | %d cache: w: %"PRId64"%% r: %"PRId64"%% "
			"size: %s (%s) / %s dq: %"PRId64" ===\n"
			, add_suffix(sess_stat.total_redundant_bytes).c_str()
			, add_suffix(sess_stat.total_failed_bytes).c_str()
			, sess_stat.num_unchoked, sess_stat.allowed_upload_slots
			, sess_stat.up_bandwidth_bytes_queue
			, sess_stat.up_bandwidth_queue
			, sess_stat.down_bandwidth_bytes_queue
			, sess_stat.down_bandwidth_queue
			, sess_stat.disk_write_queue
			, sess_stat.disk_read_queue
			, (cs.blocks_written - cs.writes) * 100 / cs.blocks_written
			, cs.blocks_read_hit * 100 / cs.blocks_read
			, add_suffix(boost::int64_t(cs.cache_size) * 16 * 1024).c_str()
			, add_suffix(boost::int64_t(cs.read_cache_size) * 16 * 1024).c_str()
			, add_suffix(boost::int64_t(cs.total_used_buffers) * 16 * 1024).c_str()
			, cs.queued_bytes);
		out += str;

		snprintf(str, sizeof(str), "==== optimistic unchoke: %d unchoke counter: %d peerlist: %d ====\n"
			, sess_stat.optimistic_unchoke_counter, sess_stat.unchoke_counter, sess_stat.peerlist_size);
		out += str;

#ifndef TORRENT_DISABLE_DHT
		if (show_dht_status)
		{
			snprintf(str, sizeof(str), "DHT nodes: %d DHT cached nodes: %d "
				"total DHT size: %"PRId64" total observers: %d\n"
				, sess_stat.dht_nodes, sess_stat.dht_node_cache, sess_stat.dht_global_nodes
				, sess_stat.dht_total_allocations);
			out += str;

			int bucket = 0;
			for (std::vector<dht_routing_bucket>::iterator i = sess_stat.dht_routing_table.begin()
				, end(sess_stat.dht_routing_table.end()); i != end; ++i, ++bucket)
			{
				snprintf(str, sizeof(str)
					, "%3d [%2d, %2d] active: %d\n"
					, bucket, i->num_nodes, i->num_replacements, i->last_active);
				out += str;
			}

			for (std::vector<dht_lookup>::iterator i = sess_stat.active_requests.begin()
				, end(sess_stat.active_requests.end()); i != end; ++i)
			{
				snprintf(str, sizeof(str)
					, "  %10s [limit: %2d] "
					"in-flight: %-2d "
					"left: %-3d "
					"1st-timeout: %-2d "
					"timeouts: %-2d "
					"responses: %-2d "
					"last_sent: %-2d\n"
					, i->type
					, i->branch_factor
					, i->outstanding_requests
					, i->nodes_left
					, i->first_timeout
					, i->timeouts
					, i->responses
					, i->last_sent);
				out += str;
			}
		}
#endif

		if (print_utp_stats)
		{
			snprintf(str, sizeof(str), "uTP idle: %d syn: %d est: %d fin: %d wait: %d\n"
				, sess_stat.utp_stats.num_idle, sess_stat.utp_stats.num_syn_sent
				, sess_stat.utp_stats.num_connected, sess_stat.utp_stats.num_fin_sent
				, sess_stat.utp_stats.num_close_wait);
			out += str;
		}

		torrent_status const* st = 0;
		if (!filtered_handles.empty()) st = &get_active_torrent(filtered_handles);
		if (st && st->handle.is_valid())
		{
			torrent_handle h = st->handle;
			torrent_status const& s = *st;

			if ((print_downloads && s.state != torrent_status::seeding)
				|| print_peers)
				h.get_peer_info(peers);

			out += "====== ";
			out += st->name;
			out += " ======\n";

			if (print_peers && !peers.empty())
				print_peer_info(out, peers);

			if (print_trackers)
			{
				std::vector<announce_entry> tr = h.trackers();
				ptime now = time_now();
				for (std::vector<announce_entry>::iterator i = tr.begin()
					, end(tr.end()); i != end; ++i)
				{
					snprintf(str, sizeof(str), "%2d %-55s fails: %-3d (%-3d) %s %s %5d \"%s\" %s\n"
						, i->tier, i->url.c_str(), i->fails, i->fail_limit, i->verified ? "OK " : "-  "
						, i->updating ? "updating"
						: !i->will_announce(now) ? ""
						: to_string(total_seconds(i->next_announce - now), 8).c_str()
						, i->min_announce > now ? total_seconds(i->min_announce - now) : 0
						, i->last_error ? i->last_error.message().c_str() : ""
						, i->message.c_str());
					out += str;
				}
			}

			if (print_downloads)
			{
				std::vector<cached_piece_info> pieces;
				ses.get_cache_info(h.info_hash(), pieces);

				h.get_download_queue(queue);

				std::sort(queue.begin(), queue.end(), boost::bind(&partial_piece_info::piece_index, _1)
					< boost::bind(&partial_piece_info::piece_index, _2));

				std::sort(pieces.begin(), pieces.end(), boost::bind(&cached_piece_info::last_use, _1)
					> boost::bind(&cached_piece_info::last_use, _2));

				for (std::vector<cached_piece_info>::iterator i = pieces.begin();
					i != pieces.end(); ++i)
				{
					partial_piece_info* pp = 0;
					partial_piece_info tmp;
					tmp.piece_index = i->piece;
					std::vector<partial_piece_info>::iterator ppi
						= std::lower_bound(queue.begin(), queue.end(), tmp
						, boost::bind(&partial_piece_info::piece_index, _1)
						< boost::bind(&partial_piece_info::piece_index, _2));
					if (ppi != queue.end() && ppi->piece_index == i->piece) pp = &*ppi;

					print_piece(pp, &*i, peers, out);

					if (pp) queue.erase(ppi);
				}

				for (std::vector<partial_piece_info>::iterator i = queue.begin()
					, end(queue.end()); i != end; ++i)
				{
					print_piece(&*i, 0, peers, out);
				}
				snprintf(str, sizeof(str), "%s %s: read cache %s %s: downloading %s %s: cached %s %s: flushed\n"
					, esc("34;7"), esc("0") // read cache
					, esc("33;7"), esc("0") // downloading
					, esc("36;7"), esc("0") // cached
					, esc("32;7"), esc("0")); // flushed
				out += str;
				out += "___________________________________\n";
			}

			if (print_file_progress
				&& s.state != torrent_status::seeding
				&& s.has_metadata)
			{
				std::vector<size_type> file_progress;
				h.file_progress(file_progress);
				boost::intrusive_ptr<torrent_info> ti = h.torrent_file();
				for (int i = 0; i < ti->num_files(); ++i)
				{
					bool pad_file = ti->file_at(i).pad_file;
					if (!show_pad_files && pad_file) continue;
					int progress = ti->file_at(i).size > 0
						? file_progress[i] * 1000 / ti->file_at(i).size : 1000;

					char const* color = (file_progress[i] == ti->file_at(i).size)
						? "32" : "33";

					snprintf(str, sizeof(str), "%s %s %-5.2f%% %s %s%s\n",
						progress_bar(progress, 100, color).c_str()
						, pad_file ? esc("34") : ""
						, progress / 10.f
						, add_suffix(file_progress[i]).c_str()
						, ti->files().file_name(i).c_str()
						, pad_file ? esc("0") : "");
					out += str;
				}

				out += "___________________________________\n";
			}

		}

		if (print_log)
		{
			for (std::deque<std::string>::iterator i = events.begin();
				i != events.end(); ++i)
			{
				out += "\n";
				out += *i;
			}
		}

		clear_home();
		puts(out.c_str());
		fflush(stdout);

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

