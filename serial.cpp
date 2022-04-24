#include "serial.h"

#include <stdint.h>
#include <string.h>



uint8_t get_crc(uint8_t byte, uint16_t crc_old) {
	uint8_t crc = byte ^ (crc_old & (1 << 8)) ^ (crc_old & (1 << 6)) ^ (crc_old & (1 << 3)) ^ (crc_old & (1 << 1));

	return crc;
}

data_serializer::data_serializer(callback_type user_callback):
	data{ 0 }, callback{ user_callback }{ }

void data_serializer::serialize_data(const data_serializer::packet& p) {
	data.data[0] = p.start;
	data.data[1] = p.type;
	data.data[2] = p.length;

	uint8_t crc = data_serializer::initial_crc;
	
	for (uint8_t i = 0; i < p.length; ++i) {
		crc = get_crc(p.data[i], crc);
	}

	data.data[3] = crc;
	
	strcpy((char*)&data.data[4], (char*)p.data);
}

void data_serializer::deserialize_data(uint8_t byte) {
	static uint8_t state = 0;
	static uint8_t i = 0;

	switch (state) {
	case 0:
		if (byte == packet_start) {
			data.start = byte; state = 1;
		}
		break;

	case 1:
		data.type = byte; state = 2;
		break;

	case 2:
		if (byte <= 200) {
			data.length = byte; state = 3;
		}
		else {
			state = 0;
		}
		break;

	case 3:
		data.crc = byte; state = 4;
		break;

	case 4:
		if (data.length == i) {
			state = 5;
		}
		else {
			data.data[i++] = byte;
		}
		break;

	case 5:
		uint8_t tmp_crc = initial_crc;

		for(uint8_t i = 0; i < data.length; ++i){
			tmp_crc = get_crc(data.data[i], tmp_crc);
		}

		if (tmp_crc == data.crc) {
			callback(data);
		}

		state = 0;
	break;
	}
}

const uint8_t* data_serializer::get_data()const {
	return data.data;
}