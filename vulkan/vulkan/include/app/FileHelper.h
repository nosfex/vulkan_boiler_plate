#ifndef _FILEHELPER_H_
#define _FILEHELPER_H_
#include <fstream>
#include <iostream>
#include <vector>
class FileHelper
{
public:
	static std::vector<char> ReadFile(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("failed to open file!");
		}

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		std::cout << filename.c_str() << " fileSize" << fileSize;
		file.close();

		return buffer;
	}

	static std::string ContentDir;
};
#endif // !_FILEHELPER_H_
