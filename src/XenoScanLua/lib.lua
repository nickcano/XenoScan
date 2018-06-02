function table.icontains(t, val)
	for _, v in ipairs(t) do
		if (val == v) then
			return true
		end
	end
	return false
end

TYPE_DEFINITIONS = 
{
	["ascii"] = {size = false, isNumeric = false, isString = true, type = SCAN_VARIANT_ASCII_STRING},
	["widestring"] = {size = false, isNumeric = false, isString = true, type = SCAN_VARIANT_WIDE_STRING},
	["uint8"] = {size = 1, isNumeric = true, isString = false, type = SCAN_VARIANT_UINT8},
	["int8"] = {size = 1, isNumeric = true, isString = false, type = SCAN_VARIANT_INT8},
	["uint16"] = {size = 2, isNumeric = true, isString = false, type = SCAN_VARIANT_UINT16},
	["int16"] = {size = 2, isNumeric = true, isString = false, type = SCAN_VARIANT_INT16},
	["uint32"] = {size = 4, isNumeric = true, isString = false, type = SCAN_VARIANT_UINT32},
	["int32"] = {size = 4, isNumeric = true, isString = false, type = SCAN_VARIANT_INT32},
	["uint64"] = {size = 8, isNumeric = true, isString = false, type = SCAN_VARIANT_UINT64},
	["int64"] = {size = 8, isNumeric = true, isString = false, type = SCAN_VARIANT_INT64},
	["double"] = {size = 8, isNumeric = true, isString = false, type = SCAN_VARIANT_DOUBLE},
	["float"] = {size = 4, isNumeric = true, isString = false, type = SCAN_VARIANT_FLOAT},
	["filetime64"] = {size = 8, isNumeric = true, isString = false, type = SCAN_VARIANT_FILETIME64},
	["ticktime32"] = {size = 4, isNumeric = true, isString = false, type = SCAN_VARIANT_TICKTIME32},
}

local function generateTypeData()
	local refNames = {}
	for tname, tdata in pairs(TYPE_DEFINITIONS) do
		_G[tname] = function(name) return {__name = name, __type = tdata.type} end
		tdata.construct = _G[tname]
		tdata.name = tname
		refNames[#refNames + 1] = tname
	end

	for _, tname in ipairs(refNames) do
		TYPE_DEFINITIONS[tostring(_G[tname])] = TYPE_DEFINITIONS[tname]
		TYPE_DEFINITIONS[TYPE_DEFINITIONS[tname].type] = TYPE_DEFINITIONS[tname]
	end
end
generateTypeData()


ATTACHED_PROCESSES = {}
Process = {}
Process.__index = Process

PROCESS_ATTACH_KEY = "proc"

assert(table.icontains(ATTACH_TARGET_NAMES, PROCESS_ATTACH_KEY), "expected a native proc target type!")

function Process.new(pid)
	local this = {}
	if (not ATTACHED_PROCESSES[pid]) then
		setmetatable(this, Process)
		this._pid = pid

		if (type(pid) == 'string') then
			assert(table.icontains(ATTACH_TARGET_NAMES, pid), "Target with type '" .. pid .. "' not implemented!")
			this.__nativeObject = attach(pid, 0)
		else
			this.__nativeObject = attach(PROCESS_ATTACH_KEY, this._pid)
		end

		assert(this.__nativeObject, "Failed to attach to process '" .. tostring(pid) .. "'!")
		ATTACHED_PROCESSES[pid] = this
	else
		this = ATTACHED_PROCESSES[pid]
	end
	return this
end
setmetatable(Process, {__call = function(_, ...) return Process.new(...) end})

function Process:destroy()
	local this = type(self) == 'table' and self or Process.new(self)

	destroy(this.__nativeObject)
	ATTACHED_PROCESSES[this._pid] = nil
	self = nil
end

function Process:newScan()
	local this = type(self) == 'table' and self or Process.new(self)

	return newScan(this.__nativeObject)
end

function Process:getResultsSize()
	local this = type(self) == 'table' and self or Process.new(self)

	return getScanResultsSize(this.__nativeObject)
end

function Process:getResults(offset, count)
	local this = type(self) == 'table' and self or Process.new(self)

	count = count or this:getResultsSize()
	if (count == 0) then return false end
	offset = offset or 0

	local result, message = getScanResults(this.__nativeObject, offset, count)
	assert(result, message)
	return result
end

function Process:findDataStructures(typename)
	local this = type(self) == 'table' and self or Process.new(self)
	return getDataStructures(this.__nativeObject, typename)[typename]
end

function Process:__validateMemoryValueForReadWrite(valueType)
	local vtype = type(valueType)
	if (vtype == 'table') then
		assert(valueType.__type, "Memory read or write must have strong type")
		assert(TYPE_DEFINITIONS[valueType.__type], "Invalid type specified: " .. table.show(valueType, ""))
		return valueType
	elseif (vtype == 'function') then
		local retType = TYPE_DEFINITIONS[tostring(valueType)] and valueType() or nil
		assert(retType, "Invalid type specified: " .. tostring(valueType))
		return retType
	else
		local typeInfo = TYPE_DEFINITIONS[valueType]
		assert(typeInfo, "Invalid type specified: " .. tostring(valueType))
		assert(typeInfo.construct, "Invalid type fetched: " .. table.show(typeInfo, ""))
		return typeInfo.construct()
	end
end


function Process:readMemory(address, offset, valueType)
	local this = type(self) == 'table' and self or Process.new(self)
	if valueType == nil then
		valueType = offset
		offset = 0
	end

	local readValue = this:__validateMemoryValueForReadWrite(valueType)
	return readMemory(this.__nativeObject, address, offset, readValue.__type)
end

function Process:writeMemory(address, offset, value)
	local this = type(self) == 'table' and self or Process.new(self)
	if value == nil then
		value = offset
		offset = 0
	end

	-- write an entire structure
	if (type(value) == "table" and value.__schema) then
		for _, v in ipairs(value.__schema) do
			if (v.__arraylen) then
				local val = value[v.__name]
				local valSize = sizeof(v.__type)
				assert(type(val) == "table", "Table expected as array container!")
				for index, aval in ipairs(val) do
					if (aval and type(aval) ~= 'table') then
						assert(index <= v.__arraylen, "Index is outside of arrays bounds (index: " .. index .. " bounds: " .. v.__arraylen .. ")!")
						assert(index > 0, "Index must be greater than zero (index: " .. index .. ")!")
						this:writeMemory(address, offset + ((index - 1) * valSize), {__name = aval, __type = v.__type})
					end
				end
			else
				local val = value[v.__name]
				if (val and type(val) ~= 'table') then
					this:writeMemory(address, offset, {__name = val, __type = v.__type})
				end
			end
			offset = offset + sizeof(v)
		end
		return true
	end

	-- write a single value
	local writeValue = this:__validateMemoryValueForReadWrite(value)
	assert(writeValue.__name, "No value specified for write: " .. table.show(writeValue, ""))
	return writeMemory(this.__nativeObject, address, offset, tostring(writeValue.__name), writeValue.__type)
end

TYPE_MODE_LOOSE = 1
TYPE_MODE_TIGHT = 2
TYPE_MODE_EXACT = 3

typeModeMap =
{
	["string"] = {
		[TYPE_MODE_LOOSE] = SCAN_INFER_TYPE_ALL_TYPES,
		[TYPE_MODE_TIGHT] = SCAN_INFER_TYPE_STRING_TYPES,
		[TYPE_MODE_EXACT] = SCAN_INFER_TYPE_STRING_TYPES
	},
	["number"] = {
		[TYPE_MODE_LOOSE] = SCAN_INFER_TYPE_ALL_TYPES,
		[TYPE_MODE_TIGHT] = SCAN_INFER_TYPE_NUMERIC_TYPES,
		[TYPE_MODE_EXACT] = SCAN_INFER_TYPE_NUMERIC_TYPES
	}
}

comparatorModeMap =
{
	["=="] = SCAN_COMPARE_EQUALS,
	["="] = SCAN_COMPARE_EQUALS,

	[">"] = SCAN_COMPARE_GREATER_THAN,
	[">="] = SCAN_COMPARE_GREATER_THAN_OR_EQUALS,
	["=>"] = SCAN_COMPARE_GREATER_THAN_OR_EQUALS,

	["<"] = SCAN_COMPARE_LESS_THAN,
	["<="] = SCAN_COMPARE_LESS_THAN_OR_EQUALS,
	["=<"] = SCAN_COMPARE_LESS_THAN_OR_EQUALS
}

function Process:scanFor(scanValue, scanComparator, typeMode)
	local this = type(self) == 'table' and self or Process.new(self)

	typeMode = typeMode or TYPE_MODE_EXACT
	scanComparator = type(scanComparator) == 'string' and comparatorModeMap[scanComparator] or scanComparator
	scanComparator = scanComparator or SCAN_COMPARE_EQUALS

	local raw_scanValue = nil
	local raw_scanType = nil
	local raw_scanTypeMode = nil

	if (typeModeMap[type(scanValue)]) then
		-- it's a basic primitive type
		raw_scanValue = {value = tostring(scanValue)}
		raw_scanType = 0
		raw_scanTypeMode = typeModeMap[type(scanValue)][typeMode]
	elseif type(scanValue) == "table" then
		if (scanValue.__schema) then
			--[[
				It's a structure of specific primitive types.
				Only exact scans and comparisons are allowed.
			]]
			assert(scanComparator == SCAN_COMPARE_EQUALS, "Structures can only be scanned using SCAN_COMPARE_EQUALS")

			raw_scanValue = scanValue
			raw_scanType = SCAN_VARIANT_STRUCTURE
			raw_scanTypeMode = SCAN_INFER_TYPE_EXACT
		elseif (scanValue.__name and scanValue.__type) then
			--[[
				It's a specific primitive type.
				When not used as strcture members, these types
				Will be constructed such that the name actually contains
				the value to search for. Thus __name is the value.
			]]
			raw_scanValue = {value = tostring(scanValue.__name)}
			raw_scanType = scanValue.__type
			raw_scanTypeMode = SCAN_INFER_TYPE_EXACT
		elseif (scanValue.__min and scanValue.__max) then
			error("Cannot search for range without specific primitive type. Try range(uint32, min, max).")
		end
	end

	local message = ""
	local success = (raw_scanValue and raw_scanTypeMode)
	if (success) then
		success, message = runScan(this.__nativeObject, raw_scanValue, raw_scanType, raw_scanTypeMode, scanComparator)
	else
		message = "Unable to deduce scan details for Lua type '" .. type(scanValue) ..  "': " .. table.show(scanValue, "")
	end

	assert(success, (message or "") .. "\n" .. table.show(scanValue, ""))
	return success, message
end

function array(input, len)
	local msg = "Can only make an array out of strongly-typed objects"
	assert(type(input) == 'table', msg)
	assert(input.__type, msg)
	assert(input.__name, msg)
	assert(input.__arraylen == nil, "Arrays cannot be nested!")
	assert(len > 0, "Arrays must have positive, non-zero length! (" .. len .. " specified)")

	local def = TYPE_DEFINITIONS[input.__type]
	assert(def, "Invalid type specified: "  .. table.show(input, ""))
	assert(def.isNumeric, "Arrays only support numeric types!")

	input.__arraylen = len
	return input
end

function range(a, b, c)
	local ALLOWED_INPUT_TYPES = {["number"] = true}

	local valueTransform = function(v) return v end
	if (c ~= nil) then
		local def = TYPE_DEFINITIONS[tostring(a)]
		assert(def, "Invalid type specified:"  .. table.show(a, ""))
		assert(def.isNumeric, "Ranges only support numeric types!")
		valueTransform = a
	end

	local values = (c ~= nil) and {b, c} or {a, b}
	assert(#values == 2, "Expected either '(type, min, max)' or '(min, max)' as arguments")

	for _, v in pairs(values) do
		assert(ALLOWED_INPUT_TYPES[type(v)], "Inputs must be numbers. Got '" .. type(v) .. "' with value: " .. v)
	end

	return valueTransform({__min = math.min(unpack(values)), __max = math.max(unpack(values))})
end

function struct(...)
	local structure = {}
	structure.__schema = {}

	for _, v in ipairs({...}) do
		assert(v.__name, "Structure cannot ccontain unnamed value!")
		assert(structure[v.__name] == nil, "Duplicate name entry '" .. v.__name .. "' in structure!")
		assert(v.__schema == nil, "Nested custom types are not yet supported (you have a struct in a struct)")
		assert((v.__arraylen or 1) > 0, "Structure cannot contain 0-length array!")

		local def = TYPE_DEFINITIONS[v.__type]
		assert(def, "Invalid type specified:"  .. table.show(v, ""))
		assert(def.isNumeric, "Structures only support numeric types!")

		structure.__schema[#structure.__schema + 1] = {__type = v.__type, __name = v.__name, __arraylen = v.__arraylen}
		structure[v.__name] = {}

		if (v.__arraylen) then
			while (#structure[v.__name] < v.__arraylen) do
				structure[v.__name][#structure[v.__name] + 1] = {}
			end
		end
	end

	return structure
end

function sizeof(t)
	local vtype = type(t)

	if (vtype == 'table') then
		if (t.__schema) then
			local size = 0
			for _, v in ipairs(t.__schema) do
				size = size + sizeof(v)
			end
			return size
		else
			assert(t.__type, "No type specified by table: " .. table.show(t, ""))
			assert(TYPE_DEFINITIONS[t.__type], "Invalid type specified: " .. table.show(t, ""))
			return sizeof(t.__type) * (t.__arraylen or 1)
		end
	else
		local typeInfo = (vtype == 'function') and TYPE_DEFINITIONS[tostring(t)] or TYPE_DEFINITIONS[t]
		assert(typeInfo, "Invalid type specified: " .. tostring(t))
		assert(typeInfo.size, "Unsizable type :" .. table.show(typeInfo, ""))
		return typeInfo.size
	end
end

function offsetof(struct, val)
	assert(type(struct) == "table", "Expected a table, got " .. type(struct))
	assert(struct.__schema, "No schema found for structure")
	assert(type(val) == "string", "Expected field name as a string, got " .. type(val))
	assert(struct[val], "Named value doesnt exist in structure: " .. val)
	
	local offset = 0
	for _, v in ipairs(struct.__schema) do
		if (v.__name == val) then
			return offset
		end
		offset = offset + sizeof(v)
	end
	return nil
end

function table.show(t, name, indent)
	local cart     -- a container
	local autoref  -- for self references

	--[[ counts the number of elements in a table
	local function tablecount(t)
		local n = 0
		for _, _ in pairs(t) do n = n+1 end
		return n
	end
	]]
	-- (RiciLake) returns true if the table is empty
	local function isemptytable(t) return next(t) == nil end

	local function basicSerialize (o)
		local so = tostring(o)
		if type(o) == "function" then
			local info = debug.getinfo(o, "S")
			-- info.name is nil because o is not a calling level
			if info.what == "C" then
				return string.format("%q", so .. ", C function")
			else 
				-- the information is defined through lines
				return string.format("%q", so .. ", defined in (" ..
					 info.linedefined .. "-" .. info.lastlinedefined ..
					 ")" .. info.source)
			end
		elseif type(o) == "number" or type(o) == "boolean" then
			return so
		else
			return string.format("%q", so)
		end
	end

	local function addtocart (value, name, indent, saved, field)
		indent = indent or ""
		saved = saved or {}
		field = field or name

		cart = cart .. indent .. field

		if type(value) ~= "table" then
			cart = cart .. " = " .. basicSerialize(value) .. ";\n"
		else
			if saved[value] then
				cart = cart .. " = {}; -- " .. saved[value] 
								.. " (self reference)\n"
				autoref = autoref ..  name .. " = " .. saved[value] .. ";\n"
			else
				saved[value] = name
				--if tablecount(value) == 0 then
				if isemptytable(value) then
					cart = cart .. " = {};\n"
				else
					cart = cart .. " = {\n"
					for k, v in pairs(value) do
						k = basicSerialize(k)
						local fname = string.format("%s[%s]", name, k)
						field = string.format("[%s]", k)
						-- three spaces between levels
						addtocart(v, fname, indent .. "   ", saved, field)
					end
					cart = cart .. indent .. "};\n"
				end
			end
		end
	end

	name = name or "__unnamed__"
	if type(t) ~= "table" then
		return name .. " = " .. basicSerialize(t)
	end
	cart, autoref = "", ""
	addtocart(t, name, indent)
	return cart .. autoref
end


function string.starts(String,Start)
	return string.sub(String,1,string.len(Start))==Start
end

function string.ends(String,End)
	return End=='' or string.sub(String,-string.len(End))==End
end