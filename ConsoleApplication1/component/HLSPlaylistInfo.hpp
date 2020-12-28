//  HLSPlaylistInfo.hpp
//  Created by Krishna Gudipati on 8/14/18.

#ifndef HLSPlaylistInfo_hpp
#define HLSPlaylistInfo_hpp

#include <sstream>
#include <vector>

using namespace std;

class HLSPlaylistInfo {
    
    public :
        HLSPlaylistInfo(string url);
        HLSPlaylistInfo();
        ~HLSPlaylistInfo();
    
    // https://devstreaming-cdn.apple.com/videos/streaming/examples/bipbop_4x3/bipbop_4x3_variant.m3u8
    private:
        string m_completePlaylistUrlPath;
        string m_baseUrlPath;                 // https://devstreaming-cdn.apple.com/videos/streaming/examples/bipbop_4x3
    
        string m_protocolIdentifier;          // https
        string m_host;                        // devstreaming-cdn.apple.com
        string m_playlistRootName;            // bipbop_4x3
        string m_mainPlaylistName;            // bipbop_4x3_variant.m3u8
    
    public:
        string getCompleteUrlPath();
        string getPlaylistRootName();
        string getMainPlaylistName();
        string getBaseUrlPath();
        bool extractPlaylistInfo(string url);
};

#endif /* HLSPlaylistInfo_hpp */
