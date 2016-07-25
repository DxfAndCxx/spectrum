--
-- author       :   丁雪峰
-- time         :   2016-07-22 13:16:11
-- email        :   fengidri@yeah.net
-- version      :   1.0.1
-- description  :
--
--
function spectrum_config()
    sp.opt.file_log = 't/ngx_logs'
    --sp.file_log = '/home/vagrant/tmp/test.log'
    sp.opt.file_pattern = 't/pattern'
end

function spectrum_record_read()
    print("********************************spectrum_record_read")

    local vars_names = {"remote_addr",
    "remote_user",
    "time_local",
    "request_method",
    "scheme",
    "http_host",
    "url",
    "server_protocol",
    "status",
    "body_bytes_sent",
    "x_request_id",
    "request_time"}

    for _, k in ipairs(vars_names) do
        print(k, '\t\t\t\t: ', sp.record.vars[k])
    end
    print("********************************spectrum_record_read")
end


local record_num = 0;

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
