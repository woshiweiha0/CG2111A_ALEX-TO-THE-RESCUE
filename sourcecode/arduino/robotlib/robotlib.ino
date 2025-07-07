#include <AFMotor.h>
// Motor control
#define FRONT_LEFT   4 // M4 on the driver shield
#define FRONT_RIGHT  3 // M1 on the driver shield
#define BACK_LEFT    1 // M3 on the driver shield
#define BACK_RIGHT   2 // M2 on the driver shield

AF_DCMotor motorFL(FRONT_LEFT);
AF_DCMotor motorFR(FRONT_RIGHT);
AF_DCMotor motorBL(BACK_LEFT);
AF_DCMotor motorBR(BACK_RIGHT);

void move(float speed, int direction)
{
  int speed_scaled = (speed/100.0) * 255;
  motorFL.setSpeed(speed_scaled);
  motorFR.setSpeed(speed_scaled);
  motorBL.setSpeed(speed_scaled);
  motorBR.setSpeed(speed_scaled);

  switch(direction)
    {
      case BACK:
        motorFL.run(BACKWARD);
        motorFR.run(BACKWARD);
        motorBL.run(FORWARD);
        motorBR.run(FORWARD); 
      break;
      case GO:
        motorFL.run(FORWARD);
        motorFR.run(FORWARD);
        motorBL.run(BACKWARD);
        motorBR.run(BACKWARD); 
      break;
      case CCW:
        motorFL.run(BACKWARD);
        motorFR.run(FORWARD);
        motorBL.run(FORWARD);
        motorBR.run(BACKWARD); 
      break;
      case CW:
        motorFL.run(FORWARD);
        motorFR.run(BACKWARD);
        motorBL.run(BACKWARD);
        motorBR.run(FORWARD); 
      break;
      case STOP:
      default:
        motorFL.run(STOP);
        motorFR.run(STOP);
        motorBL.run(STOP);
        motorBR.run(STOP); 
    }
}

void forward(float dist, float speed)
{
  if(dist>0)
    deltaDist=dist;
  else
    deltaDist=9999999;

  newDist = forwardDist + deltaDist;
  
  dir = (TDirection) FORWARD;
  move(speed, FORWARD);

 
}

void backward(float dist, float speed)
{
  if(dist>0)
    deltaDist=dist;
  else
    deltaDist=9999999;

  newDist = reverseDist + deltaDist;
  
  dir = (TDirection) BACKWARD;
  move(speed, BACKWARD);
}

void ccw(float dist, float speed)
{
  dir = (TDirection) LEFT;
  move(speed, CCW);
}

void cw(float dist, float speed)
{
  dir = (TDirection) RIGHT;
  move(speed, CW);
}

void stop()
{
  dir = (TDirection) STOP;
  move(0, STOP);
}

void clawOpen()
{
  servoLeft.write(leftOpen);
  servoRight.write(rightOpen);  

}

void clawClose()
{
  servoLeft.write(leftClose);
  servoRight.write(rightClose);
}

void dropMed()
{
  servoMed.write(medOpen);
  delay(1000);
  servoMed.write(medClose);
}


int getAvgFreq() {
  int reading;
  int total = 0;
  for (int i = 0; i < 5; i++) {
    reading = pulseIn(sensorOut, LOW);
    total += reading;
    delay(20);
  }
  return total / 5;
}

void sendColor() {
  TPacket colorPacket;
  colorPacket.packetType = PACKET_TYPE_RESPONSE;
  colorPacket.command = RESP_COLOR;
  
  colorPacket.params[0] = redFreq;
  colorPacket.params[1] = greenFreq;
  colorPacket.params[2] = blueFreq;
  
  sendResponse(&colorPacket);  
}

struct ColorSample {
  const char* label;
  int r, g, b;
};

//Red and Green average from 20 readings
ColorSample colorDB[] = {
  {"RED", 71, 235, 179},
  {"GREEN", 59, 51, 54}
};

#define NUM_SAMPLES 2

const char* classifyColor(int r, int g, int b) {
  float minDist = 1e9;
  const char* nearest = "NO COLOR";

  for (int i = 0; i < NUM_SAMPLES; i++) {
    float dist = sqrt(pow(r - colorDB[i].r, 2) + pow(g - colorDB[i].g, 2) + pow(b - colorDB[i].b, 2));
    if (dist < minDist) {
      minDist = dist;
      nearest = colorDB[i].label;
    }
  }

  // Optional threshold to reduce false positives
  if (minDist > 100.0)
    return "NO COLOR";

  return nearest;
}


void scanColor()
{
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  delay(100);
  redFreq = getAvgFreq();
  delay(100);

  digitalWrite(S2, HIGH); digitalWrite(S3, HIGH);
  delay(100);
  greenFreq = getAvgFreq();
  delay(100);

  digitalWrite(S2, LOW); digitalWrite(S3, HIGH);
  delay(100);
  blueFreq = getAvgFreq();
  delay(100);

  const char* colorLabel = classifyColor(redFreq, greenFreq, blueFreq);
  dbprintf("Detected Color: %s", (char*) colorLabel);
}

/*
void printLiveColor()
{
  int r, g, b;

  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  digitalWrite(S2, LOW); digitalWrite(S3, LOW);
  delay(100);
  r = getAvgFreq();

  digitalWrite(S2, HIGH); digitalWrite(S3, HIGH);
  delay(100);
  g = getAvgFreq();

  digitalWrite(S2, LOW); digitalWrite(S3, HIGH);
  delay(100);
  b = getAvgFreq();

  dbprintf("LIVE RGB -> R: %d, G: %d, B: %d", r, g, b);
}
*/
