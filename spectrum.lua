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
    for _, v in ipairs(sp.pattern.fields) do
        print(v, '\t\t\t\t: ', sp.record.vars[v])
    end
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
