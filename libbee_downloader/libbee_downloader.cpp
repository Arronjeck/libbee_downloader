// libbee_downloader.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "curl.h"
#include <iostream>
#include <string>
#include <regex>

using namespace std;

static char m3u8[1024];
static char g_useragent[1024];
static char g_proxy[100];
static char g_maxlen[50];
static double g_maxduration = 0.0;
//static size_t totalsize = 0;
static unsigned long long int totalsize = 0;
static char strtotalsize[100];

static string baseurl;
static int isDEBUG = 0;

typedef struct targ {
	char url[1024];
	char fn[1024];
	char idxmsg[512];
} targ;

size_t writeFunction2( void *ptr, size_t size, size_t nmemb, string *data ) {
	data->append( ( char * )ptr, size * nmemb );
	return size * nmemb;
}

static void *downts( void *arg )
{
	targ *ta = ( targ * )arg;
	string fullurl = ta->url;;
	string fn = ta->fn;
	string idxmsg = ta->idxmsg;
	string statusmsg = " [OK]";
	char recvsize[20];
	//sprintf(recvsize,"          ");
	sprintf_s( recvsize, 20, "----------" );
	//cout << fn << endl;
	auto curl = curl_easy_init();
	if ( curl ) {
		curl_easy_setopt( curl, CURLOPT_URL, fullurl.c_str() );
		curl_easy_setopt( curl, CURLOPT_NOPROGRESS, 1L );
		//curl_easy_setopt(curl, CURLOPT_USERPWD, "user:pass");
		curl_easy_setopt( curl, CURLOPT_USERAGENT, g_useragent );
		curl_easy_setopt( curl, CURLOPT_MAXREDIRS, 50L );
		curl_easy_setopt( curl, CURLOPT_TCP_KEEPALIVE, 1L );
		curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L );
		curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L );
		curl_easy_setopt( curl, CURLOPT_TIMEOUT_MS, 10000L );
		if ( strlen( g_proxy ) > 0 ) {
			curl_easy_setopt( curl, CURLOPT_PROXY, g_proxy );
		}
		string response_string;
		string header_string;
		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, writeFunction2 );
		curl_easy_setopt( curl, CURLOPT_WRITEDATA, &response_string );
		curl_easy_setopt( curl, CURLOPT_HEADERDATA, &header_string );

		char *url;
		long response_code;
		double elapsed;
		curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &response_code );
		curl_easy_getinfo( curl, CURLINFO_TOTAL_TIME, &elapsed );
		curl_easy_getinfo( curl, CURLINFO_EFFECTIVE_URL, &url );

		CURLcode res = curl_easy_perform( curl );
		if ( res != CURLE_OK )
		{
			if ( isDEBUG ) {
				fprintf( stderr, "thread curl_easy_perform() failed: %s\n", curl_easy_strerror( res ) );
			}
			if ( isDEBUG ) {
				cout << "pthread remove " << fn << endl;
			}
			//cout << "*** failed [" << fn << "] ***" <<endl;
			statusmsg = " [Failed]";
			remove( fn.c_str() );
		}
		else
		{
			if ( isDEBUG ) {
				cout << "thread response_code " << response_code << endl;
			}
			if ( isDEBUG ) {
				cout << "thread response_string size " << response_string.size() << endl;
			}
			//cout << "elapsed       " << elapsed << endl;
			//cout << "effective url " << url << endl;
			//cout << "response_string \n" << response_string << endl;
			//cout << "header_string \n" << header_string << endl;
			unsigned long long int rsize = response_string.size();
			if ( rsize > 0 )
			{
				//cout << fn << endl;
				FILE *tsh = NULL;
				fopen_s( &tsh, fn.c_str(), "wb" );
				if ( tsh )
				{
					fwrite( response_string.c_str(), 1, rsize, tsh );
					fclose( tsh );
				}
				totalsize += rsize;
				if ( totalsize < 1024 ) {
					sprintf_s( strtotalsize, 100, "%10ld   ", totalsize );
				}
				else if ( totalsize < 1024 * 1024 ) {
					sprintf_s( strtotalsize, 100, "%10.03f KB", ( double )totalsize / 1024 );
				}
				else if ( totalsize < 1024 * 1024 * 1024 ) {
					sprintf_s( strtotalsize, 100, "%10.03f MB", ( double )totalsize / 1024 / 1024 );
				}
				else {
					sprintf_s( strtotalsize, 100, "%10.03f GB", ( double )totalsize / 1024 / 1024 / 1024 );
				}

				if ( rsize < 1024 ) {
					sprintf_s( recvsize, 20, "%7ld   ", rsize );
				}
				else if ( rsize < 1024 * 1024 ) {
					sprintf_s( recvsize, 20, "%7.02f KB", ( double )rsize / 1024 );
				}
				else if ( rsize < 1024 * 1024 * 1024 ) {
					sprintf_s( recvsize, 20, "%7.02f MB", ( double )rsize / 1024 / 1024 );
				}
				else {
					sprintf_s( recvsize, 20, "%7.02f MB", ( double )rsize / 1024 / 1024 / 1024 );
				}

			}
			else
			{
				//cout << "*** response error [" << fn << "] ***" <<endl;
				statusmsg = " [Response error]";
				remove( fn.c_str() );
			}
		}
	}
	cout << " " << idxmsg << " " << recvsize << strtotalsize << " " << fn << statusmsg << endl;
	curl_easy_cleanup( curl );
	curl = NULL;
	if ( isDEBUG ) {
		cout << "pthread exit" << endl;
	}

	return NULL;
}

string getbaseurl( string url )
{
	string text = url;
	string pattern = "((.*)/)";
	regex express( pattern );
	regex_iterator<string::const_iterator> begin( text.cbegin(), text.cend(), express );
	for ( auto iter = begin; iter != sregex_iterator(); iter++ )
	{
		return iter->str();
	}
}


int main()
{
	FILE *m3u8h = NULL;
	fopen_s( &m3u8h, m3u8, "wb" );
	if ( m3u8h ) {
		fclose( m3u8h );
	}
	//curl_global_init( CURL_GLOBAL_DEFAULT );
	auto curl = curl_easy_init();

	static char urlm3u8[1024] = "http://112.84.104.209:8000/hls/1001/index.m3u8?key=20205202020520";
	baseurl = getbaseurl( urlm3u8 );
	if ( isDEBUG ) {
		cout << baseurl << endl;
	}

	string fullurl;
	cout << "Hello World!\n";
	cout << fullurl << endl;


	curl_easy_cleanup( curl );
	curl = NULL;
	//curl_global_cleanup();

}