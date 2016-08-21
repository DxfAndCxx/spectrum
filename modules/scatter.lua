

local math_ceil = math.ceil

local _M = {}

local mt = { __index = _M }


function _M.new(self, iter, min, max)
    min = min or 0
    return setmetatable({iter = iter, min = min, max=max,
                  scatter = {}, _max = 0, _total = 0, _remain = 0}, mt)
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

function _M.print(handle, scatter, ...)
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
            if handle then
                 s = handle(s)
                 e = handle(e)
            end

	    print(s, '-', e, ':',
              string.format("%7.2f%%", scatter[i] * 100 / total),
              scatter[i]
              )
         end
     end
     print("Total: ", total, "Remain: ", remain)

end


return _M
