// ConsoleApplication1.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "curl/curl.h"
#include <string>
#include <regex>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "libcurl64.lib")

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
static int isDEBUG = 1;

static double duration = 0.0;
static char strduration[512];
static unsigned int dlidx = 1;
static int isMAX = 0;

static unsigned int dlcount = 0;
static unsigned int firstdl = 0;
static unsigned int dladdcount = 0;
static unsigned int checknodowncount = 0;

typedef struct targ {
	char url[1024];
	char fn[1024];
	char idxmsg[512];
} targ;

size_t writeFunction2( void *ptr, size_t size, size_t nmemb, string *data ) {
	data->append( ( char * )ptr, size * nmemb );
	return size * nmemb;
}

void putmsg( const char *msg )
{
	if ( !msg ) {
		return;
	}
	FILE *m3u8h = NULL;
	fopen_s( &m3u8h, m3u8, "ab+" );
	if ( m3u8h ) {
		fwrite( msg, 1, strlen( msg ), m3u8h );
		fwrite( "\r\n", 1, 2, m3u8h );
		fclose( m3u8h );
	}
}

string getindex( string text )
{
	string ret = "";
	string pattern = "([0-9]{1,15})";
	regex express( pattern );
	regex_iterator<string::const_iterator> begin( text.cbegin(), text.cend(), express );
	for ( auto iter = begin; iter != sregex_iterator(); iter++ )
	{
		ret = iter->str();
	}
	if ( ret == "" || ret.size() <= 0 ) {
		return text;
	}
	return ret;
}


double getlen( string text )
{
	double rd = 0.0;
	string ret = "";
	string pattern = "([0-9.]{1,8})";
	regex express( pattern );
	regex_iterator<string::const_iterator> begin( text.cbegin(), text.cend(), express );
	for ( auto iter = begin; iter != sregex_iterator(); iter++ )
	{
		ret = iter->str();
	}
	if ( ret == "" || ret.size() <= 0 ) {
		return 0.0;
	}
	rd = atof( ret.c_str() );
	duration = duration + rd;

	int dhh = ( unsigned int )( duration / 3600 );
	int dmm = ( unsigned int )( duration ) % 3600 / 60;
	int dss = ( unsigned int )( duration ) % 3600 % 60;
	int dms = ( unsigned int )( duration * 1000 ) % 1000;
	sprintf_s( strduration, 512, "%4d %5.03f %02d:%02d:%02d.%03d",
			   dlidx, rd, dhh, dmm, dss, dms );
	dlidx++;
	if ( g_maxduration != 0.0 && duration >= g_maxduration ) {
		isMAX = 1;
	}
	return rd;
}

void string_replace( string &strBig, const string &strsrc, const string &strdst )
{
	string::size_type pos = 0;
	string::size_type srclen = strsrc.size();
	string::size_type dstlen = strdst.size();

	while ( ( pos = strBig.find( strsrc, pos ) ) != string::npos )
	{
		strBig.replace( pos, srclen, strdst );
		pos += dstlen;
	}
}

void gettsurl( string str )
{
	string fullurl;
	string text = str;
	if ( dlcount == 0 )
	{
		string pattern = "(#EXT-X(.*)|#EXTM3U)";
		regex express( pattern );
		regex_iterator<string::const_iterator> begin( text.cbegin(), text.cend(), express );
		for ( auto iter = begin; iter != sregex_iterator(); iter++ )
		{
			cout << iter->str() << endl;
			putmsg( iter->str().c_str() );
		}
	}
	//string pattern = "((.*).ts)";
	string pattern = "(#EXTINF:(.*)|(.*).ts)";
	regex express( pattern );
	regex_iterator<string::const_iterator> begin( text.cbegin(), text.cend(), express );
	string extinfo;

	for ( auto iter = begin; iter != sregex_iterator(); iter++ )
	{
		fullurl = baseurl + iter->str();
		//cout << fullurl << endl;
		//cout << iter->str() << endl;
		//downts(fullurl, iter->str());
		if ( iter->str().substr( 0, 1 ) == "#" )
		{
			extinfo = iter->str();
		}
		else
		{
			string newfn = getindex( iter->str() );
			string_replace( newfn, "/", "-" );

			if ( newfn.size() <= 4 )
			{
				char tmp[50];
				sprintf_s( tmp, 50, "%05d", atoi( newfn.c_str() ) );
				newfn = tmp;
			}
			newfn = newfn + ".ts";

			if ( /*access( newfn.c_str(), 0 )*/true ) {
				getlen( extinfo );
				//cout << " " << strduration << " " << strtotalsize << " " << newfn << endl;
				//cout << newfn << endl;
				putmsg( extinfo.c_str() );
				putmsg( newfn.c_str() );
				FILE *tsh = NULL;
				fopen_s( &tsh, newfn.c_str(), "wb" );
				if ( tsh ) {
					fclose( tsh );
				}
				//pthread_t thread;
				targ ta;
				memset( &ta, 0x00, sizeof( ta ) );
				strcpy_s( ta.url, 1024, fullurl.c_str() );
				strcpy_s( ta.fn, 1024, newfn.c_str() );
				strcpy_s( ta.idxmsg, 512, strduration );
				//pthread_create( &thread, NULL, downts, &ta );
				//pthread_mutex_lock( &lock );
				//listpthreads.push_back( thread );
				//pthread_mutex_unlock( &lock );
				dladdcount++;
			}
		}
	}
	dlcount++;
}

static void *downts( void *arg )
{
	targ *ta = ( targ * )arg;
	string fullurl = ta->url;;
	string fn = ta->fn;
	string idxmsg = ta->idxmsg;
	string statusmsg = " [OK]";
	char recvsize[20];
	//sprintf_s(recvsize,"          ");
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
		//cout << iter->str() << endl;
		return iter->str();
	}
}

size_t writeFunction( void *ptr, size_t size, size_t nmemb, string *data ) {
	data->append( ( char * )ptr, size * nmemb );
	return size * nmemb;
}

int main()
{
	//char urlm3u8[1024] = "http://112.84.104.209:8000/hls/1001/index.m3u8?key=20205202020520";
	char urlm3u8[1024] = "https://api.github.com/repos/whoshuu/cpr/contributors?anon=true&key=value";
	curl_global_init( CURL_GLOBAL_DEFAULT );
	baseurl = getbaseurl( urlm3u8 );
	if ( isDEBUG ) {
		cout << baseurl << endl;
	}
	CURLcode res;
	auto curl = curl_easy_init();
	if ( curl ) {
		while ( 1 ) {
			dladdcount = 0;
			//curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/repos/whoshuu/cpr/contributors?anon=true&key=value");
			curl_easy_setopt( curl, CURLOPT_URL, urlm3u8 );
			curl_easy_setopt( curl, CURLOPT_NOPROGRESS, 1L );
			//curl_easy_setopt(curl, CURLOPT_USERPWD, "user:pass");
			curl_easy_setopt( curl, CURLOPT_USERAGENT, g_useragent );
			curl_easy_setopt( curl, CURLOPT_MAXREDIRS, 50L );
			curl_easy_setopt( curl, CURLOPT_TCP_KEEPALIVE, 1L );
			curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0L );
			curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, 0L );
			curl_easy_setopt( curl, CURLOPT_TIMEOUT_MS, 5000L );
			if ( strlen( g_proxy ) > 0 ) {
				curl_easy_setopt( curl, CURLOPT_PROXY, g_proxy );
			}

			string response_string;
			string header_string;
			curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, writeFunction );
			curl_easy_setopt( curl, CURLOPT_WRITEDATA, &response_string );
			curl_easy_setopt( curl, CURLOPT_HEADERDATA, &header_string );

			char *url;
			long response_code;
			double elapsed;
			curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &response_code );
			curl_easy_getinfo( curl, CURLINFO_TOTAL_TIME, &elapsed );
			curl_easy_getinfo( curl, CURLINFO_EFFECTIVE_URL, &url );

			res = curl_easy_perform( curl );
			if ( res != CURLE_OK )
			{
				if ( isDEBUG ) {
					fprintf( stderr, "main curl_easy_perform() failed: %s\n", curl_easy_strerror( res ) );
				}
			}
			else
			{
				if ( isDEBUG ) {
					cout << "main response_code " << response_code << endl;
				}
				cout << "elapsed       " << elapsed << endl;
				cout << "effective url " << url << endl;
				cout << "response_string \n" << response_string << endl;
				cout << "header_string \n" << header_string << endl;

				if ( response_code == 0 ) {
					continue;
				}
				if ( response_code == 200 )
				{
					// skip first ts
					if ( firstdl != 0 ) {
						gettsurl( response_string );
					}
					firstdl++;
				}
				else
				{
					//waitThreads();
					Sleep( 1000 );
					if ( isDEBUG ) {
						cout << "#EXT-X-ENDLIST\r\n" << endl;
					}
					putmsg( "#EXT-X-ENDLIST" );
					if ( isDEBUG ) {
						cout << "main response_code " << response_code << "\n" << endl;
					}
					break;
				};
			}
			char exitflag = '\0';
			/*if ( _kbhit() )
			{
				exitflag = _getch();
				if ( exitflag == 'q' || exitflag == 'Q' )
				{
					waitThreads();
					if ( isDEBUG ) {
						cout << "#EXT-X-ENDLIST\r\n" << endl;
					}
					putmsg( "#EXT-X-ENDLIST" );
					break;
				}
			}*/

			if ( dladdcount > 0 ) {
				checknodowncount = 0;
			}
			else {
				checknodowncount++;
			}
			//printf("dladdcount %d checknodowncount %ld\n", dladdcount, checknodowncount);

			// No files were downloaded in 60 seconds
			if ( checknodowncount >= 60 || isMAX )
			{
				//waitThreads();
				if ( isDEBUG ) {
					cout << "#EXT-X-ENDLIST\r\n" << endl;
				}
				putmsg( "#EXT-X-ENDLIST" );
				break;
			}
			Sleep( 1000 * 1000 );
			//usleep( 1000 * 1000 );
			//sleep(1);
		}
	}
	curl_easy_cleanup( curl );
	curl = NULL;
	curl_global_cleanup();
	if ( isDEBUG ) {
		cout << "main exit" << endl;
	}
	return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧:
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
