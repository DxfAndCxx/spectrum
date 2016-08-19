local _M = {}


function _M.number_fmt(n)
     if n < 1024 then
         return string.format("%.2f", n)
     end

     n = n / 1024
     if n < 1024 then
         return string.format("%.2fK", n)
     end

     n = n / 1024
     if n < 1024 then
         return string.format("%.2fM", n)
     end

     n = n / 1024
     if n < 1024 then
         return string.format("%.2fG", n)
     end

     n = n / 1024
     return string.format("%.2fT", n)
end

return _M
