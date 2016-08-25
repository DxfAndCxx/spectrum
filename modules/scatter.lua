

local math_ceil = math.ceil

local _M = {}

local mt = { __index = _M }

local function reduce_show(opt, scatter, ...)
    if opt.msg then
        print(opt.msg)
    end

    local scatters = {...}

    local total = scatter.total
    local remain = scatter.remain
    local iter = scatter.iter
    local max = scatter.max

    for _, s in ipairs(scatters) do
        for k, v in pairs(s.scatter) do
            if nil == scatter.scatter[k] then
                scatter.scatter[k] = v
            else
                scatter.scatter[k] = v + scatter.scatter[k]
            end
            if max > s.max then
                max = s.max
            end
        end
        total = s.total + total
        remain = s.remain + remain
    end

    scatter = scatter.scatter

     local s, e
     for i=1, max do
         if nil ~= scatter[i] then
            s = iter * (i - 1)
            e = iter * i
            if opt.fmt then
                 s = opt.fmt(s)
                 e = opt.fmt(e)
            end

	    print(s, '-', e, ':',
              string.format("%7.2f%%", scatter[i] * 100 / total),
              scatter[i]
              )
         end
     end
     print("Total: ", total, "Remain: ", remain)

end


function _M.new(opt)
    local t = {}
    local script = {}

    if type(opt.field) ~= 'function' then
        return nil, 'opt.field must be function'
    end


    t.scatter = {}
    t.iter    = opt.iter  or  0
    t.min     = opt.min   or  0
    t.max     = opt.max
    t._max    = 0
    t._total  = 0
    t._remain = 0
    t.msg     = opt.msg

    t = setmetatable(t, mt)


    script.iter = function()
        t:update(opt.field())
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
      if self.min and v < self.min then
              self._remain = self._remain + 1
              return
      end
      if self.max and v > self.max then
              self._remain = self._remain + 1
              return
      end

      local i = math_ceil(v / self.iter)
      if i > self._max then
          self._max = i
      end

      self._total = self._total + 1

      if nil == self.scatter[i] then
           self.scatter[i] = 1
      else
           self.scatter[i] = self.scatter[i] + 1
      end
      return i
end

function _M.map(self)
    return {scatter = self.scatter, iter = self.iter, total = self._total,
          remain = self._remain, max = self._max}
end



return _M
