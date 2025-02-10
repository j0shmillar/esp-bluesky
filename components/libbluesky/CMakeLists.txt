
idf_component_register(
    SRCS "bluesky.c" "../jansson/src/hashtable.c" "../jansson/src/memory.c" "../jansson/src/strbuffer.c" "../jansson/src/value.c" "../jansson/src/load.c" "../jansson/src/error.c" "../jansson/src/strconv.c" "../jansson/src/dump.c" "../jansson/src/utf.c" "../jansson/src/pack_unpack.c"
    INCLUDE_DIRS "." "../jansson/src" "../jansson/include"
    REQUIRES esp_http_client jansson mbedtls
)