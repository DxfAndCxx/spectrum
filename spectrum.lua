--
-- author       :   丁雪峰
-- time         :   2016-07-22 13:16:11
-- email        :   fengidri@yeah.net
-- version      :   1.0.1
-- description  :
--

function spectrum_record()
    local time = sp.record.time;
    sp.record.append("cache_status", 'H');
--    print("time: ", time)
end


local record_num = 0;

function spectrum_record_iter()
    record_num = record_num + 1
    print("cache_status: ", sp.record.cache_status, ' ip: ', sp.record.ip)
end

function spectrum_summary()
    print("summary: record num: ", record_num)
end
