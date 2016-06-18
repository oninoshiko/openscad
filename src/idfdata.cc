#include <fstream>
#include <string>
#include <boost/algorithm/string.hpp>

#include "printutils.h"
#include "idfdata.h"

using namespace std;

IdfData::IdfData(const std::string &filename){

	std::ifstream stream(filename.c_str());

	if (!stream.good()){
		PRINTB("WARNING: Can't open IDF file '%s'.", filename);
		return;
	}

	std::string line;
	while(!stream.eof()){
		std::getline(stream, line);
		line = getstring(line);	
		boost::to_upper(line);	
		if(line.length()>0 && line[0] != '#'){
			if(line==".HEADER") process_header(stream); 
			else if(line==".BOARD_OUTLINE") process_board_outline(stream); 
			else process_unknown(stream, line); 

		}
	}
}

Polygon2d *IdfData::toPolygon2d(){
	int i = 0;
	Polygon2d *poly = new Polygon2d();
	while(boardOutline.size()>i){
		poly->addOutline(boardOutline[i]);
		i++;
	}
	return poly;

}

bool IdfData::process_unknown(std::ifstream &stream, std::string type){ 
	PRINTB("WARNING: '%s' not understood section", type);

	std::string endtag = ".END_"+type.substr(1, string::npos);
	while(!stream.eof()){
		std::getline(stream, type);
		boost::trim(type);
		boost::to_upper(type);
		if(endtag == type) return true;		
	}
	return false;
}

bool IdfData::process_header(std::ifstream &stream){
	std::string line, stok;
	float ftok;
	int itok;
	
	//parse record 2
	//type IDFver srcID date BoardVer
	std::getline(stream, line);
	stok = getstring(line);
	boost::to_upper(stok);
	if (stok != "BOARD_FILE"){
		PRINTB("ERROR: unsupported file type '%s'", stok);
		return false;
	}

	ftok = getfloat(line);
	if (ftok != 3.0){
		PRINTB("ERROR: Only IDF version 3.0 is supported, not version '%f'", ftok);
		return false;
	}
	//stok = getstring(line); //Discard Source System ID
	//stok = getstring(line); //Discard Date
	//itok = getint(line);    //Discard Boardfile Version Number


	//parse record 3
	//BRDname units
	std::getline(stream, line);
	stok = getstring(line);   //Disacrd Boardname

	stok = getstring(line);
	boost::to_upper(stok);
	if(stok == "THOU") conversion=0.0254;
	else if (stok == "MM") conversion=1;
	else  PRINTB("ERROR: Units must be either 'MM' or 'THOU', not '%s'.", stok);

	//parse record 4
	//.END_HEADER
	std::getline(stream, line);
	boost::trim_right(line);
	boost::to_upper(line);
	if (line != ".END_HEADER"){
		PRINT("ERROR: '.END_HEADER' not record 4 of HEADER section");
		return false;
	}
	return true;
}

bool IdfData::process_board_outline(std::ifstream &stream){
	std::string line;

	//parse record 2
	//thickness
	std::getline(stream, line);
	thickness = getfloat(line);

	//Because record 3 can occur any number of times, we check if it's record 4
	//then parse it as 3 if it's not. to_upper is okay here, because record 3 only
	//has ints and flots
	unsigned int label;
	float x, y;
	while(std::getline(stream, line)){
		boost::trim(line);
		boost::to_upper(line);
		PRINTB("DEBUG: %s", line);
		//parse record 4
		//.END_BOARD_OUTLINE
		if (line==".END_BOARD_OUTLINE") return true;
	
		//parse record 3 (multiple)
		//Label x y angle
		label = getint(line);
		x = getfloat(line);
		y = getfloat(line);
		if (getfloat(line) != 0) PRINT("WARNING: arc or circle ignored, treating as line.");

		if(boardOutline.size() < label+1) boardOutline.push_back(Outline2d());
		boardOutline[label].vertices.push_back(Vector2d(x,y));
	}
	return false;
}

std::string IdfData::getstring(std::string &line){
	boost::trim(line);
	std::string r="";
	if (line[0] == '"'){
		std::string::size_type i = line.find('"', 1);
		r = line.substr(1, i-1);
		line.erase(0,i+1);
	} else {
		std::string::size_type i = line.find(' ', 0);
		r = line.substr(0, i);
		line.erase(0,i+1);
	}
	return r;
}

int IdfData::getint(std::string &line){
	int r=0;
	std::string::size_type i=0;

	boost::trim(line);
	try{
		r = std::stoi(line, &i);
	}catch(exception& e){
		PRINTB("Exception: '%s'.", e.what());
	}
	line.erase(0, i+1);
	return r;
}

float IdfData::getfloat(std::string &line){
	float r=0.0;
	std::string::size_type i=0;

	boost::trim(line);
	try {
		r = std::stof(line, &i);
	}catch(exception& e){
		PRINTB("Exception: '%s'.", e.what());
	}
	line.erase(0, i+1);
	return r;
}
