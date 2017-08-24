local rootPath = ngx.var.document_root
local dataPath = rootPath .. '/data/'
local ffi = require "ffi"
local cf = ffi.load(rootPath .. '/webapp/lib/cf.so')

ffi.cdef[[
typedef struct cuckoo_bucket{
    uint16_t entry[4];
} cuckoo_bucket;
typedef struct cuckoo_filter{
    unsigned count;
    unsigned b_num;
    unsigned h_offset;
    cuckoo_bucket bucket[];
} cuckoo_filter;

cuckoo_filter* get_storage_cf(const char* file_name);
int cf_check_in(cuckoo_filter* cf, const char* str, int len);
int free_cf(cuckoo_filter *cf, const char *file_name);

unsigned int strlen(char *s);
void *malloc(size_t size);
void free(void *ptr);
]]

local _M = {}

local ptr_list = {}

--return 1 在 0 不在 -1查询失败
function _M.checkIn(self, groupName, md5)
	if ptr_list[groupName] == nil then
		ptr_list[groupName] = cf.get_storage_cf(dataPath .. groupName .. ".data")
	end

	return cf.cf_check_in(ptr_list[groupName], md5, string.len(md5))
end

function _M.free_cf(self, groupName)
	if ptr_list[groupName] == nil then
		return true
	end
	local ret = cf.free_cf(ptr_list[groupName], dataPath .. groupName .. ".data")
	if ret == -1 then
		return false
	end

	ptr_list[groupName] = nil
	return true
end

return _M
