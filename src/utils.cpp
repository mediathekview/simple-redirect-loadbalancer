//
// Created by Christian Franzke on 2019-01-29.
//

#include "utils.h"

#include <syslog.h>
#include <iostream>
#include <vector>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

using namespace boost::algorithm;

/**
 * Thread pool used for keeping syslog off the main exec thread.
 */
static boost::asio::thread_pool syslog_pool ( std::thread::hardware_concurrency() );

void log ( log_level level, std::string msg ) {
    switch ( level ) {
    case WARNING:
        std::cerr << "mv_redirect_server WARNING: " << msg << std::endl;
        break;

    case INFO:
        std::cerr << "mv_redirect_server INFO: " << msg << std::endl;
        break;

    case ERROR:
        std::cerr << "mv_redirect_server ERROR: " << msg << std::endl;
        break;
    }

    /*boost::asio::post ( syslog_pool, [level, msg]() {
        syslog ( level, "%s", msg.c_str() );
    } );*/
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
