#include <iostream>

#include "serial.h"

data_serializer::packet test_data{
	data_serializer::packet_start,
	1,
	(uint8_t)strlen("Hello World how are you?"),
	0,
	"Hello World how are you?"
};

class serial_test {
private:
	static data_serializer serializer;
public:
	static void callback(const data_serializer::packet& p) {
		printf(
			"Start: %d\nType: %d\nLen: %d\nCRC: %d\nData: %s\n",
			p.start,
			p.type,
			p.length,
			p.crc,
			p.data
		);
	}

	static void main(void){
		static uint8_t buffer[data_serializer::header_len + data_serializer::max_data_len];

		serializer.serialize_data(test_data);
		strcpy((char*)buffer, (char*)serializer.get_data());
		std::cout << "Sending: " << (int*)buffer << std::endl;
		
		auto tmp = &buffer[0];

		for (uint8_t i = 0; i < 200; ++i) {
			serializer.deserialize_data(*tmp++);
		}
	}
};

data_serializer serial_test::serializer{ serial_test::callback };


int main(void){
	serial_test::main();
}