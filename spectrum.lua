--
-- author       :   丁雪峰
-- time         :   2016-07-22 13:16:11
-- email        :   fengidri@yeah.net
-- version      :   1.0.1
-- description  :






function spectrum_config()
    sp.opt.file_log = 't/ngx_logs'
    sp.opt.file_log = '/home/vagrant/tmp/test.log'
    sp.opt.file_pattern = 't/pattern'
end

local record_num = 0;
local retry_num = 0;



function spectrum_record_read()
    local record = sp.record
    local vars = sp.record.vars

    local pos = vars.upstream_addr:find(',')

    local body_bytes_sent = tonumber(vars.body_bytes_sent)
    local request_time = tonumber(vars.request_time)

    if body_bytes_sent < 1024 * 5 then
        record.drop()
        return
    end

    if  request_time < 1 then
        record.drop()
        return
    end

    if body_bytes_sent / request_time < 100 * 1024 then
        record.drop()
        return
    end

    record_num = record_num + 1

    if pos ~= nil then
        retry_num = retry_num + 1
        record.append("retry", '1')
        record.append("upstream_addr_1", vars.upstream_addr:sub(1, pos))
    else
        record.append("retry", '0')
        record.append("upstream_addr_1", vars.upstream_addr)
    end


--    print("retry: ", vars.retry, ' upstream_addr_1: ', vars.upstream_addr_1)
end

function spectrum_record_read_end()
    print("record num: ",  record_num);
    print("retry num: ", retry_num)
end



--function spectrum_record_iter()
--    record_num = record_num + 1
--    print("cache_status: ", sp.record.vars.cache_status)
--end
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
