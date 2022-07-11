#pragma once

/*
    <usage documentation>
*/

#include "sfall/sfall.h"
#include "sfall/lib.arrays.h"
#include "sfall/lib.strings.h"

#define INI_SETTINGS_DEBUG_ERROR_MSG_PREFIX "[IniSetting Error] "
#define INI_SETTINGS_VALID_LOAD_OPTIONS "prefix_types,section_maps,concat_fields,store_array,field_pointers"
#define INI_SETTINGS_PREFIX_INT "i"
#define INI_SETTINGS_PREFIX_FLOAT "f"
#define INI_SETTINGS_PREFIX_STRING "s"
#define INI_SETTINGS_PREFIX_INT_ARRAY "ai"
#define INI_SETTINGS_PREFIX_FLOAT_ARRAY "af"
#define INI_SETTINGS_PREFIX_STRING_ARRAY "as"
#define INI_SETTINGS_PREFIX_FIELD_POINTER "p"
#define INI_SETTINGS_FIELD_SECTION_MAP_SEPARATOR "."
#define INI_SETTINGS_FIELD_CONCAT_SEPARATOR ":"

procedure __IniSettings_AddFieldsToConfig(variable options, variable config_map, variable section_map, variable section_fields) begin
    variable field_name, field_value;

    if options.concat_fields then begin
        variable concat_field_found;
        while true do begin
            concat_field_found = false;
            foreach field_name: field_value in section_fields begin
                if is_in_string(field_name, INI_SETTINGS_FIELD_CONCAT_SEPARATOR) then begin
                    concat_field_found = true;
                    variable field_name_parts = string_split(field_name, INI_SETTINGS_FIELD_CONCAT_SEPARATOR);
                    variable field_name_without_separator = field_name_parts[0];
                    if map_contains_key(section_fields, field_name_without_separator) then
                        section_fields[field_name_without_separator] = section_fields[field_name_without_separator] + field_value;
                    else
                        section_fields[field_name_without_separator] = field_value;
                    section_fields[field_name] = 0;
                    break;
                end
            end
            if not concat_field_found then break;
        end
    end

    if options.prefix_types then begin
        foreach field_name: field_value in section_fields begin
            if strlen(field_name) < 3 then begin
                debug_msg(INI_SETTINGS_DEBUG_ERROR_MSG_PREFIX "Invalid ini field name: '" + field_name + "'. Too short. Expected prefix (i,f,s,ai,af,as,p) followed by name.");
                return false;
            end
            if string_starts_with(field_name, INI_SETTINGS_PREFIX_INT) then
                section_map[substr(field_name, 1, 0)] = atoi(field_value);
            else if string_starts_with(field_name, INI_SETTINGS_PREFIX_FLOAT) then
                section_map[substr(field_name, 1, 0)] = atof(field_value);
            else if string_starts_with(field_name, INI_SETTINGS_PREFIX_STRING) then
                section_map[substr(field_name, 1, 0)] = field_value;
            else if string_starts_with(field_name, INI_SETTINGS_PREFIX_INT_ARRAY) then begin
                variable int_array = string_split_ints(field_value, ",");
                if options.store_array then fix_array(int_array);
                section_map[substr(field_name, 2, 0)] = int_array;
            end else if string_starts_with(field_name, INI_SETTINGS_PREFIX_FLOAT_ARRAY) then begin
                variable float_array = string_split(field_value, ",");
                variable i, value;
                foreach i: value in float_array float_array[i] = atof(value);
                if options.store_array then fix_array(float_array);
                section_map[substr(field_name, 2, 0)] = float_array;
            end else if string_starts_with(field_name, INI_SETTINGS_PREFIX_STRING_ARRAY) then begin
                variable string_array = string_split(field_value, ",");
                if options.store_array then fix_array(string_array);
                section_map[substr(field_name, 2, 0)] = string_array;
            end else begin
                if options.field_pointers and string_starts_with(field_name, INI_SETTINGS_PREFIX_FIELD_POINTER) then begin
                    variable target = config_map;
                    variable target_parts = string_split(field_value, INI_SETTINGS_FIELD_SECTION_MAP_SEPARATOR);
                    variable target_part;
                    foreach target_part in target_parts begin
                        if map_contains_key(target, target_part) then begin
                            target = target[target_part];
                        end else begin
                            debug_msg(INI_SETTINGS_DEBUG_ERROR_MSG_PREFIX "Target for field pointer '" + field_name + "' not found: " + field_value + ".");
                            return false;
                        end
                    end
                    section_map[substr(field_name, 1, 0)] = target;
                end else begin
                    debug_msg(INI_SETTINGS_DEBUG_ERROR_MSG_PREFIX "Invalid ini field name: '" + field_name + "'. Expected prefix (i,f,s,ai,af,as,p) but found none.");
                    return false;
                end
            end
        end
    end

    return true;
end

procedure IniSettings_LoadIni(variable ini_path, variable options) begin
    if not options then options = {};

    // Verify option names
    variable valid_options = string_split(INI_SETTINGS_VALID_LOAD_OPTIONS, ",");
    variable option_name, option_value;
    foreach option_name: option_value in options
        if scan_array(valid_options, option_name) == -1 then begin // not found!
            debug1f("Unsupported IniSettings option: %s", option_name);
            return;
        end

    // Set default for options which were not explicitly provided
    foreach option_name in valid_options
        if not map_contains_key(options, option_name) then
            options[option_name] = true;

    variable config = {};

    // Store the array in memory for this game session
    if options.store_array then fix_array(config);

    variable section_names = get_ini_sections(ini_path);
    variable section_name;

    foreach section_name in section_names begin
        variable section = config;

        if is_in_string(section_name, INI_SETTINGS_FIELD_SECTION_MAP_SEPARATOR) and options.section_maps then begin
            variable section_name_parts = string_split(section_name, ".");
            variable section_name_part;
            foreach section_name_part in section_name_parts begin
                if map_contains_key(section, section_name_part) then
                    section = section[section_name_part];
                else begin
                    variable section_map = {};
                    if options.store_array then fix_array(section_map);
                    section[section_name_part] = section_map;
                    section = section_map;
                end
            end

            variable section_fields = get_ini_section(ini_path, section_name);
            variable valid = __IniSettings_AddFieldsToConfig(options, config, section, section_fields);
            if not valid then return;

        end else begin
            display_msg("TODO - [section] without .");
            // section = config[section_name];
            // if not section then begin

            // end
        end
    end

    return config;
end
