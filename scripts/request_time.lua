local scatter = require "modules/scatter"
--local monip = require "monip"
local utils = require "modules/utils"



local opt = {
    iter = 2048 * 2,
    min = 0,
    max = 1000,
    msg = "request_time scatter",
    fmt = utils.number_fmt,
}

opt.field = function()
   local vars = sp.record.vars

   return vars.request_time
end


scatter.new(opt)




