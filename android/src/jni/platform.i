%module(directors="1") PlatformModule
%{
#include "src/platform.h"
%}

%include "std_string.i"
%include "std_vector.i"

%feature("director");

%copyctor item_info;

%template(item_info_vector) std::vector<item_info>;
%template(item_inventory_info_vector) std::vector<item_inventory_info>;

%include "src/platform.h"

