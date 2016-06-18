#pragma once
#include <string>
#include <fstream>
#include <vector>

#include "Polygon2d.h"

class IdfData {
	public:
		IdfData(const std::string &filename);
		Polygon2d *toPolygon2d();

	private:
		float conversion; //multiplyer to convert from the file's units to mm
		float thickness;
		std::vector<Outline2d> boardOutline;

		//support functions
		std::string getstring(std::string &line);
		int getint(std::string &line);
		float getfloat(std::string &line);
		bool process_header(std::ifstream &stream);
		bool process_board_outline(std::ifstream &stream);
		bool process_unknown(std::ifstream &stream, std::string type);
};
