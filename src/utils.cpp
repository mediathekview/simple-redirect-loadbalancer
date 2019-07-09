//
// Created by Christian Franzke on 2019-01-29.
//

#include "utils.h"

#include <syslog.h>
#include <iostream>
#include <vector>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <unordered_set>

using namespace boost::algorithm;

/**
 * Thread pool used for keeping syslog off the main exec thread.
 */
static boost::asio::thread_pool syslog_pool ( std::thread::hardware_concurrency() );

void log ( log_level level, std::string msg ) {
    static boost::format formatter ( "mv_redirect_server %1: %2" );
    switch ( level ) {
    case WARNING:
        formatter % "WARNING" % msg;
        break;

    case INFO:
        formatter % "INFO" % msg;
        break;

    case ERROR:
        formatter % "ERROR" % msg;
        break;
    }

    boost::asio::post ( syslog_pool, [level, msg]() {
        //syslog ( level, "%s", msg.c_str() );
        std::cerr << formatter << std::endl;
    } );
}

void fail ( boost::system::error_code ec, char const *what ) {
    try {
        static boost::format formatter ( "%1: %2" );
        formatter % what % ec.message();
        log ( ERROR, formatter.str() );
    } catch ( boost::io::bad_format_string& ) {
        log ( ERROR, "Failed to format string in fail()" );
    } catch ( std::exception& e ) {
        log ( ERROR, e.what() );
    } catch ( ... ) {
        log ( ERROR, "Unknown exception in fail()" );
    }
}

/**
 * Do some basic checks on destination.
 * Those things should not occur in our destination
 */
bool destination_is_invalid ( const std::string& destination ) {
    if ( contains ( destination, ".php" ) )
        return true;

    if ( contains ( destination, ".html" ) )
        return true;

    if ( contains ( destination, ".js" ) )
        return true;

    if ( contains ( destination, "127.0.0.1" ) )
        return true;

    if ( contains ( destination, "wget" ) )
        return true;

    if ( contains ( destination, "curl" ) )
        return true;

    return false;
}

class BlackList {
    public:
    BlackList() {};

    bool is_blocked ( const boost::asio::ip::address& addr ) {
        return false;
    };
    private:
    struct endpoint_hash {
        std::size_t operator() ( boost::asio::ip::udp::endpoint const& ep ) const noexcept
        {
            auto accum = std::size_t ( 0 );
            auto combine = [&accum] ( auto&& arg ) {
                boost::hash_combine ( accum, arg );
            };

            combine ( ep.port() );
            if ( auto&& addr = ep.address(); addr.is_v4() ) {
                combine ( addr.to_v4().to_ulong() );
            } else
            {
                combine ( addr.to_v6().to_bytes() );
            }
            combine ( ep.port() );
            return accum;
        }
    };
    std::unordered_set<boost::asio::ip::udp::endpoint, endpoint_hash> blockList;
};


BlackList blacklister;

bool ip_is_blocked ( const boost::asio::ip::address& addr ) {

    return blacklister.is_blocked ( addr );
}
