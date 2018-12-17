json = freeswitch.JSON()

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

url1 = "http://httpbin.org/status/200"
url2 = "http://httpbin.org/status/404"
url3 = "http://httpbin.org/status/403"
url4 = "http://httpbin.org/redirect-to?url=httpbin.org"

function http_response(url)
    curl = freeswitch.cURL()
    curl:setopt(freeswitch.CURLOPT_HEADER, TRUE)
    curl:setopt(freeswitch.CURLOPT_NOBODY, TRUE)
    ret = curl:get(url)
    if ret == nil then
        return nil
    else
        http_code = curl:get_info_int(freeswitch.CURLINFO_RESPONSE_CODE)
        return http_code
    end
end

assert(http_response(url1) == 200)
assert(http_response(url2) == 404)
assert(http_response(url3) == 403)
-- assert(http_response(url4) == 302)

url = "http://example.com"
function get_url_content(url)
    curl = freeswitch.cURL()
    curl:setopt(freeswitch.CURLOPT_URL, url)
    curl:setopt(freeswitch.CURLOPT_CONNECTTIMEOUT, 5)
    curl:setopt(freeswitch.CURLOPT_TIMEOUT, 5)
    ret = curl:get(url)
    if ret == nil then
        return nil
    else
        b = curl:get_body()
        kvlog("body", b)
        http_code = curl:get_info_int(freeswitch.CURLINFO_RESPONSE_CODE)

        if http_code >= 200 and http_code<300 then
            return b
        else
            return http_code
        end

    end
end

print(get_url_content(url))


url1 = "https://httpbin.org"

function get_request(url)
    curl = freeswitch.cURL()
    curl:setopt(freeswitch.CURLOPT_FOLLOWLOCATION, 1)
    curl:setopt(freeswitch.CURLOPT_AUTOREFERER, 1)
    curl:setopt(freeswitch.CURLOPT_CONNECTTIMEOUT, 5)
    curl:setopt(freeswitch.CURLOPT_TIMEOUT, 10)
    curl:setopt(freeswitch.CURLOPT_SSL_VERIFYHOST, 0)
    curl:setopt(freeswitch.CURLOPT_SSL_VERIFYPEER, 0)
    ret = curl:get(url)
    if ret == nil then
        return nil
    else
        b = curl:get_body()
        kvlog("body", b)
        code = curl:get_info_int(freeswitch.CURLINFO_RESPONSE_CODE)
        return code

    end
end

assert(get_request(url1) == 200)

function post_request(url, data)
    curl = freeswitch.cURL()
    curl:setopt(freeswitch.CURLOPT_URL, url)
    curl:setopt(freeswitch.CURLOPT_FOLLOWLOCATION, 1)
    curl:setopt(freeswitch.CURLOPT_AUTOREFERER, 1)
    curl:setopt(freeswitch.CURLOPT_CONNECTTIMEOUT, 5)
    curl:setopt(freeswitch.CURLOPT_TIMEOUT, 10)
    curl:setopt(freeswitch.CURLOPT_SSL_VERIFYHOST, 0)
    curl:setopt(freeswitch.CURLOPT_SSL_VERIFYPEER, 0)
    ret = curl:post({url = url,
    headers = {['X-Tested-By'] = 'Seven'},
    -- data = '{"a": 1}',
    data = data,
    dataType = 'application/json'
    })
    if ret == nil then
        return nil
    else
        b = curl:get_body()
        kvlog("body", b)
        code = curl:get_info_int(freeswitch.CURLINFO_RESPONSE_CODE)
        return code

    end
end

assert(post_request("https://httpbin.org/status/200", '{"a":1}') == 200)

stream:write("+OK")
