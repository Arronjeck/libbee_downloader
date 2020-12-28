//  HLSPlaylistDownloader.hpp
//  Created by Krishna Gudipati on 8/14/18.

#ifndef HLSPlaylistDownloader_hpp
#define HLSPlaylistDownloader_hpp

#include <sstream>
#include <fstream>
#include <mutex>
#include <atomic>
#include <process.h>
#include <vadefs.h>

using namespace std;

#define NAME_LINE 40

typedef struct __THREAD_DATA
{
	mutex mtx_start;	// start download flag

	string sUrl;
	string readBuffer;
	int iRangeBegin;
	int iRangeEnd;
	atomic_bool bDoComplete;
	atomic_bool bStartFlag;

	void Reset( string url, int iBegin, int iEnd )
	{
		sUrl = url;
		readBuffer.clear();
		iRangeBegin = iBegin;
		iRangeEnd = iEnd;
		bDoComplete = false;
		bStartFlag = true;
	}
	__THREAD_DATA()
	{
		sUrl = "";
		readBuffer.clear();
		iRangeBegin = 0;
		iRangeEnd = 0;
		bDoComplete = false;
		bStartFlag = false;
	}
} THREAD_DATA;

typedef struct __MONITOR_DATA
{
	string sUrl;
	string sDurition;
	atomic_bool bStartFlag;

	__MONITOR_DATA()
	{
		sUrl = "index.m3u8";
		sDurition = "";
		bStartFlag = false;
	}

} MONITOR_DATA;

class HLSPlaylistDownloader {

public:
	enum {MAX_THREAD_NUM = 3};
public :
	HLSPlaylistDownloader();
	~HLSPlaylistDownloader();

private:
	string m_url;         // Original url supplied by the user
	string m_outputFile;
	ofstream m_hlsstream; // Every file that is being downloaded by this class this ofstream is used to save the downloaded file

	static mutex m_mtxFile;
	MONITOR_DATA m_m3u8Info;
	uintptr_t m_hThreadMonitor;
	uintptr_t m_hThreadHandle[3];
	THREAD_DATA m_DownInfo[3];
	uintptr_t m_hThreadHandleNext[3];
	THREAD_DATA m_DownInfoNext[3];
	static atomic_bool m_bExitFlag;

	long getItemLenth( const char *url );
	long getItemLenth1( const char *url );
	static long long getItemTime( const char *url );
	// Curl reladed methods
	bool downloadItem( const char *url ); // Every file is downloaded using this method
	static bool downloadItem( const char *url, string &strReadBuffer ); // Every file is downloaded using this method
	bool downloadBlock( const char *url, int nBegin, int nEnd );
	static bool downloadBlockEx( const char *url, int nBegin, int nEnd, string &sBuffer );
	static size_t save_header( void *ptr, size_t size, size_t nmemb, void *data );
	static size_t curlCallBack( void *curlData, size_t size, size_t receievedSize, void *writeToFileBuffer );
	static unsigned downloadFunc( void *pArg );
	static unsigned DoM3u8Monitor( void *pArg );
public:
	void InitDownloadThread();
	void DeleteDownloadThread();
	void setDownloadInfo( string urlPath, string outfile );
	bool StartMonitor( string strUrl, string strDurition );
	bool downloadPlaylist();
	bool downloadPlaylistEx( string sUrl, string sName );
	void downloadMedia( string sUrl, string sName );
	void downloadMediaOne( string sUrl, string sName );
	void downloadMediaTwo( string sUrl, string sName );
	void downloadMediaThree( string sUrl, string sName );
	void downloadPlaylistMedia( string baseUrlPath, string playlistPath, string destination );
	void downloadIndividualPlaylist( string baseUrlPath, string playlistName, string destination );
};

#endif /* HLSPlaylistDownloader_hpp */
