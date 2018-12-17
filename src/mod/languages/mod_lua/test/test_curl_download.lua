freeswitch.consoleLog("INFO", "curl ...\n")
curl = freeswitch.cURL()

function file_exists(path)
	if not path then return false end
	local f = io.open(path,"r")
	if f ~=nil then
		io.close(f)
		return true
	else
		return false
	end
end

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

function md5_check(save_path)
	local handle = io.popen("md5sum " .. save_path)
	local md5 = handle:read("*a")
	handle:close()
	md5 = md5:sub(1, 32)
	kvlog('md5', md5)

	return md5
end

curl:setopt(freeswitch.CURLOPT_VERBOSE, 1)
curl:set_timeout(600);

wav_url = "http://files.freeswitch.org/eg/2006/agent.wav"
wav_path = "/tmp/agent.wav"

ret = curl:download(wav_url, wav_path)
kvlog('test_save_path', wav_path)
kvlog('url', wav_url)
kvlog('code', ret.code)
assert(ret.code == 200)
kvlog('ok', ret.ok)
kvlog('error', ret.error)
kvlog('content_type', ret.content_type)
assert(ret.content_type == "audio/x-wav")
kvlog('content_length', ret.content_length)
kvlog('real_save_path', wav_path)
kvlog("file_exists", file_exists(wav_path))

md5 = md5_check(wav_path)
assert(md5 == "0c3e904162bb1dc17bc55fbcbd164c68")


curl:reset_state();

wav_path = "/tmp"

save_path = curl:gen_save_path(wav_url, wav_path)
ret = curl:download(wav_url, wav_path)
kvlog('test_save_path', wav_path)
kvlog('url', wav_url)
kvlog('code', ret.code)
assert(ret.code == 200)
kvlog('ok', ret.ok)
kvlog('error', ret.error)
kvlog('content_type', ret.content_type)
kvlog('content_length', ret.content_length)
kvlog('real_save_path', save_path)

md5 = md5_check(save_path)
assert(md5 == "0c3e904162bb1dc17bc55fbcbd164c68")
kvlog("file_exists", file_exists(save_path))

curl:reset_state();

wav_path = "/tmp/"

save_path = curl:gen_save_path(wav_url, wav_path)
ret = curl:download(wav_url, wav_path)
kvlog('test_save_path', save_path)
kvlog('url', wav_url)
kvlog('code', ret.code)
assert(ret.code == 200)
kvlog('ok', ret.ok)
kvlog('error', ret.error)
kvlog('content_type', ret.content_type)
kvlog('content_length', ret.content_length)
kvlog('real_save_path', save_path)

md5 = md5_check(save_path)
assert(md5 == "0c3e904162bb1dc17bc55fbcbd164c68")
kvlog("file_exists", file_exists(save_path))


curl:reset_state();

ret = curl:download(wav_url, "")
kvlog('test_save_path', ret.save_path)
kvlog('url', wav_url)
kvlog('code', ret.code)
assert(ret.code == 200)
kvlog('ok', ret.ok)
kvlog('error', ret.error)
kvlog('content_type', ret.content_type)
kvlog('content_length', ret.content_length)


curl:reset_state();

ret = curl:download(wav_url, nil)
kvlog('test_save_path', ret.save_path)
kvlog('url', wav_url)
kvlog('code', ret.code)
assert(ret.code == nil)
kvlog('ok', ret.ok)
kvlog('error', ret.error)
kvlog('content_type', ret.content_type)
kvlog('content_length', ret.content_length)


curl:reset_state();

curl:set_max_bytes(1024 * 1024 * 2048)

bz2_url = "http://files.freeswitch.org/freeswitch-releases/freeswitch-1.0.0.tar.bz2"
save_path = "/tmp/freeswitch-1.0.0.tar.bz2"

ret = curl:download({
	url = bz2_url,
	savePath = save_path,
	async = true
})

assert(ret == nil)

-- important for async, make sure download thread is started
freeswitch.msleep(500)

while (not curl:is_finished()) do
	content_length = curl:get_info_double(freeswitch.CURLINFO_CONTENT_LENGTH_DOWNLOAD)
	freeswitch.consoleLog("DEBUG", "downloading ..." ..
		curl:get_bytes() .. "/" .. content_length .. "\n")
	freeswitch.msleep(2000)
end

code = curl:get_info_long(freeswitch.CURLINFO_RESPONSE_CODE)
kvlog('code', code)
assert(code == 200)

md5 = md5_check(save_path)
assert(md5 == "427c3eb6108cd5aef1794ff7fd599b76")
kvlog('test_save_path', save_path)
kvlog('url', bz2_url)
kvlog("file_exists", file_exists(save_path))

curl:reset();

wav_path = "/tmp/test.wav"

ret = curl:download({url = wav_url,
	header = {['X-XTRA-AUTH-ID'] = 'seven'},
	savePath = wav_path
})

kvlog('url', wav_url)
code = curl:get_response_code()
assert(code == 200)
kvlog('ok', ret.ok)
kvlog('error', ret.error)
content_type = curl:get_content_type()
kvlog('content_type', content_type)
content_length = curl:get_content_length()
kvlog('content_length', content_length)

md5 = md5_check(wav_path)
assert(md5 == "0c3e904162bb1dc17bc55fbcbd164c68")
kvlog('test_save_path', wav_path)
kvlog('url', wav_url)
kvlog('real_save_path', wav_path)
kvlog("file_exists", file_exists(wav_path))

stream:write("+OK")
