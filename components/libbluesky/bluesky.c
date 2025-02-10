/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Brian J. Downs
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"

#include "jansson.h"
#include "bluesky.h"

#define BS_RES_JSON_HEADER "Content-Type: application/json"
#define BS_REQ_JSON_HEADER "Accept: application/json"
#define TOKEN_HEADER_SIZE 4096 // TODO make dynamic
#define DID_MAX_SIZE 2048
#define DEFAULT_URL_SIZE 2048
#define STACK_SIZE 8192

static const char *TAG = "BLUESKY_CLIENT";
static char did[256];
static char token[400];
static char cur_handle[256];
static char refresh_token[400];
static char token_header[TOKEN_HEADER_SIZE];

#define API_BASE                     "https://bsky.social/xrpc"
#define AUTH_URL API_BASE            "/com.atproto.server.createSession"
#define RESOLVE_HANDLE_URL API_BASE  "/com.atproto.identity.resolveHandle"
#define POST_FEED_URL API_BASE       "/com.atproto.repo.createRecord"

#include <time.h>
#include <stdio.h>

char* get_current_time_rfc3339()
{
    time_t now;
    struct tm timeinfo;
    char *time_str = malloc(30 * sizeof(char));  // Allocate enough space for RFC-3339 format
    
    time(&now);
    gmtime_r(&now, &timeinfo);  // Get the current time in UTC
    strftime(time_str, 30, "%Y-%m-%dT%H:%M:%SZ", &timeinfo);  // Format as RFC-3339
    
    return time_str;
}

// HTTP Event Handler
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    bs_client_response_t *response = (bs_client_response_t *)evt->user_data;
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            char *ptr = realloc(response->resp, response->size + evt->data_len + 1);
            if (ptr == NULL) {
                ESP_LOGE(TAG, "Failed to allocate memory for response");
                return ESP_FAIL;
            }
            response->resp = ptr;
            memcpy(&(response->resp[response->size]), evt->data, evt->data_len);
            response->size += evt->data_len;
            response->resp[response->size] = 0;
            break;
        default:
            break;
    }
    return ESP_OK;
}

// Modified bs_client_post function
bs_client_response_t* bs_client_post(const char *msg)
{
    bs_client_response_t *response = calloc(1, sizeof(bs_client_response_t));
    esp_http_client_config_t config = {
        .url = POST_FEED_URL,
        .event_handler = http_event_handler,
        .user_data = response,
        .method = HTTP_METHOD_POST,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        response->err_msg = strdup("Failed to initialize HTTP client");
        return response;
    }
    esp_http_client_set_header(client, "Authorization", token_header);
    
    json_t *root = json_object();
    
    // Get current timestamp for createdAt field
    char *createdAt = get_current_time_rfc3339();
    
    // Make sure msg is valid JSON
    json_error_t json_error;
    json_t *record_json = json_loads(msg, 0, &json_error);
    if (!record_json) {
        response->err_msg = strdup("Invalid JSON message for record");
        esp_http_client_cleanup(client);
        free(createdAt);
        return response;
    }
    
    // Add required fields to the request body
    json_object_set_new(root, "repo", json_string(did));
    json_object_set_new(root, "collection", json_string("app.bsky.feed.post"));
    json_object_set_new(root, "record", record_json);
    json_object_set_new(record_json, "createdAt", json_string(createdAt));  // Add createdAt field
    
    char *post_data = json_dumps(root, 0);
    
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Accept", "application/json");
    esp_http_client_set_header(client, "Authorization", token_header);
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        response->err_msg = strdup(esp_err_to_name(err));
    }
    response->resp_code = esp_http_client_get_status_code(client);
    
    json_decref(root);
    free(post_data);
    free(createdAt);  // Don't forget to free memory for createdAt
    esp_http_client_cleanup(client);
    return response;
}

static bs_client_response_t* bs_client_response_new()
{
    return (bs_client_response_t *)calloc(1, sizeof(bs_client_response_t));
}

void bs_client_response_free(bs_client_response_t *res)
{
    if (res) {
        if (res->resp) free(res->resp);
        if (res->err_msg) free(res->err_msg);
        free(res);
    }
}

bs_client_response_t* bs_client_authenticate(const char *handle, const char *app_password)
{
    bs_client_response_t *response = bs_client_response_new();
    esp_http_client_config_t config = {
        .url = "https://bsky.social/xrpc/com.atproto.server.createSession",
        .event_handler = http_event_handler,
        .user_data = response,
        .method = HTTP_METHOD_POST,
        .crt_bundle_attach = esp_crt_bundle_attach
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        response->err_msg = strdup("Failed to initialize HTTP client");
        return response;
    }
    esp_http_client_set_header(client, "Authorization", token_header);

    json_t *root = json_object();
    json_object_set_new(root, "identifier", json_string(handle));
    json_object_set_new(root, "password", json_string(app_password));
    char *post_data = json_dumps(root, JSON_COMPACT);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        response->err_msg = strdup(esp_err_to_name(err));
    } else {
        response->resp_code = esp_http_client_get_status_code(client);
    }
    
    // Parse the response to extract the DID
    json_error_t j_error;
    json_t *json_response = json_loads(response->resp, 0, &j_error);
    if (!json_response) {
        response->err_msg = strdup(j_error.text);
        json_decref(root);
        free(post_data);
        esp_http_client_cleanup(client);
        return response;
    }

    // Extract DID and other tokens from the response
    const char *did_value = NULL;
    const char *t = NULL;
    const char *rt = NULL;
    json_unpack(json_response, "{s:s, s:s, s:s}", "accessJwt", &t, "refreshJwt", &rt, "did", &did_value);

    if (did_value) {
        strncpy(did, did_value, sizeof(did) - 1);  // Store the DID in the global variable
    } else {
        response->err_msg = strdup("DID not found in response");
    }
    ESP_LOGI(TAG, "Received DID: %s", did);
    ESP_LOGI(TAG, "Authentication response: %s", response->resp);


    // Store tokens and clean up
    if (t) {
        strncpy(token, t, sizeof(token) - 1);
    }
    if (rt) {
        strncpy(refresh_token, rt, sizeof(refresh_token) - 1);
    }
    snprintf(token_header, TOKEN_HEADER_SIZE, "Bearer %s", token);

    json_decref(json_response);
    json_decref(root);
    free(post_data);
    esp_http_client_cleanup(client);

    return response;
}

int bs_client_init(const char *handle, const char *app_password, char *error)
{
    strcpy(cur_handle, handle);
    bs_client_response_t *res = bs_client_authenticate(handle, app_password);
    if (res->err_msg) {
        strcpy(error, res->err_msg);
        bs_client_response_free(res);
        return -1;
    }

    json_error_t j_error;
    json_t *root = json_loads(res->resp, 0, &j_error);
    if (!root) {
        strcpy(error, j_error.text);
        bs_client_response_free(res);
        return -1;
    }

    const char *t = NULL;
    const char *rt = NULL;
    if (json_unpack(root, "{s:s, s:s}", "accessJwt", &t, "refreshJwt", &rt) != 0) {
        res->err_msg = strdup("Failed to parse accessJwt and refreshJwt");
        json_decref(root);
        bs_client_response_free(res);
        return -1;
    }

    strncpy(token, t, sizeof(token) - 1);
    strncpy(refresh_token, rt, sizeof(refresh_token) - 1);
    snprintf(token_header, TOKEN_HEADER_SIZE, "Bearer %s", token);

    bs_client_response_free(res);
    json_decref(root);

    ESP_LOGI(TAG, "Free stack size after auth: %d", uxTaskGetStackHighWaterMark(NULL));

    return 0;
}
