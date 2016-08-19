

local math_ceil = math.ceil

local _M = {}

local mt = { __index = _M }


function _M.new(self, iter, min, max)
    min = min or 0
    return setmetatable({iter = iter, min = min, max=max, scatter = {}, _max = 0, _total = 0, _remain = 0}, mt)
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

function _M.print(self, handle)
     local s, e
     for i=1,self._max do
         if nil ~= self.scatter[i] then
            s = self.iter * (i - 1)
            e = self.iter * i
            if handle then
                 s = handle(s)
                 e = handle(e)
            end
            
	    print(s, '-', e, ':', string.format("%7.2f%%", self.scatter[i] * 100 / self._total))
         end
     end
     print("Total: ", self._total, "Remain: ", self._remain)
         
end


return _M
