--
-- author       :   丁雪峰
-- time         :   2016-08-26 02:18:40
-- email        :   fengidri@yeah.net
-- version      :   1.0.1
-- description  :
--



local math_ceil = math.ceil

local _M = {}

local mt = { __index = _M }

local function reduce_show(opt, stat, ...)
    print('')
    print("**************************************************")
    opt.limit = opt.limit or 0
    if opt.msg then
        print(opt.msg)
    end

    local stats = {...}

    local total = stat.total

    for _, s in ipairs(stats) do
        for k, v in pairs(s.stat) do
            if nil == stat.stat[k] then
                stat.stat[k] = v
            else
                stat.stat[k] = v + stat.stat[k]
            end
        end
        total = s.total + total
    end

    local list = {}

    for k, v in pairs(stat.stat) do
        table.insert(list, {k, v})
    end

    local sort_fn = function(a, b)
        return a[2] < b[2]
    end
    table.sort(list, sort_fn)


    for _, pa in ipairs(list) do
        local k, v, p
        k = pa[1]
        v = pa[2]
        p = v * 100 / total
        if p > opt.limit then
            print(k,  ':', string.format("%7.2f%%", p), v)
        end
    end
     print("Total: ", total)
end


function _M.new(opt)
    local t = {}
    local script = {}

    if type(opt.field) ~= 'function' then
        return nil, 'opt.field must be function'
    end

    t.stat = {}
    t._max    = 0
    t._total  = 0
    t._remain = 0
    t.msg     = opt.msg

    t = setmetatable(t, mt)


    if opt.field then
        script.iter = function()
           local v = opt.field()
           if nil == v then return end
            t:update(v)
        end
    end


    script.map = function()
        return t:map()
    end

    script.reduce = function(...)
        reduce_show(opt, ...)
    end

    scripts.append(script)
end


function _M.update(self, v)
      self._total = self._total + 1

      if nil == self.stat[v] then
           self.stat[v] = 1
      else
           self.stat[v] = self.stat[v] + 1
      end
end

function _M.map(self)
    return {stat = self.stat, total = self._total}
end



return _M
