//  main.cpp
//  Created by Krishna Gudipati on 8/14/18.

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <ctime>
#include <queue>

#include "component/HLSPlaylistInfo.hpp"
#include "component/HLSPlaylistDownloader.hpp"
#include "component/HLSPlaylistUtilities.hpp"
#include <crtdbg.h>


using namespace std;

//const string playlistRootUrl = "https://devstreaming-cdn.apple.com/videos/streaming/examples/bipbop_4x3/bipbop_4x3_variant.m3u8";
const string playlistRootUrl = "http://112.84.104.209:8000/hls/1001/index.m3u8";

int auto_download()
{
	HLSPlaylistInfo *playlistInfo = new HLSPlaylistInfo();
	string playListRootDirectory = "E:/linux/curl_down/Ymp";

	// Prompt user to feed play list URL and directory path where to store the play list locally
	//if ( !HLSPlaylistUtilities::fetchAndvalidateUserInput( &playListRootDirectory, playlistInfo ) ) {
	//	// User choose to exit with out providing inputs
	//	return 0;
	//}

	if ( !playlistInfo->extractPlaylistInfo( playlistRootUrl ) ) {
		cout << "ERROR : " << playlistRootUrl << " is an invalid/corrupted url. Provide new url name" << endl;
		return 0;
	}
	playListRootDirectory = playListRootDirectory + "/" + playlistInfo->getPlaylistRootName();
	HLSPlaylistUtilities::deleteDirectory( playListRootDirectory );

	if ( HLSPlaylistUtilities::createDirectory( playListRootDirectory ) != 0 ) {
		cout << "ERROR : Not able to create directory : " << playListRootDirectory << ". Provide new directory name" << endl;
		return 0;
	}

	// The directory where actually downloading starts.
	string localDownloadFileName = playListRootDirectory + "/" + playlistInfo->getMainPlaylistName();

	HLSPlaylistDownloader *hlsDownloader = new HLSPlaylistDownloader();

	// First file is the root play list (index) - eg. bipbop_4x3_variant.m3u8
	// Supply the downloader the complete URL and the local directory where index file should be saved
	hlsDownloader->setDownloadInfo( playlistInfo->getCompleteUrlPath(), localDownloadFileName );

	vector <string> playListItems;

	if ( hlsDownloader->downloadPlaylist() ) {
		// The root play list is successfully downloaded. Now parse that file and make a list of sub play lists
		playListItems = HLSPlaylistUtilities::buildList( localDownloadFileName );
	}

	if ( playListItems.size() > 0 ) {
		// Browse thru each of those play lists and download the index files
		for ( vector<string>::iterator item = playListItems.begin(); item != playListItems.end(); ++item ) {
			// Download inde file and as well as the filesequences mentioned in the index file
#if 1
			hlsDownloader->downloadPlaylistMedia( playlistInfo->getBaseUrlPath(), *item, playListRootDirectory );
#else
			hlsDownloader->downloadIndividualPlaylist( playlistInfo->getBaseUrlPath(), *item, playListRootDirectory );
#endif
		}
	}

	cout << endl << "                -----***** Playlist Download COMPLETED *****-----               " << endl << flush;

	playlistInfo = NULL;
	hlsDownloader = NULL;

	return 0;
}

int sigle_download()
{
	string mmmTsUrl = "https://devstreaming-cdn.apple.com/videos/streaming/examples/bipbop_4x3/gear1/fileSequence0.ts";
	string mmmm = "https://devstreaming-cdn.apple.com/videos/streaming/examples/bipbop_4x3/bipbop_4x3_variant.m3u8";

	string m3u8Url = "http://112.84.104.209:8000/hls/1001/index.m3u8?key=20205202020520"; //
	string singleTsUrl = "http://112.84.104.209:8000/hls/test1/68979.ts";
	string fileName = "fileSequence0.ts";
	string localDownloadFileName = "E:/linux/curl_down/Ymp/100";
	HLSPlaylistUtilities::deleteDirectory( localDownloadFileName );
	HLSPlaylistUtilities::createDirectory( localDownloadFileName );
	HLSPlaylistDownloader *hlsDownloader = new HLSPlaylistDownloader();
	hlsDownloader->setDownloadInfo( singleTsUrl, localDownloadFileName );

	clock_t dwLastTime, dwCountTime;

	dwLastTime = clock();
	hlsDownloader->downloadPlaylistEx( m3u8Url, localDownloadFileName + "/0.m3u8" );
	dwCountTime = clock() - dwLastTime;;
	cout << "0. download m3u8 time: " << dwCountTime << " ms" << endl;

	dwLastTime = clock();
	hlsDownloader->downloadMedia( singleTsUrl, localDownloadFileName + "/1.ts" );
	dwCountTime = clock() - dwLastTime;;
	cout << "1. single download time: " << dwCountTime << " ms" << endl;

	dwLastTime = clock();
	hlsDownloader->downloadMediaOne( singleTsUrl, localDownloadFileName + "/2.ts" );
	dwCountTime = clock() - dwLastTime;;
	cout << "2. Multi-block download time: " << dwCountTime << " ms" << endl;

	dwLastTime = clock();
	hlsDownloader->downloadMediaTwo( singleTsUrl, localDownloadFileName + "/3.ts" );
	dwCountTime = clock() - dwLastTime;;
	cout << "3. Multi-Thread-block download time: " << dwCountTime << " ms" << endl;

	delete hlsDownloader;

	return 0;
}


int m3u8_monitor()
{
	string m3u8Url = "http://112.84.104.209:8000/hls/1001/index.m3u8?key=20205202020520";
	string localDownloadFileDir = "E:/linux/curl_down/Ymp/100";
	HLSPlaylistDownloader hlsDownloader;
	hlsDownloader.StartMonitor( m3u8Url, localDownloadFileDir );
	hlsDownloader.setDownloadInfo( "http://112.84.104.209:8000/hls/1001/", localDownloadFileDir );
	hlsDownloader.StartDownloader();

	return 0;
}

int main( void )
{
	//sigle_download();
	m3u8_monitor();
	system( "pause" );

	return 0;
}
