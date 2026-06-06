#ifndef GUARD_5F172F26A6E3485C970BA6B301ED6400
#define GUARD_5F172F26A6E3485C970BA6B301ED6400

#include <cstddef>
#include <stdint.h>

class CommonHeader {
  private:
    enum CommonHeaderE {
        CH_ACCEPT = 0,
        CH_ACCEPT_CHARSET,
        CH_ACCEPT_ENCODING,
        CH_ACCEPT_LANGUAGE,
        CH_ACCEPT_RANGES,
        CH_AGE,
        CH_ALLOW,
        CH_AUTHORIZATION,
        CH_CACHE_CONTROL,
        CH_COOKIE,
        CH_CONNECTION,
        CH_CONTENT_ENCODING,
        CH_CONTENT_LANGUAGE,
        CH_CONTENT_LENGTH,
        CH_CONTENT_LOCATION,
        CH_CONTENT_MD5,
        CH_CONTENT_RANGE,
        CH_CONTENT_TYPE,
        CH_DATE,
        CH_ETAG,
        CH_EXPECT,
        CH_EXPIRES,
        CH_FROM,
        CH_HOST,
        CH_IF_MATCH,
        CH_IF_MODIFIED_SINCE,
        CH_IF_NONE_MATCH,
        CH_IF_RANGE,
        CH_IF_UNMODIFIED_SINCE,
        CH_LAST_MODIFIED,
        CH_LOCATION,
        CH_MAX_FORWARDS,
        CH_PRAGMA,
        CH_PROXY_AUTHENTICATE,
        CH_PROXY_AUTHORIZATION,
        CH_RANGE,
        CH_REFERER,
        CH_RETRY_AFTER,
        CH_SERVER,
        CH_TE,
        CH_TRAILER,
        CH_TRANSFER_ENCODING,
        CH_UPGRADE,
        CH_USER_AGENT,
        CH_VARY,
        CH_VIA,
        CH_WARNING,
        CH_WWW_AUTHENTICATE,
        _MAX_SIZE,
        CH_INVALID, // I hate c++98
    } header;
    CommonHeader(CommonHeaderE header);

  public:
    static const CommonHeader ACCEPT;
    static const CommonHeader ACCEPT_CHARSET;
    static const CommonHeader ACCEPT_ENCODING;
    static const CommonHeader ACCEPT_LANGUAGE;
    static const CommonHeader ACCEPT_RANGES;
    static const CommonHeader AGE;
    static const CommonHeader ALLOW;
    static const CommonHeader AUTHORIZATION;
    static const CommonHeader CACHE_CONTROL;
    static const CommonHeader COOKIE;
    static const CommonHeader CONNECTION;
    static const CommonHeader CONTENT_ENCODING;
    static const CommonHeader CONTENT_LANGUAGE;
    static const CommonHeader CONTENT_LENGTH;
    static const CommonHeader CONTENT_LOCATION;
    static const CommonHeader CONTENT_MD5;
    static const CommonHeader CONTENT_RANGE;
    static const CommonHeader CONTENT_TYPE;
    static const CommonHeader DATE;
    static const CommonHeader ETAG;
    static const CommonHeader EXPECT;
    static const CommonHeader EXPIRES;
    static const CommonHeader FROM;
    static const CommonHeader HOST;
    static const CommonHeader IF_MATCH;
    static const CommonHeader IF_MODIFIED_SINCE;
    static const CommonHeader IF_NONE_MATCH;
    static const CommonHeader IF_RANGE;
    static const CommonHeader IF_UNMODIFIED_SINCE;
    static const CommonHeader LAST_MODIFIED;
    static const CommonHeader LOCATION;
    static const CommonHeader MAX_FORWARDS;
    static const CommonHeader PRAGMA;
    static const CommonHeader PROXY_AUTHENTICATE;
    static const CommonHeader PROXY_AUTHORIZATION;
    static const CommonHeader RANGE;
    static const CommonHeader REFERER;
    static const CommonHeader RETRY_AFTER;
    static const CommonHeader SERVER;
    static const CommonHeader TE;
    static const CommonHeader TRAILER;
    static const CommonHeader TRANSFER_ENCODING;
    static const CommonHeader UPGRADE;
    static const CommonHeader USER_AGENT;
    static const CommonHeader VARY;
    static const CommonHeader VIA;
    static const CommonHeader WARNING;
    static const CommonHeader WWW_AUTHENTICATE;
    static const size_t MAX_SIZE = _MAX_SIZE;
    CommonHeader();
    CommonHeader(const CommonHeader &other);
    CommonHeader &operator=(const CommonHeader &other);
    ~CommonHeader();

    static bool try_from_string(const uint8_t *s, size_t len,
                                CommonHeader &header);
    const char *as_cstr() const;
};

#endif