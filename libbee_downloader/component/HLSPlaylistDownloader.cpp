//  HLSPlaylistDownloader.cpp
//  Created by Krishna Gudipati on 8/14/18.

#include "HLSPlaylistDownloader.hpp"
#include "HLSPlaylistUtilities.hpp"
#include <curl/curl.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "libcurl64.lib")

atomic_bool HLSPlaylistDownloader::m_bExitFlag = false;
mutex HLSPlaylistDownloader::m_mtxFile;
string HLSPlaylistDownloader::m_strBuffer = "";
long long HLSPlaylistDownloader::m_llLastModifyTime = 0l;
mutex HLSPlaylistDownloader::m_s_hlsListLock;
vector<string> HLSPlaylistDownloader::m_s_hlsList;
vector<string> HLSPlaylistDownloader::m_s_rmvList;
mutex HLSPlaylistDownloader::m_mtxDownLock;
queue<string> HLSPlaylistDownloader::m_downQueue;
mutex HLSPlaylistDownloader::m_mtxCompelteLock;
queue<string> HLSPlaylistDownloader::m_compelteQueue;
THREAD_DATA HLSPlaylistDownloader::m_ShareInfo[GROUP_NUM][THREAD_NUM];
atomic_bool HLSPlaylistDownloader::m_IsDownloading[GROUP_NUM];

HLSPlaylistDownloader::HLSPlaylistDownloader()
{
	//InitDownloadThread();
	//InitDownloader();
	m_hThreadMonitor = NULL;
}

HLSPlaylistDownloader::~HLSPlaylistDownloader() {
	DeleteDownloadThread();
	//DeleteDownloader();
	if ( m_hThreadMonitor != NULL )
	{
		WaitForSingleObject( ( HANDLE * )m_hThreadMonitor, INFINITE );
		CloseHandle( ( HANDLE )m_hThreadMonitor );
		m_hThreadMonitor = NULL;
	}
}

void HLSPlaylistDownloader::InitDownloadThread()
{
	m_bExitFlag = false;

	unsigned int threadID;
	for ( int i = 0; i < MAX_THREAD_NUM; ++i )
	{
		if ( m_hThreadHandle[i] != NULL )
		{
			continue;
		}
		m_hThreadHandle[i] = _beginthreadex( NULL, 0, &HLSPlaylistDownloader::downloadFunc, &( m_DownInfo[i] ), 0, &threadID );
	}
}

void HLSPlaylistDownloader::DeleteDownloadThread()
{
	m_bExitFlag = true;

	if ( m_hThreadHandle[0] != NULL )
	{
		WaitForMultipleObjectsEx( sizeof( m_hThreadHandle ), ( HANDLE * )m_hThreadHandle, FALSE, INFINITE, FALSE );
		for ( int i = 0; i < MAX_THREAD_NUM; ++i )
		{
			CloseHandle( ( HANDLE )m_hThreadHandle[i] );
			m_hThreadHandle[i] = NULL;
		}
	}
}

void HLSPlaylistDownloader::InitDownloader()
{
	m_bExitFlag = false;
	unsigned int threadID;
	for ( int i = 0; i < GROUP_NUM; ++i )
	{
		for ( int j = 0; j < THREAD_NUM; ++j )
		{
			if ( m_hHandle[i][j] != NULL )
			{
				continue;
			}
			m_ShareInfo[i][j].sGroup = "group";
			char szName[20] {0};
			sprintf_s( szName, 20, "group_%d", i );
			m_ShareInfo[i][j].sGroup = szName;
			m_hHandle[i][j] = _beginthreadex( NULL, 0, &HLSPlaylistDownloader::downloadPro, &( m_ShareInfo[i][j] ), 0, &threadID );
		}
	}
}

void HLSPlaylistDownloader::DeleteDownloader()
{
	m_bExitFlag = true;

	for ( int i = 0; i < GROUP_NUM; ++i )
	{
		if ( m_hHandle[i][0] == NULL )
		{
			continue;
		}
		WaitForMultipleObjectsEx( sizeof( m_hHandle[i] ), ( HANDLE * )m_hHandle[i], FALSE, INFINITE, FALSE );
		for ( int j = 0; j < THREAD_NUM; ++j )
		{
			CloseHandle( ( HANDLE )m_hHandle[i][j] );
			m_hHandle[i][j] = NULL;
		}
	}
}

void HLSPlaylistDownloader::setDownloadInfo( string urlPath, string outfile ) {
	m_url = urlPath;
	m_outputFile = outfile;
}

void HLSPlaylistDownloader::setDownloadInfoEx( string urlBasePath, string outputDir )
{
	m_baseUrlPath = urlBasePath;
	m_outputDir = outputDir;
}

bool HLSPlaylistDownloader::StartMonitor( string strUrl, string strDurition )
{
	if ( NULL != m_hThreadMonitor )
	{
		cout << "monitor already started" << endl;
		return true;
	}
	unsigned int threadID;
	m_m3u8Info.sUrl = strUrl;
	m_m3u8Info.sDurition = strDurition;
	m_m3u8Info.bStartFlag = true;
	m_bExitFlag = false;
	m_hThreadMonitor = _beginthreadex( NULL, 0, &HLSPlaylistDownloader::DoM3u8Monitor, &m_m3u8Info, 0, &threadID );

	return 0;
}

bool HLSPlaylistDownloader:: downloadPlaylist() {
	m_hlsstream = ofstream( m_outputFile, ios::out | ios::app );
	return downloadItem( m_url.c_str() );
}

size_t HLSPlaylistDownloader::curlCallBack( void *curlData, size_t size, size_t receievedSize, void *writeToFileBuffer ) {
	// Append to the buffer and at the end output fiile is written from this buffer
	( ( string * ) writeToFileBuffer )->append( ( char * ) curlData, size * receievedSize );
	return size * receievedSize;
}

unsigned HLSPlaylistDownloader::downloadFunc( void *pArg )
{
	THREAD_DATA *pInfo = ( THREAD_DATA * )( pArg );
	if ( pInfo == NULL )
	{
		cout << "[Error]: Invalid parameter, thread " << GetCurrentThreadId() << " end" << endl;
		return -1;
	}
	else
	{
		cout << "Thread " << GetCurrentThreadId() << " begin" << endl;
	}

	while ( 1 ) {
		if ( m_bExitFlag ) {
			break;
		}

		if ( pInfo->bStartFlag )
		{
			downloadBlockEx( pInfo->sUrl.c_str(), pInfo->iRangeBegin, pInfo->iRangeEnd, pInfo->readBuffer );
			pInfo->bDoComplete = true;
			pInfo->bStartFlag = false;
		}
	}

	cout << __FUNCTION__ << " Thread " << GetCurrentThreadId() << " end" << endl;
	return 0;
}

vector<string> quick_match(
	__in const vector<string> oldList,
	__in const vector<string> newList,
	__out vector<string> &removeList
)
{
	vector<string> latestQueue;

	if ( oldList.size() <= 0 )
	{
		latestQueue = newList;
		return latestQueue;
	}

	if ( newList.size() <= 0 )
	{
		latestQueue = oldList;
		return latestQueue;
	}

	vector<string>::const_iterator itBegin;
	vector<string>::const_iterator itEnd;
	vector<string>::const_iterator it2 = newList.begin();

	bool bFind = false;
	for ( vector<string>::const_iterator it1 = oldList.begin(); it1 != oldList.end() && it2 != newList.end(); ++it1 )
	{
		if ( bFind )
		{
			latestQueue.push_back( *it1 );
		}
		else
		{
			if ( *it1 == *it2 )
			{
				latestQueue.push_back( *it1 );
				bFind = true;
			}
			else
			{
				removeList.push_back( *it1 );
			}
		}

	}
	//
	if ( !bFind )
	{
		latestQueue = newList;
		return latestQueue;
	}


	bFind = false;
	vector<string>::iterator it3 = latestQueue.begin();
	for ( vector<string>::const_iterator it1 = newList.begin(); it1 != newList.end(); ++it1 )
	{
		if ( it3 == latestQueue.end() )
		{
			latestQueue.push_back( *it1 );
			continue;
		}
		else if ( *it1 != *it3 ) {
			*it3 = *it1;
			cout << "[ERROR] Not equal[old " << *it3 << "]:[new " << *it1 << "]" << endl;
		}

		++it3;
	}
	return latestQueue;
}

unsigned HLSPlaylistDownloader::DoM3u8Monitor( void *pArg )
{
	MONITOR_DATA *m3u8Info = ( MONITOR_DATA * )( pArg );
	if ( m3u8Info == NULL )
	{
		cout << __FUNCTION__ << "[Error]: Invalid parameter, thread " << GetCurrentThreadId() << " end" << endl;
		return -1;
	}
	else
	{
		cout << __FUNCTION__ << " Thread " << GetCurrentThreadId() << " begin : " << m3u8Info->sUrl << endl;
	}

	string strFileName = "index.m3u8";
	int iBegin = m3u8Info->sUrl.rfind( '/' );
	int iEnd = m3u8Info->sUrl.find( "?key=" );
	if ( iBegin != string::npos )
	{
		iBegin += 1;
		strFileName = m3u8Info->sUrl.substr( iBegin, iEnd - iBegin );
	}

	vector<string> lastList;
	long long llTime = -1;
	while ( 1 ) {

		clock_t dwLastTime, dwCountTime;
		dwLastTime = clock();
		if ( m_bExitFlag ) {
			break;
		}
		if ( !m3u8Info->bStartFlag )
		{
			continue;
		}
		long long llCurTime = getItemTime( m3u8Info->sUrl.c_str() );
		if ( llCurTime == -1 )
		{
			cout << __FUNCTION__ << "[Error] get item time fail." << endl;
		}
		else if ( llCurTime != llTime )
		{
			//cout << __FUNCTION__ << " m3u8 file changed." << endl;
			llTime = llCurTime;
			string localDownloadFileName = m3u8Info->sDurition + "/" + strFileName;
			string strBuffer;
			if ( downloadItem( m3u8Info->sUrl.c_str(), strBuffer ) )
			{
				vector<string> tsList =  HLSPlaylistUtilities::buildListFromStream( strBuffer );

				{
					unique_lock<mutex> lock( m_mtxFile );
					m_strBuffer = strBuffer;
					m_llLastModifyTime = llTime;
				}
				ofstream m3u8Index = ofstream( localDownloadFileName, ios::out | ios::trunc | ios::binary );
				if ( m3u8Index.is_open() )
				{
					m3u8Index << strBuffer;
				}
				m3u8Index.close();
				vector<string> rmList;
				vector<string> totalList;
				totalList = quick_match( lastList, tsList, rmList );
				//cout << "Changed New=" << tsList.size() << " Old=" << lastList.size() << endl;
// 				for ( vector<string>::iterator it = totalList.begin(); it != totalList.end(); ++it )
// 				{
// 					cout << "item: " << *it << endl;
// 				}
// 				for ( vector<string>::iterator it = rmList.begin(); it != rmList.end(); ++it )
// 				{
// 					cout << "Rmoved item: " << *it << endl;
// 				}

				OnCallbackM3u8Change( totalList, rmList );

				dwCountTime = clock() - dwLastTime;;
				//cout << "run loop m3u8 cost time: " << dwCountTime << "ms" << endl;
			}
		}
		else
		{
			//cout << __FUNCTION__ << " m3u8 file not changed." << endl;
		}

		chrono::milliseconds dura( 1000 );
		this_thread::sleep_for( dura );
	}

	cout << __FUNCTION__ << " Thread " << GetCurrentThreadId() << " end" << endl;
	return 0;
}

long long HLSPlaylistDownloader::GetM3u8File( string &str )
{
	unique_lock<mutex> lock( m_mtxFile );
	str = m_strBuffer;
	return m_llLastModifyTime;
}

unsigned HLSPlaylistDownloader::downloadSingle( void *pArg )
{
	THREAD_DATA *pInfo = ( THREAD_DATA * )( pArg );
	if ( pInfo == NULL )
	{
		cout << __FUNCTION__ << "[Error]: Invalid parameter, thread " << GetCurrentThreadId() << " end" << endl;
		return -1;
	}
	else
	{
		cout << __FUNCTION__ << " Thread " << GetCurrentThreadId() << " begin" << endl;
	}

	while ( true )
	{
		if ( m_bExitFlag )
		{
			return 0;
		}

		if ( pInfo->bStartFlag )
		{
			downloadItem( pInfo->sUrl.c_str(), pInfo->readBuffer );

			ofstream fs = ofstream( pInfo->sSavePath, ios::out | ios::trunc | ios::binary );
			fs << pInfo->readBuffer;
			fs.close();

			pInfo->bDoComplete = true;
			pInfo->bStartFlag = false;
		}
		else
		{
			//cout << "Thread " << pInfo->sGroup << " is waiting..." << endl;
			chrono::milliseconds dura( 500 );
			this_thread::sleep_for( dura );
		}
	}

	cout << __FUNCTION__ << " Thread " << GetCurrentThreadId() << " end" << endl;
	return 0;
}

unsigned HLSPlaylistDownloader::downloadPro( void *pArg )
{

	THREAD_DATA *pInfo = ( THREAD_DATA * )( pArg );
	if ( pInfo == NULL )
	{
		cout << "[Error]: Invalid parameter, thread " << GetCurrentThreadId() << " end" << endl;
		return -1;
	}
	else
	{
		cout << "Thread " << GetCurrentThreadId() << " begin" << endl;
	}

	while ( 1 ) {
		if ( m_bExitFlag ) {
			break;
		}

		if ( pInfo->bStartFlag )
		{
			cout << "Thread " << pInfo->sGroup << " start downloading..." << endl;
			downloadBlockEx( pInfo->sUrl.c_str(), pInfo->iRangeBegin, pInfo->iRangeEnd, pInfo->readBuffer );
			pInfo->bDoComplete = true;
			pInfo->bStartFlag = false;
		}
		else
		{
			//cout << "Thread " << pInfo->sGroup << " is waiting..." << endl;
			chrono::milliseconds dura( 500 );
			this_thread::sleep_for( dura );
		}
	}

	cout << __FUNCTION__ << " Thread " << GetCurrentThreadId() << " end" << endl;
	return 0;
}

size_t HLSPlaylistDownloader::save_header( void *ptr, size_t size, size_t nmemb, void *data )
{
	return ( size_t )( size * nmemb );
}

void HLSPlaylistDownloader::OnCallbackM3u8Change( const vector<string> &hlsList, const vector<string> &rmvList )
{
	//cout << " m3u8 file changed." << endl;
	unique_lock<mutex> lock( m_s_hlsListLock );
	m_s_hlsList = hlsList;
	m_s_rmvList = rmvList;
	DownloadDetermining();
}

bool HLSPlaylistDownloader::RemoveCompelteFile()
{
	lock_guard<mutex> lock( m_mtxCompelteLock );
	if ( m_compelteQueue.empty() )
	{
		return false;
	}

	while ( m_compelteQueue.size() > COMP_NUM )
	{
		string strFileName = m_compelteQueue.front();
		// delete file
		//system("del ");
		m_compelteQueue.pop();
	}

	cout << "Compelte queue size: " << m_compelteQueue.size() << endl;
}

bool HLSPlaylistDownloader::AddDownItems()
{
	lock_guard<mutex> lock( m_mtxDownLock );
	lock_guard<mutex> utlock( m_mtxCompelteLock );

	string strlast = "";
	bool bFirstFlag = false;
	if ( m_downQueue.size() > 0 )
	{
		strlast = m_downQueue.back();
	}
	else if ( m_compelteQueue.size() > 0 )
	{
		strlast = m_compelteQueue.back();
	}
	else
	{
		bFirstFlag = true;
	}

	vector<string>::const_iterator it = m_s_hlsList.begin();
	int iCount = 0;
	while ( iCount < DOWN_NUM && m_downQueue.size() < DOWN_NUM && it != m_s_hlsList.end() )
	{
		if ( bFirstFlag )
		{
			m_downQueue.push( *it );
		}
		else if ( strlast == *it )
		{
			bFirstFlag = true;
		}

		iCount++;
		it++;
	}

	if ( bFirstFlag == false )
	{
		it = m_s_hlsList.begin();
		while ( m_downQueue.size() < DOWN_NUM && it != m_s_hlsList.end() )
		{
			m_downQueue.push( *it++ );
		}
	}

	return true;
}


bool HLSPlaylistDownloader::RemoveDownItems( bool bSuccess, string baseDir )
{
	lock_guard<mutex> lock( m_mtxDownLock );
	if ( m_downQueue.size() <= 0 )
	{
		return false;
	}

	if ( bSuccess )
	{
		lock_guard<mutex> utlock( m_mtxCompelteLock );
		m_compelteQueue.push( m_downQueue.front() );
		while ( m_compelteQueue.size() > COMP_NUM )
		{
			string strFileName = m_compelteQueue.front();
			string strDelCmd = "del \"" + baseDir + '\\' + strFileName + "\"";
			// delete file
			system( strDelCmd.c_str() );
			//system("rm p3.dat");
			m_compelteQueue.pop();
		}
	}
	m_downQueue.pop();

	return true;
}

void HLSPlaylistDownloader::DownloadDetermining()
{
	// remove out of time
	//RemoveCompelteFile();

	// add new download item
	AddDownItems();
}

long HLSPlaylistDownloader::getItemLenth( const char *url )
{
	double len = 0.0;
	clock_t dwLastTime = clock();
	string readBuffer;

	CURL *handle = curl_easy_init();
	curl_easy_setopt( handle, CURLOPT_URL, url );
	curl_easy_setopt( handle, CURLOPT_HEADER, 1 ); //只要求header头
	curl_easy_setopt( handle, CURLOPT_NOBODY, 1 ); //不需求body
	//curl_easy_setopt( handle, CURLOPT_HEADERFUNCTION, save_header );
	curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, curlCallBack );
	curl_easy_setopt( handle, CURLOPT_WRITEDATA, &readBuffer );
	curl_easy_setopt( handle, CURLOPT_SSL_VERIFYPEER, false );
	if ( curl_easy_perform( handle ) == CURLE_OK ) {

		CURLE_OK == curl_easy_getinfo( handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &len );
	}
	else {
		len = -1;

	}
	curl_easy_cleanup( handle );

	clock_t dwCountTime = clock() - dwLastTime;
	cout << __FUNCTION__ << " cost time: " << dwCountTime << " ms" << endl;

	return static_cast<int>( len );


}

long HLSPlaylistDownloader::getItemLenth1( const char *url )
{
	double len = 0.0f;
	clock_t dwLastTime = clock();
	string readBuffer;

	CURL *handle = curl_easy_init();
	curl_easy_setopt( handle, CURLOPT_URL, url );
	curl_easy_setopt( handle, CURLOPT_CUSTOMREQUEST, "GET" );
	curl_easy_setopt( handle, CURLOPT_HEADER, 1 ); //只要求header头
	curl_easy_setopt( handle, CURLOPT_NOBODY, 1 );
	curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, curlCallBack );
	curl_easy_setopt( handle, CURLOPT_WRITEDATA, &readBuffer );
	curl_easy_setopt( handle, CURLOPT_SSL_VERIFYPEER, false );
	if ( curl_easy_perform( handle ) == CURLE_OK )
	{
		curl_easy_getinfo( handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &len );
	}
	else {
		len = -1.0f;
	}
	curl_easy_cleanup( handle );

	clock_t dwCountTime = clock() - dwLastTime;
	cout << __FUNCTION__ << " cost time: " << dwCountTime << " ms" << endl;

	return static_cast<int>( len );
}

long long HLSPlaylistDownloader::getItemTime( const char *url )
{
	CURL *curl;
	string readBuffer;
	CURLcode result = CURL_LAST;
	long long llTime = -1ll;

	curl = curl_easy_init();
	if ( curl ) {
		curl_easy_setopt( curl, CURLOPT_URL, url );
		/* Ask for filetime */
		curl_easy_setopt( curl, CURLOPT_CUSTOMREQUEST, "GET" );
		curl_easy_setopt( curl, CURLOPT_HEADER, 1 ); //只要求header头
		curl_easy_setopt( curl, CURLOPT_NOBODY, 1 );
		curl_easy_setopt( curl, CURLOPT_FILETIME, 1L );
		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, curlCallBack );
		curl_easy_setopt( curl, CURLOPT_WRITEDATA, &readBuffer );
		curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, false );
		result = curl_easy_perform( curl );
		if ( CURLE_OK == result ) {
			if ( readBuffer.length() > 0 )
			{
				int nPos = readBuffer.find( "last-modified: " );
				if ( nPos != string::npos )
				{
					nPos += strlen( "last-modified: " );
					int nPosEnd = readBuffer.find( "\r\n", nPos );
					string strTime = readBuffer.substr( nPos, nPosEnd - nPos );
					llTime = atoll( strTime.c_str() );
				}
			}
		}
		/* always cleanup */
		curl_easy_cleanup( curl );
	}
	return llTime;
}

bool HLSPlaylistDownloader::downloadItem( const char *url ) {
	// All downloaders either downloading binary/text user this method.
	// m_hlsstream file be initiated with file path with proper mode (text/binary) before calling this method.

	bool success = false;
	clock_t dwLastTime = clock();
	if ( m_hlsstream.is_open() ) {
		CURL *curl;
		CURLcode result = CURL_LAST;
		string readBuffer;

		curl = curl_easy_init();
		if ( curl != NULL ) {
			struct curl_slist	*pHeader = nullptr;
			string strKey = "key=20205202020520";
			pHeader = curl_slist_append( pHeader, strKey.c_str() );

			string strFullUrl = url;
#if 1
			strFullUrl.append( "?key=20205202020520" );
#endif
			curl_easy_setopt( curl, CURLOPT_URL, strFullUrl.c_str() );
#ifdef _DEBUG
			//curl_easy_setopt( curl, CURLOPT_PROXY, "127.0.0.1:8888" );
#endif
			curl_easy_setopt( curl, CURLOPT_HTTPHEADER, pHeader );
			curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, curlCallBack );
			curl_easy_setopt( curl, CURLOPT_WRITEDATA, &readBuffer );
			curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, false );
			result = curl_easy_perform( curl );
			curl_easy_cleanup( curl );

			m_hlsstream << readBuffer << endl;
		}

		m_hlsstream.close();

		success = ( result == 0 );
	}

	clock_t dwCountTime = clock() - dwLastTime;
	cout << __FUNCTION__ << " cost time: " << dwCountTime << " ms" << endl;

	return success;
}

bool HLSPlaylistDownloader::downloadItem( const char *url, string &strReadBuffer )
{
	if ( url == NULL )
	{
		return false;
	}

	bool success = false;
	strReadBuffer.clear();
	//clock_t dwLastTime = clock();
	if ( true ) {
		CURL *curl;
		CURLcode result = CURL_LAST;
		string readBuffer;

		curl = curl_easy_init();
		if ( curl != NULL ) {
			curl_easy_setopt( curl, CURLOPT_URL, url );
#ifdef _DEBUG
			//curl_easy_setopt( curl, CURLOPT_PROXY, "127.0.0.1:8888" );
#endif
			curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, curlCallBack );
			curl_easy_setopt( curl, CURLOPT_WRITEDATA, &strReadBuffer );
			curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, false );
			result = curl_easy_perform( curl );
			curl_easy_cleanup( curl );
		}
		success = ( result == 0 );
	}

	//clock_t dwCountTime = clock() - dwLastTime;
	//cout << __FUNCTION__ << " cost time: " << dwCountTime << " ms" << endl;

	return success;
}

bool HLSPlaylistDownloader::downloadBlock( const char *url, int nBegin, int nEnd )
{
	bool success = false;
	clock_t dwLastTime = clock();
	CURL *curl;
	CURLcode result = CURL_LAST;
	string readBuffer;

	if ( !m_hlsstream.is_open() ) {
		return success;
	}

	curl = curl_easy_init();
	if ( curl != NULL ) {
		struct curl_slist	*pHeader = nullptr;
		string strKey = "key=20205202020520";
		pHeader = curl_slist_append( pHeader, strKey.c_str() );

		string strFullUrl = url;
		//strFullUrl.append( "?key=20205202020520" );
		curl_easy_setopt( curl, CURLOPT_URL, strFullUrl.c_str() );
#ifdef _DEBUG
		//curl_easy_setopt( curl, CURLOPT_PROXY, "127.0.0.1:8888" );
#endif
		char szRange[80] {0};
		sprintf_s( szRange, "%d-%d", nBegin, nEnd );
		curl_easy_setopt( curl, CURLOPT_RANGE, szRange );
		curl_easy_setopt( curl, CURLOPT_HTTPHEADER, pHeader );
		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, curlCallBack );
		curl_easy_setopt( curl, CURLOPT_WRITEDATA, &readBuffer );
		curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, false );
		result = curl_easy_perform( curl );
		curl_easy_cleanup( curl );

		m_hlsstream << readBuffer;

	}

	success = ( result == 0 );

	clock_t dwCountTime = clock() - dwLastTime;
	cout << __FUNCTION__ << " cost time: " << dwCountTime << " ms" << endl;

	return success;
}

bool HLSPlaylistDownloader::downloadBlockEx( const char *url, int nBegin, int nEnd, string &sBuffer )
{
	bool success = false;
	clock_t dwLastTime = clock();
	CURL *curl;
	CURLcode result = CURL_LAST;

	sBuffer.clear();

	curl = curl_easy_init();
	if ( curl != NULL ) {
		struct curl_slist	*pHeader = nullptr;
		string strKey = "key=20205202020520";
		pHeader = curl_slist_append( pHeader, strKey.c_str() );

		string strFullUrl = url;
		//strFullUrl.append( "?key=20205202020520" );
		curl_easy_setopt( curl, CURLOPT_URL, strFullUrl.c_str() );
#ifdef _DEBUG
		//curl_easy_setopt( curl, CURLOPT_PROXY, "127.0.0.1:8888" );
#endif
		char szRange[80] { 0 };
		sprintf_s( szRange, "%d-%d", nBegin, nEnd );
		curl_easy_setopt( curl, CURLOPT_RANGE, szRange );
		curl_easy_setopt( curl, CURLOPT_HTTPHEADER, pHeader );
		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, curlCallBack );
		curl_easy_setopt( curl, CURLOPT_WRITEDATA, &sBuffer );
		curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, false );
		result = curl_easy_perform( curl );
		curl_easy_cleanup( curl );
	}

	success = ( result == 0 );

	clock_t dwCountTime = clock() - dwLastTime;
	cout << __FUNCTION__ << " [" << nBegin << "-" << nEnd << "]cost time: " << dwCountTime << " ms" << endl;

	return success;
}

void HLSPlaylistDownloader::downloadPlaylistMedia( string baseUrlPath, string playlistPath, string destination ) {

	// This method downloads a playlist index file, parse it, make a list of filesequences and downloads tall he filesequences.

	m_url = baseUrlPath + playlistPath;

	string localDownloadSequenceName = destination + "/" + playlistPath;

	m_hlsstream = ofstream( localDownloadSequenceName, ios::out | ios::app | ios::binary );

	downloadItem( m_url.c_str() );

	cout << "." << flush;

	cout << " " << playlistPath << " downloaded" << endl;
}

bool HLSPlaylistDownloader::downloadPlaylistEx( string sUrl, string sName )
{
	long long lltm = getItemTime( sUrl.c_str() );

	m_hlsstream = ofstream( sName, ios::out | ios::app );
	return downloadItem( sUrl.c_str() );
}

void HLSPlaylistDownloader::RunDownloader()
{
	m_bExitFlag = false;

	uintptr_t hHandle[GROUP_NUM] {NULL};
	THREAD_DATA thInfo[GROUP_NUM];

	unsigned int threadID;
	for ( int i = 0; i < GROUP_NUM; ++i )
	{
		//m_IsDownloading[i] = false;
		thInfo[i].bStartFlag = false;
		thInfo[i].sGroup = "Single Thread";
		hHandle[i] = _beginthreadex( NULL, 0, &HLSPlaylistDownloader::downloadSingle, &thInfo[i], 0, &threadID ); ;
	}

	while ( true )
	{
		if ( m_bExitFlag )
		{
			return;
		}

		string strDownItem = "";
		if ( GetDownloadFristObject( strDownItem ) )
		{
			bool isBeasuy = true;
			for ( int i = 0; i < GROUP_NUM; ++i )
			{
				if ( thInfo[i].bStartFlag == false )
				{
					//cout << "begind down " << m_baseUrlPath + strDownItem << endl;
					isBeasuy = false;
					thInfo[i].Reset( m_baseUrlPath + strDownItem, m_outputDir + '/' + strDownItem, 0, 0 );
					RemoveDownItems( true, m_outputDir );
					//cout << "End down " << m_outputDir + '/' + strDownItem << endl;
					break;
				}
			}
		}

		this_thread::sleep_for( chrono::milliseconds( 500 ) );
	}

	m_bExitFlag = false;
	WaitForMultipleObjectsEx( sizeof( hHandle ), ( HANDLE * )hHandle, FALSE, INFINITE, FALSE );

	for ( int i = 0; i < GROUP_NUM; ++i )
	{
		CloseHandle( ( HANDLE )( hHandle[i] ) );
		hHandle[i] = NULL;
	}

	return;
}

void HLSPlaylistDownloader::downloadMedia( string sUrl, string sName )
{
	m_hlsstream = ofstream( sName, ios::out | ios::app | ios::binary );
	downloadItem( sUrl.c_str() );
	cout << "." << flush;
	cout << " " << sName << " downloaded" << endl;
}

void HLSPlaylistDownloader::downloadMediaOne( string sUrl, string sName )
{
	long dwLen = getItemLenth( sUrl.c_str() );
	m_hlsstream = ofstream( sName, ios::out | ios::app | ios::binary );
	int iPart = static_cast<int>( dwLen / 3 );
	int iLeft = dwLen % iPart;
	for ( int i = 0; i < 3; ++i )
	{
		int iBegin, iEnd;
		iBegin = i * iPart;
		iEnd = ( i + 1 ) * iPart;
		if ( i == 3 )
		{
			iEnd = ( i + 1 ) * iPart + iLeft;
		}
		downloadBlock( sUrl.c_str(), iBegin, iEnd );
	}
	m_hlsstream.close();
	cout << "." << flush;
	cout << " " << sName << " downloaded" << endl;
}

void HLSPlaylistDownloader::downloadMediaTwo( string sUrl, string sName )
{
	long lLen = getItemLenth1( sUrl.c_str() );
	m_hlsstream = ofstream( sName, ios::out | ios::app | ios::binary );

	int iPart = static_cast<int>( lLen / MAX_THREAD_NUM );
	int iLeft = lLen % iPart;
	for ( int i = 0; i < MAX_THREAD_NUM; ++i )
	{
		int iBegin, iEnd;
		iBegin = i * iPart;
		iEnd = ( i + 1 ) * iPart;
		if ( i == 3 )
		{
			iEnd = ( i + 1 ) * iPart + iLeft;
		}

		if ( m_DownInfo[i].bStartFlag == false )
		{
			m_DownInfo[i].Reset( "single", sUrl.c_str(), iBegin, iEnd );
		}
		else
		{
			_ASSERT( false );
		}
	}

	bool bAllComplete = true;
	while ( 1 )
	{
		bAllComplete = true;
		for ( int i = 0; i < MAX_THREAD_NUM; ++i )
		{
			if ( m_DownInfo[i].bStartFlag || m_DownInfo[i].bDoComplete == false )
			{
				bAllComplete = false;
			}
		}
		if ( bAllComplete )
		{
			break;
		}
		else
		{
			//Sleep( 100 );
			this_thread::sleep_for( chrono::milliseconds( 100 ) );
		}
	}

	for ( int i = 0; i < MAX_THREAD_NUM; ++i )
	{
		m_hlsstream << m_DownInfo[i].readBuffer;
	}

	m_hlsstream.close();
	cout << "." << flush;
	cout << " " << sName << " downloaded" << endl;
}

void HLSPlaylistDownloader::downloadMediaThree( string sUrl, string sName )
{
	double dwLen = getItemLenth( sUrl.c_str() );
	m_hlsstream = ofstream( sName, ios::out | ios::app | ios::binary );
	downloadBlock( sUrl.c_str(), 0, 1000 );
	downloadBlock( sUrl.c_str(), 1001, int( dwLen ) );
	m_hlsstream.close();
	cout << "." << flush;
	cout << " " << sName << " downloaded" << endl;
}

void HLSPlaylistDownloader::downloadIndividualPlaylist( string baseUrlPath, string playlistPath, string destination ) {

	// This method downloads a playlist index file, parse it, make a list of filesequences and downloads tall he filesequences.

	m_url = baseUrlPath + playlistPath;

	// Create a directory based on the index item path. Download the index file for this play list
	// And then parse the index file, make a list of all sequence files to be downloaded
	// Download all the sequence files and store in this directory.

	vector <string> nameItems = HLSPlaylistUtilities::tokenize( playlistPath, '/' );

	if ( !( nameItems.size() > 0 ) ) {
		return;
	}

	string playlistName = nameItems[0];
	string playlistLocalPath = destination + "/" + playlistName;

	if ( HLSPlaylistUtilities::createDirectory( playlistLocalPath ) != 0 ) {
		cout << "ERROR : Not able to create directory : " << endl << playlistLocalPath << endl;
		return;
	}

	string localDownloadFileName = destination + "/" + playlistPath;
	// Index file is a text file. Open input stream in text mode
	m_hlsstream = ofstream( localDownloadFileName, ios::out | ios::app );

	if ( downloadItem( m_url.c_str() ) ) {

		// Index file successfully downloaded

		cout << " Downloading " << playlistPath << " ";

		// Parse the index file and make a list of sequences to be downloaded
		vector<string> playlistStreams = HLSPlaylistUtilities::buildList( localDownloadFileName );

		if ( playlistStreams.size() > 0 ) {
			// download streams - fileSequence1.*
			for ( vector<string>::iterator sequence = playlistStreams.begin(); sequence != playlistStreams.end(); ++sequence ) {
				string localDownloadSequenceName = playlistLocalPath + "/" + *sequence;
				m_url = baseUrlPath + playlistName + "/" + *sequence;
				// sequence files are binary hence open input file in binary mode
				m_hlsstream = ofstream( localDownloadSequenceName, ios::out | ios::app | ios::binary );
				// thread th(&HLSPlaylistDownloader::downloadItem, this, m_url.c_str());
				//th.join();
				downloadItem( m_url.c_str() );
				cout << "." << flush;
			}
			cout << " " << playlistStreams.size() << " sequences downloaded" << endl;
		}
	}
}
