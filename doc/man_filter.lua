local Def = {}

function Def:new(d)
    -- init with empty def
    if d == nil then
        d = {
            start_idx = nil,
            end_idx = nil,
            def_par = nil,
            content = {},
        }
    end
    setmetatable(d, self)
    self.__index = self
    return d
end

function Def:init(start_idx, el)
    self.start_idx = start_idx
    self.def_par = el
end

function Def:append(el)
    if self.start_idx ~= nil then
        table.insert(self.content, el)
    end
end

function Def:stop(end_idx)
    if self.start_idx == nil then
        return nil
    end
    local out = self:new({
        start_idx = self.start_idx,
        end_idx = end_idx,
        def_par = self.def_par,
        content = self.content,
    })
    self.start_idx = nil
    self.end_idx = nil
    self.def_par = nil
    self.content = {}
    return out
end

function Def:to_string()
    return string.format("start: %d, end: %d, def_par: %s", self.start_idx, self.end_idx, self.def_par)
end

function find_defs(doc)
    local defs = {}
    local idx = 0
    local def = Def:new()

    -- find defintions:
    -- * start at paragraphs with `word` ...
    -- * stop at next definition or next header
    local filter = {
        traverse = "topdown",
        Para = function(el)
            idx = idx + 1

            local new_def_start = #el.content >= 1 and el.content[1].tag == "Code"

            if new_def_start then
                local newd = def:stop(idx - 1)
                table.insert(defs, newd)

                def:init(idx, el.content)
            else
                def:append(el)
            end
            return nil, false
        end,
        Block = function(el)
            idx = idx + 1
            def:append(el)
            -- stop exploring after one nesting level
            return nil, false
        end,
        Header = function(el)
            idx = idx + 1
            local newd = def:stop(idx - 1)
            table.insert(defs, newd)
            return nil, false
        end,
    }

    doc:walk(filter)
    local newd = def:stop(idx - 1)
    table.insert(defs, newd)

    return defs
end

function convert_defs(doc, defs)
    local idx = 0
    local out_blocks = {}

    local convert_defs = {
        traverse = "topdown",
        Block = function(el)
            idx = idx + 1
            for _, d in ipairs(defs) do
                if idx == d.end_idx then
                    local dl = pandoc.DefinitionList({ { d.def_par, { d.content } } })
                    table.insert(out_blocks, dl:walk())
                    return {}, false
                end
                if idx >= d.start_idx and idx < d.end_idx then
                    -- drop
                    return {}, false
                end
            end
            table.insert(out_blocks, el:walk())
            return nil, false
        end,
    }

    doc:walk(convert_defs)

    return pandoc.Pandoc(out_blocks, doc.meta)
end

-- for <2.17 compatibility
-- equivalent to `doc:walk(filter)`
local function walk_doc(doc, filter)
    local div = pandoc.Div(doc.blocks)
    local blocks = pandoc.walk_block(div, filter).content
    return pandoc.Pandoc(blocks, doc.meta)
end

local function extract_title(doc)
    local title = {}
    local section
    local filter = {
        Header = function(el)
            local f = {
                Str = function(el)
                    if el.text:find("%(1%)") ~= nil then
                        section = "General Commands Manual"
                    elseif el.text:find("%(5%)") ~= nil then
                        section = "File Formats Manual"
                    end
                    table.insert(title, el)
                end,
                Inline = function(el)
                    table.insert(title, el)
                end,
            }
            if el.level == 1 then
                pandoc.walk_block(el, f)
                return {} -- drop
            end
            return nil
        end,
    }

    doc = walk_doc(doc, filter)

    local to_inline = function(s)
        local r = {}
        for w in s:gmatch("%S+") do
            table.insert(r, pandoc.Str(w))
            table.insert(r, pandoc.Space())
        end
        table.remove(r, #r)
        return r
    end

    if section ~= nil then
        for _, e in ipairs({
            pandoc.Space(),
            pandoc.Str("rofi"),
            pandoc.Space(),
            pandoc.Str("|"),
            table.unpack(to_inline(section)),
        }) do
            table.insert(title, e)
        end
    end

    doc.meta = pandoc.Meta({
        title = pandoc.MetaInlines(title),
    })

    return doc
end

local function decrement_heading(doc)
    local filter = {
        Header = function(el)
            if el.level > 1 then
                el.level = el.level - 1
                return el
            end
            return nil
        end,
    }

    doc = walk_doc(doc, filter)
    return doc
end

local function code_in_strong(doc)
    local filter = {
        Code = function(el)
            return pandoc.Strong(el.text)
        end,
    }

    doc = walk_doc(doc, filter)
    return doc
end

--- Run filtering function through whole document
--
--  * find argument definitions: paragraph starting with inline code (`-arg`)
--  * replace the paragraphs until the end of the definition with a DefinitionList
--  * extract metadata title from main heading
--  * decrement heading from 1 for better display
--  * convert inline code text to Strong as usual in man pages
function Pandoc(doc)
    if PANDOC_VERSION >= pandoc.types.Version("2.17.0") then
        -- 2.17 is required for topdown traversal
        local defs = find_defs(doc)
        doc = convert_defs(doc, defs)
    end

    doc = extract_title(doc)

    doc = decrement_heading(doc)

    doc = code_in_strong(doc)

    return doc
end
