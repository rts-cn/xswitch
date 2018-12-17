#ifndef FREESWITCH_LUA_H
#define FREESWITCH_LUA_H

extern "C" {
#include "lua.h"
#include <lauxlib.h>
#include <lualib.h>
#include "mod_lua_extra.h"
}
#include <switch_apr.h>
#include <switch_cpp.h>
#include <switch_curl.h>
#include <string>

#ifndef lua_pushglobaltable
#define lua_pushglobaltable(L) lua_pushvalue(L,LUA_GLOBALSINDEX)
#endif

typedef struct{
  lua_State* L;
  int idx;
}SWIGLUA_FN;

#define SWIGLUA_FN_GET(fn) {lua_pushvalue(fn.L,fn.idx);}

typedef struct{
  lua_State* L;
  int idx;
}SWIGLUA_TABLE;

#define SWIGLUA_TABLE_GET(fn) {lua_pushvalue(fn.L,fn.idx);}

typedef int DbhQueryRowsReturn;

namespace LUA {
	class Session:public CoreSession {
	  private:
		virtual void do_hangup_hook();
		lua_State *getLUA();
		lua_State *L;
		int hh;
		int mark;
	  public:
		    Session();
		    Session(char *uuid, CoreSession * a_leg = NULL);
		     Session(switch_core_session_t *session);
		                     ~Session();
		                      SWITCH_MOD_DECLARE(virtual void) destroy(const char *err = NULL);

		virtual bool begin_allow_threads();
		virtual bool end_allow_threads();
		virtual void check_hangup_hook();

		virtual switch_status_t run_dtmf_callback(void *input, switch_input_type_t itype);
		void unsetInputCallback(void);
		void setInputCallback(char *cbfunc, char *funcargs = NULL);
		void setHangupHook(char *func, char *arg = NULL);
		bool ready();
		int originate(CoreSession * a_leg_session, char *dest, int timeout);

		char *cb_function;
		char *cb_arg;
		char *hangup_func_str;
		char *hangup_func_arg;
		void setLUA(lua_State * state);

	};

  class Dbh {
    protected:
      switch_cache_db_handle_t *dbh;
      char *err;
      bool m_connected;
      static int query_callback(void *pArg, int argc, char **argv, char **cargv);
    public:
      Dbh(char *dsn, char *user = NULL, char *pass = NULL);
      ~Dbh();
      bool release();
      bool connected();
      bool test_reactive(char *test_sql, char *drop_sql = NULL, char *reactive_sql = NULL);
      bool query(char *sql, SWIGLUA_FN lua_fun);
      DbhQueryRowsReturn query_rows(lua_State* L, char *sql);
      int affected_rows();
      char *last_error();
      void clear_error();
      int load_extension(const char *extension);
  };

  class JSON {
    private:
		  bool _encode_empty_table_as_object;
		  bool _return_unformatted_json;
    public:
      JSON();
      ~JSON();
      cJSON *decode(const char *);
      std::string encode(SWIGLUA_TABLE table);
      cJSON *execute(const char *);
      cJSON *execute(SWIGLUA_TABLE table);
      std::string execute2(const char *);
      std::string execute2(SWIGLUA_TABLE table);
      void encode_empty_table_as_object(bool flag);
      void return_unformatted_json(bool flag);
      static int cJSON2LuaTable(lua_State *L, cJSON *json);
      void LuaTable2cJSON(lua_State *L, int index, cJSON **json);
  };

  class cURL {
    private:
      switch_event_t *_response_header_event;
      switch_CURL *handle;
      SWIGLUA_FN _lua_fun;
      switch_stream_handle_t stream;

      switch_memory_pool_t *_pool;
      switch_thread_t *_multi_thread;

      int _fd;
      switch_size_t _bytes;
      switch_size_t _max_bytes;
      const char *uploadfile;

      char _channel_uuid[128];
      int _n_threads;

      lua_State *L;
      lua_State *getLUA();

      void set_fun(SWIGLUA_FN *fun);
      cJSON *request(SWIGLUA_TABLE table);
      char* parse_table(SWIGLUA_TABLE table, cJSON *reply, switch_curl_slist_t **used_headers);
    public:
      cURL();
      ~cURL();
      CURLM *multi_handle;

      void setLUA(lua_State * state);

      cJSON *get(const char *url);
      cJSON *get(SWIGLUA_TABLE table);
      cJSON *get(const char *url, SWIGLUA_FN fun);
      cJSON *download(const char *url, const char *save_path);
      cJSON *download(SWIGLUA_TABLE table);
      cJSON *upload(SWIGLUA_TABLE table);

      cJSON *post(SWIGLUA_TABLE table);
      cJSON *put(SWIGLUA_TABLE table);
      cJSON *patch(SWIGLUA_TABLE table);
      // delete is a keyword in c++, use Delete instead
      cJSON *Delete(SWIGLUA_TABLE table);

      std::string get_body();

      void save(void *ptr, size_t size);

      switch_CURL *get_handle();
      SWIGLUA_FN *get_fun();

      char *get_info_string(int info);
      int get_info_int(int info);
      long get_info_long(int info);
      double get_info_double(int info);
      int get_response_code();
      char *get_content_type();
      switch_size_t get_content_length();
      char *gen_save_path(const char *url, const char *save_path);

      // reset everything to initial state
      void reset();
      // reset internal state except curl handle
      void reset_state();

      void set_max_bytes(switch_size_t max_bytes) {
        _max_bytes = (switch_size_t)max_bytes;
      }

      switch_CURLcode set_timeout(int timeout);
      switch_CURLcode set_connect_timeout(int timeout);
      switch_CURLcode set_basic_auth(const char *user, const char *pass);
      switch_CURLcode set_bearer_auth(const char *token);

      switch_CURLcode setopt(int opt, char *val);
      switch_CURLcode setopt(int opt, long val);
      switch_CURLcode setopt(int opt, SWIGLUA_FN lua_fun);
      switch_CURLcode perform();

      void add_response_header(const char *k, const char *v);
      Event *get_response_headers();

      bool is_finished() { return _n_threads == 0; };
      int get_fd() { return _fd; }

      switch_size_t get_bytes();
      switch_size_t get_max_bytes();
      void set_bytes(switch_size_t bytes) {
        _bytes = bytes;
      }

      void dec_threads() { _n_threads--; }
      void inc_threads() { _n_threads++; }

      void set_channel_uuid(const char *uuid) { switch_set_string(_channel_uuid, uuid); }
      const char *get_channel_uuid() { return _channel_uuid; }
  };
}
#endif
