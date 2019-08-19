#include <mod_lua_utils.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

// copied from switch_xml.c, should keep in sync

static char *expand_vars(char *buf, char *ebuf, switch_size_t elen, switch_size_t *newlen, const char **err)
{
	char *var, *val;
	char *rp = buf;
	char *wp = ebuf;
	char *ep = ebuf + elen - 1;

	if (!(var = strstr(rp, "$${"))) {
		*newlen = strlen(buf);
		strcpy(ebuf, buf);
		return ebuf;
	}

	rp = buf = switch_must_strdup(buf);

	while (*rp && wp < ep) {
		if (*rp == '$' && *(rp + 1) == '$' && *(rp + 2) == '{') {
			char *e = switch_find_end_paren(rp + 2, '{', '}');

			if (e) {
				rp += 3;
				var = rp;
				*e++ = '\0';
				rp = e;
				if ((val = switch_core_get_variable_dup(var))) {
					char *p;
					for (p = val; p && *p && wp < ep; p++) {
						*wp++ = *p;
					}
					free(val);
				}
				continue;
			} else if (err) {
				*err = "unterminated ${var}";
			}
		}

		if (wp < ep) *wp++ = *rp++;
	}

	if (wp == ep) {
		free(buf);
		return NULL;
	}

	*wp++ = '\0';
	*newlen = strlen(ebuf);

	free(buf);
	return ebuf;
}

char *expand(char *buf)
{
	char *bp;
	switch_size_t eblen = strlen(buf) * 2;
	char *ebuf = switch_must_malloc(eblen);
	const char *err = NULL;
	switch_size_t newlen = 0;

	memset(ebuf, 0, eblen);

	while (!(bp = expand_vars(buf, ebuf, eblen, &newlen, &err))) {
		eblen *= 2;
		ebuf = switch_must_realloc(ebuf, eblen);
		memset(ebuf, 0, eblen);
	}

	if (err) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Parse Error [%s][%s]\n", buf, err);
		free(ebuf);
		return NULL;
	}

	return ebuf;
}

char *md5(const char *input)
{
	char *output = switch_must_malloc(SWITCH_MD5_DIGEST_STRING_SIZE);
	switch_md5_string(output, input, strlen(input));
	return output;
}

int match(const char *input, const char *pattern)
{
	switch_regex_t *re = NULL;
	int ovector[30];
	int proceed;
	int i;

	return switch_regex_perform(input, pattern, &re, ovector, sizeof(ovector) / sizeof(ovector[0]));
}
