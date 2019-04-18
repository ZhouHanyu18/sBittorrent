
#ifndef ALLTORRENTINFORMATION_H
#define ALLTORRENTINFORMATION_H
#include <string>
#include <vector>
struct Attribute
{
	int queue_pos;
	std::string status;
	std::string name = "������...";
	std::string download_rate = "O.0B/s";
	std::string download = "O.0B";
	std::string upload_rate = "O.0B/s";
	std::string upload = "O.0B";
	std::string time_download;	//������
	std::string time_upload;	//���ϴ�
	std::string size;
	float per;
	int peers;
	int seeds;
};

struct AllTorrent
{
	int size = 0;
	std::vector <Attribute> item;
	std::string total_download_rate = "O.0B/s";
	std::string total_download = "O.0B";
	std::string total_upload_rate = "O.0B/s";
	std::string total_upload = "O.0B";
	int dht_Node;
};

#endif //ALLTORRENTINFORMATION_H