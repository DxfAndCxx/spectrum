
local table = require "table"
local string = require "string"

local type = type
local next = next
local ipairs = ipairs
local insert = table.insert
local concat = table.concat
local str_gmatch = string.gmatch

local _M = {}

local MAP = {
    M = "marco",
    T = "ats",
    V = "vista",
    S = "shanks",
    K = "kuzan",
    O = "source",
}


-- T.594.M.2, T.594.H.1, V.ctn-zj-hgh-093
-- Project.Node.Cache.Disk
local function split_via_header(via)
    local t = {}
    via = via:gsub("[^,]+$", '')

    for field in str_gmatch(via, [[[^, ]+]]) do
        local field_t = {}
        for f in str_gmatch(field, [[[^%.]+]]) do
            insert(field_t, f)
        end

        if not next(field_t) then
            return
        end

        insert(t, 1, field_t)
    end

    return t
end


function _M.status(via)
    if type(via) == "table" then
        via = concat(via, ", ")
    end

    if type(via) ~= "string" or #via == 0 then
        return 'M'
    end

    local vias = split_via_header(via)
    if not vias or not next(vias) then
        return 'M'
    end

    local ats_level = 0
    local status = "M"
    for _, field in ipairs(vias) do
        local project = MAP[field[1]] or "unknown"
        if project == "ats" then
            ats_level = ats_level + 1

            local flag = field[3]
            if flag ~= "M" and status == "M" then
                status = flag .. ats_level
                break
            end
        end
    end

    return status
end

return _M
