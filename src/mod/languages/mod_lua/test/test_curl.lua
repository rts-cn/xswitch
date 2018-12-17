freeswitch.consoleLog("INFO", "curl ...\n")
curl = freeswitch.cURL()

url = "http://httpbin.org"

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

function md5(str)
	api = freeswitch.API()
	hash = api:execute("md5", str)
	return hash
end

-- encoding
function b64enc(data)
	local b='ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/' -- You will need this for encoding/decoding
	return ((data:gsub('.', function(x)
		local r,b='',x:byte()
		for i=8,1,-1 do r=r..(b%2^i-b%2^(i-1)>0 and '1' or '0') end
		return r;
	end)..'0000'):gsub('%d%d%d?%d?%d?%d?', function(x)
		if (#x < 6) then return '' end
		local c=0
		for i=1,6 do c=c+(x:sub(i,i)=='1' and 2^(6-i) or 0) end
		return b:sub(c+1,c+1)
	end)..({ '', '==', '=' })[#data%3+1])
end


curl:setopt(freeswitch.CURLOPT_VERBOSE, 1)

-- simple curl, returns CURLcode
curl:setopt(freeswitch.CURLOPT_URL, url)
ret = curl:perform()
print("ret .........." .. ret)
assert(ret == freeswitch.CURLE_OK)
code = curl:get_info_int(freeswitch.CURLINFO_RESPONSE_CODE)
kvlog(code)
assert(code == 200)
code = curl:get_response_code()
assert(code == 200)
body = curl:get_body()
kvlog("body: " .. body:len())

-- call reset_state for any new requests
curl:reset_state()

-- RESTful curl, returns a table or nil
ret = curl:get(url)
kvlog('code', ret.code)
assert(ret.code == 200)
kvlog('ok', ret.ok)
assert(ret.ok)
kvlog('error', ret.error)
kvlog('content_type', ret.content_type)
kvlog('content_length', ret.content_length)
kvlog('headers', serialize(ret.headers))

body = curl:get_body()
assert(body)

curl:reset_state()

ret = curl:get(url .. "/basic-auth/user/pass")
kvlog('url', url)
kvlog('code', ret.code)
assert(ret.code == 401)
kvlog('ok', ret.ok)
kvlog('error', ret.error)
kvlog('content_type', ret.content_type)
kvlog('content_length', ret.content_length)
kvlog('headers', serialize(ret.headers))
body = curl:get_body()
kvlog('body', body:len())

curl:reset_state()

-- puts params in a table
ret = curl:get({url = url .. "/basic-auth/user/pass", headers = {['Authorization'] = "Basic " .. b64enc("user:pass")}})
kvlog('url', url)
kvlog('code', ret.code)
assert(ret.code == 200)
kvlog('ok', ret.ok)
kvlog('error', ret.error)
kvlog('content_type', ret.content_type)
kvlog('content_length', ret.content_length)
kvlog('headers', serialize(ret.headers))
b = curl:get_body()
kvlog('body', b:len())

-- reset internal state and handle
curl:reset()

-- Baisc Auth
curl:setopt(freeswitch.CURLOPT_VERBOSE, 1)
curl:set_basic_auth("user", "pass")
ret = curl:get({url = url .. "/basic-auth/user/pass"})
kvlog('url', url)
kvlog('code', ret.code)
assert(ret.code == 200)
kvlog('ok', ret.ok)
kvlog('error', ret.error)
kvlog('content_type', ret.content_type)
kvlog('content_length', ret.content_length)
kvlog('headers', serialize(ret.headers))
b = curl:get_body()
kvlog('body', b:len())

curl:reset()

-- callback function
local save_file_name = "index.html"
local save_file = io.open(save_file_name, "w+")
assert(save_file)

ret = curl:get(url, function(data)
	content_length = curl:get_content_length()

	if (content_length and (not content_length_printed)) then
		freeswitch.consoleLog("INFO", "content_length: " .. tostring(content_length) .. "\n")
		content_length_printed = true
	end

	headers = curl:get_response_headers()
	if (headers) then
		-- freeswitch.consoleLog("ERR", headers:serialize() .. "\n")
	end

	save_file:write(data)
end)

save_file:close()

freeswitch.consoleLog(INFO, "get(url, fun)\n")
kvlog('url', url)
kvlog('code', ret.code)
assert(ret.code == 200)
kvlog('ok', ret.ok)
kvlog('error', ret.error)
kvlog('content_type', ret.content_type)
kvlog('content_length', ret.content_length)
kvlog('headers', serialize(ret.headers))

curl:reset()

-- post with custom headers
ret = curl:post({url = url .. '/post',
	headers = {['X-Tested-By'] = 'Seven'},
	data = {a = 1},
	dataType = 'application/json',
	method = 'POST',
})
kvlog('url', url)
kvlog('code', ret.code)
assert(ret.code == 200)
kvlog('ok', ret.ok)
kvlog('error', ret.error)
kvlog('content_type', ret.content_type)
kvlog('content_length', ret.content_length)
kvlog('headers', serialize(ret.headers))
body = curl:get_body()
kvlog('body', body:len())

code = curl:get_info_int(freeswitch.CURLINFO_RESPONSE_CODE)
ctype = curl:get_info_string(freeswitch.CURLINFO_CONTENT_TYPE)

kvlog('info code', code)
assert(code == 200)
kvlog('info content_type', ctype)
ctype = curl:get_content_length()
kvlog('info content_type', ctype)

curl:reset()

curl:setopt(freeswitch.CURLOPT_VERBOSE, 1)


ret = curl:put({url= url .. "/status/200"})
assert(ret.code == 200)

curl:reset()

ret = curl:delete({url= url})
assert(ret.code == 405)

curl:reset()

ret = curl:delete({url= url .. "/status/200"})
assert(ret.code == 200)

stream:write("+OK")
