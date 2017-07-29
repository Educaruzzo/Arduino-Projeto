#include<Wire.h>
#include<I2Cdev.h>
#include<MPU6050.h>
/*
=======OFFSETS=======
| AcX = -878.57
 | AcY = -243.53
 | AcZ = 15273.38
 | GyX = -327.67
 | GyY = 168.84
 | GyZ = -90.91
*/
/*
Sensor readings with offsets:  -4  1 16379 -1  0 0
Your offsets: 1283  977 2389  82  -43 22
*/

#define CAL_ACX 1283
#define CAL_ACY 977
#define CAL_ACZ 2389
#define CAL_GYX 82
#define CAL_GYY -43
#define CAL_GYZ 22

#define ACCEL_X_OFFS_H 0x06
#define ACCEL_Y_OFFS_H 0x08
#define ACCEL_Z_OFFS_H 0x0A
#define GYR_X_OFFS 0x13
#define GYR_Y_OFFS 0x15
#define GYR_Z_OFFS 0x17
 
#define PARADO 0      // Sentado ou em pé
#define DEITADO 1
#define EM_MOVIMENTO 2
#define INDEFINIDO 3

#define LIMITE_OSCILACAO_MOVIMENTO 2400
#define LIMITE_SUPERIOR_VARIACAO_QUEDA 35000
#define LIMITE_INFERIOR_VARIACAO_QUEDA 1000

//Endereco I2C do MPU6050
const int MPU = 0x68;  
//Variaveis para armazenar valores dos sensores
int AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ, Norma;
int antAcX,antAcY,antAcZ;
int Pitch, Roll, Yaw;
int Estado, Movimento;

void setup()
{
  Serial.begin(9600);
  InicializaMPU();
  CalibraValores();
  delay(1000);
  Estado = INDEFINIDO;
  antAcX = 0;
  antAcY = 0;
  antAcZ = 0;
}

void loop()
{
  CapturaValores();

  Norma = sqrt(square(AcX) + square(AcY) + square(AcZ));
  //Serial.print(" | Norma = "); Serial.println(Norma);
  
  Pitch = FunctionsPitchRoll(AcX, AcY, AcZ);
  Roll = FunctionsPitchRoll(AcY, AcX, AcZ);
  Yaw = FunctionsPitchRoll(AcZ, AcY, AcX);
  
  ExibeValoresViaSerial();
  Movimento = (abs((AcX - antAcX) + (AcY - antAcY) + (AcZ - antAcZ)) >= LIMITE_OSCILACAO_MOVIMENTO);
  Estado = VerificaEstado();
  
  DetectaQueda();
  
  antAcX = AcX;
  antAcY = AcY;
  antAcZ = AcZ;
  
  //Aguarda 1000 ms e reinicia o processo
  delay(1000);
}

void InicializaMPU() {
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0); 
  Wire.endTransmission(true);
  delay(1000);
}

void CapturaValores() {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  
  //Solicita os dados do sensor
  Wire.requestFrom(MPU,14,true);  
  
  //Armazena o valor dos sensores nas variaveis correspondentes
  AcX= Wire.read()<<8|Wire.read();  //0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)     
  AcY= Wire.read()<<8|Wire.read();  //0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ= Wire.read()<<8|Wire.read();  //0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  Tmp= Wire.read()<<8|Wire.read();  //0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  GyX= Wire.read()<<8|Wire.read();  //0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  GyY= Wire.read()<<8|Wire.read();  //0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  GyZ= Wire.read()<<8|Wire.read();  //0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
}

int FunctionsPitchRoll(double A, double B, double C){
  double DatoA, DatoB, Value;
  DatoA = A;
  DatoB = (B*B) + (C*C);
  DatoB = sqrt(DatoB);
  
  Value = atan2(DatoA, DatoB);
  Value = Value * 180/3.14;
  
  return (int) Value;
}

void ExibeValoresViaSerial() {
  //Envia valor X do acelerometro para a serial
  Serial.print("AcX = "); Serial.print(AcX);
   
  //Envia valor Y do acelerometro para a serial
  Serial.print(" | AcY = "); Serial.print(AcY);
   
  //Envia valor Z do acelerometro para a serial
  Serial.print(" | AcZ = "); Serial.print(AcZ);
   
  //Envia valor da temperatura para a serial
  //Calcula a temperatura em graus Celsius
  Serial.print(" | Tmp = "); Serial.print(Tmp/340.00+36.53);
   
  //Envia valor X do giroscopio para a serial
  Serial.print(" | GyX = "); Serial.print(GyX);
   
  //Envia valor Y do giroscopio para a serial
  Serial.print(" | GyY = "); Serial.print(GyY);
   
  //Envia valor Z do giroscopio para a serial
  Serial.print(" | GyZ = "); Serial.println(GyZ);
   
  //Envia valor do Pitch para a serial
  Serial.print(" | Pitch = "); Serial.print(Pitch);
   
  //Envia valor do Roll para a serial
  Serial.print(" | Roll = "); Serial.print(Roll);
   
  //Envia valor do Yaw para a serial
  Serial.print(" | Yaw = "); Serial.println(Yaw);

  //Envia o Estado para a serial
  switch (Estado) {
    case PARADO:
      Serial.print(" | Estado = "); Serial.println("==Parado==");
      break;
    case DEITADO:
      Serial.print(" | Estado = "); Serial.println("==Deitado==");
      break;
    case EM_MOVIMENTO:
      Serial.print(" | Estado = "); Serial.println("==Em Movimento==");
      break;
    case INDEFINIDO:
      Serial.print(" | Estado = "); Serial.println("==Indefinido==");
      break;
    default:
      Serial.print(" | Estado = "); Serial.println("==Indefinido==");
  }
}

int VerificaEstado() {

  if (abs(Pitch) >= 45) {
    if (Movimento) {
      return(EM_MOVIMENTO);
    }
    return (PARADO);
  }

  if ((abs(Roll) > 45)||(abs(Yaw) > 45)) {
    return (DEITADO);
  }
  return INDEFINIDO;
}

void DetectaQueda() {

  if ((Norma <= LIMITE_INFERIOR_VARIACAO_QUEDA)||(Norma > LIMITE_SUPERIOR_VARIACAO_QUEDA)) {
    if (Estado != PARADO) {
      Serial.println(" CAIU!!"); 
    }
    else {
      Serial.println(" FALSA QUEDA!!"); 
    }
  }
}

void CalibraValores() {
  I2Cdev::writeWord(MPU, ACCEL_X_OFFS_H, CAL_ACX);
  I2Cdev::writeWord(MPU, ACCEL_Y_OFFS_H, CAL_ACY);
  I2Cdev::writeWord(MPU, ACCEL_Z_OFFS_H, CAL_ACZ);
  I2Cdev::writeWord(MPU, GYR_X_OFFS, CAL_GYX);
  I2Cdev::writeWord(MPU, GYR_Y_OFFS, CAL_GYY);
  I2Cdev::writeWord(MPU, GYR_Z_OFFS, CAL_GYZ);
}

