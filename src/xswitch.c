/*
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2020-2023, Seven Du <dujinfang@gmail.com>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Seven Du <dujinfang@gmail.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Seven Du <dujinfang@gmail.com>
 *
 * xswitch.c -- xswitch functions
 *
 */

#include <xswitch.h>
// curl req

#define BUFFER_LEN 1024

static size_t file_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
	register unsigned int realsize = (unsigned int)(size * nmemb);
	http_data_t *http_data = data;

	if (http_data && !http_data->body_buffer) {
		switch_buffer_create_dynamic(&(http_data->body_buffer), BUFFER_LEN, BUFFER_LEN * 1024, 0);
	}

	if (http_data) switch_buffer_write(http_data->body_buffer, ptr, realsize);

	return realsize;
}

static size_t header_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
	register unsigned int realsize = (unsigned int)(size * nmemb);
	http_data_t *http_data = data;
	char *header = NULL;

	header = switch_core_alloc(http_data->pool, realsize + 1);
	switch_copy_string(header, ptr, realsize);
	header[realsize] = '\0';
	// printf("%s\n", header);
	http_data->headers = switch_curl_slist_append(http_data->headers, header);

	return realsize;
}

SWITCH_DECLARE(http_data_t *) xswitch_http_post(const char *url, const char *data, switch_memory_pool_t *pool)
{
	return xswitch_http_request(XSWITCH_CM_POST, url, (void *)data, strlen(data), NULL, pool, 0, 0);
}

SWITCH_DECLARE(http_data_t *) xswitch_http_get(const char *url, switch_memory_pool_t *pool)
{
	return xswitch_http_request(XSWITCH_CM_GET, url, NULL, 0, NULL, pool, 0, 0);
}

SWITCH_DECLARE(http_data_t *)
xswitch_http_request(int method, const char *url, const void *data, size_t datalen, switch_curl_slist_t *headers,
			 switch_memory_pool_t *pool, int curl_connect_timeout, int curl_timeout)
{
	switch_CURL *curl_handle = NULL;
	long http_res = 0;
	http_data_t *http_data = NULL;
	char *content_type_res = NULL;

	http_data = switch_core_alloc(pool, sizeof(http_data_t));
	http_data->pool = pool;

	// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "url: %s, data: %s\n", url, data);
	curl_handle = switch_curl_easy_init();

	if (curl_connect_timeout == 0) curl_connect_timeout = 3;
	if (curl_timeout == 0) curl_timeout = 30;

	switch_curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, curl_connect_timeout);
	switch_curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, curl_timeout);

	if (!strncasecmp(url, "https", 5)) {
		// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Not verifying TLS cert for %s; connection is not
		// secure\n", url);
		switch_curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0);
		switch_curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
	}

	if (method == XSWITCH_CM_GET) {
		switch_curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1);
	} else {
		switch_curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, datalen);
		switch_curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data);
	}

	if (headers) {
		switch_curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
	}

	// switch_curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1);
	switch_curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1);
	switch_curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 15);
	switch_curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	switch_curl_easy_setopt(curl_handle, CURLOPT_URL, url);
	switch_curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
	switch_curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_callback);
	switch_curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, file_callback);
	switch_curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)http_data);
	switch_curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)http_data);
	switch_curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "freeswitch-curl/1.0");

	http_data->perform_code = switch_curl_easy_perform(curl_handle);

	switch_curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_res);
	switch_curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &content_type_res);
	http_data->code = http_res;
	http_data->content_type = switch_core_strdup(pool, content_type_res);
	switch_curl_easy_cleanup(curl_handle);
	if (headers) switch_curl_slist_free_all(headers);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "code: %ld, content_type_res: %s\n", http_res,
					  http_data->content_type);

	if (http_data->body_buffer) {
		switch_size_t body_len = 0;
		body_len = switch_buffer_inuse(http_data->body_buffer);
		http_data->body_size = body_len;
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "body_len: %zd\n", body_len);
	}

	return http_data;
}

SWITCH_DECLARE(void) xswitch_setup_iptables(switch_memory_pool_t *pool)
{
    char *node_ip = getenv("K8S_NODE_IP");
    char *pod_ip = getenv("K8S_POD_IP");
    char *rtp_start = getenv("RTP_START");
    char *rtp_end = getenv("RTP_END");
    char url[1024];
    char data[1024];
    http_data_t *response = NULL;

    if (!node_ip || !pod_ip || !rtp_start || !rtp_end || !switch_true(switch_core_get_variable("xswitch_setup_iptables"))) {
        return;
    }

    switch_snprintf(url, sizeof(url), "http://%s:8082/iptables", node_ip);
    switch_snprintf(data, sizeof(data), "cip=%s&rtp_start=%s&rtp_end=%s", pod_ip, rtp_start, rtp_end);

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "POST %s %s\n", url, data);

    response = xswitch_http_post(url, (const char *)data, pool);

	if (response && response->body_buffer) {
        const char *body = NULL;
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "code: %ld\n", response->code);
		switch_buffer_peek_zerocopy(response->body_buffer, (const void **)&body);
		if (response->code == 200) {
		} else if (body) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "%s\n", body);
		}

		switch_buffer_destroy(&response->body_buffer);
	}

	if (response) {
		if (response->headers) switch_curl_slist_free_all(response->headers);
	}
}
