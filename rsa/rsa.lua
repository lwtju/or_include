local rootPath = ngx.var.document_root
local priKeyPath = rootPath .. '/conf/prikey.pem'
local ffi = require "ffi"
local lua_rsa = ffi.load(rootPath .. '/lib/lua_rsa.so')

ffi.cdef[[
unsigned int strlen(char *s);
char* encrypt(const char *str, const char *pubkey_path);
char* decrypt(const char *str, const char *prikey_path);
void free(void *ptr);
]]

local _M = {}

function _M.decrypt(self, encryptedStr)
	local decryptedStr = lua_rsa.decrypt(encryptedStr, priKeyPath)
	local ret = ffi.string(decryptedStr,  128)
	ffi.C.free(decryptedStr)
	
	return string.gsub(ret, "^%z*(.-)%z*$", "%1")
end

return _M
