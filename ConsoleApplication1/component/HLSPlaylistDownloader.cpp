//  HLSPlaylistDownloader.cpp
//  Created by Krishna Gudipati on 8/14/18.

#include "HLSPlaylistDownloader.hpp"
#include "HLSPlaylistUtilities.hpp"
#include <curl/curl.h>
#include <iostream>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "libcurl64.lib")

HLSPlaylistDownloader::HLSPlaylistDownloader() {
}

HLSPlaylistDownloader::~HLSPlaylistDownloader() {
}

void HLSPlaylistDownloader::setDownloadInfo( string urlPath, string outfile ) {
	m_url = urlPath;
	m_outputFile = outfile;
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

bool HLSPlaylistDownloader::downloadItem( const char *url ) {
	// All downloaders either downloading binary/text user this method.
	// m_hlsstream file be initiated with file path with proper mode (text/binary) before calling this method.

	bool success = false;

	if ( m_hlsstream.is_open() ) {
		CURL *curl;
		CURLcode result = CURL_LAST;
		string readBuffer;

		curl = curl_easy_init();
		if ( curl != NULL ) {
			curl_easy_setopt( curl, CURLOPT_URL, url );
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

	return success;
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
