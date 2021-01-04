//  main.cpp
//  Created by Krishna Gudipati on 8/14/18.

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <ctime>
#include <queue>
#include <boost/filesystem.hpp>

#include "component/HLSPlaylistInfo.hpp"
#include "component/HLSPlaylistDownloader.hpp"
#include "component/HLSPlaylistUtilities.hpp"
#include <crtdbg.h>

#include "webserver/server_http.hpp"

using namespace std;
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

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

time_t get_last_mtime( string strPathFile )
{
	struct stat result;
	if ( stat( strPathFile.c_str(), &result ) == 0 )
	{
		auto mod_time = result.st_mtime;
	}

	return result.st_mtime;
}

int m3u8_monitor()
{
	string m3u8Url = "http://112.84.104.209:8000/hls/1001/index.m3u8?key=20205202020520";
	string localDownloadFileDir = "./web";

	//
	HttpServer server;
	server.config.port = 8080;
	server.default_resource["GET"] = []( shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request )
	{
		try {

			cout << "webserver: request for " << request->path << endl;

			auto web_root_path = boost::filesystem::canonical( "web" );
			//string strRoot = web_root_path->c_str();
			string strRoot = web_root_path.string();
			auto path = strRoot + request->path;
			//auto path = boost::filesystem::canonical( web_root_path / request->path );
			// Check if path is within web_root_path
// 			if ( distance( web_root_path.begin(), web_root_path.end() ) > distance( path.begin(), path.end() ) ||
// 					!equal( web_root_path.begin(), web_root_path.end(), path.begin() ) ) {
// 				throw invalid_argument( "path must be within root path" );
// 			}
// 			if ( boost::filesystem::is_directory( path ) ) {
// 				path /= "index.html";
// 			}

			// Uncomment the following line to enable Cache-Control
			// header.emplace("Cache-Control", "max-age=86400");

			SimpleWeb::CaseInsensitiveMultimap header;
			if ( request->path.find( "index.m3u8" ) != string::npos )
			{
				string strFile;
				long long llTime = HLSPlaylistDownloader::GetM3u8File( strFile );
				auto length = strFile.size();
				header.emplace( "accept-ranges", "bytes" );
				unsigned int endIndex = length;
				header.emplace( "content-range", "bytes 0-" + to_string( endIndex - 1 ) + "/" + to_string( length ) );
				header.emplace( "content-type", "application/octet-stream" );
				header.emplace( "connection", "keep-alive" );
				header.emplace( "content-length", to_string( length ) );
				header.emplace( "last-modified", to_string( llTime ) );
				response->write( header );
				response->write( strFile.c_str(), length );
				cout << strFile << endl;

			}
			else
			{
				auto ifs = make_shared<ifstream>();
				ifs->open( path.c_str(), ifstream::in | ios::binary | ios::ate );

				if ( *ifs ) {
					auto length = ifs->tellg();
					ifs->seekg( 0, ios::beg );
					header.emplace( "accept-ranges", "bytes" );
					unsigned int endIndex = length;
					header.emplace( "content-range", "bytes 0-" + to_string( endIndex - 1 ) + "/" + to_string( length ) );
					header.emplace( "content-type", "application/octet-stream" );
					header.emplace( "connection", "keep-alive" );
					header.emplace( "content-length", to_string( length ) );
					long long mt = get_last_mtime( path.c_str() );
					header.emplace( "last-modified", to_string( mt ) );
					response->write( header );

					// Trick to define a recursive function within this scope (for example purposes)
					class FileServer {
					public:
						static void read_and_send( const shared_ptr<HttpServer::Response> &response, const shared_ptr<ifstream> &ifs ) {
							// Read and send 128 KB at a time
							static vector<char> buffer( 131072 ); // Safe when server is running on one thread
							streamsize read_length;
							if ( ( read_length = ifs->read( &buffer[0], static_cast<streamsize>( buffer.size() ) ).gcount() ) > 0 ) {
								response->write( &buffer[0], read_length );
								if ( read_length == static_cast<streamsize>( buffer.size() ) ) {
									response->send( [response, ifs]( const SimpleWeb::error_code & ec ) {
										if ( !ec ) {
											read_and_send( response, ifs );
										}
										else {
											cerr << "Connection interrupted" << endl;
										}
									} );
								}
							}
						}
					};
					FileServer::read_and_send( response, ifs );
				}
				else {
					_ASSERT( FALSE );
					throw invalid_argument( "could not read file" );
				}
			}
		}
		catch ( const exception &e ) {
			response->write( SimpleWeb::StatusCode::client_error_bad_request, "Could not open path " + request->path + ": " + e.what() );
		}
	};

	server.on_error = []( shared_ptr<HttpServer::Request> /*request*/, const SimpleWeb::error_code & /*ec*/ ) {
		// Handle errors here
		// Note that connection timeouts will also call this handle with ec set to SimpleWeb::errc::operation_canceled
	};

	thread server_thread( [&server]() {
		// Start server
		server.start();
	} );

	//
	HLSPlaylistDownloader hlsDownloader;
	hlsDownloader.StartMonitor( m3u8Url, localDownloadFileDir );
	hlsDownloader.setDownloadInfoEx( "http://112.84.104.209:8000/hls/1001/", localDownloadFileDir );
	hlsDownloader.RunDownloader();



	return 0;
}

int main( void )
{
	//sigle_download();
	cout << "main begin" << endl;
	m3u8_monitor();
	cout << "main end" << endl;
	system( "pause" );

	return 0;
}
