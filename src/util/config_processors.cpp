/*
 * config_processors.cpp
 *
 *  Created on: Sep 24, 2015
 *      Author: Enis Bayramoglu
 */
#include "config_processors.h"
#include <scroll_ext/config_utils.hpp>
#include <utils/orxHashTable.h>
#include <utils/orxString.h>
#include <core/orxConfig.h>
#include <core/orxCommand.h>
#include <util/list.h>

struct substring {
    const char * begin;
    const char * end;
};

substring make_substring(const char * b, const char * e) {
    substring result; result.begin = b; result.end = e;
    return result;
}

bool empty(substring str) {
	return str.begin == str.end;
}

template <size_t BUFSIZE>
void copy_to_buffer(char (&buffer)[BUFSIZE], substring str) {
    size_t len = str.end - str.begin;
    size_t count = len<BUFSIZE-1 ? len : BUFSIZE-1;
    strncpy(buffer, str.begin, count);
    buffer[count] = 0;
}

struct split_result {
    substring first;
    substring rest;
};

class StringSet {
    orxHASHTABLE * table;
public:
    StringSet(): table(orxHashTable_Create(7, orxHASHTABLE_KU32_FLAG_NONE, orxMEMORY_TYPE_TEMP)) {}
    bool insert(const char * str) {
        orxU64 id = orxString_GetID(str);
        bool result = orxHashTable_Get(table, id);
        if(!result) {
            orxHashTable_Add(table, id, (void *)1);
        }
        return result;
    }
    bool insert(substring str) {
        char buffer[64];
        copy_to_buffer(buffer, str);
        return insert(buffer);
    }

    class iterator {
        orxHASHTABLE * table;
        orxHANDLE cur_it;
        orxHANDLE next_it;
    public:
        iterator(orxHASHTABLE * table): table(table), cur_it(orxNULL), next_it(orxHANDLE_UNDEFINED){}
        iterator(orxHASHTABLE * table, orxHANDLE it): table(table) {
            orxU64 id;
            void * val;
            cur_it = it;
            next_it = orxHashTable_GetNext(table, cur_it, &id, &val);
        }
        const char * operator*() {
            orxU64 id;
            void * val;
            orxHashTable_GetNext(table, cur_it, &id, &val);
            return orxString_GetFromID(id);
        }
        iterator & operator++() {
            orxU64 id;
            void * val;
            cur_it = next_it;
            next_it = orxHashTable_GetNext(table,next_it,&id, &val);
            return *this;
        }
        bool operator!=(const iterator & other) {
            return table != other.table || next_it != other.next_it;
        }
    };

    iterator begin() {return iterator(table, orxNULL);}
    iterator end() {return iterator(table); }

    ~StringSet() {orxHashTable_Delete(table);}
};

split_result split(substring s, char delim) {
    split_result result;
    result.first.begin = s.begin;
    result.first.end = s.begin;
    result.rest.end = s.end;
    while(*result.first.end != delim && result.first.end != s.end) {
        result.first.end++;
    }
    if(*result.first.end == delim) result.rest.begin = result.first.end+1;
    else result.rest.begin = result.first.end;
    return result;
}

struct link_properties {
    bool immediate;
    link_properties(): immediate(false) {}
    link_properties merge(const link_properties & other) {
        link_properties result;
        result.immediate = {immediate || other.immediate};
        return result;
    }
};


bool IsWhiteSpace(char c) {
    return c==' ' || c== '\t' || c=='\n';
}


bool non_word(char c) {
    if(c == '!' or IsWhiteSpace(c)) return true;
    else return false;
}

struct item_analysis {
    link_properties left_properties;
    substring item_name;
    link_properties right_properties;
};


item_analysis analyzeItem(substring item) {
    item_analysis result;
    result.item_name.begin = orxNULL;
    const char * i = item.begin;
    for(;i<item.end; i++) {
        char c = *i;
        if(c == '!') result.left_properties.immediate = true;
        else if(IsWhiteSpace(c)) continue;
        else break;
    }
    for(const char * j=i; j<item.end; j++) {
        char c = *j;
        if(non_word(c)) {
            result.item_name = make_substring(i,j);
            i=j;
            break;
        }
    }
    if(result.item_name.begin == orxNULL) {
        result.item_name = make_substring(i, item.end);
        return result;
    }
    else {
        for(; i<item.end; i++) {
            char c = *i;
            if(c == '!') result.right_properties.immediate = true;
            else if(IsWhiteSpace(c)) continue;
            else orxASSERT(0 && "%s is not a proper item.", item.begin);
        }
        return result;
    }
}


const size_t STACK_BUF_SIZE = 32;
class ConfigListBuilder {
public:
    ConfigListBuilder & operator << (const char * str) {
        list.push_back(Persist(str));
        return *this;
    }
    void WriteToConfig(const char * section) {
        size_t count = list.size();
        if(count < STACK_BUF_SIZE) WriteWithBuffer(section, count);
        else {
            const char ** strings = new const char *[count];
            list.copy_to_buffer(strings, count);
            orxConfig_SetListString(section, strings, count);
            delete[] strings;
        }
    }
private:
    orx::LinkList<const char *> list;
    void WriteWithBuffer(const char * section, size_t count) {
        const char * strings[STACK_BUF_SIZE];
        list.copy_to_buffer(strings);
        orxConfig_SetListString(section, strings, count);
    }
};


uint last_section_id = 0;
void InitConfigProcessors() {
    last_section_id = 0;
}


const char * generate_section_name() {
    char result[128];
    orxString_Print(result, "_g_%d", last_section_id);
    last_section_id++;
    return Persist(result);
}


void addAnimIfNew(StringSet & animations, ConfigListBuilder & animslist, const char * name) {
    bool new_anim = animations.insert(name);
    if(new_anim) animslist << name;
}


void generateLinkSection(ConfigListBuilder & lb,
                         substring src,
                         substring dst,
                         const link_properties & p) {
    auto section_name = generate_section_name();
    lb << section_name;
    char buffer[64];
    copy_to_buffer(buffer, src);
    auto src_section = orxConfig_GetString(buffer);

    copy_to_buffer(buffer, dst);
    auto dst_section = orxConfig_GetString(buffer);
    ConfigSectionGuard guard(section_name);
    orxConfig_SetString("Source", src_section);
    orxConfig_SetString("Destination", dst_section);
    if(p.immediate) orxConfig_SetString("Property", "immediate");
}


void processItemPair(StringSet & as, ConfigListBuilder & lb, substring src, substring dst) {
    item_analysis a_src = analyzeItem(src);
    item_analysis a_dst = analyzeItem(dst);
    as.insert(a_src.item_name);
    as.insert(a_dst.item_name);
    generateLinkSection(lb, a_src.item_name, a_dst.item_name, a_src.right_properties.merge(a_dst.left_properties));
}


void processSubtermPair(StringSet & as, ConfigListBuilder & lb, substring left, substring right) {
     for(split_result left_alternative = split(left, ',');
         !empty(left_alternative.first);
         left_alternative = split(left_alternative.rest, ',')) {
         for(split_result right_alternative = split(right, ',');
             !empty(right_alternative.first);
             right_alternative = split(right_alternative.rest, ',')) {
             processItemPair(as, lb, left_alternative.first, right_alternative.first);
         }
     }
}


void orxFASTCALL ProcessAnimGraphCommand(orxU32 _u32ArgNumber, const orxCOMMAND_VAR *_astArgList, orxCOMMAND_VAR *_pstResult) {
    orxASSERT(_u32ArgNumber == 1 && _astArgList[0].eType == orxCOMMAND_VAR_TYPE_STRING);
    ProcessAnimGraph(_astArgList[0].zValue);
}

const char * links_key = "AnimationGraph";

extern "C" {

void ProcessAnimGraph(const char * section) {
    ConfigSectionGuard guard(section);

    orxASSERT(orxConfig_HasValue(links_key) &&
            "ProcessAnimGraph needs a %s key in section %s", links_key, section);

    ConfigListBuilder links_preprocessor;
    for(orxU32 i=0, endi=orxConfig_GetListCounter(links_key);i<endi;++i) {
        const char * term = orxConfig_GetListString(links_key, i);
        if(term[0]=='+') {
            ConfigSectionGuard("_config_processor_tmp");
            orxConfig_SetString("_tmp", term+1);
            for(orxU32 j=0, endj=orxConfig_GetListCounter("_tmp");j<endj;++j) {
                links_preprocessor << orxConfig_GetListString("_tmp", j);
            }
        } else links_preprocessor << term;
    }
    links_preprocessor.WriteToConfig(links_key);

    StringSet animations;
    ConfigListBuilder link_list_builder;
    for(orxU32 i=0, endi=orxConfig_GetListCounter(links_key);i<endi;++i) {
        const char * term_ = orxConfig_GetListString(links_key, i);
        substring term = {term_, term_ + strlen(term_)};
        orxASSERT(strlen(term_)>0 && "Empty animation graph term");
        split_result subterms = split(term, ':');
        if(empty(subterms.rest)) {
            animations.insert(subterms.first);
            generateLinkSection(link_list_builder, subterms.first, subterms.first, link_properties());
        } else {
            substring right;
            do {
                substring left = subterms.first;
                subterms = split(subterms.rest,':');
                right = subterms.first;
                processSubtermPair(animations, link_list_builder, left, right);
            } while(!empty(subterms.rest));
        }
    }


    ConfigListBuilder anim_list_builder;
    for(StringSet::iterator it = animations.begin(); it!= animations.end(); ++it) {
        anim_list_builder << orxConfig_GetString(*it);
    }

    link_list_builder.WriteToConfig("LinkList");
    anim_list_builder.WriteToConfig("AnimationList");
}

void RegisterProcessAnimGraphCommand() {
    orxCOMMAND_VAR_DEF arg;
    arg.eType = orxCOMMAND_VAR_TYPE_STRING;
    arg.zName = "AnimationGraphSection";
    orxCOMMAND_VAR_DEF ret;
    ret.eType = orxCOMMAND_VAR_TYPE_NONE;
    ret.zName = "None";
    orxCommand_Register("Config.ProcessAnimGraph", ProcessAnimGraphCommand, 1, 0, &arg, &ret);
}

const orxSTRING orxObjectX_GetAnimForTag(orxOBJECT *_pstObject, const orxSTRING _zAnimTag) {

    // Push the object section
    ConfigSectionGuard object_guard( orxObject_GetName(_pstObject) );

    // Push the AnimationSet section
    ConfigSectionGuard animset_guard( orxConfig_GetString("AnimationSet") );

    return orxConfig_GetString(_zAnimTag);

}


orxSTATUS orxObjectX_SetCurrentAnimByTag(orxOBJECT *_pstObject, const orxSTRING _zAnimTag) {

    return orxObject_SetCurrentAnim(_pstObject, orxObjectX_GetAnimForTag(_pstObject, _zAnimTag) );
}


orxSTATUS orxObjectX_SetTargetAnimByTag(orxOBJECT *_pstObject, const orxSTRING _zAnimTag) {

    return orxObject_SetTargetAnim(_pstObject, orxObjectX_GetAnimForTag(_pstObject, _zAnimTag) );
}


orxBOOL orxObjectX_CurrentAnimHasTag(orxOBJECT *_pstObject, const orxSTRING _zAnimTag) {

    return orxObject_IsCurrentAnim(_pstObject, orxObjectX_GetAnimForTag(_pstObject, _zAnimTag) );
}


orxBOOL orxObjectX_TargetAnimHasTag(orxOBJECT *_pstObject, const orxSTRING _zAnimTag) {

    return orxObject_IsTargetAnim(_pstObject, orxObjectX_GetAnimForTag(_pstObject, _zAnimTag) );
}

}
