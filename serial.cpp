#include "serial.h"
#include <stdint.h>

// | 0xA1 | type | data_len | lower crc byte | higher crc byte | data |  

class data_serializer {
public:
	enum {
		header_len = 5, //start, type, data_len, 16bit crc
		packet_start = 0xA1,
		max_data_len = 200,
		initial_crc = 0xFFFF
	};

	struct packet {
		uint8_t start;
		uint8_t type;
		uint8_t length;
		uint16_t crc;
		uint8_t data[max_data_len + header_len];
	};
private:
	packet data;
public:
	data_serializer();
	void serialize_data(const packet& p);
	void deserialize_data(uint8_t data);
	const uint8_t* get_data()const;
};