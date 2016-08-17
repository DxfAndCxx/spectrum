--
-- author       :   丁雪峰
-- time         :   2016-07-22 13:16:11
-- email        :   fengidri@yeah.net
-- version      :   1.0.1
-- description  :



local record_num = 0;
local retry_num = 0;



function spectrum_record_read()
    local record = sp.record
    local vars = sp.record.vars
    record.append("test_str", 'str')
    record.append("test_int", 23)

    --local pos = vars.upstream_addr:find(',')




    --local body_bytes_sent = tonumber(vars.body_bytes_sent)
    --local request_time = tonumber(vars.request_time)

    --if body_bytes_sent < 1024 * 5 then
    --    record.drop()
    --    return
    --end

    --if  request_time < 1 then
    --    record.drop()
    --    return
    --end

    --if body_bytes_sent / request_time < 100 * 1024 then
    --    record.drop()
    --    return
    --end

    --record_num = record_num + 1
    print("----------------------")
    print(vars.http_host)
    print(vars.body_bytes_sent)
    print(vars.request_time)
    print(vars.upstream_status)
    print(vars.upstream_addr)
    --if pos ~= nil then
    --    retry_num = retry_num + 1
    --    record.append("retry", '1')
    --    record.append("upstream_addr_1", vars.upstream_addr:sub(1, pos))
    --else
    --    record.append("retry", '0')
    --    record.append("upstream_addr_1", vars.upstream_addr)
    --end


--    print("retry: ", vars.retry, ' upstream_addr_1: ', vars.upstream_addr_1)
end

function spectrum_summary()
    local t= {num = sp.num,
        droped = sp.num_droped,
        nomatch = sp.num_nomatch,
        num_errmatch =  sp.num_errmatch,
        test = 2.3
    };
    return t;
end



function spectrum_record_iter()
    print(sp.record.vars.test_str)
    print(sp.record.vars.test_int)
    for _, k in ipairs(sp.record.keys()) do
        print(k)
    end
    --record_num = record_num + 1
    --print("cache_status: ", sp.record.vars.cache_status)
end
--
--function spectrum_record_iter_end()
--    record_num = record_num + 1
--    print('record_num: ', record_num)
--end
--
--function spectrum_summary()
--
--    print("summary: record num: ", record_num)
--end
--
