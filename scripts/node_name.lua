--
-- author       :   丁雪峰
-- time         :   2016-08-26 03:20:43
-- email        :   fengidri@yeah.net
-- version      :   1.0.1
-- description  :
--
local stat = require "modules/stat"


local opt = {
    msg = "node name stat"
}

opt.field = function()
   local vars = sp.record.vars

   return vars.node_name

end


stat.new(opt)
