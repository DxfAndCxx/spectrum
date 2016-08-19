local scatter = require "modules/scatter"


local scatter_request_time = scatter:new(50, 0, 1000)


local _M = {}

function _M.iter()
   local vars = sp.record.vars

   scatter_request_time:update(vars.request_time)
end

function _M.summary()
     scatter_request_time:print()
end


return _M
