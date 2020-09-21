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
 * xswitch.h -- xswitch functions header
 *
 */

#ifndef FREESWITCH_XSWITCH_H
#define FREESWITCH_XSWITCH_H

#include <switch.h>
#include <switch_curl.h>

SWITCH_BEGIN_EXTERN_C

#define XSWITCH_CM_GET 0
#define XSWITCH_CM_POST 1

typedef struct http_data_s {
	switch_buffer_t *body_buffer;
	switch_memory_pool_t *pool;
	char *content_type;
	int err;
	long code;
	switch_size_t body_size;
	switch_curl_slist_t *headers;
	switch_CURLcode perform_code;
} http_data_t;

SWITCH_DECLARE(http_data_t *) xswitch_http_post(const char *url, const char *data, switch_memory_pool_t *pool);
SWITCH_DECLARE(http_data_t *) xswitch_http_get(const char *url, switch_memory_pool_t *pool);
SWITCH_DECLARE(http_data_t *) xswitch_http_request(int method, const char *url, const void *data, size_t datalen, switch_curl_slist_t *headers,
                                                   switch_memory_pool_t *pool, int curl_connect_timeout, int curl_timeout);

SWITCH_END_EXTERN_C
#endif
/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
