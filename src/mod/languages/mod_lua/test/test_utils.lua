freeswitch.setGlobalVariable("domain", "test.domain")
domain = freeswitch.expand("$${domain}")
assert(domain == "test.domain")
freeswitch.consoleLog("INFO", domain .. "\n")

freeswitch.setGlobalVariable("domain", "test.domain.is-very-long.com")
domain = freeswitch.expand("$${domain}")
freeswitch.consoleLog("INFO", domain .. "\n")
assert(domain == "test.domain.is-very-long.com")

domain = freeswitch.expand("domain=$${domain}")
assert(domain == "domain=test.domain.is-very-long.com")

domain = freeswitch.expand("$${domain")
assert(domain == nil)

domain = freeswitch.expand("domain")
assert(domain == "domain")

str = freeswitch.md5("str")
freeswitch.consoleLog("INFO", str .. "\n")
assert(str == "341be97d9aff90c9978347f66f945b77")

ok = freeswitch.match("str str", "t")
assert(ok)

ok = freeswitch.match("str str", "1")
assert(not ok)

stream:write("+OK")
