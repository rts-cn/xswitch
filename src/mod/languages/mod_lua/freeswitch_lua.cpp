
#include <switch.h>
#include "freeswitch_lua.h"
using namespace LUA;

extern "C" {
	int docall(lua_State * L, int narg, int nresults, int perror, int fatal);
};

Session::Session():CoreSession()
{
	cb_function = cb_arg = hangup_func_str = hangup_func_arg = NULL;
	hh = mark = 0;
}

Session::Session(char *nuuid, CoreSession *a_leg):CoreSession(nuuid, a_leg)
{
	cb_function = cb_arg = hangup_func_str = hangup_func_arg = NULL;
	hh = mark = 0;
}

Session::Session(switch_core_session_t *new_session):CoreSession(new_session)
{
	cb_function = cb_arg = hangup_func_str = hangup_func_arg = NULL;
	hh = mark = 0;
}
static switch_status_t lua_hanguphook(switch_core_session_t *session_hungup);


void Session::destroy(const char *err)
{

	if (!allocated) {
		return;
	}

	if (session) {
		if (!channel) {
			channel = switch_core_session_get_channel(session);
		}
		switch_channel_set_private(channel, "CoreSession", NULL);
		switch_core_event_hook_remove_state_change(session, lua_hanguphook);
	}

	switch_safe_free(hangup_func_str);
	switch_safe_free(hangup_func_arg);
	switch_safe_free(cb_function);
	switch_safe_free(cb_arg);

	unsetInputCallback();

	CoreSession::destroy();


	if (!zstr(err)) {
		lua_pushstring(L, err);
		lua_error(L);
	}

}


Session::~Session()
{
	destroy();
}

bool Session::begin_allow_threads()
{
	do_hangup_hook();
	return true;
}

bool Session::end_allow_threads()
{
	do_hangup_hook();
	return true;
}

void Session::setLUA(lua_State * state)
{
	L = state;

	if (session && allocated && uuid) {
		lua_setglobal(L, uuid);
		lua_getglobal(L, uuid);
	}

}

int Session::originate(CoreSession *a_leg_session, char *dest, int timeout)
{
	if (zstr(dest)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Missing destination.\n");
		return 0;
	}

	int x = CoreSession::originate(a_leg_session, dest, timeout);

	if (x) {
		setLUA(L);
	}

	return x;
}

lua_State *Session::getLUA()
{
	if (!L) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Doh!\n");
	}
	return L;
}


bool Session::ready()
{
	bool r;

	if (!session) {
		return false;
	}
	sanity_check(false);
	r = switch_channel_ready(channel) != 0;
	do_hangup_hook();

	return r;
}

void Session::check_hangup_hook()
{
	if (hangup_func_str && (hook_state == CS_HANGUP || hook_state == CS_ROUTING)) {
		hh++;
	}
}

void Session::do_hangup_hook()
{
	if (hh && !mark) {
		int arg_count = 2;
		mark++;

		if (!getLUA()) {
			return;
		}

		lua_getglobal(L, (char *) hangup_func_str);
		lua_getglobal(L, uuid);

		lua_pushstring(L, hook_state == CS_HANGUP ? "hangup" : "transfer");

		if (hangup_func_arg) {
			lua_getglobal(L, (char *) hangup_func_arg);
			arg_count++;
		}

		docall(L, arg_count, 1, 1, 0);

		const char *err = lua_tostring(L, -1);

		switch_channel_set_variable(channel, "lua_hangup_hook_return_val", err);

		if (!zstr(err)) {

			if (!strcasecmp(err, "exit") || !strcasecmp(err, "die")) {
				lua_error(L);
			} else {
				lua_pop(L, 1);
			}
		} else {
			lua_pop(L, 1);
		}


		if (channel) {
			switch_channel_set_private(channel, "CoreSession", NULL);
		}

		if (session) {
			switch_core_event_hook_remove_state_change(session, lua_hanguphook);
		}
		switch_safe_free(hangup_func_str);

	}
}

static switch_status_t lua_hanguphook(switch_core_session_t *session_hungup)
{
	switch_channel_t *channel = switch_core_session_get_channel(session_hungup);
	Session *coresession = NULL;
	switch_channel_state_t state = switch_channel_get_state(channel);

	if (session_hungup) {

		channel = switch_core_session_get_channel(session_hungup);

		if (channel) {
			void *vs = switch_channel_get_private(channel, "CoreSession");
			if (vs) {
				coresession = (Session *) vs;
			}
		}

		if (!(coresession && coresession->hook_state)) {
			return SWITCH_STATUS_FALSE;
		}

		if (coresession && coresession->allocated && (state == CS_HANGUP || state == CS_ROUTING) && coresession->hook_state != state) {
			coresession->hook_state = state;
			coresession->check_hangup_hook();
			switch_core_event_hook_remove_state_change(session_hungup, lua_hanguphook);
		}
	}

	return SWITCH_STATUS_SUCCESS;
}


void Session::setHangupHook(char *func, char *arg)
{

	sanity_check_noreturn;

	switch_safe_free(hangup_func_str);
	switch_safe_free(hangup_func_arg);

	if (func) {
		hangup_func_str = strdup(func);
		if (!zstr(arg)) {
			hangup_func_arg = strdup(arg);
		}
		switch_channel_set_private(channel, "CoreSession", this);
		hook_state = switch_channel_get_state(channel);
		switch_core_event_hook_add_state_change(session, lua_hanguphook);
	}
}

void Session::unsetInputCallback(void)
{
	sanity_check_noreturn;
	switch_safe_free(cb_function);
	switch_safe_free(cb_arg);
	args.input_callback = NULL;
	ap = NULL;
	switch_channel_clear_flag_recursive(channel, CF_QUEUE_TEXT_EVENTS);
}

void Session::setInputCallback(char *cbfunc, char *funcargs)
{

	sanity_check_noreturn;

	switch_safe_free(cb_function);
	if (cbfunc) {
		cb_function = strdup(cbfunc);
	}

	switch_safe_free(cb_arg);
	if (funcargs) {
		cb_arg = strdup(funcargs);
	}

	args.buf = this;
	switch_channel_set_private(channel, "CoreSession", this);

	args.input_callback = dtmf_callback;
	ap = &args;

	switch_channel_set_flag_recursive(channel, CF_QUEUE_TEXT_EVENTS);

}

switch_status_t Session::run_dtmf_callback(void *input, switch_input_type_t itype)
{
	const char *ret;

	if (!getLUA()) {
		return SWITCH_STATUS_FALSE;;
	}

	switch (itype) {
	case SWITCH_INPUT_TYPE_DTMF:
		{
			switch_dtmf_t *dtmf = (switch_dtmf_t *) input;
			char str[3] = "";
			int arg_count = 3;
			int r;

			lua_getglobal(L, (char *) cb_function);
			lua_getglobal(L, uuid);

			lua_pushstring(L, "dtmf");

			lua_newtable(L);
			lua_pushstring(L, "digit");
			str[0] = dtmf->digit;
			lua_pushstring(L, str);
			lua_rawset(L, -3);

			lua_pushstring(L, "duration");
			lua_pushnumber(L, dtmf->duration);
			lua_rawset(L, -3);

			if (!zstr(cb_arg)) {
				lua_getglobal(L, (char *) cb_arg);
				arg_count++;
			}

			r = docall(L, arg_count, 1, 1, 0);

			if (!r) {
				ret = lua_tostring(L, -1);
				lua_pop(L, 1);
			} else {
				ret = "SCRIPT_ERROR";
			}

			return process_callback_result((char *) ret);
		}
		break;
	case SWITCH_INPUT_TYPE_EVENT:
		{
			switch_event_t *event = (switch_event_t *) input;
			int arg_count = 3;


			lua_getglobal(L, (char *) cb_function);
			lua_getglobal(L, uuid);
			lua_pushstring(L, "event");
			mod_lua_conjure_event(L, event, "__Input_Event__", 1);
			lua_getglobal(L, "__Input_Event__");

			if (!zstr(cb_arg)) {
				lua_getglobal(L, (char *) cb_arg);
				arg_count++;
			}

			if (!docall(L, arg_count, 1, 1, 0)) {
				ret = lua_tostring(L, -1);
				lua_pop(L, 1);
			} else {
				ret = "SCRIPT_ERROR";
			}

			return process_callback_result((char *) ret);
		}
		break;
	}

	return SWITCH_STATUS_SUCCESS;
}


Dbh::Dbh(char *dsn, char *user, char *pass)
{
	dbh = NULL;
	err = NULL;
	char *tmp = NULL;

	if (!zstr(user) || !zstr(pass)) {
		tmp = switch_mprintf("%s%s%s%s%s", dsn,
							 zstr(user) ? "" : ":",
							 zstr(user) ? "" : user,
							 zstr(pass) ? "" : ":",
							 zstr(pass) ? "" : pass
							 );

		dsn = tmp;
	}

	if (!zstr(dsn) && switch_cache_db_get_db_handle_dsn(&dbh, dsn) == SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG10, "DBH handle %p Connected.\n", (void *) dbh);
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Connection failed.  DBH NOT Connected.\n");
	}

	switch_safe_free(tmp);

}

Dbh::~Dbh()
{
	if (dbh) release();

	clear_error();
}

void Dbh::clear_error()
{
	switch_safe_free(err);
}

char *Dbh::last_error()
{
	return err;
}

bool Dbh::release()
{
  if (dbh) {
	  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG10, "DBH handle %p released.\n", (void *) dbh);
	  switch_cache_db_release_db_handle(&dbh);
	  return true;
  }

  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "DBH NOT Connected.\n");
  return false;
}

bool Dbh::connected()
{
	return dbh ? true : false;
}

bool Dbh::test_reactive(char *test_sql, char *drop_sql, char *reactive_sql)
{
  if (dbh) {
    if (!zstr(test_sql) && !zstr(reactive_sql)) {
      if (switch_cache_db_test_reactive(dbh, test_sql, drop_sql, reactive_sql) == SWITCH_TRUE) {
        return true;
      }
    } else {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Missing parameters.\n");
    }
  } else {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "DBH NOT Connected.\n");
  }
  return false;
}

int Dbh::query_callback(void *pArg, int argc, char **argv, char **cargv)
{
  SWIGLUA_FN *lua_fun = (SWIGLUA_FN *)pArg;
	int ret = 0;

  lua_pushvalue(lua_fun->L, lua_fun->idx); /* get the lua callback function onto the stack */

  lua_newtable(lua_fun->L); /* push a row (table) */

  for (int i = 0; i < argc; i++) {
    lua_pushstring(lua_fun->L, switch_str_nil(cargv[i]));
    lua_pushstring(lua_fun->L, switch_str_nil(argv[i]));
    lua_settable(lua_fun->L, -3);
  }

  if (docall(lua_fun->L, 1, 1, 1, 0)) {
	  return 1;
  }

  ret = lua_tonumber(lua_fun->L, -1);
  lua_pop(lua_fun->L, 1);

  if (ret != 0) {
	  return 1;
  }

  return 0; /* 0 to continue with next row */
}

bool Dbh::query(char *sql, SWIGLUA_FN lua_fun)
{
  clear_error();

  if (zstr(sql)) {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Missing SQL query.\n");
    return false;
  }

  if (dbh) {
    if (lua_fun.L) {
      if (switch_cache_db_execute_sql_callback(dbh, sql, query_callback, &lua_fun, &err) == SWITCH_STATUS_SUCCESS) {
        return true;
      }
    } else { /* if no lua_fun arg is passed from Lua, an empty initialized struct will be sent - see freeswitch.i */
      if (switch_cache_db_execute_sql(dbh, sql, &err) == SWITCH_STATUS_SUCCESS) {
        return true;
      }
    }
  } else {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "DBH NOT Connected.\n");
  }
  return false;
}

struct query_callback_data {
    lua_State *L;
    int stack_index;
    int *row_num;
};

int query2_callback(void *pArg, int argc, char **argv, char **columnNames)
{
    struct query_callback_data *data = (struct query_callback_data *) pArg;
    lua_State *tL = data->L;
    lua_createtable(tL, 0, argc);
    for (int i = 0; i < argc; i++) {
        lua_pushstring(tL, argv[i]);
        lua_setfield(tL, -2, switch_str_nil(columnNames[i]));
    }
    lua_rawseti(tL, data->stack_index + 2, (*data->row_num)++);
    return 0;
}

DbhQueryRowsReturn Dbh::query_rows(lua_State* L, char *sql)
{
    int stack_index = lua_gettop(L);
    clear_error();
    lua_pushboolean(L, 0); // result success error: stack_index + 1
    lua_newtable(L);       // the rows: stack_index + 2
    lua_pushnil(L);        // error message if any: stack_index + 3

    if (zstr(sql)) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Missing SQL query.\n");
        lua_pushstring(L, "Missing SQL query.");
	lua_replace(L, stack_index + 3);
        return 3;
    }

    if (dbh) {
        int index = 1;
        struct query_callback_data pData = {L, stack_index, &index};

        if (switch_cache_db_execute_sql_callback(dbh, sql, query2_callback, &pData, &err) == SWITCH_STATUS_SUCCESS) {
            // no errors
	    lua_pushboolean(L, 1);
            lua_replace(L, stack_index + 1);
        } else {
            lua_pushstring(L, !zstr(err) ? err : "Failed to execute sql query");
	    lua_replace(L, stack_index + 3);
        }
    } else {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "DBH NOT Connected.\n");
        lua_pushstring(L, "DBH NOT Connected.");
	lua_replace(L, stack_index + 3);
    }

    return 3;
}


int Dbh::affected_rows()
{
  if (dbh) {
    return switch_cache_db_affected_rows(dbh);
  }

  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "DBH NOT Connected.\n");
  return 0;
}

int Dbh::load_extension(const char *extension)
{
  if (zstr(extension)) {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Missing extension name.\n");
    return 0;
  }

  if (dbh) {
    return switch_cache_db_load_extension(dbh, extension);
  }

  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "DBH NOT Connected.\n");
  return 0;
}

JSON::JSON()
{
	_encode_empty_table_as_object = true;
}

JSON::~JSON()
{
}

void JSON::encode_empty_table_as_object(bool flag)
{
	_encode_empty_table_as_object = flag;
}

void JSON::return_unformatted_json(bool flag)
{
	_return_unformatted_json = flag;
}

cJSON *JSON::decode(const char *str)
{
	cJSON *json = cJSON_Parse(str);
	return json;
}

#define ADDITEM(json, k, v) do { \
	if (return_array > 0) { cJSON_AddItemToArray(json, v);} else { cJSON_AddItemToObject(json, k, v); } \
} while (0)

void JSON::LuaTable2cJSON(lua_State *L, int index, cJSON **json)
{
	int return_array = -1;

    // Push another reference to the table on top of the stack (so we know
    // where it is, and this function can work for negative, positive and
    // pseudo indices
    lua_pushvalue(L, index);
    // stack now contains: -1 => table
    lua_pushnil(L);
    // stack now contains: -1 => nil; -2 => table
    while (lua_next(L, -2)) {
        // stack now contains: -1 => value; -2 => key; -3 => table
        // copy the key so that lua_tostring does not modify the original
        lua_pushvalue(L, -2);
        // stack now contains: -1 => key; -2 => value; -3 => key; -4 => table

        const char *key = lua_tostring(L, -1);
        // switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "key: %s\n", key);

		if (return_array < 0) {
			if (lua_isnumber(L, -1) && lua_tonumber(L, -1) == 1) {
				return_array = 1;
				*json = cJSON_CreateArray();
			} else {
				return_array = 0;
				*json = cJSON_CreateObject();
			}
		}

		switch_assert(*json);

		if (lua_type(L, -2) == LUA_TNUMBER) {
			ADDITEM(*json, key, cJSON_CreateNumber(lua_tonumber(L, -2)));
		} else if (lua_type(L, -2) == LUA_TSTRING) {
			ADDITEM(*json, key, cJSON_CreateString(lua_tostring(L, -2)));
		} else if (lua_type(L, -2) == LUA_TBOOLEAN) {
			ADDITEM(*json, key, cJSON_CreateBool(lua_toboolean(L, -2)));
		} else if (lua_isnil(L, -2)) {
			ADDITEM(*json, key, cJSON_CreateNull());
		} else if (lua_isnone(L, -2)) {
			// ADDITEM(*json, key, cJSON_CreateNone());
		} else if (lua_istable(L, -2)) {
			cJSON *child = NULL;
			LuaTable2cJSON(L, -2, &child);
			if (child) {
				ADDITEM(*json, key, child);
			} else { // empty table?
				ADDITEM(*json, key, _encode_empty_table_as_object ? cJSON_CreateObject() : cJSON_CreateArray());
			}
		}

        // pop value + copy of key, leaving original key
        lua_pop(L, 2);
        // stack now contains: -1 => key; -2 => table
    }

    // stack now contains: -1 => table (when lua_next returns 0 it pops the key
    // but does not push anything.)
    // Pop table
    lua_pop(L, 1);
    // Stack is now the same as it was on entry to this function
}

std::string JSON::encode(SWIGLUA_TABLE lua_table)
{
	lua_State *L = lua_table.L;
	cJSON *json = NULL;

	luaL_checktype(L, lua_table.idx, LUA_TTABLE);
	LuaTable2cJSON(L, -1, &json);

	if (!json) {
		json = _encode_empty_table_as_object ? cJSON_CreateObject() : cJSON_CreateArray();
	}

	char *s = _return_unformatted_json ? cJSON_PrintUnformatted(json) : cJSON_Print(json);
	std::string result = std::string(s);
	free(s);
	cJSON_Delete(json);
	return result;
}

int JSON::cJSON2LuaTable(lua_State *L, cJSON *json) {
	cJSON *current = NULL;

	if (!json) return 0;

	lua_newtable(L);

	if (json->type == cJSON_Object) {
		for (current = json->child; current; current = current->next) {
			// printf("type: %d %s\n", current->type, current->string);
			switch (current->type) {
				case cJSON_String:
					lua_pushstring(L, current->valuestring);
					lua_setfield(L, -2, current->string);
					break;
				case cJSON_Number:
					lua_pushnumber(L, current->valuedouble);
					lua_setfield(L, -2, current->string);
					break;
				case cJSON_True:
					lua_pushboolean(L, 1);
					lua_setfield(L, -2, current->string);
					break;
				case cJSON_False:
					lua_pushboolean(L, 0);
					lua_setfield(L, -2, current->string);
					break;
				case cJSON_Object:
					JSON::cJSON2LuaTable(L, current);
					lua_setfield(L, -2, current->string);
					break;
				case cJSON_Array:
					JSON::cJSON2LuaTable(L, current);
					lua_setfield(L, -2, current->string);
					break;
				default:
					break;
			}
		}
	} else if (json->type == cJSON_Array) {
		int i = 1;

		for (current = json->child; current; current = current->next) {
			// printf("array type: %d %s\n", current->type, current->valuestring);
			switch (current->type) {
				case cJSON_String:
					lua_pushinteger(L, i++);
					lua_pushstring(L, current->valuestring);
					lua_settable(L, -3);
					break;
				case cJSON_Number:
					lua_pushinteger(L, i++);
					lua_pushnumber(L, current->valuedouble);
					lua_settable(L, -3);
					break;
				case cJSON_True:
					lua_pushinteger(L, i++);
					lua_pushboolean(L, 1);
					lua_settable(L, -3);
					break;
				case cJSON_False:
					lua_pushinteger(L, i++);
					lua_pushboolean(L, 0);
					lua_settable(L, -3);
					break;
				case cJSON_Object:
					lua_pushinteger(L, i++);
					JSON::cJSON2LuaTable(L, current);
					lua_settable(L, -3);
					break;
				default:
					break;
			}
		}
	}

	return 1;
}

cJSON *JSON::execute(const char *str)
{
	cJSON *cmd = cJSON_Parse(str);
	cJSON *reply = NULL;

	if (cmd) {
		switch_json_api_execute(cmd, NULL, &reply);
	}

	cJSON_Delete(cmd);

	return reply;
}

cJSON *JSON::execute(SWIGLUA_TABLE table)
{
	lua_State *L = table.L;
	cJSON *json = NULL;
	cJSON *reply = NULL;

	luaL_checktype(L, table.idx, LUA_TTABLE);
	LuaTable2cJSON(L, -1, &json);

	switch_json_api_execute(json, NULL, &reply);
	cJSON_Delete(json);
	return reply;
}

std::string JSON::execute2(const char *str)
{
    cJSON *reply = execute(str);
    char *s = _return_unformatted_json ? cJSON_PrintUnformatted(reply) : cJSON_Print(reply);
    std::string result = std::string(s);
    free(s);
    cJSON_Delete(reply);
    return result;
}

std::string JSON::execute2(SWIGLUA_TABLE table)
{
    cJSON *reply = execute(table);
    char *s = _return_unformatted_json ? cJSON_PrintUnformatted(reply) : cJSON_Print(reply);
    std::string result = std::string(s);
    free(s);
    cJSON_Delete(reply);
    return result;
}

static switch_bool_t cJSON_GetObjectBool(const cJSON *object, const char *string) {
	cJSON *cj = cJSON_GetObjectItem(object, string);

	if (cj && cj->type == cJSON_True) return SWITCH_TRUE;

	return SWITCH_FALSE;
}

static int str_end_with(const char *s1, const char *s2)
{
	char *p = const_cast<char *>(strrchr(s1, *s2));
	return p && (strcmp(p, s2) == 0);
}

static int is_directory(const char *path) {
	struct stat statbuf;
	if (zstr(path)) return 0;
	if (stat(path, &statbuf) != 0) return 0;
	return S_ISDIR(statbuf.st_mode);
}

static const char *find_ext(const char *in)
{
	const char *p = in + (strlen(in) - 1);

	while(p >= in && *p) {
		if (*p == '/') return NULL;
		if (*p == '.') return (p+1);
		p--;
	}

	return NULL;
}

static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
	register unsigned int realsize = (unsigned int) (size * nmemb);
	cURL *curl = (cURL *)data;
	curl->save(ptr, realsize);

	return realsize;
}

static size_t lua_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
	cURL *curl = (cURL *)data;
	int ret = 0;
	register unsigned int realsize = (unsigned int) (size * nmemb);
	SWIGLUA_FN *lua_fun = curl->get_fun();

	// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "lua callback %d\n", realsize);

	lua_pushvalue(lua_fun->L, lua_fun->idx); /* get the lua callback function onto the stack */
	lua_pushlstring(lua_fun->L, (char *)ptr, realsize);

	if ((ret = docall(lua_fun->L, 1, 1, 1, 0))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Lua function returned error: %d\n", ret);
		return 0;
	}

	ret = lua_tonumber(lua_fun->L, -1);
	lua_pop(lua_fun->L, 1);

	return realsize;
}

static size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
	char *p = buffer;
	const char *key = buffer;
	const char *val = NULL;
	size_t len = size * nitems;
	cURL *curl = (cURL *)userdata;

	/* received header is nitems * size long in 'buffer' NOT ZERO TERMINATED */
	// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "%s\n", buffer);

	while (1) {
		if (*p == ':') {
			*p++ = '\0';
			len--;

			while (*p == ' ' && len > 0) {
				p++; // skip any SPACEs
				len--;
			}

			val = p;
		}

		if (*p == '\r' || *p == '\n') {
			*p = '\0';
			break;
		}

		if (--len == 0) break;

		p++;
	}

	if (val) {
		curl->add_response_header(key, val);
	}

	return nitems * size;
}

static size_t download_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
	register unsigned int realsize = (unsigned int) (size * nmemb);
	cURL *curl = (cURL *)data;
	int x, wrote = 0, sanity = 1000;
	unsigned char *buffer = (unsigned char *) ptr;
	int fd = curl->get_fd();
	switch_size_t bytes = curl->get_bytes();
	switch_size_t max_bytes = curl->get_max_bytes();

	bytes += realsize;
	curl->set_bytes(bytes);

	if (bytes > max_bytes) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Oversized file detected [%" SWITCH_SIZE_T_FMT " bytes]\n", bytes);
		return 0;
	}

	do {
		x = write(fd, buffer + wrote, realsize - wrote);
		// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Successful read %d bytes\n", x);
		if (x > 0) {
			wrote += x;
		} else {
			switch_cond_next();
		}
	} while ((unsigned)wrote != realsize && (x == -1 && (errno == EAGAIN || errno == EINTR)) && --sanity);

	if (wrote != (int) realsize) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Short write! fd:%d %d out of %d [%s]\n", fd, wrote, realsize, strerror(errno));
	}

	return wrote;
}

static char *generate_download_write_path(const char *url, const char *save_path)
{
	const char *newext = NULL;
	const char *ext = NULL;
	char *write_path = NULL;
	time_t rawtime;
	struct tm * lt;
	time (&rawtime);
	lt = localtime(&rawtime);

	/* save_path is directory */
	if (is_directory(save_path)) {
		char *digest = NULL;
		ext = find_ext(url);
		if (newext && (!(ext && !strcmp(ext, newext)))) {
			ext = newext;
		}

		if (zstr(ext)) ext = "raw";

		digest = (char *) malloc((SWITCH_MD5_DIGEST_STRING_SIZE + 1) * sizeof(char));
		switch_md5_string(digest, (void *) url, strlen(url));
		write_path = switch_mprintf("%s%s%d%d%d%d%d%d-%s.%s",save_path, str_end_with(save_path, SWITCH_PATH_SEPARATOR) ? "" : SWITCH_PATH_SEPARATOR, lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec, digest, ext);
		switch_safe_free(digest);
		return write_path;
	}

	/* save_path is normal file */
	if (strstr(save_path, SWITCH_PATH_SEPARATOR)) {// /tmp/a.wav
		write_path = switch_mprintf("%s", save_path);
	} else {// a.wav
		write_path = switch_mprintf("%s%s%d%d%d%d%d%d-%s", SWITCH_GLOBAL_dirs.temp_dir, SWITCH_PATH_SEPARATOR, lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec, save_path);
	}

	return write_path;
}


static void *SWITCH_THREAD_FUNC multi_thread_run(switch_thread_t *thread, void *obj)
{
	cURL *curl = (cURL *)obj;

	long code = 0;

	CURLMsg *msg = NULL; /* for picking up messages with the transfer status */
	int msgs_left = 0; /* how many messages are left */
	int still_running = 1;
	int repeats = 0;
	CURLMcode mc = CURLM_OK;

	curl->inc_threads();

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "curl_multi thread run\n");

	while (still_running) {
		int numfds;

		mc = curl_multi_perform(curl->multi_handle, &still_running);

		if(mc != CURLM_OK) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "curl multi perform failed, %s[%d]\n", curl_multi_strerror(mc), mc);
			goto end;
		}

		mc = curl_multi_wait(curl->multi_handle, NULL, 0, 100, &numfds);

		if(mc != CURLM_OK) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "curl multi wait failed, %s[%d]\n", curl_multi_strerror(mc), mc);
			goto end;
		}
	}

	while((msg = curl_multi_info_read(curl->multi_handle, &msgs_left))) {
		if(msg->msg == CURLMSG_DONE && msgs_left == 0) break;
	}

	if (curl->get_channel_uuid()) {
		switch_core_session_t *session = NULL;

		session = switch_core_session_locate(curl->get_channel_uuid());

		if (session) {
			switch_event_t *event = NULL;

			if (switch_event_create(&event, SWITCH_EVENT_CLONE) == SWITCH_STATUS_SUCCESS) {
				switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Event-Type", "lua::curl");
				switch_event_add_header_string(event, SWITCH_STACK_BOTTOM, "Unique-ID", curl->get_channel_uuid());

				switch_event_add_body(event, "%s", curl->get_body().c_str());

				if (switch_core_session_queue_event(session, &event) != SWITCH_STATUS_SUCCESS) {
					switch_log_printf(SWITCH_CHANNEL_UUID_LOG(curl->get_channel_uuid()), SWITCH_LOG_ERROR, "Error delivering event!\n");
					switch_event_destroy(&event);
				}
			}

			switch_core_session_rwunlock(session);
		}
	}

end:
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "curl_multi thread done\n");
	curl->dec_threads();

	return NULL;
}

cURL::cURL()
{
	stream.data = NULL;
	handle = switch_curl_easy_init();
	switch_assert(handle);
	_response_header_event = NULL;

	_pool = NULL;
	_multi_thread = NULL;
	multi_handle = NULL;
	_n_threads = 0;

	reset_state();
}

cURL::~cURL()
{
	reset_state();

	if (stream.data) {
		free(stream.data);
		stream.data = NULL;
	}

	if (multi_handle) {
		if (handle) {
			curl_multi_remove_handle(multi_handle, handle);
		}
		curl_multi_cleanup(multi_handle);
		multi_handle = NULL;
	}

	if (handle) {
		switch_curl_easy_cleanup(handle);
	}

	if (_pool) {
		switch_core_destroy_memory_pool(&_pool);
	}
}

void cURL::setLUA(lua_State * state)
{
	L = state;
}

lua_State *cURL::getLUA()
{
	if (!L) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Doh!\n");
	}
	return L;
}

void cURL::set_fun(SWIGLUA_FN *lua_fun)
{
	_lua_fun.L = lua_fun->L;
	_lua_fun.idx = lua_fun->idx;
}

void cURL::reset()
{
	if (_multi_thread) {
		switch_status_t status;
		switch_thread_join(&status, _multi_thread);
		_multi_thread = NULL;
	}

	if (multi_handle) {
		curl_multi_remove_handle(multi_handle, handle);
		curl_multi_cleanup(multi_handle);
		multi_handle = NULL;
	}

	curl_easy_reset(handle);

	// set default params
	switch_curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 3);
	switch_curl_easy_setopt(handle, CURLOPT_TIMEOUT, 5);
	switch_curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0);
	switch_curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0);

	switch_curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
	switch_curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 15);
	switch_curl_easy_setopt(handle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	switch_curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
	switch_curl_easy_setopt(handle, CURLOPT_USERAGENT, "freeswitch-lua-curl/1.8");

	reset_state();
}

void cURL::reset_state()
{
	if (_response_header_event) switch_event_destroy(&_response_header_event);

	if (stream.data) {
		free(stream.data);
		stream.data = NULL;
	}

	SWITCH_STANDARD_STREAM(stream);

	memset(_channel_uuid, 0, sizeof(_channel_uuid));

	if (_multi_thread) {
		switch_status_t status;
		switch_thread_join(&status, _multi_thread);
		_multi_thread = NULL;
	}

	_bytes = 0;
	_max_bytes = 0;
	_fd = -1;
}

char* cURL::parse_table(SWIGLUA_TABLE table, cJSON *reply, switch_curl_slist_t **used_headers)
{
	const char *url = NULL;;
	const char *method = NULL;
	lua_State *L = table.L;
	switch_curl_slist_t *headers = NULL;
	char *post_data = NULL;
	int is_get = 1;

	SWIGLUA_TABLE_GET(table);

	lua_getfield(L, -1, "url");
	url = lua_tostring(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, -1, "method");
	method = lua_tostring(L, -1);

	if (method) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Request method: %s\n", method);

		if (!strcasecmp(method, "POST") || !strcasecmp(method, "PUT") || !strcasecmp(method, "PATCH") || !strcasecmp(method, "DELETE")) {
			switch_curl_easy_setopt(handle, CURLOPT_HTTPGET, 0);
			switch_curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, method);
			is_get = 0;
		} else {
			switch_curl_easy_setopt(handle, CURLOPT_HTTPGET, 1);
			is_get = 1;
		}
	}

	lua_pop(L, 1);

	lua_getfield(L, -1, "headers");

	if (lua_istable(L, -1)) {
		lua_pushnil(L);
		// stack now contains: -1 => nil; -2 => headers
		while (lua_next(L, -2)) {
			// stack now contains: -1 => value; -2 => key; -3 => table
			// copy the key so that lua_tostring does not modify the original
			lua_pushvalue(L, -2);
			// stack now contains: -1 => key; -2 => value; -3 => key; -4 => table

			const char *key = lua_tostring(L, -1);
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "key: %s\n", key);

			if (lua_isstring(L, -2)) { // value
				char header[1024];
				const char *value = lua_tostring(L, -2);

				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "value: %s\n", value);
				switch_snprintf(header, sizeof(header), "%s: %s", key, value);
				headers = switch_curl_slist_append(headers, header);
			}

			lua_pop(L, 2);
		}
	}

	lua_pop(L, 1);

	lua_getfield(L, -1, "dataType");
	const char *dataType = lua_tostring(L, -1);

	if (dataType) {
		char header[1024];
		switch_snprintf(header, sizeof(header), "Content-Type: %s", dataType);
		headers = switch_curl_slist_append(headers, header);
	}

	if (headers) {
		switch_curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
		*used_headers = headers;
		headers = NULL;
	}

	lua_pop(L, 1);

	lua_getfield(L, -1, "data");

	if (lua_isstring(L, -1)) {
		size_t datalen = 0;
		const char *data = lua_tolstring(L, -1, &datalen);

		// todo need to dup this before it's get invalid in stack?

		if (data && datalen) {
			switch_curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, datalen);
			switch_curl_easy_setopt(handle, CURLOPT_POSTFIELDS, data);
		}
	} else if (!is_get && lua_istable(L, -1) && dataType && switch_stristr("json", dataType)) {
		cJSON *json = NULL;
		JSON *JS = new JSON();
		JS->LuaTable2cJSON(L, -1, &json);
		delete(JS);
		post_data = cJSON_Print(json);
		cJSON_Delete(json);
		switch_curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, strlen(post_data));
		switch_curl_easy_setopt(handle, CURLOPT_POSTFIELDS, post_data);
	}

	lua_pop(L, 1);

	lua_getfield(L, -1, "savePath");

	if (lua_isstring(L,-1)) {
		const char *save_path = lua_tostring(L, -1);

		_fd = open(save_path, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP |S_IROTH);

		if (_fd < 0) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Failed to open file %s\n", save_path);
		} else {
			cJSON_AddStringToObject(reply, "save_path", save_path);
		}
	}

	lua_pop(L, 1);

	lua_getfield(L, -1, "uploadfile");

	if (lua_isstring(L,-1)) {
		uploadfile = lua_tostring(L, -1);
	}

	lua_pop(L, 1);

	lua_getfield(L, -1, "async");

	if (lua_isboolean(L, -1)) {
		if (lua_toboolean(L, -1) && !multi_handle) { // async
			multi_handle = curl_multi_init();
			switch_assert(multi_handle);

			curl_multi_add_handle(multi_handle, handle);
		}
	}

	lua_pop(L, 1);
	lua_getfield(L, -1, "channel_uuid");

	if (lua_isstring(L, -1)) {
		switch_set_string(_channel_uuid, lua_tostring(L, -1));
	}

	lua_pop(L, 1);

	switch_curl_easy_setopt(handle, CURLOPT_URL, url);
	switch_curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_callback);
	switch_curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)this);

	return post_data;
}

std::string cURL::get_body()
{
	std::string s;

	if (stream.data) {
		s = std::string((char *)stream.data, stream.data_len);
	}

	return s;
}

void cURL::save(void *ptr, size_t size)
{
	stream.raw_write_function(&stream, (uint8_t *)ptr, size);
}

switch_CURL *cURL::get_handle()
{
	return handle;
}

SWIGLUA_FN *cURL::get_fun()
{
	return &_lua_fun;
}

int cURL::get_response_code()
{
	int code = 0;
	switch_curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &code);
	return code;
}

char *cURL::get_content_type()
{
	char *content_type = NULL;
	switch_curl_easy_getinfo(handle, CURLINFO_CONTENT_TYPE, &content_type);
	return content_type;
}

switch_size_t cURL::get_content_length()
{
	double content_length = 0;
	switch_curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);
	return (switch_size_t)content_length;
}

char *cURL::get_info_string(int info)
{
	char *ret = NULL;
	switch_curl_easy_getinfo(handle, info, &ret);
	return ret;
}

int cURL::get_info_int(int info)
{
	int ret = 0;
	switch_curl_easy_getinfo(handle, info, &ret);
	return ret;
}

long cURL::get_info_long(int info)
{
	long ret = 0;
	switch_curl_easy_getinfo(handle, info, &ret);
	return ret;
}

double cURL::get_info_double(int info)
{
	double ret = 0;
	switch_curl_easy_getinfo(handle, info, &ret);
	return ret;
}

char *cURL::gen_save_path(const char *url, const char *save_path)
{
	return generate_download_write_path(url, save_path);
}

switch_CURLcode cURL::set_timeout(int timeout)
{
	return switch_curl_easy_setopt(handle, CURLOPT_TIMEOUT, timeout);
}

switch_CURLcode cURL::set_connect_timeout(int timeout)
{
	return switch_curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, timeout);
}

switch_CURLcode cURL::set_basic_auth(const char *user, const char *pass)
{
	char user_pass[128];
	switch_snprintf(user_pass, sizeof(user_pass), "%s:%s", user, pass);
	curl_easy_setopt(handle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	curl_easy_setopt(handle, CURLOPT_USERPWD, user_pass);

	return CURLE_OK;
}

switch_CURLcode cURL::set_bearer_auth(const char *token)
{
#ifdef CURLAUTH_BEARER
	curl_easy_setopt(handle, CURLOPT_HTTPAUTH, CURLAUTH_BEARER);
	curl_easy_setopt(handle, CURLOPT_XOAUTH2_BEARER, token);
#else
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Bearer Auth not supported\n");
#endif
	return CURLE_OK;
}

switch_CURLcode cURL::setopt(int opt, char *val)
{
	return switch_curl_easy_setopt(handle, (CURLoption)opt, val);
}

switch_CURLcode cURL::setopt(int opt, long val)
{
	return switch_curl_easy_setopt(handle, (CURLoption)opt, val);
}

switch_CURLcode cURL::setopt(int opt, SWIGLUA_FN lua_fun)
{
	switch_CURLcode code = CURLE_OK;

	if (lua_fun.L) {
		code = switch_curl_easy_setopt(handle, (CURLoption)opt, lua_callback);
		set_fun(&lua_fun);

		if (opt == CURLOPT_WRITEFUNCTION) {
			switch_curl_easy_setopt(handle, CURLOPT_WRITEDATA, this);
		}
	}

	return code;
}

switch_CURLcode cURL::perform()
{
	return switch_curl_easy_perform(handle);
}

void cURL::add_response_header(const char *k, const char *v)
{
	if (!this->_response_header_event) {
		switch_event_create(&_response_header_event, SWITCH_EVENT_CLONE);
	}

	switch_assert(_response_header_event);
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG10, "add header [%s=%s]\n", k, v);
	switch_event_add_header_string(_response_header_event, SWITCH_STACK_BOTTOM, k, v);
}

Event *cURL::get_response_headers() {
	lua_State *L = getLUA();

	if (!_response_header_event) {
		return NULL;
	}

	if (!L) return NULL;

	// will be freed by the %newobject
	return new Event(_response_header_event);
}

switch_size_t cURL::get_bytes()
{
	return _bytes;
}

switch_size_t cURL::get_max_bytes()
{
	return _max_bytes;
}

cJSON *cURL::get(const char *url)
{
	long code = 0;
	double content_length = 0;
	char *content_type = NULL;
	cJSON *reply = cJSON_CreateObject();
	switch_CURLcode ret;

	switch_curl_easy_setopt(handle, CURLOPT_HTTPGET, 1);
	switch_curl_easy_setopt(handle, CURLOPT_URL, url);
	switch_curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_callback);
	switch_curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)this);

	ret = switch_curl_easy_perform(handle);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "code: %ld, content_type: %s, len: %f\n", code, content_type, content_length);

	if (ret != CURLE_OK) {
		cJSON_AddNumberToObject(reply, "code", 0 - ret);
		cJSON_AddStringToObject(reply, "error", switch_curl_easy_strerror(ret));
		cJSON_AddBoolToObject(reply, "ok", 0);
	} else {
		switch_curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &code);
		switch_curl_easy_getinfo(handle, CURLINFO_CONTENT_TYPE, &content_type);
		switch_curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);

		cJSON_AddNumberToObject(reply, "code", code);
		cJSON_AddBoolToObject(reply, "ok", (code > 199 && code < 300) ? 1 : 0);
		cJSON_AddNumberToObject(reply, "content_length", content_length);
		cJSON_AddStringToObject(reply, "content_type", content_type);
	}

	return reply;
}

cJSON *cURL::get(SWIGLUA_TABLE table)
{
	switch_curl_easy_setopt(handle, CURLOPT_HTTPGET, 1);
	return request(table);
}

cJSON *cURL::get(const char *url, SWIGLUA_FN fun)
{
	long code = 0;
	double content_length = 0;
	char *content_type = NULL;
	cJSON *reply = cJSON_CreateObject();
	switch_CURLcode ret;

	set_fun(&fun);

	switch_curl_easy_setopt(handle, CURLOPT_HTTPGET, 1);
	switch_curl_easy_setopt(handle, CURLOPT_URL, url);
	switch_curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, lua_callback);
	switch_curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)this);
	switch_curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, header_callback);
	switch_curl_easy_setopt(handle, CURLOPT_HEADERDATA, (void *)this);

	ret = switch_curl_easy_perform(handle);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "code: %ld, content_type: %s, len: %f\n", code, content_type, content_length);

	if (ret != CURLE_OK) {
		cJSON_AddNumberToObject(reply, "code", 0 - ret);
		cJSON_AddStringToObject(reply, "error", switch_curl_easy_strerror(ret));
		cJSON_AddBoolToObject(reply, "ok", 0);
	} else {
		switch_curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &code);
		switch_curl_easy_getinfo(handle, CURLINFO_CONTENT_TYPE, &content_type);
		switch_curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);

		cJSON_AddNumberToObject(reply, "code", code);
		cJSON_AddBoolToObject(reply, "ok", (code > 199 && code < 300) ? 1 : 0);
		cJSON_AddNumberToObject(reply, "content_length", content_length);
		cJSON_AddStringToObject(reply, "content_type", content_type);
	}

	return reply;
}

cJSON *cURL::download(const char *url, const char *save_path)
{
	SWIGLUA_TABLE table;
	lua_State *L = NULL;

	if (!save_path) { // todo return more data
		return cJSON_CreateObject();
	}

	L = luaL_newstate(); //lua_init();
	table.L = L;
	table.idx = -1;
	lua_newtable(L);

	char *write_path = generate_download_write_path(url, save_path);

	lua_pushstring(L, "url");
	lua_pushstring(L, url);
	lua_rawset(L, -3);
	lua_pushstring(L, "savePath");
	lua_pushstring(L, write_path);
	lua_rawset(L, -3);

	cJSON *reply = download(table);
	lua_gc(L, LUA_GCCOLLECT, 0);
	lua_close(L);
	free(write_path);

	return reply;
}

cJSON *cURL::download(SWIGLUA_TABLE table)
{
	long code = 0;
	double content_length = 0;
	char *content_type = NULL;
	cJSON *reply = NULL;
	switch_CURLcode ret;
	char *post_data = NULL;
	switch_curl_slist_t *used_headers = NULL;

	_fd = -1;
	_bytes = 0;

	if (_max_bytes == 0) {
		_max_bytes = 1024 * 1024 * 2048L;
	}

	post_data = parse_table(table, reply, &used_headers);

	switch_curl_easy_setopt(handle, CURLOPT_HTTPGET, 1);
	switch_curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, download_callback);
	switch_curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)this);

	if (multi_handle) {
		switch_threadattr_t *thd_attr = NULL;

		if (!_pool) {
			switch_core_new_memory_pool(&_pool);
		}

		switch_threadattr_create(&thd_attr, _pool);
		// switch_threadattr_detach_set(thd_attr, 1);
		switch_threadattr_stacksize_set(thd_attr, SWITCH_THREAD_STACKSIZE);
		// switch_threadattr_priority_set(thd_attr, SWITCH_PRI_REALTIME);
		switch_thread_create(&_multi_thread, thd_attr, multi_thread_run, (void *)this, _pool);
		switch_assert(post_data == NULL);
		return NULL;
	} else {
		ret = switch_curl_easy_perform(handle);

		if (used_headers) {
			switch_curl_slist_free_all(used_headers);
			used_headers = NULL;
		}
	}

	reply = cJSON_CreateObject();
	switch_assert(reply);

	if (ret != CURLE_OK) {
		cJSON_AddNumberToObject(reply, "code", 0 - ret);
		cJSON_AddStringToObject(reply, "error", switch_curl_easy_strerror(ret));
		cJSON_AddBoolToObject(reply, "ok", 0);
	} else {
		switch_curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &code);
		switch_curl_easy_getinfo(handle, CURLINFO_CONTENT_TYPE, &content_type);
		switch_curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);

		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "code: %ld, content_type: %s, len: %f\n", code, content_type, content_length);

		cJSON_AddNumberToObject(reply, "code", code);
		cJSON_AddBoolToObject(reply, "ok", (code > 199 && code < 300) ? 1 : 0);
		cJSON_AddNumberToObject(reply, "content_length", content_length);
		cJSON_AddStringToObject(reply, "content_type", content_type);
	}

	if (_fd > -1) {
		close(_fd);
		_fd = -1;
	}

	switch_safe_free(post_data);
	return reply;
}

cJSON *cURL::upload(SWIGLUA_TABLE table)
{
	long code = 0;
	double content_length = 0;
	char *content_type = NULL;
	cJSON *reply = cJSON_CreateObject();
	switch_CURLcode ret;
	char *post_data = NULL;
	struct curl_httppost *formpost = NULL;
	struct curl_httppost *lastptr = NULL;
	switch_curl_slist_t *used_headers = NULL;

	uploadfile = NULL;

	post_data = parse_table(table, reply, &used_headers);

	if (uploadfile == NULL) return NULL;

	curl_formadd(&formpost, &lastptr,
		CURLFORM_COPYNAME, "uploadfile",
		CURLFORM_FILE, uploadfile,
		CURLFORM_END
	);

	switch_curl_easy_setopt(handle, CURLOPT_HTTPGET, 0);
	switch_curl_easy_setopt(handle, CURLOPT_HTTPPOST, formpost);

	ret = switch_curl_easy_perform(handle);

	if (used_headers) {
		curl_slist_free_all(used_headers);
		used_headers = NULL;
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "code: %ld, content_type: %s, len: %f\n", code, content_type, content_length);

	if (ret != CURLE_OK) {
		cJSON_AddNumberToObject(reply, "code", 0 - ret);
		cJSON_AddStringToObject(reply, "error", switch_curl_easy_strerror(ret));
		cJSON_AddBoolToObject(reply, "ok", 0);
	} else {
		switch_curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &code);
		switch_curl_easy_getinfo(handle, CURLINFO_CONTENT_TYPE, &content_type);
		switch_curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);

		cJSON_AddNumberToObject(reply, "code", code);
		cJSON_AddBoolToObject(reply, "ok", (code > 199 && code < 300) ? 1 : 0);
		cJSON_AddNumberToObject(reply, "content_length", content_length);
		cJSON_AddStringToObject(reply, "content_type", content_type);
	}

    curl_formfree(formpost);
	switch_safe_free(post_data);
	return reply;
}

cJSON *cURL::post(SWIGLUA_TABLE table) {
	switch_curl_easy_setopt(handle, CURLOPT_HTTPGET, 0);
	return request(table);
}

cJSON *cURL::put(SWIGLUA_TABLE table) {
	switch_curl_easy_setopt(handle, CURLOPT_HTTPGET, 0);
	switch_curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "PUT");
	return request(table);
}

cJSON *cURL::patch(SWIGLUA_TABLE table) {
	switch_curl_easy_setopt(handle, CURLOPT_HTTPGET, 0);
	switch_curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "PATCH");
	return request(table);
}

cJSON *cURL::Delete(SWIGLUA_TABLE table) {
	switch_curl_easy_setopt(handle, CURLOPT_HTTPGET, 0);
	switch_curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "DELETE");
	return request(table);
}

cJSON *cURL::request(SWIGLUA_TABLE table)
{
	long code = 0;
	double content_length = 0;
	char *content_type = NULL;
	cJSON *reply = NULL;
	switch_CURLcode ret;
	switch_curl_slist_t *used_headers = NULL;
	char *post_data = parse_table(table, reply, &used_headers);

	if (multi_handle) {
		switch_threadattr_t *thd_attr = NULL;

		if (!_pool) {
			switch_core_new_memory_pool(&_pool);
		}

		switch_threadattr_create(&thd_attr, _pool);
		// switch_threadattr_detach_set(thd_attr, 1);
		switch_threadattr_stacksize_set(thd_attr, SWITCH_THREAD_STACKSIZE);
		// switch_threadattr_priority_set(thd_attr, SWITCH_PRI_REALTIME);
		switch_thread_create(&_multi_thread, thd_attr, multi_thread_run, (void *)this, _pool);
		switch_assert(post_data == NULL);
		return NULL;
	} else {
		ret = switch_curl_easy_perform(handle);

		if (used_headers) {
			curl_slist_free_all(used_headers);
		}
	}

	switch_safe_free(post_data);
	post_data = NULL;
	reply = cJSON_CreateObject();
	switch_assert(reply);

	if (ret != CURLE_OK) {
		cJSON_AddNumberToObject(reply, "code", 0 - ret);
		cJSON_AddStringToObject(reply, "error", switch_curl_easy_strerror(ret));
		cJSON_AddBoolToObject(reply, "ok", 0);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "code: %d, err: %s\n", 0 - ret, switch_curl_easy_strerror(ret));
	} else {
		switch_curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &code);
		switch_curl_easy_getinfo(handle, CURLINFO_CONTENT_TYPE, &content_type);
		switch_curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "code: %ld, content_type: %s, len: %f\n", code, content_type, content_length);

		cJSON_AddNumberToObject(reply, "code", code);
		cJSON_AddBoolToObject(reply, "ok", (code > 199 && code < 300) ? 1 : 0);
		cJSON_AddNumberToObject(reply, "content_length", content_length);
		cJSON_AddStringToObject(reply, "content_type", content_type);
	}

	return reply;
}
