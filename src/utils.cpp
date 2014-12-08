#include "utils.h"
#include <algorithm>

bool HasFileExt(const std::string &filename){
	for(int i=filename.size()-1;i>=0;--i){
		if(filename[i]=='.')return true;
		if(filename[i]=='/'||filename[i]=='\\')return false;
	}
	return false;
}
std::string GetFileNameExtLC(const std::string &filename){
	size_t pos=filename.rfind('.');
	if(pos==std::string::npos || pos==filename.size()-1)return "";
	std::string ext=filename.substr(pos+1);
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	return ext;
}
