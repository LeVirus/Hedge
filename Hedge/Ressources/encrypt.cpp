#include <fstream>
#include <sstream>
#include <iostream>
#include <stdint.h>

//ENCRYPT_KEY_CONF_FILE
const uint32_t KEY = 42;
//STANDARD LEVEL
//const uint32_t KEY = 17;
//CUSTOM LEVEL
//const uint32_t KEY = 52;

//===================================================================
std::string decrypt(const std::string &str, uint32_t key)
{
		std::string strR = str;
		for(uint32_t i = 0; i < strR.size(); ++i)
		{
				strR[i] -= key;
		}
		return strR;
}


//===================================================================
std::string encrypt(const std::string &str, uint32_t key)
{
		std::string strR = str;
		for(uint32_t i = 0; i < strR.size(); ++i)
		{
				strR[i] += key;
		}
		return strR;
}

int main(int argc, char *argv[])
{
		//if(argc != 2)
		//{
		//	std::cout << "Bad num Args\n";
		//	return -1;
		//}
		//std::string path = argv[1];
//		std::ifstream inStream("./CustomLevels/ff.clvl");
		//std::ifstream inStream("./fontStandard.ini.base");
		//std::ifstream inStream("./fontStandard.ini.base");
		//std::cout << "./" + path + "/level.ini.dd \n";
		//std::ifstream inStream("./" + path + "/level.ini.dec");
		std::ifstream inStream("./standardData.ini");
//		std::ifstream inStream("./pictureData.ini.base");
		if(inStream.fail())
		{
				std::cout << "Fail\n";
				inStream.close();
				return -1;
		}
		std::ostringstream ostringStream;
		ostringStream << inStream.rdbuf();
		inStream.close();
		std::string dataString = decrypt(ostringStream.str(), KEY);

		//std::ofstream outStream("./fontData.ini");
		//std::ofstream outStream("./CustomLevels/FabbDec");
		//std::ofstream outStream("./" + path + "/level.ini");
		std::ofstream outStream("./standardData.ini.base");
	//	std::ofstream outStream("./pictureData.ini");
		outStream << dataString;
		outStream.close();
		std::cout << "OK\n";
		return 0;
}
