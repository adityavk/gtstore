#ifndef ARG_UTILS_H
#define ARG_UTILS_H

#include <string>
#include <unordered_map>
#include <iostream>

// Copied from my project-2 submission to parse command line args
inline std::unordered_map<std::string, std::string> parseArgs(int argc, char* argv[])
{
    std::unordered_map<std::string, std::string> args;
    args.reserve(argc - 1);
	for(int i = 0; i < argc; ++i)
	{
        std::string arg(argv[i]);
		if(arg.rfind("--", 0) == 0)
		{
            std::string value;
			if (i < argc - 1 && std::string(argv[i+1]).rfind("--", 0) != 0) {
                value = argv[i+1];
                ++i;
            }
            args.emplace(arg.substr(2), value);
		}
	}
	return args;
}

template<typename T>
bool inline parseIntArg(const std::unordered_map<std::string, std::string>& args, const std::string& key, T& argValue) {
    const auto it = args.find(key);
    if (it == args.end()) {
        std::cerr<<key<<" arg missing, exiting!"<< std::endl;
        return false;
    } else {
        try {
            argValue = std::stoi(it->second);
        } catch (const std::invalid_argument& e) {
            std::cerr<<"Invalid value for "<<key<<" arg: "<<e.what()<< std::endl;
            return false;
        } catch (const std::out_of_range& e) {
            std::cerr<<"Value for "<<key<<" arg out of range: "<<e.what()<< std::endl;
            return false;
        }
    }
    return true;
}

std::string inline parseStringArg(const std::unordered_map<std::string, std::string>& args, const std::string& key, const std::string& defaultVal = "") {
    const auto it = args.find(key);
    if (it != args.end()) {
        return it->second;
    }
    return defaultVal;
}

#endif // ARG_UTILS_H
