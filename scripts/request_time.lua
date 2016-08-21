local scatter = require "modules/scatter"


local scatter_request_time = scatter:new(50, 0, 1000)


local _M = {}

function _M.iter()
   local vars = sp.record.vars

   scatter_request_time:update(vars.request_time)
end

function _M.map()
   local t = {}
   --return {t = 2,  s = 'string', table = {t = 2, s = 'value'}}
   return scatter_request_time:map()
end

function _M.reduce(...)

    scatter.print(nil, ...)
   --print("## Go To reduce")
   -- local args = {...}
   -- for k, v in pairs(args) do
   --     print(k, v)
   -- end
end


return _M
