//  HLSPlaylistInfo.cpp
//  Created by Krishna Gudipati on 8/14/18.

#include "HLSPlaylistInfo.hpp"
#include "HLSPlaylistUtilities.hpp"

#include <regex>

using namespace std;

HLSPlaylistInfo::HLSPlaylistInfo(string url) {
    m_completePlaylistUrlPath = url;
}

HLSPlaylistInfo::HLSPlaylistInfo() {
}

HLSPlaylistInfo::~HLSPlaylistInfo() {
}

string HLSPlaylistInfo:: getCompleteUrlPath() {
    return m_completePlaylistUrlPath;
}

string HLSPlaylistInfo:: getPlaylistRootName() {
    return m_playlistRootName;
}

string HLSPlaylistInfo:: getMainPlaylistName() {
    return m_mainPlaylistName;
}

string HLSPlaylistInfo:: getBaseUrlPath() {
    return m_baseUrlPath;
}

bool HLSPlaylistInfo::extractPlaylistInfo(string url) {
    // This method tokenizes the url and stores individual parts like play list name, the root directory name where all the downloads should be saved
    // Eg. https://devstreaming-cdn.apple.com/videos/streaming/examples/bipbop_4x3/bipbop_4x3_variant.m3u8
    
    if (url.empty()) {
        return false;
    }
    
    //regex urlregex("https?:\\/\\/.+(\\/.+)*\\/*.m3u8");
    regex urlregex("https?:\\/\\/([a-z0-9-_.]+\\/)+[a-z0-9-_]*\\.m3u8"); // Basic http path matching; ensure link is ended with a playlist name
    if (!regex_match(url, urlregex)) {
        return false;
    }
    
    vector <string> urlItems = HLSPlaylistUtilities::tokenize(url, '/');
    
    // Minimum items in the url - http, empty string (//),host, playlist name
    if (urlItems.size() < 4) {
        return false;
    }
    
    m_completePlaylistUrlPath = url;
    
    m_protocolIdentifier = urlItems[0];                 // https
    m_host = urlItems[2];                               // devstreaming-cdn.apple.com
    m_playlistRootName = urlItems[urlItems.size() - 2]; // bipbop_4x3 - The directory under user specified directory
    m_mainPlaylistName = urlItems[urlItems.size() - 1]; // bipbop_4x3_variant.m3u8 - The first play list index file downloaded
    
    // Every download should start from this URL path
    // https://devstreaming-cdn.apple.com/videos/streaming/examples/bipbop_4x3/
    m_baseUrlPath = regex_replace(url, regex(m_mainPlaylistName), "");
    
    return true;
}
