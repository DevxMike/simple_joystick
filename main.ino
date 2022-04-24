// | 0xA1 | type | data_len | lower crc byte | higher crc byte | data |

#include <stdint.h>
#include <string.h>

class data_serializer{
public:
    enum{
        header_len = 4, // start, type, data_len, 16bit crc
        packet_start = 0xA1,
        max_data_len = 200,
        initial_crc = 0xFF
    };

    struct packet{
        uint8_t start;
        uint8_t type;
        uint8_t length;
        uint8_t crc;
        uint8_t data[max_data_len + header_len];
    };
    typedef void (*callback_type)(const packet &);

private:
    packet data;
    callback_type callback;

public:
    data_serializer(callback_type user_callback);
    void serialize_data(const packet &p);
    void deserialize_data(uint8_t data);
    const uint8_t *get_data() const;
};

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

class joystick{
public:
    struct coords{
        uint16_t x;
        uint16_t y;
    };

    static coords joystick_coords;
    static SemaphoreHandle_t joystick_coords_ready;

    static void main(void* params);
    static void read_coords(void* params);
};

joystick::coords joystick::joystick_coords{ 0 };
SemaphoreHandle_t joystick::joystick_coords_ready{ xSemaphoreCreateBinary() };

void joystick::main(void* params){
    xTaskCreatePinnedToCore(
        read_coords, 
        "ADC_Read", 
        128*8, 
        NULL, 
        0, 
        NULL, 
        0
    );

    vTaskDelete(NULL);
}

void joystick::read_coords(void* params){
    const static uint8_t x_analog_in = 35;
    const static uint8_t y_analog_in = 34;

    while(1){
        auto tmp = analogRead(x_analog_in);
        joystick_coords.x = joystick_coords.x * 0.9 + tmp * 0.1;

        tmp = analogRead(y_analog_in);
        joystick_coords.y = joystick_coords.y * 0.9 + tmp * 0.1;

        xSemaphoreGive(joystick_coords_ready);
    }

    vTaskDelete(NULL);
}

class communication{
public:
    static void main(void* params);
};

void communication::main(void* params){
    Serial.begin(115200);

    while(1){
        xSemaphoreTake(joystick::joystick_coords_ready, portMAX_DELAY);

        Serial.print("x: ");
        Serial.print(joystick::joystick_coords.x);
        Serial.print(", y: ");
        Serial.print(joystick::joystick_coords.y);
        Serial.print("\n\r");

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

void blinker(void* params){
    pinMode(2, OUTPUT);
    
    while(1){
        digitalWrite(2, HIGH);
        delay(1000);
        digitalWrite(2, LOW);
        delay(1000);
    }

    vTaskDelete(NULL);
}

void setup(){
    //core 0 tasks
    xTaskCreatePinnedToCore(
        blinker, 
        "Blinker", 
        128 * 4, 
        NULL, 
        1, 
        NULL, 
        0
    );

    xTaskCreatePinnedToCore(
        joystick::main,
        "jstck_init",
        128*8,
        NULL,
        0,
        NULL,
        0
    );

    //core 1 tasks
    xTaskCreatePinnedToCore(
        communication::main,
        "UART_com",
        128*8,
        NULL,
        0,
        NULL, 
        1
    );
}

void loop(){
    vTaskSuspend(NULL);
}