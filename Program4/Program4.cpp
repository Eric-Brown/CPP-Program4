// Program4.cpp : Defines the entry point for the console application.
//
#include <experimental\filesystem>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <ios>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <numeric>
#include <vector>
#include <functional>
#include <sstream>

double vt;
size_t width;
size_t pulse_delta;
double drop_ratio;
size_t below_drop_ratio;
enum INIVALUES
{
	VT,WIDTH, PULSE_DELTA, DROP_RATIO,BELOW_DROP_RATIO, BAD_TOKEN
};
INIVALUES FromToken(std::string token);
bool IsValidIni(std::experimental::filesystem::path path)
{
	//parameters for vt, width, pulse_delta, drop_ratio, below_drop_ratio are all read from a *.ini file
	//at program start up
	//these parameters alone should be chosen
	//lines with # are comments are are ignored
	std::stringstream buffer;
	std::ifstream initialize(path);
	if (initialize)
		std::copy(std::istream_iterator<std::string>(initialize), std::istream_iterator<std::string>(), std::ostream_iterator<std::string>(buffer));
	std::string line;
	size_t valCount = 0;

		try
		{
			std::getline(buffer, line);
			std::copy_if(line.begin(), line.end(), line.begin(), [](int ch)-> bool {return !isspace(ch); });
			if ((*line.begin()) == '#')//do something 
				line = line.substr(std::find(line.begin(), line.end(), '\n') - line.begin(), 3);
			while (line.length() > 0) {
				auto equalIter = std::find(line.begin(), line.end(), '=');
				std::string token(line.begin(), equalIter);
				++equalIter;
				std::string value(equalIter, std::find_if(equalIter, line.end(), [](int val)->bool {return isalpha(val); }));
				line = std::string(equalIter + value.length(), line.end());
				switch (FromToken(token))
				{
				case VT:
					++valCount;
					vt = std::stod(value);
					break;
				case WIDTH:
					++valCount;
					width = std::stoi(value);
					break;
				case PULSE_DELTA:
					++valCount;
					pulse_delta = std::stod(value);
					break;
				case DROP_RATIO:
					++valCount;
					drop_ratio = std::stod(value);
					break;
				case BELOW_DROP_RATIO:
					++valCount;
					below_drop_ratio = std::stoi(value);
					break;
				case BAD_TOKEN:
					return false;
				default:
					break;
				}
			}
		}
		catch (std::invalid_argument& ex)
		{
			return false;
		}
		catch (std::out_of_range& ex)
		{
			return false;
		}
		catch (std::exception& ex)
		{
			std::cout << ex.what() << std::endl;
			return false;
		}
	
	if (valCount != 5) return false;
	return true;
}

INIVALUES FromToken(std::string token)
{
	if (token.length() == 0) return BAD_TOKEN;
	if (token == "vt") return VT;
	if (token == "width") return WIDTH;
	if (token == "pulse_delta") return PULSE_DELTA;
	if (token == "drop_ratio") return DROP_RATIO;
	if (token == "below_drop_ratio") return BELOW_DROP_RATIO;
	return BAD_TOKEN;
}

std::vector<double> SmoothInput(const std::vector<double>& raw)
{
	std::vector<double> altered;
	//Transform negates the values read in
	std::transform(std::begin(raw), std::end(raw), back_inserter(altered), [](double val)->double {return -val; });
	auto weightedBegin = std::begin(altered) + 3;
	auto weightedEnd = std::end(altered) - 3; 
	//Transform is done in place
	//Transform that takes a weighted average
	std::transform(weightedBegin, weightedEnd, weightedBegin, [&](double ignored)-> double {
		double average = ((*weightedBegin - 3) + 2 * (*(weightedBegin - 2)) + 3 * (*(weightedBegin - 1)) + 3 * (*(weightedBegin)) +
			3 * (*(weightedBegin + 1)) + 2 * (*(weightedBegin + 2)) + (*weightedBegin + 3)) / 15;
		++weightedBegin;
		return average;
	});
	//"Then copy the last 3 values over to fill out the smoothed vector"
	//There is no need for this since I am doing a transform on the original vector.
	return std::vector<double>(std::begin(altered) + 3, std::end(altered));
}

bool DetectRise(double currVal, std::vector<double>::iterator position)
{
	static std::vector<double>::iterator PreservedPosition;
	if (PreservedPosition == std::vector<double>::iterator())
		PreservedPosition = position;
	return (*PreservedPosition++ + 2) - currVal > vt;
}

bool DetectPeak(double currVal, std::vector<double>::iterator position)
{
	static std::vector<double>::iterator PreservedPosition;
	if (PreservedPosition == std::vector<double>::iterator())
		PreservedPosition = position;
	return (*PreservedPosition++ + 1) < currVal;
}

std::vector<double>::const_iterator  DetectPulses(const std::vector<double>& smoothed)
{
	bool found = false;
	//find a pulse by looking for a rise over three consequtive points
	//if the rise ((y_i) + 2 - y_i) > voltage threshhold, then pulse beings at position i
		//We print out position i
	auto startCapture = smoothed.begin();
	//auto begin = std::find_if(startCapture, smoothed.end() - 2, [=](double currVal)->bool {return *(smoothed.begin() + 2) - currVal > vt; })

=	/*
	if (begin != smoothed.end()) result.push_back(begin - smoothed.begin());
	auto peak = std::find_if(smoothed.begin() + (begin - smoothed.begin()), smoothed.end() - 1, [=](double val)->bool {return val > *(smoothed.begin() + 1); });
	if (peak != smoothed.end()) result.push_back(peak - smoothed.begin());
	//move forward until data begins to decrease
	//once there is a decrease, search begins again
	while (begin != smoothed.end())
	{
		begin = std::find_if(smoothed.begin() + (peak - smoothed.begin()), smoothed.end(), [=](double currVal)->bool {return *(smoothed.begin() + (peak - smoothed.begin() + 2)) - currVal > vt; });

		//PIGGYBACKS
		//drop_ratio = number less than 1
		//below_drop_ratio = the number of values less than drop_ratio
		//pulse_delta = the gap between pulses to look for piggybacks
		//Once you have detected pulses, check for other pulses that are <= pulse_delta
		//if you have found one, find how many points btwn peak and start of next pulse (non-inclusive)
		//counted only if they are below drop_ratio * peak height
		//if this counted number exceeds below_drop_ratio omit the first pulse
		if (peak - begin <= pulse_delta)
		{
			if (std::count_if(peak, begin, [=](double val)->bool {return val < drop_ratio * (*peak); }) > below_drop_ratio)
				continue;
		}
		peak = std::find_if(smoothed.begin() + (begin - smoothed.begin()), smoothed.end() - 1, [=](double val)->bool {return val > *(smoothed.begin() + (begin - smoothed.begin())); });
		if (begin != smoothed.end() && peak != smoothed.end())
		{
			result.push_back(begin - smoothed.begin());
			result.push_back(peak - smoothed.begin());
		}
		
	}
	return result;
	*/
}

void Report(std::vector<double>::const_iterator result, const std::vector<double>& raw, const std::vector<double>& smoothed)
{
	std::vector<double> accumlated;
	while (result != smoothed.end())
	{
		std::cout << (result - smoothed.begin()) << " ";
		accumlated.push_back(std::accumulate(raw.begin() + 3 + (result - smoothed.begin()), std::adjacent_find(smoothed.begin() + (result - smoothed.begin()), smoothed.end(), [](double a, double b)->bool {return a > b; }), 0));
	}
	std::cout << std::endl;
	//std::copy(accumlated.begin(), accumlated.end(), std::ostream_iterator<std::string>(std::cout, " "));
	std::cout << std::endl;
}


int main(int argc, const char** argv)
{
	//if a dat file has no pulses ignore it
	//for those that do, report start position and areas in a strict format


	//find *.ini files
	using namespace std::experimental::filesystem;
	path currPath = current_path().parent_path();
	std::error_code code;
	recursive_directory_iterator curr(currPath, code);
	//finding all *.ini files in current directory
	while (curr != recursive_directory_iterator())
	{
//		std::cout << (*curr) << "looking at this\n";
		if ((*curr).path().extension().string().find(".ini") != std::string::npos)
			if (IsValidIni((*curr).path()))
				break;
			else
			{
				std::cout << (*curr).path() << "is not a valid *.ini file." << std::endl;
			}
		++curr;
	}
	if (curr == recursive_directory_iterator())
	{
		std::cout << "No valid *.ini found. Terminating program." << std::endl;
		return 0;
	}
	//finding all *.dat files in current directory
	curr = recursive_directory_iterator(currPath, code);
	while (curr != recursive_directory_iterator())
	{
		if ((*curr).path().extension().string().find(".dat") != std::string::npos)
		{
			//open file
			std::ifstream currFile((*curr).path());
			if (currFile.is_open())
			{
				std::cout << "Processing " << (*curr) << std::endl;
				std::vector<double> data;
				std::transform(std::istream_iterator<double>(currFile), std::istream_iterator<double>(), back_inserter(data), [](double val)->double {return -val; });
				std::vector<double> toProcess = std::move(SmoothInput(data));
				auto result = DetectPulses(toProcess);
				if (result == toProcess.end())
				{
					++curr;
					continue;
				}
				std::cout << (*curr) << ": ";
				Report(result, data, toProcess);
			}
		}
		++curr;
	}

	std::cout << "Terminating process." << std::endl;
    return 0;
}

