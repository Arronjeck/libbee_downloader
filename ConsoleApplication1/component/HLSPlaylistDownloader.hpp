//  HLSPlaylistDownloader.hpp
//  Created by Krishna Gudipati on 8/14/18.

#ifndef HLSPlaylistDownloader_hpp
#define HLSPlaylistDownloader_hpp

#include <sstream>
#include <fstream>

using namespace std;

class HLSPlaylistDownloader {
    
    public :
    HLSPlaylistDownloader();
    ~HLSPlaylistDownloader();
    
    private:
        string m_url;         // Original url supplied by the user
        string m_outputFile;
        ofstream m_hlsstream; // Every file that is being downloaded by this class this ofstream is used to save the downloaded file
    
        // Curl reladed methods
        bool downloadItem(const char* url); // Every file is downloaded using this method
        static size_t curlCallBack(void *curlData, size_t size, size_t receievedSize, void *writeToFileBuffer);
    
    public:
        void setDownloadInfo(string urlPath, string outfile);
        bool downloadPlaylist();
        void downloadIndividualPlaylist(string baseUrlPath, string playlistName, string destination);
};

#endif /* HLSPlaylistDownloader_hpp */
