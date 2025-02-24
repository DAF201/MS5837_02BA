#include <Wire.h>
#include <stdio.h>
#define NAN (-(float)(INFINITY * 0.0F))
#define int uint8_t

// fixed addresses and commands
const uint8_t sensor_address = 0x76;
const uint8_t CMD_reset = 0x1E;
const uint8_t CMD_ADC_read = 0x00;
const uint8_t CMD_PROM_read = 0xA0;
const uint8_t CMD_pressure_read = 0x4A;
const uint8_t CMD_temperature_read = 0x5A;

class MS5837_02BA
{
public:
    MS5837_02BA() {};
    // set up I2C as master and reset the sensor
    MS5837_02BA(TwoWire &wire)
    {
        println("module init starts\n");

        I2C = &wire;
        println("scanning I2C");

        // scan for devices
        for (int i = 1; i < 127; i++)
        {
            I2C->beginTransmission(i);
            int error = I2C->endTransmission();
            if (error == 0)
            {
                printf("sensor found at: %d\n", i);
                sensor_address = i;
                break;
            }
        }

        if (sensor_address == -1)
        {
            println("no sensor found");
            ready -= 1;
            return;
        }

        println("\nresetting sensor");
        I2C_Write(CMD_reset);
        delay(10);

        println("\nreading PROM");
        read_PROM();

        println("\nverifying prom");
        CRC_verify(PROM);

        println("\nstarting calculating test");
        standard_calculation();

        check_status();
    }

    void update()
    {
        standard_calculation();
    }

    void standard_calculation()
    {
        delay(20);
        I2C_Write(CMD_pressure_read);
        delay(20);
        I2C_Write(CMD_ADC_read);
        delay(20);
        D1 = I2C_read(3);
        delay(20);
        I2C_Write(CMD_temperature_read);
        delay(20);
        I2C_Write(CMD_ADC_read);
        delay(20);
        D2 = I2C_read(3);
        dT = D2 - uint32_t(PROM[5]) * 256l;
        SENS = int64_t(PROM[1]) * 65536l + (int64_t(PROM[3]) * dT) / 128l;
        OFF = int64_t(PROM[2]) * 131072l + (int64_t(PROM[4]) * dT) / 64l;
        P = (D1 * SENS / (2097152l) - OFF) / (32768l);
        TEMP = 2000l + int64_t(dT) * PROM[6] / 8388608LL;
        if ((TEMP / 100) < 20)
        {
            Ti = (11 * int64_t(dT) * int64_t(dT)) / (34359738368LL);
            OFFi = (31 * (TEMP - 2000) * (TEMP - 2000)) / 8;
            SENSi = (63 * (TEMP - 2000) * (TEMP - 2000)) / 32;
        }
        OFF2 = OFF - OFFi;
        SENS2 = SENS - SENSi;
        TEMP = (TEMP - Ti);
        P = (((D1 * SENS2) / 2097152l - OFF2) / 32768l);
        TEMP2 = TEMP / 100.0f;
        P2 = P / 100.0f;
        if ((TEMP2 >= -20.0) && (TEMP2 <= 85.0) && (P2 >= 300) && (P2 <= 1200))
        {
            if (ready < 1)
                ready += 1;
        }
        else
        {
            println("\nERROR: sensor data out of range, check sensor status");
            if ((TEMP2 >= -20.0) && (TEMP2 <= 85.0))
            {
                printf("temperature: %f is out of boundary of -20C ~ +85C", TEMP2);
                TEMP2 = NAN;
            }
            if ((P2 >= 300) && (P2 <= 1200))
            {
                printf("Pressure: %f is out of boundary of 300mbar ~ 1200mbar", P2);
                P2 = NAN;
            }
        }
    }

    void check_status()
    {
        printf("module ready: %d\n", ready);
        println("\nreading status");
    }

    float get_temperature()
    {
        return TEMP2;
    }

    float get_pressure()
    {
        return P2;
    }
    void print_PROM()
    {
        Serial.println("PROM values:");
        for (int i = 0; i < 7; i++)
        {
            Serial.println(PROM[i], BIN);
        }
    }

private:
    TwoWire *I2C;
    int sensor_address = -1;
    int ready = -1;
    uint16_t PROM[8] = {0};

    // FROM PROM
    uint16_t C1; // pressure_sensitivity__SENS_T1;
    uint16_t C2; // pressure_offset__OFF_T1;
    uint16_t C3; // temperature_coefficient_of_pressure_sensitivity__TCS;
    uint16_t C4; // temperature_coefficient_of_pressure_offset__TCO;
    uint16_t C5; // reference_temperature__T_REF;
    uint16_t C6; // temperature_coefficient_of_the_temperature__TEMPSENS;

    // FROM SENSOR
    uint32_t D1 = 0; // digital_pressure_value;
    uint32_t D2 = 0; // digital_temperature_value;

    // FROM differences calculation
    int32_t dT = 0;   // difference_between_actual_and_reference_temperature; // D2-(C5*2^8)
    int32_t TEMP = 0; // actual_temperature; // 2000+dT*(C6/2^23)

    // FROM differences calculation of actual values
    int64_t OFF = 0;  // offset_at_actual_temperature;       // C2*2^17+((C4*dT)/(2^6))
    int64_t SENS = 0; // sensitivity_at_actual_temperature; // C1*2^16+(C3*dT)/(2^7)
    int32_t P = 0;    // temperature_compensated_pressure;     //(D1*SENS/2^21-OFF)/(2^15)

    // FROM LOW TEMP FORMULA
    int32_t Ti = 0;    // 11*dT^2/2^35
    int64_t OFFi = 0;  // 31*(TEMP-2000)^2/2^3
    int64_t SENSi = 0; // 63*(TEMP-2000)^2/2^5

    // FROM FINAL CALCULATION
    int64_t OFF2 = 0;  // OFF*OFFi
    int64_t SENS2 = 0; // SENS*SENSi
    float TEMP2 = 0.0; //(TEMP-Ti)/100;
    float P2 = 0.0;    //((D1*SENS2/2^21-OFF2)/2^16)/100

    void print(int num) { Serial.print(num); }
    void print(char *str) { Serial.print(str); }
    void println(int num) { Serial.println(num); }
    void println(char *str) { Serial.println(str); }

    // simple ver
    template <typename T>
    void printf(char *pattern, T value)
    {
        char buffer[128] = {0};
        sprintf(buffer, pattern, value);
        print(buffer);
    }

    void I2C_Write(int cmd)
    {
        I2C->beginTransmission(sensor_address);
        I2C->write(cmd);
        I2C->endTransmission();
        delay(10);
    }

    uint32_t I2C_read(int size)
    {

        uint32_t buffer = 0;

        I2C->requestFrom(sensor_address, size);

        // size is either 2(PROM) or 3(data)
        if (size == 2)
        {
            buffer = I2C->read();
            buffer = (buffer << 8) | I2C->read();
        }
        else
        {
            buffer = I2C->read();
            buffer = (buffer << 8) | I2C->read();
            buffer = (buffer << 8) | I2C->read();
        }
        return buffer;
    }

    void read_PROM()
    {
        // 7*16bits of PROM data
        for (int i = 0; i < 7; i++)
        {
            I2C_Write(CMD_PROM_read + i * 2);
            PROM[i] = I2C_read(2);
            delay(10);
        }

        println("PROM values:");
        for (int i = 0; i < 7; i++)
        {
            Serial.println(PROM[i], BIN);
        }
    }

    void CRC_verify(int16_t *n_prom)
    {
        int CRC_read = PROM[0] >> 12;
        printf("\nread CRC: %d\n", CRC_read);
        int n_rem = crc4(PROM);
        printf("calculated CRC: %d\n", n_rem);

        if ((n_rem) == CRC_read)
        {
            println("CRC test passed\n");
            ready += 1;
        }
        else
        {
            println("CRC test failed\n");
        }
    }

    uint8_t
    crc4(uint16_t n_prom[])
    {
        uint16_t n_rem = 0;
        n_prom[0] = ((n_prom[0]) & 0x0FFF);
        n_prom[7] = 0;

        for (uint8_t i = 0; i < 16; i++)
        {
            if (i % 2 == 1)
            {
                n_rem ^= (uint16_t)((n_prom[i >> 1]) & 0x00FF);
            }
            else
            {
                n_rem ^= (uint16_t)(n_prom[i >> 1] >> 8);
            }
            for (uint8_t n_bit = 8; n_bit > 0; n_bit--)
            {
                if (n_rem & 0x8000)
                {
                    n_rem = (n_rem << 1) ^ 0x3000;
                }
                else
                {
                    n_rem = (n_rem << 1);
                }
            }
        }
        n_rem = ((n_rem >> 12) & 0x000F);
        return n_rem ^ 0x00;
    }
};