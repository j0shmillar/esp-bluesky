idf_component_register(
    SRCS "hello_world_main.c" "wifi.h" "wifi.c" 
    INCLUDE_DIRS "."
    REQUIRES libbluesky jansson nvs_flash esp_wifi esp-tls mbedtls
)

idf_build_set_property(COMPILE_OPTIONS "-DCONFIG_BSKY_HANDLE=\"${CONFIG_BSKY_HANDLE}\"" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DCONFIG_BSKY_PASSWORD=\"${CONFIG_BSKY_PASSWORD}\"" APPEND)