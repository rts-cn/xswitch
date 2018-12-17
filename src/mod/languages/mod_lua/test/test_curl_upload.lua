json = freeswitch.JSON()
curl = freeswitch.cURL()

function kvlog(k, v)
    if not v then v = "__nil__" end

    if type(v) == "boolean" then v = serialize(v) end

    freeswitch.consoleLog("INFO", k .. ' ' .. v .. "\n")
end

function serialize(o)
	s = ""

	if type(o) == "number" then
		s = s .. o
	elseif type(o) == "string" then
		s = s .. string.format("%q", o)
	elseif type(o) == "table" then
		s = s .. "{\n"
		for k, v in pairs(o) do
			s = s .. '  ' .. k .. ' = '
			s = s .. serialize(v)
			s = s .. ",\n"
		end
		s = s .. "}"
	elseif type(o) == "boolean" then
		if o then
			s = s .. "true"
		else
			s = s .. "false"
		end
	else
		s = s .. " [" .. type(o) .. "]"
	end

	return s
end

curl:setopt(freeswitch.CURLOPT_VERBOSE, 1)

file1 = "test_mod_lua.c"
file2 = "test_curl.lua"

url = "http://httpbin.org/post"

ret = curl:upload({
	url = url,
	headers = {
		['X-Tested-By'] = 'Seven Du',
		['Content-Type'] = 'multipart/from-data'
	},
	uploadfile = file1
})

b = curl:get_body()
kvlog('url', upload_url)
kvlog('code', ret.code)
assert(ret.code == 200)
kvlog('ok', ret.ok)
kvlog('error',ret.error)
kvlog('content_type', ret.content_type)
kvlog('content_length', ret.content_length)
kvlog('body', b)

curl:reset_state();
ret = curl:upload({
	url = url,
	headers = {
		['X-Tested-By'] = 'Seven Du',
		['Content-Type'] = 'multipart/from-data'
	},
	uploadfile = file2
})

body = curl:get_body()
kvlog('url', upload_url)
kvlog('code', ret.code)
assert(ret.code == 200)
kvlog('ok', ret.ok)
kvlog('error',ret.error)
kvlog('content_type', ret.content_type)
kvlog('content_length', ret.content_length)
kvlog('body', body)

stream:write("+OK")
