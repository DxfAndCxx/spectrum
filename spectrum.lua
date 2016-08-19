--
-- author       :   丁雪峰
-- time         :   2016-07-22 13:16:11
-- email        :   fengidri@yeah.net
-- version      :   1.0.1
-- description  :

local scripts = {}

local f = io.popen("ls scripts") 
for mod in f:lines() do 
    if mod:sub(-4, -1) == '.lua' then
      local path = 'scripts/' .. mod:sub(1, -5)
      local t = require(path)
      if type(t) == 'table' then
          table.insert(scripts, t)
      end
    end
end

function spectrum_record_read()
       for _, m in ipairs(scripts) do
           if m.read then m.read() end
       end
end


function spectrum_record_iter()
       for _, m in ipairs(scripts) do
           if m.iter then m.iter() end
       end

end

function spectrum_summary()
       for _, m in ipairs(scripts) do
           if m.summary then m.summary() end
       end
end

