

local cache_status = require "modules/cache_status"
local scatter = require "modules/scatter"
local utils = require "modules/utils"

local record_num = 0;
local retry_num = 0;
local block_num = 0;
local min_node_speed = 99999999999999999999;

local speeds = {}


local scatter_speed = scatter:new(1024 * 100, 0, 1024 * 1024 * 1)
local scatter_sent = scatter:new(1024 * 10, 0, 1024 * 1024)
local scatter_request_time = scatter:new(50, 0, 1000)

local _M = {}

function _M.read()
    local record = sp.record
    local vars = sp.record.vars


    record.append("cache_status", cache_status.status(vars.via))
    
    
    local kbps = string.match(vars.request_uri, '_(%d+)[_.]mp4[/?]')

    if not kbps then
       record.drop() 
     end
    
    record.append("min_speed", kbps * 1000 / 8)

    local upstream_addr = vars.upstreaam_addr
    local _, count
    count = 0
    if  upstream_addr then
      _, count = string.gsub(vars.upstreaam_addr, ',', ',')
    end
    record.append("retry", count)
end

return _M
