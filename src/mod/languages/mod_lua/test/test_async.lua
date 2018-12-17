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

session:consoleLog("INFO", "Testing cURL Async ...\n")

function onInput(s, type, obj)
	s:consoleLog("INFO", obj:serialize() .. "\n");

	if (type == "event") and (obj:getHeader("Event-Type") == "lua::curl") then
		-- break the playback
		return "break"
	end
end

session:setInputCallback('onInput')

curl = freeswitch.cURL()
-- async get in a new thread
curl:get({url="http://baidu.com", async=true, channel_uuid = session:get_uuid()})

session:streamFile("silence_stream://5000")

-- check if the async download is finished or not
while (not curl:is_finished()) do
	freeswitch.usleep(500)
end

code = curl:get_info_long(freeswitch.CURLINFO_RESPONSE_CODE);
assert(code == 200)
kvlog('code', code)
body = curl:get_body()
kvlog('body', body)
