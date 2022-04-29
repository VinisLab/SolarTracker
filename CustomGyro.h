#include "esphome.h"
#define mPlaca 496.0
#define cacPlacas 1020.0
#define PI 3.14159265358979323846264

class MPU6050 : public PollingComponent {

  public:
  Sensor *temperature = new Sensor();
  Sensor *Accx = new Sensor();
  Sensor *Accy = new Sensor();
  Sensor *Accz = new Sensor();
  Sensor *AngleX = new Sensor();
  Sensor *AngleY = new Sensor();
  Sensor *SunPos = new Sensor();
  Sensor *PlacaFinal = new Sensor();
  uint32_t counter = 0;
  uint32_t counter2 = 0;
  float accelgyro_temperature = 0;

  const uint8_t MPU_addr = 0x69; // I2C address of the MPU-6050

  float ToDEG(float rad){return rad*(180.0/PI);}
  float ToRAD(float deg){return deg*(PI/180.0);}
  float square(float n){return n*n; }


  MPU6050() : PollingComponent(2) { }

  void setup() override {

    Wire.begin();

    // default at power-up:
    //    Gyro at 250 degrees second
    //    Acceleration at 2g
    //    Clock source at internal 8MHz
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x6B);  // PWR_MGMT_1 register
    Wire.write(0);     // set to zero (wakes up the MPU-6050)
    Wire.endTransmission(true);
  }

  void update() override {

    int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;
    float Angle_X = 0;
    float Angle_Y = 0;
    float Sun_Pos = 0;
    float Sun_Pos_y = 0;
    float Sun_Pos_z = 0;
    float azimuth = 0;
    float elevation = 0;
    float zenith = 0;
    float a, b, c, d, x2, q, y, z;
    a = b = c = d = x2 = q = y = z = 0;

    Wire.beginTransmission(MPU_addr);
    Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_addr, 14, true); // request a total of 14 registers
    AcX = Wire.read() << 8 | Wire.read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
    AcY = Wire.read() << 8 | Wire.read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
    AcZ = Wire.read() << 8 | Wire.read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
    Tmp = Wire.read() << 8 | Wire.read(); // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
    GyX = Wire.read() << 8 | Wire.read(); // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
    GyY = Wire.read() << 8 | Wire.read(); // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
    GyZ = Wire.read() << 8 | Wire.read(); // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)

    accelgyro_temperature = Tmp / 340. + 16.81; //equation for temperature in degrees C from datasheet

    Angle_Y = ToDEG( atan( (float)AcX / (float)AcZ ) );
    Angle_X = ToDEG( atan( (float)AcY / sqrt( (float)AcZ * (float)AcZ + (float)AcX * (float)AcX ) ) ) * -1;

    azimuth = ToRAD( id(sun_azimuth).state );
    elevation = id(sun_elevation).state;
    zenith = ToRAD( 90.0 - id(sun_elevation).state ); 

    Sun_Pos_y = sin(zenith) * sin(azimuth);
    Sun_Pos_z = cos(zenith);
    Sun_Pos = ToDEG( atan( Sun_Pos_y / Sun_Pos_z) );
    Sun_Pos = fabs(Sun_Pos);

    if(Sun_Pos_y >= 0 && Sun_Pos_z < 0)
    {
      Sun_Pos = 180.0 - Sun_Pos;
    }

    if(Sun_Pos_y >= 0 && Sun_Pos_z >= 0)
    {
      Sun_Pos = Sun_Pos;
    }

    if(Sun_Pos_y < 0 && Sun_Pos_z >= 0)
    {
      Sun_Pos = -Sun_Pos;
    }

    if(Sun_Pos_y < 0 && Sun_Pos_z < 0)
    {
      Sun_Pos = -180.0 + Sun_Pos;
    }

    z = (Sun_Pos < 0) ? ToRAD(Sun_Pos + 90.0) :  ToRAD(Sun_Pos - 90.0);
    z = fabs(z);

    a = 1.0;
    b = ( (-2.0)*cacPlacas ) * cos(z);
    c = square( (float)cacPlacas ) - square( (float)mPlaca );
    d = square(b)-4.0*a*c;
    x2 = ( (-b)-sqrt(d) ) / (2.0*a);

    y = atan( (sin(z)*x2) / (cacPlacas - (cos(z)*x2) ) ); 
    y = (Sun_Pos > 0) ? ToDEG(y) : ToDEG(-y);
    
    if(Sun_Pos <= -90 || Sun_Pos >= 90) 
    {
      y = 0;
    }

    if(counter2 == 5000)
    {
      SunPos->publish_state(Sun_Pos);
      PlacaFinal->publish_state(y);
      counter2 = 0;
    }
    else
    {
      counter2++;
    }
    
    if(counter == 500)
    {
      Accx->publish_state(AcX);
      Accy->publish_state(AcY);
      Accz->publish_state(AcZ);
      AngleX->publish_state(Angle_X);
      AngleY->publish_state(Angle_Y);
      temperature->publish_state(accelgyro_temperature);
      counter = 0;
    }
    else
    {
      counter++;
    }
    
    id(currentPosition) = Angle_X;
    id(sunPosition) = Sun_Pos;
    id(placaFinal) = y;
  }

};