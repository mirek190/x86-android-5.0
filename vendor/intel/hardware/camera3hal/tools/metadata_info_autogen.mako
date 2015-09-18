## -*- coding: utf-8 -*-
/*
 * Copyright (C) 2014 The Android Open Source Project
 *               2014 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
<%doc>
    Mako template to generate MetadataInfoAutoGen.h
</%doc>
<%!
  import re
  import metadata_model
  from metadata_helpers import _is_sec_or_ins, find_parent_section, find_unique_entries, csym, find_all_sections, ctype_enum

##===========================  HELPER METHODS ==================================
  def enum_value_name(entry_name):
    out = re.sub(r'\.', "_", entry_name) + "_values"
    return out


  def static_enum_table_name_and_size(entry, enum2StringTables):
    """
     Returns a list of 2 strings with the name of the C table that describes
     the enums associated with this static metadata entry and the size
    """
    ret = ["NULL", "0"]

    if entry.enum:
        ret[0] = enum_value_name(entry.name)
        ret[1] = str(len(list(entry.enum.values)))
    else:

        tableNameAndSize =  findAssociatedEnumTable(entry, enum2StringTables)

        if tableNameAndSize[1] > 0:
           ret[0] = enum_value_name(tableNameAndSize[0])
           ret[1] = str(tableNameAndSize[1])

    return ret


  def findAssociatedEnumTable(entry, enum2StringTables):
    """
        For the given static metadata entry that is an enumList this method
        provides a list with 2 elements
            - the name of the associated enum entry that is listing.
            - The number of enum values for this enum entry
        Given the name of the static entry there are 4 rules to find a matching
        name of the enum entry:
        Rule 1: remove available from the name and change Modes--> Mode
          ex:android.control.aeAvailableModes --> android.control.aeMode
        Rule 2: remove available + remove final s + add Mode
          ex:android.control.availableEffects --> android.control.EffectMode
        Rule 3: split name by string available. remove Modes from second token
                if leftover string is present in first token add mode to token 1
            ex: android.edge.availableEdgeModes-->android.edge.mode
        Rule 4: same as rule 1 but remove .info. from the section name
            ex: android.lens.info.availableOpticalStabilization --> android.lens.opticalStabilizationMode
    """
    if entry.container:
        patternNames = []

        ## Remove .info. and sub Modes to Mode
        infolessEntryName = re.sub(r'.info.',".",entry.name.lower(),1)
        cleanEntryName = re.sub(r'modes',"mode",infolessEntryName,1)

        mo = re.match(r'(.*)available(.*)', cleanEntryName, re.I)
        if mo:
            patternNames.append( mo.group(1)+mo.group(2))    ## Rule 1
            patternNames.append( mo.group(1)+mo.group(2).rstrip('s'))
            if "mode" not in mo.group(2):                    ## Rule 2
                patternNames.append(mo.group(1)+mo.group(2).rstrip("s")+"mode")
            ## Rule 3
            tmp = re.sub(r'mode',"",mo.group(2),1)
            if tmp in mo.group(1).lower():
                patternNames.append(mo.group(1)+"mode")

     ## Now that we have the list of possible names let find them in the
     ## list of enum tables we created before
        for tableName in enum2StringTables.keys():
            for pName in patternNames:
                if pName == tableName.lower():
                    return tableName, len(enum2StringTables[tableName])


    return ["NULL", 0]

  def getArrayAttributes(entry):
    """
        Detects if the entry container is an array and stores the expected sizes
        for each dimension.
        The maximum number of dimensions is 3
        In case the size of the array in one dimension is variable this is
         marked as -1.
        It also adds the container typedef
        returns a dictionary with the following entries
        - returnItem: stores the string to be appended in the mako template
        - isArray : boolean
        - dimSizes: list of 3 sizes for each dimension
        - typedef: typedef object for the entry. this is defined in the metadata
                   model
    """
    ret = "false, {0,0,0}, 0"
    isArray = False
    dimSizes = [0,0,0]
    typedef = None
    retItem = {}

    MAX_ARRAY_DIMENSIONS = 3
    if entry.container:

        isArray = True
        dim = 0
        for s in entry.container_sizes:
            dimSizes[dim] = s
            dim = dim + 1
        if dim > MAX_ARRAY_DIMENSIONS:
            print "Warning: Array dimensions for entry"+entry.name+"bigger than max(3)"
        typedef =   entry.typedef

        ## construct the final string
        if (isArray):
            ret = "true, "
        else:
            ret = "false, "

        ret += "{"
        for d in dimSizes:
            try:
              ret = ret + str(int(d))+","
            except ValueError:
              ret = ret + "-1,"

        ret = ret.rstrip(",") + "}"
        if typedef:
            ret = ret+", "+csym(typedef.name)
        else:
            ret = ret + ", 0"

    retItem["returnItem"] = ret
    retItem["isArray"] = isArray
    retItem["dimSizes"] = dimSizes
    retItem["typedef"] = typedef
    return retItem

##===========================  MAIN CALLS =====================================
  def createEnum2StringTables(root):
    """
        Creates a dictionary where the keys is the entry name and the value
        is a list of strings that contain the entry of a table of enum and its
        string representation. This is used to find the enum value given a string
        found in our camera3_profiles
    """
    enumToStringTables = {}
    for sec in find_all_sections(root):
        for idx,entry in enumerate(find_unique_entries(sec)):
            if entry.enum:
              table = []
              for val in entry.enum.values:
                tableEntry = "\""+val.name+"\", "+csym(entry.name)+"_"+val.name
                table.append(tableEntry)
              enumToStringTables[entry.name] = table

    return enumToStringTables

  def createMeta2StringTable(root):
    """
        Creates a list of lists that contain two elements:
        1- the name of each possible metadata tag that can be used as capture
            setting or result.
        2- the C symbol associated
        We exclude the static metadata tags, and the synthetic tags that are
        created by the service layer. Only control and dynamic are listed
        in this table. The static tags are already in the other table (see 
        createStaticMetaTable)
        This table will be used to parse the name of metadata tags in the 
        camera3_profiles.xml configuration file.
    """
    metaToStringTable = []
    for sec in find_all_sections(root):
        for idx,entry in enumerate(find_unique_entries(sec)):
            if entry.kind != 'static' and not entry.synthetic:
                ## Remove redundant android. from the name
                name = re.sub('^android\.', "",entry.name)
                metaToStringTable.append([name,csym(entry.name)])
                print name+" --> "+ csym(entry.name)

    return metaToStringTable

##-----------------------------------------------------------------------------
  def getStaticArrayTypedefs(root):
    tdInfo = {}
    for sec in find_all_sections(root):
     for entry in find_unique_entries(sec):
        if entry.kind == "static" and not entry.synthetic:
            if entry.container and entry.typedef:
                tdInfo[entry.typedef.name] = entry.typedef.name
    return tdInfo

##-----------------------------------------------------------------------------
  def createStaticMetaTable(root, enum2StringTables):
    """
        Populates a table of structs of type metadata_tag_t defined in file
        Metadata.h. It parses the metadata XML model to fill each of the entries.
        This table stores details about each of the static metadata tags that
        the HAL needs to provide to the client. This static metadata is also
        called camera characteristics.
        The information stored in this table is used by the class CameraProfiles
        when is parsing the camera3_profiles.xml file.
        The information in each entry is:
         - name: text representation of the name of the metadata tag as it
                 appears in camera3_profiles.xml
         - value: C enum value for that tag
         - type: basic C type (int, float, double, byte)
         - enumTable: in case that the static metadata is an enum list this field
                      contains the name of the enum table created by the method
                      createEnum2StringTables to convert from string to enum.
         - tableLength: size of the enum table.
    """
    staticMetaHelperTable = {}
    for sec in find_all_sections(root):
      for entry in find_unique_entries(sec):
        if entry.kind == "static" and not entry.synthetic:

          ####  construct the name  ####
          if isinstance(sec, metadata_model.InnerNamespace):
              full_sec_name = find_parent_section(sec).name+"."+sec.name
          else:
              full_sec_name = sec.name

          name = "\""+full_sec_name + "."+entry.name_short+"\""

          ### type
          baseType = ctype_enum(entry.type)

          #### construct the enum name ####
          enumName =  csym(entry.name)

          #### construct name of enum table ####
          enumTableNameAndSize = static_enum_table_name_and_size(entry, enum2StringTables)

          #### construct Array attributes ####
          arrayAttributes = getArrayAttributes(entry)

          sep = ", "
          tableEntry = "{"+name + sep + enumName + sep + baseType + sep
          tableEntry += enumTableNameAndSize[0] + sep + enumTableNameAndSize[1]
          tableEntry += sep + arrayAttributes["returnItem"] +" },"
          staticMetaHelperTable[entry.name] = tableEntry

    return staticMetaHelperTable

%>\
\
##=========================================================================
/**
 * ! Do not edit this file directly !
 * ! Do not include this file directly !
 *
 * Generated automatically from metadata_info_autogen.mako
 * This file logically belongs to Metadata.h
 * check readme.txt to understand how this is used
 */

<%
  enumTables = createEnum2StringTables(metadata)
  sortedEnums = enumTables.keys()
  sortedEnums.sort()

  staticArrayTypedefs = getStaticArrayTypedefs(metadata)

  staticHelperTable = createStaticMetaTable(metadata,enumTables)

  availableControls = createMeta2StringTable(metadata)

%>\
\

 % for tableName in sortedEnums:
const metadata_value_t ${enum_value_name(tableName)}[] = {
     % for entry in enumTables[tableName]:
                 {${entry} },
     % endfor
         };

 % endfor

const metadata_value_t metadataNames[] = {
    %for entry in availableControls:
        {"${entry[0]}", ${entry[1]}},
    %endfor
};

/*
 * \enum static_metadata_array_typedef
 *
 * Enumeration of the possible types that can be stored in array container of
 * static metadata tags. These are composite types made of basic types.
 * The parsing of those should be handled differently
 *
 */
enum static_metadata_array_typedef {
        TYPEDEF_NONE = 0,
 %for td in staticArrayTypedefs:
        ${csym(td)},
 %endfor
};


const metadata_tag_t android_static_tags_table[] = {
%for sec in find_all_sections(metadata):
%  if not isinstance(sec, metadata_model.InnerNamespace):
    // ${csym(sec.name)}
%  endif
%  for entry in find_unique_entries(sec):
%   if entry.kind == "static" and not entry.synthetic:
    ${staticHelperTable[entry.name]}
%   endif
%  endfor
%endfor
};

#define STATIC_TAGS_TABLE_SIZE ${len(staticHelperTable)}

