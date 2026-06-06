#include "common_header.hpp"

#include <stdexcept>

const CommonHeader CommonHeader::ACCEPT(CommonHeader::CH_ACCEPT);
const CommonHeader
    CommonHeader::ACCEPT_CHARSET(CommonHeader::CH_ACCEPT_CHARSET);
const CommonHeader
    CommonHeader::ACCEPT_ENCODING(CommonHeader::CH_ACCEPT_ENCODING);
const CommonHeader
    CommonHeader::ACCEPT_LANGUAGE(CommonHeader::CH_ACCEPT_LANGUAGE);
const CommonHeader CommonHeader::ACCEPT_RANGES(CommonHeader::CH_ACCEPT_RANGES);
const CommonHeader CommonHeader::AGE(CommonHeader::CH_AGE);
const CommonHeader CommonHeader::ALLOW(CommonHeader::CH_ALLOW);
const CommonHeader CommonHeader::AUTHORIZATION(CommonHeader::CH_AUTHORIZATION);
const CommonHeader CommonHeader::CACHE_CONTROL(CommonHeader::CH_CACHE_CONTROL);
const CommonHeader CommonHeader::COOKIE(CommonHeader::CH_COOKIE);
const CommonHeader CommonHeader::CONNECTION(CommonHeader::CH_CONNECTION);
const CommonHeader
    CommonHeader::CONTENT_ENCODING(CommonHeader::CH_CONTENT_ENCODING);
const CommonHeader
    CommonHeader::CONTENT_LANGUAGE(CommonHeader::CH_CONTENT_LANGUAGE);
const CommonHeader
    CommonHeader::CONTENT_LENGTH(CommonHeader::CH_CONTENT_LENGTH);
const CommonHeader
    CommonHeader::CONTENT_LOCATION(CommonHeader::CH_CONTENT_LOCATION);
const CommonHeader CommonHeader::CONTENT_MD5(CommonHeader::CH_CONTENT_MD5);
const CommonHeader CommonHeader::CONTENT_RANGE(CommonHeader::CH_CONTENT_RANGE);
const CommonHeader CommonHeader::CONTENT_TYPE(CommonHeader::CH_CONTENT_TYPE);
const CommonHeader CommonHeader::DATE(CommonHeader::CH_DATE);
const CommonHeader CommonHeader::ETAG(CommonHeader::CH_ETAG);
const CommonHeader CommonHeader::EXPECT(CommonHeader::CH_EXPECT);
const CommonHeader CommonHeader::EXPIRES(CommonHeader::CH_EXPIRES);
const CommonHeader CommonHeader::FROM(CommonHeader::CH_FROM);
const CommonHeader CommonHeader::HOST(CommonHeader::CH_HOST);
const CommonHeader CommonHeader::IF_MATCH(CommonHeader::CH_IF_MATCH);
const CommonHeader
    CommonHeader::IF_MODIFIED_SINCE(CommonHeader::CH_IF_MODIFIED_SINCE);
const CommonHeader CommonHeader::IF_NONE_MATCH(CommonHeader::CH_IF_NONE_MATCH);
const CommonHeader CommonHeader::IF_RANGE(CommonHeader::CH_IF_RANGE);
const CommonHeader
    CommonHeader::IF_UNMODIFIED_SINCE(CommonHeader::CH_IF_UNMODIFIED_SINCE);
const CommonHeader CommonHeader::LAST_MODIFIED(CommonHeader::CH_LAST_MODIFIED);
const CommonHeader CommonHeader::LOCATION(CommonHeader::CH_LOCATION);
const CommonHeader CommonHeader::MAX_FORWARDS(CommonHeader::CH_MAX_FORWARDS);
const CommonHeader CommonHeader::PRAGMA(CommonHeader::CH_PRAGMA);
const CommonHeader
    CommonHeader::PROXY_AUTHENTICATE(CommonHeader::CH_PROXY_AUTHENTICATE);
const CommonHeader
    CommonHeader::PROXY_AUTHORIZATION(CommonHeader::CH_PROXY_AUTHORIZATION);
const CommonHeader CommonHeader::RANGE(CommonHeader::CH_RANGE);
const CommonHeader CommonHeader::REFERER(CommonHeader::CH_REFERER);
const CommonHeader CommonHeader::RETRY_AFTER(CommonHeader::CH_RETRY_AFTER);
const CommonHeader CommonHeader::SERVER(CommonHeader::CH_SERVER);
const CommonHeader CommonHeader::TE(CommonHeader::CH_TE);
const CommonHeader CommonHeader::TRAILER(CommonHeader::CH_TRAILER);
const CommonHeader
    CommonHeader::TRANSFER_ENCODING(CommonHeader::CH_TRANSFER_ENCODING);
const CommonHeader CommonHeader::UPGRADE(CommonHeader::CH_UPGRADE);
const CommonHeader CommonHeader::USER_AGENT(CommonHeader::CH_USER_AGENT);
const CommonHeader CommonHeader::VARY(CommonHeader::CH_VARY);
const CommonHeader CommonHeader::VIA(CommonHeader::CH_VIA);
const CommonHeader CommonHeader::WARNING(CommonHeader::CH_WARNING);
const CommonHeader
    CommonHeader::WWW_AUTHENTICATE(CommonHeader::CH_WWW_AUTHENTICATE);

CommonHeader::CommonHeader() : header(CH_INVALID) {}

CommonHeader::CommonHeader(CommonHeaderE header) : header(header) {}

CommonHeader::CommonHeader(const CommonHeader &other) : header(other.header) {}

CommonHeader &CommonHeader::operator=(const CommonHeader &other) {
    header = other.header;
    return *this;
};

CommonHeader::~CommonHeader() {}

static bool icmp(uint8_t a, uint8_t b) {
    return std::tolower(a) == std::tolower(b);
}

static bool assert_header(const uint8_t *s, size_t len,
                          const CommonHeader &source, CommonHeader &dest) {
    if (std::equal(s, s + len, source.as_cstr(), icmp)) {
        dest = source;
        return true;
    }
    return false;
}

// WARN: should be updated if the common header list changes (yes this is awful)
bool CommonHeader::try_from_string(const uint8_t *s, size_t len,
                                   CommonHeader &header) {
    switch (len) {
    case 2:
        return assert_header(s, len, CommonHeader::TE, header);
    case 3:
        switch (std::tolower(s[0])) {
        case 'a':
            return assert_header(s, len, CommonHeader::AGE, header);
        case 'v':
            return assert_header(s, len, CommonHeader::VIA, header);
        }
        break;
    case 4:
        switch (std::tolower(s[0])) {
        case 'd':
            return assert_header(s, len, CommonHeader::DATE, header);
        case 'e':
            return assert_header(s, len, CommonHeader::ETAG, header);
        case 'f':
            return assert_header(s, len, CommonHeader::FROM, header);
        case 'h':
            return assert_header(s, len, CommonHeader::HOST, header);
        case 'v':
            return assert_header(s, len, CommonHeader::VARY, header);
        }
        break;
    case 5:
        switch (std::tolower(s[0])) {
        case 'a':
            return assert_header(s, len, CommonHeader::ALLOW, header);
        case 'r':
            return assert_header(s, len, CommonHeader::RANGE, header);
        }
        break;
    case 6:
        switch (std::tolower(s[0])) {
        case 'a':
            return assert_header(s, len, CommonHeader::ACCEPT, header);
        case 'c':
            return assert_header(s, len, CommonHeader::COOKIE, header);
        case 'e':
            return assert_header(s, len, CommonHeader::EXPECT, header);
        case 'p':
            return assert_header(s, len, CommonHeader::PRAGMA, header);
        case 's':
            return assert_header(s, len, CommonHeader::SERVER, header);
        }
        break;
    case 7:
        switch (std::tolower(s[0])) {
        case 'e':
            return assert_header(s, len, CommonHeader::EXPIRES, header);
        case 'r':
            return assert_header(s, len, CommonHeader::REFERER, header);
        case 't':
            return assert_header(s, len, CommonHeader::TRAILER, header);
        case 'u':
            return assert_header(s, len, CommonHeader::UPGRADE, header);
        case 'w':
            return assert_header(s, len, CommonHeader::WARNING, header);
        }
        break;
    case 8:
        switch (std::tolower(s[0])) {
        case 'i':
            switch (std::tolower(s[3])) {
            case 'm':
                return assert_header(s, len, CommonHeader::IF_MATCH, header);
            case 'r':
                return assert_header(s, len, CommonHeader::IF_RANGE, header);
            }
            break;
        case 'l':
            return assert_header(s, len, CommonHeader::LOCATION, header);
        }
        break;
    case 10:
        switch (std::tolower(s[0])) {
        case 'c':
            return assert_header(s, len, CommonHeader::CONNECTION, header);
        case 'u':
            return assert_header(s, len, CommonHeader::USER_AGENT, header);
        }
        break;
    case 11:
        switch (std::tolower(s[0])) {
        case 'c':
            return assert_header(s, len, CommonHeader::CONTENT_MD5, header);
        case 'r':
            return assert_header(s, len, CommonHeader::RETRY_AFTER, header);
        }
        break;
    case 12:
        switch (std::tolower(s[0])) {
        case 'c':
            return assert_header(s, len, CommonHeader::CONTENT_TYPE, header);
        case 'm':
            return assert_header(s, len, CommonHeader::MAX_FORWARDS, header);
        }
        break;
    case 13:
        switch (std::tolower(s[0])) {
        case 'a':
            switch (std::tolower(s[1])) {
            case 'c':
                return assert_header(s, len, CommonHeader::ACCEPT_RANGES,
                                     header);
            case 'u':
                return assert_header(s, len, CommonHeader::AUTHORIZATION,
                                     header);
            }
            break;
        case 'c':
            switch (std::tolower(s[1])) {
            case 'a':
                return assert_header(s, len, CommonHeader::CACHE_CONTROL,
                                     header);
            case 'o':
                return assert_header(s, len, CommonHeader::CONTENT_RANGE,
                                     header);
            }
            break;
        case 'i':
            return assert_header(s, len, CommonHeader::IF_NONE_MATCH, header);
        case 'l':
            return assert_header(s, len, CommonHeader::LAST_MODIFIED, header);
        }
        break;
    case 14:
        switch (std::tolower(s[0])) {
        case 'a':
            return assert_header(s, len, CommonHeader::ACCEPT_CHARSET, header);
        case 'c':
            return assert_header(s, len, CommonHeader::CONTENT_LENGTH, header);
        }
        break;
    case 15:
        switch (std::tolower(s[7])) {
        case 'e':
            return assert_header(s, len, CommonHeader::ACCEPT_ENCODING, header);
        case 'l':
            return assert_header(s, len, CommonHeader::ACCEPT_LANGUAGE, header);
        }
        break;
    case 16:
        switch (std::tolower(s[0])) {
        case 'c':
            switch (std::tolower(s[8])) {
            case 'e':
                return assert_header(s, len, CommonHeader::CONTENT_ENCODING,
                                     header);
            case 'l':
                switch (std::tolower(s[9])) {
                case 'a':
                    return assert_header(s, len, CommonHeader::CONTENT_LANGUAGE,
                                         header);
                case 'o':
                    return assert_header(s, len, CommonHeader::CONTENT_LOCATION,
                                         header);
                }
            }
            break;
        case 'w':
            return assert_header(s, len, CommonHeader::WWW_AUTHENTICATE,
                                 header);
        }
        break;
    case 17:
        switch (std::tolower(s[0])) {
        case 'i':
            return assert_header(s, len, CommonHeader::IF_MODIFIED_SINCE,
                                 header);
        case 't':
            return assert_header(s, len, CommonHeader::TRANSFER_ENCODING,
                                 header);
        }
        break;
    case 18:
        return assert_header(s, len, CommonHeader::PROXY_AUTHENTICATE, header);
    case 19:
        switch (std::tolower(s[0])) {
        case 'i':
            return assert_header(s, len, CommonHeader::IF_UNMODIFIED_SINCE,
                                 header);
        case 'p':
            return assert_header(s, len, CommonHeader::PROXY_AUTHORIZATION,
                                 header);
        }
    };
    return false;
}

const char *CommonHeader::as_cstr() const {
    switch (header) {
    case CH_ACCEPT:
        return "Accept";
    case CH_ACCEPT_CHARSET:
        return "Accept-Charset";
    case CH_ACCEPT_ENCODING:
        return "Accept-Encoding";
    case CH_ACCEPT_LANGUAGE:
        return "Accept-Language";
    case CH_ACCEPT_RANGES:
        return "Accept-Ranges";
    case CH_AGE:
        return "Age";
    case CH_ALLOW:
        return "Allow";
    case CH_AUTHORIZATION:
        return "Authorization";
    case CH_CACHE_CONTROL:
        return "Cache-Control";
    case CH_COOKIE:
        return "Cookie";
    case CH_CONNECTION:
        return "Connection";
    case CH_CONTENT_ENCODING:
        return "Content-Encoding";
    case CH_CONTENT_LANGUAGE:
        return "Content-Language";
    case CH_CONTENT_LENGTH:
        return "Content-Length";
    case CH_CONTENT_LOCATION:
        return "Content-Location";
    case CH_CONTENT_MD5:
        return "Content-MD5";
    case CH_CONTENT_RANGE:
        return "Content-Range";
    case CH_CONTENT_TYPE:
        return "Content-Type";
    case CH_DATE:
        return "Date";
    case CH_ETAG:
        return "ETag";
    case CH_EXPECT:
        return "Expect";
    case CH_EXPIRES:
        return "Expires";
    case CH_FROM:
        return "From";
    case CH_HOST:
        return "Host";
    case CH_IF_MATCH:
        return "If-Match";
    case CH_IF_MODIFIED_SINCE:
        return "If-Modified-Since";
    case CH_IF_NONE_MATCH:
        return "If-None-Match";
    case CH_IF_RANGE:
        return "If-Range";
    case CH_IF_UNMODIFIED_SINCE:
        return "If-Unmodified-Since";
    case CH_LAST_MODIFIED:
        return "Last-Modified";
    case CH_LOCATION:
        return "Location";
    case CH_MAX_FORWARDS:
        return "Max-Forwards";
    case CH_PRAGMA:
        return "Pragma";
    case CH_PROXY_AUTHENTICATE:
        return "Proxy-Authenticate";
    case CH_PROXY_AUTHORIZATION:
        return "Proxy-Authorization";
    case CH_RANGE:
        return "Range";
    case CH_REFERER:
        return "Referer";
    case CH_RETRY_AFTER:
        return "Retry-After";
    case CH_SERVER:
        return "Server";
    case CH_TE:
        return "TE";
    case CH_TRAILER:
        return "Trailer";
    case CH_TRANSFER_ENCODING:
        return "Transfer-Encoding";
    case CH_UPGRADE:
        return "Upgrade";
    case CH_USER_AGENT:
        return "User-Agent";
    case CH_VARY:
        return "Vary";
    case CH_VIA:
        return "Via";
    case CH_WARNING:
        return "Warning";
    case CH_WWW_AUTHENTICATE:
        return "WWW-Authenticate";
    default:
        throw std::runtime_error("use of an invalid header");
    }
}