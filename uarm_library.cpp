/*!
   \file uarm_library.cpp
   \brief uArm Library for Arduino
   \author Joe Song
   \update Alex Tan, Dave Corboy
   \license GNU
   \copyright(c) 2016 UFactory Team. All right reserved
 */
#include "uarm_library.h"

uArmClass uarm;

unsigned int sys_tick;

uArmClass::uArmClass()
{
  /*TCNT2   = 0;
  TCCR2A = (1 << WGM21); // Configure timer 2 for CTC mode
  TCCR2B = (1 << CS22); // Start timer at Fcpu/64
  TIMSK2 = (1 << OCIE2A); // Enable CTC interrupt
  OCR2A   = 249; // Set CTC compare value with a prescaler of 64  2ms*/
}

/*!
   \brief check the arm status
   \Return true:free; false:busy
 */
bool uArmClass::available()
{
  if(move_times!=255)
  {
    return false;
  }
  if (Serial.available())
  {
    return false;
  }

  return true;
}

/*!
   \brief process the uarm movement
   \no parameter
*/
void uArmClass::arm_process_commands()
{
  //move_to
  if(sys_tick%40==0)
  {
    
    if(move_times <= INTERP_INTVLS)
    {
      //Serial.println(move_times,DEC);
      y_array[move_times] = y_array[move_times] - LEFT_SERVO_OFFSET;//assembling offset
      z_array[move_times] = z_array[move_times] - RIGHT_SERVO_OFFSET;//assembling offset
      if(move_times==INTERP_INTVLS)//get the ideal data
      {
        Serial.println(x_array[move_times],DEC);
        Serial.println(y_array[move_times],DEC);
        Serial.println(z_array[move_times],DEC);
      }
      read_servo_calibration_data(&x_array[move_times],&y_array[move_times],&z_array[move_times]);
      write_servos_angle(x_array[move_times], y_array[move_times], z_array[move_times]);

      //hand rot

      move_times++;
      if(move_times > INTERP_INTVLS)
      {
        move_times = 255;//disable the move
      }
    }
  }
  //buzzer work
  if(buzzerStopTime!=0)
  {
    buzzerStopTime--;
  }
  else
  {
    noTone(BUZZER);
  }
}

void uArmClass::arm_setup()
{
  TCNT0   = 0;
  TCCR0A = (1 << WGM01); // Configure timer 0 for CTC mode
  TCCR0B = 0x03; // Start timer at Fcpu/64
  TIMSK0 = (1 << OCIE0A); // Enable CTC interrupt
  OCR0A   = 249; // Set CTC compare value with a prescaler of 64  1ms
  pinMode(BTN_D4,INPUT_PULLUP);//special mode for calibration
  if(digitalRead(4)==LOW)
  {
    while(digitalRead(4)==LOW);
        
    write_servos_angle(90,90,0);
    while(1);
  }
  pinMode(BTN_D7,INPUT_PULLUP);
  pinMode(BUZZER,OUTPUT);
}

/*!
   \brief Use BUZZER for Alert
   \param times Beep Times
   \param runTime How Long from High to Low
   \param stopTime How Long from Low to High
 */
void uArmClass::alert(byte times, byte runTime, byte stopTime)
{
        for(int ct=0; ct < times; ct++)
        {
                delay(stopTime);
                digitalWrite(BUZZER, HIGH);
                delay(runTime);
                digitalWrite(BUZZER, LOW);
        }
}

/*!
   \brief get the calibration data from the external eeprom
   \param rot the calibration data of rotation
   \param left the calibration data of left
   \param right the calibration data of right
 */

void uArmClass::read_servo_calibration_data(double *rot, double *left, double *right)// takes 1~2ms
{
  /*unsigned char calibration_data[DATA_LENGTH]; //get the calibration data around the data input
  unsigned int min_data_calibration_address;
  double closest_data, another_closest_data;
  unsigned int deltaA = 0xffff, deltaB = 0, i, i_min = 0;*/
//rot calibration data
  calibration_data_to_servo_angle(rot,ROT_SERVO_ADDRESS);
  /*min_data_calibration_address = (((unsigned int)(*rot)  - (DATA_LENGTH >> 2)) * 2);
    Serial.println(min_data_calibration_address,DEC);
  iic_readbuf(calibration_data, EXTERNAL_EEPROM_DEVICE_ADDRESS, ROT_SERVO_ADDRESS + min_data_calibration_address, DATA_LENGTH);
  for(i=0;i<(DATA_LENGTH >> 1);i++)
  {
      deltaB = abs ((calibration_data[i+i]<<8) + calibration_data[1+(i+i)] - (*rot) * 10);
      Serial.println(deltaB,DEC);
      if(deltaA > deltaB)
      {
        i_min = i;
        deltaA = deltaB;
      } 
  }

  closest_data = ((calibration_data[i_min+i_min]<<8) + calibration_data[1+(i_min+i_min)])/10.0;//transfer the dat from ideal data to servo angles
  if((*rot) >= closest_data)
  {
    another_closest_data = ((calibration_data[i_min+i_min+2]<<8) + calibration_data[3+i_min+i_min])/10.0;//bigger than closest
    if(another_closest_data == closest_data)
    {
      *rot = min_data_calibration_address/2 + i_min + 1 + 0.5;
    }
    else
    {
      *rot = 1.0 * (*rot - closest_data) / (another_closest_data - closest_data) + min_data_calibration_address/2 + i_min + 1;
    }
  }
  else
  {
    another_closest_data = ((calibration_data[i_min+i_min-2]<<8) + calibration_data[i_min+i_min-1])/10.0;//smaller than closest
    if(another_closest_data == closest_data)
    {
      *rot = min_data_calibration_address/2 + i_min + 0.5;
    }
    else
    {
      *rot = 1.0 * (*rot - another_closest_data) / (closest_data - another_closest_data) + min_data_calibration_address/2 + i_min;
    }
  }
  //*rot = ((calibration_data[i_min+i_min]<<8) + calibration_data[1+(i_min+i_min)])/10.0;*/
//left calibration data
  calibration_data_to_servo_angle(left,LEFT_SERVO_ADDRESS);
  /*deltaA = 0xffff;
  deltaB = 0;
  min_data_calibration_address = (((unsigned int)(*left) - (DATA_LENGTH >> 2)) * 2);
  iic_readbuf(calibration_data, EXTERNAL_EEPROM_DEVICE_ADDRESS, LEFT_SERVO_ADDRESS + min_data_calibration_address, DATA_LENGTH);
  for(i=0;i<(DATA_LENGTH >> 1);i++)
  {
      deltaB = abs ((calibration_data[i+i]<<8) + calibration_data[1+(i+i)] - (*left) * 10);
      if(deltaA > deltaB)
      {
        i_min = i;
        deltaA = deltaB;
      } 
  }

  closest_data = ((calibration_data[i_min+i_min]<<8) + calibration_data[1+(i_min+i_min)])/10.0;//transfer the dat from ideal data to servo angles
  if((*left) >= closest_data)
  {
    another_closest_data = ((calibration_data[i_min+i_min+2]<<8) + calibration_data[3+i_min+i_min])/10.0;//bigger than closest
    if(another_closest_data == closest_data)
    {
      *left = min_data_calibration_address/2 + i_min + 1 + 0.5;
    }
    else
    {
      *left = 1.0 * (*left - closest_data) / (another_closest_data - closest_data) + min_data_calibration_address/2 + i_min + 1;
    }
  }
  else
  {
    another_closest_data = ((calibration_data[i_min+i_min-2]<<8) + calibration_data[i_min+i_min-1])/10.0;//smaller than closest
    if(another_closest_data == closest_data)
    {
      *left = min_data_calibration_address/2 + i_min + 0.5;
    }
    else
    {
      *left = 1.0 * (*left - another_closest_data) / (closest_data - another_closest_data) + min_data_calibration_address/2 + i_min;
    }
  }
  //*left = ((calibration_data[i_min+i_min]<<8) + calibration_data[1+(i_min+i_min)])/10.0;*/
//right calibration data
  calibration_data_to_servo_angle(right,RIGHT_SERVO_ADDRESS);
  /*deltaA = 0xffff;
  deltaB = 0;
  min_data_calibration_address = (((unsigned int)(*right) - (DATA_LENGTH >> 2)) * 2);
  iic_readbuf(calibration_data, EXTERNAL_EEPROM_DEVICE_ADDRESS, RIGHT_SERVO_ADDRESS + min_data_calibration_address, DATA_LENGTH);
  for(i=0;i<(DATA_LENGTH >> 1);i++)
  {
      deltaB = abs ((calibration_data[i+i]<<8) + calibration_data[1+(i+i)] - (*right) * 10);
      if(deltaA > deltaB)
      {
        i_min = i;
        deltaA = deltaB;
      } 
  }

  closest_data = ((calibration_data[i_min+i_min]<<8) + calibration_data[1+(i_min+i_min)])/10.0;//transfer the dat from ideal data to servo angles
  if((*right) >= closest_data)
  {
    another_closest_data = ((calibration_data[i_min+i_min+2]<<8) + calibration_data[3+i_min+i_min])/10.0;//bigger than closest
    if(another_closest_data == closest_data)
    {
      *right = min_data_calibration_address/2 + i_min + 1 + 0.5;
    }
    else
    {
      *right = 1.0 * (*right - closest_data) / (another_closest_data - closest_data) + min_data_calibration_address/2 + i_min + 1;
    }
  }
  else
  {
    another_closest_data = ((calibration_data[i_min+i_min-2]<<8) + calibration_data[i_min+i_min-1])/10.0;//smaller than closest
    if(another_closest_data == closest_data)
    {
      *right = min_data_calibration_address/2 + i_min + 0.5;
    }
    else
    {
      *right = 1.0 * (*right - another_closest_data) / (closest_data - another_closest_data) + min_data_calibration_address/2 + i_min;
    }
  }
  //*right = ((calibration_data[i_min+i_min]<<8) + calibration_data[1+(i_min+i_min)])/10.0;*/
} 

/*!
   \brief check the external eeprom and transfer the ideal data to real angle data
   \param data the address of the variable
   \param address the section starting address of the external eeprom 
*/
void uArmClass::calibration_data_to_servo_angle(double *data,unsigned int address)
{
  unsigned char calibration_data[DATA_LENGTH]; //get the calibration data around the data input
  unsigned int min_data_calibration_address;
  double closest_data, another_closest_data;
  unsigned int deltaA = 0xffff, deltaB = 0, i, i_min = 0;
  deltaA = 0xffff;
  deltaB = 0;
  min_data_calibration_address = (((unsigned int)(*data) - (DATA_LENGTH >> 2)) * 2);
  iic_readbuf(calibration_data, EXTERNAL_EEPROM_DEVICE_ADDRESS, address + min_data_calibration_address, DATA_LENGTH);
  for(i=0;i<(DATA_LENGTH >> 1);i++)
  {
      deltaB = abs ((calibration_data[i+i]<<8) + calibration_data[1+(i+i)] - (*data) * 10);
      if(deltaA > deltaB)
      {
        i_min = i;
        deltaA = deltaB;
      } 
  }

  closest_data = ((calibration_data[i_min+i_min]<<8) + calibration_data[1+(i_min+i_min)])/10.0;//transfer the dat from ideal data to servo angles
  if((*data) >= closest_data)
  {
    another_closest_data = ((calibration_data[i_min+i_min+2]<<8) + calibration_data[3+i_min+i_min])/10.0;//bigger than closest
    if(another_closest_data == closest_data)
    {
      *data = min_data_calibration_address/2 + i_min + 1 + 0.5;
    }
    else
    {
      *data = 1.0 * (*data - closest_data) / (another_closest_data - closest_data) + min_data_calibration_address/2 + i_min + 1;
    }
  }
  else
  {
    another_closest_data = ((calibration_data[i_min+i_min-2]<<8) + calibration_data[i_min+i_min-1])/10.0;//smaller than closest
    if(another_closest_data == closest_data)
    {
      *data = min_data_calibration_address/2 + i_min + 0.5;
    }
    else
    {
      *data = 1.0 * (*data - another_closest_data) / (closest_data - another_closest_data) + min_data_calibration_address/2 + i_min;
    }
  }  
}

/*!
   \brief Write 4 Servo Angles, servo_rot, servo_left, servo_right, servo_hand_rot
   \param servo_rot_angle SERVO_ROT_NUM
   \param servo_left_angle SERVO_LEFT_NUM
   \param servo_right_angle SERVO_RIGHT_NUM
   \param servo_hand_rot_angle SERVO_HAND_ROT_NUM
   \return SUCCESS, FAILED
 */
int uArmClass::write_servos_angle(double servo_rot_angle, double servo_left_angle, double servo_right_angle, double servo_hand_rot_angle)
{
        attach_all();
        write_servos_angle(servo_rot_angle, servo_left_angle, servo_right_angle);
        write_servo_angle(SERVO_HAND_ROT_NUM,servo_hand_rot_angle,true);
}

/*!
   \brief Write 3 Servo Angles, servo_rot, servo_left, servo_right
   \param servo_rot_angle SERVO_ROT_NUM
   \param servo_left_angle SERVO_LEFT_NUM
   \param servo_right_angle SERVO_RIGHT_NUM
   \return SUCCESS, FAILED
 */
int uArmClass::write_servos_angle(double servo_rot_angle, double servo_left_angle, double servo_right_angle)
{

        write_servo_angle(SERVO_ROT_NUM,servo_rot_angle,true);
        write_servo_angle(SERVO_LEFT_NUM,servo_left_angle,true);
        write_servo_angle(SERVO_RIGHT_NUM,servo_right_angle,true);

        // refresh logical servo angle cache
        //cur_rot = servo_rot_angle;
        //cur_left = servo_left_angle;
        //cur_right = servo_right_angle;
}

/*!
   \brief Write the angle to Servo
   \param servo_number SERVO_ROT_NUM, SERVO_LEFT_NUM, SERVO_RIGHT_NUM, SERVO_HAND_ROT_NUM
   \param servo_angle Servo target angle, 0.00 - 180.00
   \param writeWithoffset True: with Offset, False: without Offset
 */
void uArmClass::write_servo_angle(byte servo_number, double servo_angle, boolean writeWithoffset)
{
        attach_servo(servo_number);
        //servo_angle = writeWithoffset ? (servo_angle + read_servo_offset(servo_number)) : servo_angle;
        // = constrain(servo_angle,0.0,180.0);
        switch(servo_number)
        {
        case SERVO_ROT_NUM:       g_servo_rot.write(servo_angle);
                cur_rot = servo_angle;
                break;
        case SERVO_LEFT_NUM:      g_servo_left.write(servo_angle);
                cur_left = servo_angle;
                break;
        case SERVO_RIGHT_NUM:     g_servo_right.write(servo_angle);
                cur_right = servo_angle;
                break;
        case SERVO_HAND_ROT_NUM:  g_servo_hand_rot.write(servo_angle);
                cur_hand = servo_angle;
                break;
        default:                  break;
        }
}

/*!
   \brief Attach All Servo
   \note Warning, if you attach left servo & right servo without a movement, it might be caused a demage
 */
void uArmClass::attach_all()
{
        attach_servo(SERVO_ROT_NUM);
        attach_servo(SERVO_LEFT_NUM);
        attach_servo(SERVO_RIGHT_NUM);
        attach_servo(SERVO_HAND_ROT_NUM);
}

/*!
   \brief Attach Servo, if servo has not been attached, attach the servo, and read the current Angle
   \param servo number SERVO_ROT_NUM, SERVO_LEFT_NUM, SERVO_RIGHT_NUM, SERVO_HAND_ROT_NUM
 */
void uArmClass::attach_servo(byte servo_number)
{
        switch(servo_number) {
        case SERVO_ROT_NUM:
                if(!g_servo_rot.attached()) {
                        g_servo_rot.attach(SERVO_ROT_PIN);
                        //cur_rot = read_servo_angle(SERVO_ROT_NUM);
                }
                break;
        case SERVO_LEFT_NUM:
                if (!g_servo_left.attached()) {
                        g_servo_left.attach(SERVO_LEFT_PIN);
                        //cur_left = read_servo_angle(SERVO_LEFT_NUM);
                }
                break;
        case SERVO_RIGHT_NUM:
                if (!g_servo_right.attached()) {
                        g_servo_right.attach(SERVO_RIGHT_PIN);
                        //cur_right = read_servo_angle(SERVO_RIGHT_NUM);
                }
                break;
        case SERVO_HAND_ROT_NUM:
                if (!g_servo_hand_rot.attached()) {
                        g_servo_hand_rot.attach(SERVO_HAND_PIN);
                        //cur_hand = read_servo_angle(SERVO_HAND_ROT_NUM);
                }
                break;
        }
}

/*!
   \brief Detach All servo, you could move the arm
 */
void uArmClass::detach_all_servos()
{
        g_servo_rot.detach();
        g_servo_left.detach();
        g_servo_right.detach();
        g_servo_hand_rot.detach();
}

/*!
   \brief Detach Servo by Servo Number, SERVO_ROT_NUM, SERVO_LEFT_NUM, SERVO_RIGHT_NUM, SERVO_HAND_ROT_NUM
   \param servo_number SERVO_ROT_NUM, SERVO_LEFT_NUM, SERVO_RIGHT_NUM, SERVO_HAND_ROT_NUM
 */
void uArmClass::detach_servo(byte servo_number)
{
        switch(servo_number) {
        case SERVO_ROT_NUM:
                g_servo_rot.detach();
                break;
        case SERVO_LEFT_NUM:
                g_servo_left.detach();
                break;
        case SERVO_RIGHT_NUM:
                g_servo_right.detach();
                break;
        case SERVO_HAND_ROT_NUM:
                g_servo_hand_rot.detach();
                break;
        }
}

/*!
   \brief Read Servo Offset from EEPROM. From OFFSET_START_ADDRESS, each offset occupy 4 bytes in rom
   \param servo_num SERVO_ROT_NUM, SERVO_LEFT_NUM, SERVO_RIGHT_NUM, SERVO_HAND_ROT_NUM
   \return Return servo offset
 */
double uArmClass::read_servo_offset(byte servo_num)
{
        double manual_servo_offset = 0.0f;
        EEPROM.get(MANUAL_OFFSET_ADDRESS + servo_num * sizeof(manual_servo_offset), manual_servo_offset);
        return manual_servo_offset;
}

/*!
   \brief Convert the Analog to Servo Angle, by this formatter, angle = intercept + slope * analog
   \param input_analog Analog Value
   \param servo_num SERVO_ROT_NUM, SERVO_LEFT_NUM, SERVO_RIGHT_NUM, SERVO_HAND_ROT_NUM
   \param withOffset true, false
   \return Return Servo Angle
 */
double uArmClass::analog_to_angle(int input_analog, byte servo_num)
{
  unsigned char adc_calibration_data[DATA_LENGTH],data[4]; //get the calibration data around the data input
  unsigned int min_data_calibration_address, max_calibration_data, min_calibration_data;
  unsigned int angle_range_min, angle_range_max;
  switch(servo_num)
  {
    case  SERVO_ROT_NUM:      iic_readbuf(&data[0], EXTERNAL_EEPROM_DEVICE_ADDRESS, ROT_SERVO_ADDRESS + 360, 2);//get the min adc calibration data for the map() function
                              iic_readbuf(&data[2], EXTERNAL_EEPROM_DEVICE_ADDRESS, ROT_SERVO_ADDRESS + 360 + 358, 2);//get the max adc calibraiton data for the map() function
                              break;
    case  SERVO_LEFT_NUM:     iic_readbuf(&data[0], EXTERNAL_EEPROM_DEVICE_ADDRESS, LEFT_SERVO_ADDRESS + 360, 2);//get the min adc calibration data for the map() function
                              iic_readbuf(&data[2], EXTERNAL_EEPROM_DEVICE_ADDRESS, LEFT_SERVO_ADDRESS + 360 + 358, 2);//get the max adc calibraiton data for the map() function
                              break;
    case  SERVO_RIGHT_NUM:    iic_readbuf(&data[0], EXTERNAL_EEPROM_DEVICE_ADDRESS, RIGHT_SERVO_ADDRESS + 360, 2);//get the min adc calibration data for the map() function
                              iic_readbuf(&data[2], EXTERNAL_EEPROM_DEVICE_ADDRESS, RIGHT_SERVO_ADDRESS + 360 + 358, 2);//get the max adc calibraiton data for the map() function
                              break;
    default:                  break;
  }

  max_calibration_data = (data[2]<<8) + data[3];
  min_calibration_data = (data[0]<<8) + data[1];
  //Serial.println("++++++++++++++++++++++");
  //Serial.println(max_calibration_data,DEC);
  //Serial.println(min_calibration_data,DEC);

  angle_range_min = map(input_analog, min_calibration_data, max_calibration_data, 1, 180) - (DATA_LENGTH>>2);
  min_data_calibration_address = (angle_range_min * 2);
  switch(servo_num)
  {
    case  SERVO_ROT_NUM:      iic_readbuf(adc_calibration_data, EXTERNAL_EEPROM_DEVICE_ADDRESS, ROT_SERVO_ADDRESS + min_data_calibration_address + 360, DATA_LENGTH);//360 means the adc calibration data offset
                              break;
    case  SERVO_LEFT_NUM:     iic_readbuf(adc_calibration_data, EXTERNAL_EEPROM_DEVICE_ADDRESS, LEFT_SERVO_ADDRESS + min_data_calibration_address + 360, DATA_LENGTH);//360 means the adc calibration data offset
                              break;
    case  SERVO_RIGHT_NUM:    iic_readbuf(adc_calibration_data, EXTERNAL_EEPROM_DEVICE_ADDRESS, RIGHT_SERVO_ADDRESS + min_data_calibration_address + 360, DATA_LENGTH);//360 means the adc calibration data offset
                              break;
    default:                  break;    
  }

  unsigned int deltaA = 0xffff, deltaB = 0, i, i_min = 0;
  for(i=0;i<(DATA_LENGTH >> 1);i++)
  {
      deltaB = abs ((adc_calibration_data[i+i]<<8) + adc_calibration_data[1+(i+i)] - input_analog);
      if(deltaA > deltaB)
      {
        i_min = i;
        deltaA = deltaB;
      }
  }

  angle_range_min = angle_range_min + i_min;
  angle_range_max = angle_range_min + 1;
  //Serial.println(angle_range_min,DEC);
  //Serial.println(angle_range_max,DEC);
  if((((adc_calibration_data[i_min+i_min]<<8) + adc_calibration_data[1+i_min+i_min]) - input_analog) >= 0)//determine if the current value bigger than the input_analog
  {
    //angle_rang_min = map(input_analog, min_calibration_data, max_calibration_data, 0, 180) - (DATA_LENGTH>>2);
    max_calibration_data = (adc_calibration_data[i_min+i_min]<<8) + adc_calibration_data[i_min+i_min+1];
    min_calibration_data = (adc_calibration_data[i_min+i_min-2]<<8) + adc_calibration_data[i_min+i_min-1];

  }
  else
  {
    angle_range_min++;//change the degree range
    angle_range_max++;
    max_calibration_data = (adc_calibration_data[i_min+i_min+2]<<8) + adc_calibration_data[i_min+i_min+3];
    min_calibration_data = (adc_calibration_data[i_min+i_min]<<8) + adc_calibration_data[i_min+i_min+1];
  }
 
  if(min_calibration_data < max_calibration_data)//return the angle
  {
    return ( 1.0 * (input_analog - min_calibration_data)/(max_calibration_data - min_calibration_data) + angle_range_min);
  }
  else
  {
    return (angle_range_min + angle_range_max) / 2.0;//angle from 1-180 but the address from 0-179
  } 
}

/** Calculate the angles from given coordinate x, y, z to theta_1, theta_2, theta_3
**/
/*!
   \brief Calculate the angles from given coordinate x, y, z to theta_1, theta_2, theta_3
   \param x X axis
   \param y Y axis
   \param z Z axis
   \param theta_1 SERVO_ROT_NUM servo angles
   \param theta_2 SERVO_LEFT_NUM servo angles
   \param theta_3 SERVO_RIGHT_NUM servo angles
 */
unsigned char uArmClass::coordinate_to_angle(double x, double y, double z, double& theta_1, double& theta_2, double& theta_3)//theta_1:rotation angle   theta_2:the angle of lower arm and horizon   theta_3:the angle of upper arm and horizon
{
  double x_in = 0.0;
  double z_in = 0.0;
  double right_all = 0.0;
  double sqrt_z_x = 0.0;
  double phi = 0.0;
  double theta_triangle = 0.0;
  z_in = (z - MATH_L1) / MATH_L3;

  if(y<0)
  {
  #ifdef DEBUG_MODE
    Serial.write("ERR0:coordinate_to_angle");
  #endif
    return OUT_OF_RANGE;
  }       
  // Calculate value of theta 1
  if(x==0)
  {
    theta_1 = 90;
  }
  else
  {
    //theta_1 = atan(y / x)*MATH_TRANS;

    if (x > 0)
    {
      theta_1 = atan(y / x)*MATH_TRANS;//angle tranfer 0-180 CCW
      //theta_1 = 180 - atan(y / x)*MATH_TRANS;//angle tranfer 0-180 CW
    }
    if (x < 0) 
    {
      theta_1 = 180 + atan(y / x)*MATH_TRANS;//angle tranfer  0-180 CCW
      //theta_1 = atan( (- y) / x)*MATH_TRANS;//angle tranfer  0-180 CW
    }

  }
                
  // Calculate value of theta 3
  if(theta_1!=90)
  {
    x_in = (x / cos(theta_1 / MATH_TRANS) - MATH_L2) / MATH_L3;
  }
  else
  {
    x_in = (y - MATH_L2) / MATH_L3;
  }


  phi = atan(z_in / x_in)*MATH_TRANS;//phi is the angle of line (from joint 2 to joint 4) with the horizon

  sqrt_z_x = sqrt(z_in*z_in + x_in*x_in);

  right_all = (sqrt_z_x*sqrt_z_x + MATH_L43*MATH_L43 - 1) / (2 * MATH_L43 * sqrt_z_x);//cosin law
  theta_3 = acos(right_all)*MATH_TRANS;//cosin law

  // Calculate value of theta 2
  right_all = (sqrt_z_x*sqrt_z_x + 1 - MATH_L43*MATH_L43) / (2 * sqrt_z_x);//cosin law
  theta_2 = acos(right_all)*MATH_TRANS;//cosin law

  //right_all = (MATH_L43*MATH_L43 + 1 - sqrt_z_x*sqrt_z_x) / (2 * MATH_L43);//cosin law
  //theta_triangle = acos(right_all)*MATH_TRANS;//used to detect the if theta_2>90 or not

  theta_2 = theta_2 + phi;
  theta_3 = theta_3 - phi;
  #ifdef DEBUG_MODE
  Serial.println("angle of servos(phi,theta_1,theta_2,theta_3)\n");
  Serial.println(phi,DEC);
  Serial.println(theta_1,DEC);
  Serial.println(theta_2,DEC);
  Serial.println(theta_3,DEC);
  #endif
  //determine if the angle can be reached
  if(x_in<=0)
  {
  #ifdef DEBUG_MODE
    Serial.write("ERR1:coordinate_to_angle");
  #endif
    return OUT_OF_RANGE;
  }
  if(((theta_2 - LEFT_SERVO_OFFSET) < L3_MIN_ANGLE)||((theta_2 - LEFT_SERVO_OFFSET) > L3_MAX_ANGLE))
  {
  #ifdef DEBUG_MODE
    Serial.write("ERR2:coordinate_to_angle");
  #endif
    return OUT_OF_RANGE;
  }
  if(((theta_3 - RIGHT_SERVO_OFFSET) < L4_MIN_ANGLE)||((theta_3 - RIGHT_SERVO_OFFSET) > L4_MAX_ANGLE))
  {
   #ifdef DEBUG_MODE
    Serial.write("ERR3:coordinate_to_angle");
  #endif
    return OUT_OF_RANGE;
  }
  if(((180 - theta_3 - theta_2)>L4L3_MAX_ANGLE)||((180 - theta_3 - theta_2)<L4L3_MIN_ANGLE))
  {
  #ifdef DEBUG_MODE
    Serial.write("ERR4:coordinate_to_angle");
  #endif
    return OUT_OF_RANGE;
  }
  /*if ((180 - theta_2 + theta_3)<L4L3_MIN_ANGLE)
  {
  #ifdef DEBUG_MODE
    Serial.write("ERR5:coordinate_to_angle");
  #endif
    return OUT_OF_RANGE;
  }*/


  return IN_RANGE;
}

/*!
   \brief Write Sretch & Height.
   \description This is an old control method to uArm. Using uarm's Stretch and height, , Height from -180 to 150
   \param armStretch Stretch from 0 to 195
   \param armHeight Height from -150 to 150
 */
void uArmClass::write_stretch_height(double armStretch, double armHeight){
/*        if(EEPROM.read(CALIBRATION_STRETCH_FLAG) != CONFIRM_FLAG) {
                alert(3, 200, 200);
                return;
        }
        double offsetL = 0;
        double offsetR = 0;

        EEPROM.get(OFFSET_STRETCH_START_ADDRESS, offsetL);
        EEPROM.get(OFFSET_STRETCH_START_ADDRESS + 4, offsetR);
        armStretch = constrain(armStretch, ARM_STRETCH_MIN, ARM_STRETCH_MAX) + 68;
        armHeight  = constrain(armHeight, ARM_HEIGHT_MIN, ARM_HEIGHT_MAX);
        double xx = armStretch*armStretch + armHeight*armHeight;
        double xxx = ARM_B2 - ARM_A2 + xx;
        double angleB = acos((armStretch*xxx+armHeight*sqrt(4.0*ARM_B2*xx-xxx*xxx))/(xx*2.0*ARM_B))* RAD_TO_DEG;
        double yyy = ARM_A2-ARM_B2+xx;
        double angleA =acos((armStretch*yyy-armHeight*sqrt(4.0*ARM_A2*xx-yyy*yyy))/(xx*2.0*ARM_A))* RAD_TO_DEG;
        int angleR =(int)(angleB + offsetR - 4);//int angleR =angleB + 40 + offsetR;
        int angleL =(int)(angleA + offsetL + 16);//int angleL =25 + angleA + offsetL;
        angleL = constrain(angleL, 5 + offsetL, 145 + offsetL);
        write_left_right_servo_angle(angleL,angleR,true);*/
}

/*!
   \brief get the current rot left right angles
 */
void uArmClass::get_current_rotleftright()
{
 /* unsigned int dat[8], temp;
  unsigned char i=0,j=0;
  for(i=0;i<8;i++){
    dat[i] = analogRead(SERVO_ROT_ANALOG_PIN);
  }
  for(i=0;i<8;i++){
    for(j=0;i+j<7;j++){
      if(dat[j]>dat[j+1]){
        temp = dat[j];
        dat[j] = dat[j+1];
        dat[j+1] = temp;
      }
    }
  }
  cur_rot = uarm.analog_to_angle((dat[2]+dat[3]+dat[4]+dat[5])/4,SERVO_ROT_NUM);
  for(i=0;i<8;i++){
    dat[i] = analogRead(SERVO_LEFT_ANALOG_PIN);
  }
  for(i=0;i<8;i++){
    for(j=0;i+j<7;j++){
      if(dat[j]>dat[j+1]){
        temp = dat[j];
        dat[j] = dat[j+1];
        dat[j+1] = temp;
      }
    }
  }
  cur_left = uarm.analog_to_angle((dat[2]+dat[3]+dat[4]+dat[5])/4,SERVO_LEFT_NUM);
  for(i=0;i<8;i++){
    dat[i] = analogRead(SERVO_RIGHT_ANALOG_PIN);
  }
  for(i=0;i<8;i++){
    for(j=0;i+j<7;j++){
      if(dat[j]>dat[j+1]){
        temp = dat[j];
        dat[j] = dat[j+1];
        dat[j+1] = temp;
      }
    }
  }
  cur_right = uarm.analog_to_angle((dat[2]+dat[3]+dat[4]+dat[5])/4,SERVO_RIGHT_NUM);*/
//check the calibration data and transfer the servo angle to the calibrated real angle
servo_angle_to_calibration_data(&cur_rot, ROT_SERVO_ADDRESS);
servo_angle_to_calibration_data(&cur_left, LEFT_SERVO_ADDRESS);
servo_angle_to_calibration_data(&cur_right, RIGHT_SERVO_ADDRESS);
//Serial.println(cur_rot, DEC);
//Serial.println((unsigned int)cur_rot, DEC);
/*iic_readbuf(ideal_angle, EXTERNAL_EEPROM_DEVICE_ADDRESS, ROT_SERVO_ADDRESS + (((unsigned int)cur_rot - 2) << 1), 4);
cur_rot = (double)(((ideal_angle[2] << 8) + ideal_angle[3]) - ((ideal_angle[0] << 8) + ideal_angle[1])) * (cur_rot - (unsigned int)cur_rot) + ((ideal_angle[0] << 8) + ideal_angle[1]);
cur_rot = cur_rot / 10.0;*/

//Serial.println(cur_rot, DEC);
//Serial.println(cur_left, DEC);
//Serial.println(cur_right, DEC);
}

void uArmClass::servo_angle_to_calibration_data(double *data, unsigned int address)
{
  unsigned int dat[8], temp;
  unsigned char i=0,j=0;
  for(i=0;i<8;i++){
    switch(address)
    {
      case ROT_SERVO_ADDRESS: dat[i] = analogRead(SERVO_ROT_ANALOG_PIN);break;
      case LEFT_SERVO_ADDRESS: dat[i] = analogRead(SERVO_LEFT_ANALOG_PIN);break;
      case RIGHT_SERVO_ADDRESS: dat[i] = analogRead(SERVO_RIGHT_ANALOG_PIN);break;
      default:break;
    }
  }
  for(i=0;i<8;i++){//BULB to get the most accuracy data
    for(j=0;i+j<7;j++){
      if(dat[j]>dat[j+1]){
        temp = dat[j];
        dat[j] = dat[j+1];
        dat[j+1] = temp;
      }
    }
  }
  switch(address)
  {
    case ROT_SERVO_ADDRESS: (*data) = uarm.analog_to_angle((dat[2]+dat[3]+dat[4]+dat[5])/4,SERVO_ROT_NUM);break;
    case LEFT_SERVO_ADDRESS: (*data) = uarm.analog_to_angle((dat[2]+dat[3]+dat[4]+dat[5])/4,SERVO_LEFT_NUM);break;
    case RIGHT_SERVO_ADDRESS: (*data) = uarm.analog_to_angle((dat[2]+dat[3]+dat[4]+dat[5])/4,SERVO_RIGHT_NUM);break;
    default:break;
  }
  //check the external eeprom and transfer the data to ideal angle
  unsigned char ideal_angle[4];
  iic_readbuf(ideal_angle, EXTERNAL_EEPROM_DEVICE_ADDRESS, address + (((unsigned int)(*data) - 1) << 1), 4);
  (*data) = (double)(((ideal_angle[2] << 8) + ideal_angle[3]) - ((ideal_angle[0] << 8) + ideal_angle[1])) * ((*data) - (unsigned int)(*data)) + ((ideal_angle[0] << 8) + ideal_angle[1]);
  (*data) = (*data) / 10.0;
}

/*!
   \brief Calculate X,Y,Z to g_current_x,g_current_y,g_current_z
   \param *cur_rot the address of value we want to caculate
   \param *cur_left the address of value we want to caculate
   \param *cur_right the address of value we want to caculate
   \param *g_currnet_x the address of value we want to caculate
   \param *g_current_y the address of value we want to caculate
   \param *g_current_z the address of value we want to caculate
   \param for movement is the flage to detect if we should get the real current angle of the uarm
 */
void uArmClass::get_current_xyz(double *cur_rot, double *cur_left , double *cur_right, double *g_current_x, double *g_current_y, double *g_current_z, bool for_movement )
{
  if(for_movement==true){
    get_current_rotleftright();
  }

  //add the offset first
  *cur_left = *cur_left + LEFT_SERVO_OFFSET;
  *cur_right = *cur_right + RIGHT_SERVO_OFFSET;

  double stretch = MATH_L3 * cos((*cur_left) / MATH_TRANS) + MATH_L4 * cos((*cur_right) / MATH_TRANS) + MATH_L2;
  double height = MATH_L3 * sin((*cur_left) / MATH_TRANS) - MATH_L4 * sin((*cur_right) / MATH_TRANS) + MATH_L1;
  *g_current_x = stretch * cos((*cur_rot) / MATH_TRANS);
  *g_current_y = stretch * sin((*cur_rot) / MATH_TRANS);
  *g_current_z = height;
//Serial.println(*g_current_x, DEC);
//Serial.println(*g_current_y, DEC);
//Serial.println(*g_current_z, DEC);
}

/*!
   \brief "Genernate the position array"
   \param start_val Start Position
   \param end_val End Position
   \param interp_vals interpolation array
   \param ease_type INTERP_EASE_INOUT_CUBIC, INTERP_LINEAR, INTERP_EASE_INOUT, INTERP_EASE_IN, INTERP_EASE_OUT
*/
void uArmClass::interpolate(double start_val, double end_val, double *interp_vals, byte ease_type) {
        if(ease_type == INTERP_EASE_INOUT_CUBIC)// make sure all the data are still in range
        {
          start_val = start_val/10.0;
          end_val = end_val/10.0;
        }
        double delta = end_val - start_val;
        for (byte f = 0; f < INTERP_INTVLS; f++) {
                switch (ease_type) {
                case INTERP_LINEAR://linear moving
                        *(interp_vals+f) = delta * f / INTERP_INTVLS + start_val;
                        break;
                case INTERP_EASE_INOUT://
                {
                        float t = f / (INTERP_INTVLS / 2.0);
                        if (t < 1) {
                                *(interp_vals+f) = delta / 2 * t * t + start_val;
                        } else {
                                t--;
                                *(interp_vals+f)= -delta / 2 * (t * (t - 2) - 1) + start_val;
                        }
                }
                break;
                /*case INTERP_EASE_IN:
                {
                        float t = (float)f / INTERP_INTVLS;
                        *(interp_vals+f) = delta * t * t + start_val;
                }
                break;
                case INTERP_EASE_OUT:
                {
                        float t = (float)f / INTERP_INTVLS;
                        *(interp_vals+f) = -delta * t * (t - 2) + start_val;
                }
                break;*/
                case INTERP_EASE_INOUT_CUBIC: // this is a compact version of Joey's original cubic ease-in/out
                {
                        float t = (float)f / INTERP_INTVLS;
                        //*(interp_vals+f) = 10.0*(start_val + (3 * delta) * (t * t) + (-2 * delta) * (t * t * t));
                        *(interp_vals+f) = 10.0 * (start_val + t* t * delta * (3 + (-2) * t));
                }
                break;
                }
        }
}

/*!
   \brief Move To, Action Control Core Function
   \param x X Axis Value
   \param y Y Axis Value
   \param z Z Axis Value
   \param hand_angle Hand Axis
   \param relative_flags ABSOLUTE, RELATIVE
   \param enable_hand Enable Hand Axis
*/

unsigned char uArmClass::move_to(double x, double y, double z, double hand_angle, byte relative_flags, double time, byte ease_type, boolean enable_hand) {
  // get current angles of servos


  // deal with relative xyz positioning
  if(relative_flags == RELATIVE)
  {
    x = g_current_x + x;
    y = g_current_x + y;
    z = g_current_z + z;
    //hand_angle = current_hand + hand_angle;
  }

  // find target angles
  double tgt_rot;
  double tgt_left;
  double tgt_right;
  //  detect if the xyz coordinate are in the range
  if(coordinate_to_angle(x, y, z, tgt_rot, tgt_left, tgt_right) == OUT_OF_RANGE)
  {
    return OUT_OF_RANGE_IN_DST;
  }

  //calculate the length and use the longest to determine the numbers of interpolation
  unsigned int delta_rot=abs(tgt_rot-cur_rot);
  unsigned int delta_left=abs(tgt_left-cur_left);
  unsigned int delta_right=abs(tgt_right-cur_right);

  INTERP_INTVLS = max(delta_rot,delta_left);
  INTERP_INTVLS = max(INTERP_INTVLS,delta_right);

  INTERP_INTVLS = (INTERP_INTVLS<60) ? INTERP_INTVLS : 60;
  INTERP_INTVLS = INTERP_INTVLS * time;// speed determine the number of interpolation
  //INTERP_INTVLS = 1;

  //if (time > 0)
  //{

    interpolate(g_current_x, x, x_array, ease_type);// /10 means to make sure the t*t*t is still in the range
    interpolate(g_current_y, y, y_array, ease_type);
    interpolate(g_current_z, z, z_array, ease_type);


    //give the final destination value to the array
    x_array[INTERP_INTVLS] = x;
    y_array[INTERP_INTVLS] = y;
    z_array[INTERP_INTVLS] = z;

    double rot, left, right;
    for (byte i = 0; i <= INTERP_INTVLS; i++)
    {
      
      if(coordinate_to_angle(x_array[i], y_array[i], z_array[i], rot, left, right) == OUT_OF_RANGE)//check if all the data in range
      {
        return OUT_OF_RANGE_IN_PATH;
      }
      else
      {
        //change to the rot/left/right angles
        x_array[i] = rot;
        y_array[i] = left;
        z_array[i] = right;
      }
    }

    //INTERP_INTVLS>>=2;
    //interpolate(cur_hand, hand_angle, hand_array, ease_type);
    //hand_array[INTERP_INTVLS] = hand_angle;

g_current_x=x;
g_current_y=y;
g_current_z=z;
cur_rot=rot;
cur_left=left;
cur_right=right;
    move_times=0;//start to caculate the movement
    /*for (byte i = 0; i < INTERP_INTVLS; i++)
    {
      double rot, left, right;
      if(coordinate_to_angle(x_array[i], y_array[i], z_array[i], rot, left, right) == OUT_OF_RANGE)
      {
        return OUT_OF_RANGE;
      }

      left = left - LEFT_SERVO_OFFSET;//assembling offset
      right = right - RIGHT_SERVO_OFFSET;//assembling offset
      read_servo_calibration_data(&rot,&left,&right);

      //if(enable_hand)
      write_servos_angle(rot, left, right, hand_array[i]);
      //else
      //  write_servos_angle(rot, left, right);
      delay(time * 1000 / INTERP_INTVLS);
    }*/
  //}

      
// the destination
/*  tgt_left = tgt_left - LEFT_SERVO_OFFSET;//assembling offset
  tgt_right = tgt_right - RIGHT_SERVO_OFFSET;//assembling offset    
  

//#ifdef DEBUG_MODE
Serial.println(tgt_rot,DEC);
Serial.println(tgt_left,DEC);
Serial.println(tgt_right,DEC);
//#endif
  read_servo_calibration_data(&tgt_rot,&tgt_left,&tgt_right);
    // set final target position at end of interpolation or atOnce
    //if (enable_hand)
  write_servos_angle(tgt_rot, tgt_left, tgt_right, hand_angle);
    //else
    //  write_servo_angle(tgt_rot, tgt_left, tgt_right);*/

  return IN_RANGE;
}

/*!
   \brief Calculate Y
   \param theta_1
   \param theta_2
   \param theta_3
   \return Y Axis Value
*/
double uArmClass::angle_to_coordinate_y(double theta_1, double theta_2, double theta_3)
{
        double l5_2 = (MATH_L2 + MATH_L3*cos(theta_2 / MATH_TRANS) + MATH_L4*cos(theta_3 / MATH_TRANS));

        return -sin(abs(theta_1 / MATH_TRANS))*l5_2;
}

/*!
   \brief Close Gripper
*/
void uArmClass::gripper_catch()
{
        pinMode(GRIPPER, OUTPUT);
        digitalWrite(GRIPPER, LOW); // gripper catch
        g_gripper_reset = true;
}

/*!
   \brief Release Gripper
*/
void uArmClass::gripper_release()
{
        if(g_gripper_reset)
        {
                pinMode(GRIPPER, OUTPUT);
                digitalWrite(GRIPPER, HIGH); // gripper release
                g_gripper_reset= false;
        }
}

/*!
   \brief Turn on Pump
*/
void uArmClass::pump_on()
{

        pinMode(PUMP_EN, OUTPUT);
        pinMode(VALVE_EN, OUTPUT);
        digitalWrite(VALVE_EN, LOW);
        digitalWrite(PUMP_EN, HIGH);
}

/*!
   \brief Turn off Pump
*/
void uArmClass::pump_off()
{
        pinMode(PUMP_EN, OUTPUT);
        pinMode(VALVE_EN, OUTPUT);
        digitalWrite(VALVE_EN, HIGH);
        digitalWrite(PUMP_EN, LOW);
        delay(50);
        digitalWrite(VALVE_EN,LOW);
}

/*!
  \brief systick
*/

ISR(TIMER0_COMPA_vect) 
{ 
  sys_tick++;
  PORTB ^= 0x01;
}
//*************************************uart communication**************************************//
String uArmClass::runCommand(String cmnd){
    
    // To save memory, create the "[OK" and "]\n" right now, in flash memory
    String ok    = F("[ok"); 
    String endB  = F("]\n");

    // sMove Command---------------------------------------------------------
    if(cmnd.indexOf(F("sMove")) >= 0){
      String moveParameters[] = {F("X"), F("Y"), F("Z"), F("S")};
      String errorResponse    = isValidCommand(cmnd, moveParameters, 4);
      if(errorResponse.length() > 0){return errorResponse;}
      //  Create action and respond
      double values[4];
      getCommandValues(cmnd, moveParameters, 4, values);
      if(move_to(values[0], values[1], values[2])!=IN_RANGE)
      {
        return F("[R2:fail]\n");
      }
      return F("[R1:succeed]\n");
      
    }else
    //gSimuX#Y#Z#-------------------------------------------------------------
    if(cmnd.indexOf(F("gSimu")) >= 0){
      String moveParameters[] = {F("X"), F("Y"), F("Z")};
      String errorResponse    = isValidCommand(cmnd, moveParameters, 3);
      if(errorResponse.length() > 0){return errorResponse;}
      //  Create action and respond
      double values[3];
      getCommandValues(cmnd, moveParameters, 3, values);
      switch(move_to(values[0], values[1], values[2]))
      {
        case IN_RANGE             :move_times=255;//disable move
                                  return F("[R1:in range]\n");
                                  break;
        case OUT_OF_RANGE_IN_PATH :move_times=255;//disable move
                                  return F("[R2:fail in dst]\n");
                                  break;
        case OUT_OF_RANGE_IN_DST  :move_times=255;//disable move
                                  return F("[R3:fail in path]\n");
                                  break;
        default:break;
      }

    }else

    //gVer---------------------------------------------------------------------
    if(cmnd.indexOf(F("gVer")) >= 0){
      return "[Ver " + String(current_ver) + "]\n";

    }else

    // sServoN#V#--------------------------------------------------------------
    if(cmnd.indexOf(F("sServo")) >= 0)
    {
       String servoSetParameters[] = {F("N"), F("V")};
       String errorResponse        = isValidCommand(cmnd, servoSetParameters, 2);
       if(errorResponse.length() > 0) {return errorResponse;}
       
       double values[2];
       getCommandValues(cmnd, servoSetParameters, 2, values);
       // Clamp the angle between 0 and 180
       if(values[1] < 0)
       {
        values[1] = 0;
       } 
       else if(values[1] > 180) 
       { 
        values[1] = 180;
       } 
       switch((unsigned int)values[0])
       {
        case SERVO_ROT_NUM:   g_servo_rot.write(values[1]);
                              break;
        case SERVO_LEFT_NUM:  g_servo_left.write(values[1]);
                              break;
        case SERVO_RIGHT_NUM: g_servo_right.write(values[1]);
                              break;
        case SERVO_HAND_ROT_NUM: g_servo_hand_rot.write(values[1]);
                                 break;
        default: break;
       }
       return F("[ok]\n");
    }else

    //sPumpV#------------------------------------------------------------------
    if(cmnd.indexOf(F("sPump")) >= 0){

       String servoSetParameters[] = {F("V")};
       String errorResponse        = isValidCommand(cmnd, servoSetParameters, 1);
       if(errorResponse.length() > 0) {return errorResponse;}
       
       double values[1];
       getCommandValues(cmnd, servoSetParameters, 1, values);
       if(values[0]==0)//off
       {
        pump_off();
       }else//on
       {
        pump_on();
       }
       return F("[ok]\n");
    }else
    
    //gPump---------------------------------------------------------------------
    if(cmnd.indexOf(F("sPump")) >= 0){
      return F("[R1:sucking objs]");//return "[R2:sucking nothing]";return "[R1:off]";
    }else

    //sGripperV#----------------------------------------------------------------
    if(cmnd.indexOf(F("sGripper")) >= 0){

       String servoSetParameters[] = {F("V")};
       String errorResponse        = isValidCommand(cmnd, servoSetParameters, 1);
       if(errorResponse.length() > 0) {return errorResponse;}
       
       double values[1];
       getCommandValues(cmnd, servoSetParameters, 1, values);
       if(values[0]==0)//release
       {
        gripper_release();
       }else//catch
       {
        gripper_catch();
       }
       return F("[ok]\n");
    }else

    //gGipper-------------------------------------------------------------------
    if(cmnd.indexOf(F("gGripper")) >= 0){
      return F("[R1:grabbing objs]");//return "[R2:grabbing nothing]";return "[R1:off]";
    }else

    // sAttachS#----------------------------------------------------------------
    if(cmnd.indexOf(F("sAttach")) >= 0){
      String attachParameters[] = {F("S")};
      String errorResponse = isValidCommand(cmnd, attachParameters, 1);
      if(errorResponse.length() > 0) {return errorResponse;}
      double values[1];
      getCommandValues(cmnd, attachParameters, 1, values);
      if((values[0]>=0)&&(values[0]<=3)){
        switch((unsigned int)values[0])
        {
          case SERVO_ROT_NUM:   attach_servo(SERVO_ROT_NUM);
                                break;
          case SERVO_LEFT_NUM:  attach_servo(SERVO_LEFT_NUM);
                                break;
          case SERVO_RIGHT_NUM: attach_servo(SERVO_RIGHT_NUM);
                                break;
          case SERVO_HAND_ROT_NUM: attach_servo(SERVO_HAND_ROT_NUM);
                                  break;
          default: break;
        }
        return F("[R1:ok]\n");
      }else{
        return F("[R2:wrong servo num]\n");
      }
       
    }else

    
    // sDetachS#----------------------------------------------------------------
    if(cmnd.indexOf(F("sDetach")) >= 0){
      String detachParameters[] = {F("S")};
      String errorResponse      = isValidCommand(cmnd, detachParameters, 1);
      if(errorResponse.length() > 0) {return errorResponse;}
      
      double values[1];
      getCommandValues(cmnd, detachParameters, 1, values);
      if((values[0]>=0)&&(values[0]<=3)){
        switch((unsigned int)values[0])
        {
          case SERVO_ROT_NUM:   g_servo_rot.detach();
                                break;
          case SERVO_LEFT_NUM:  g_servo_left.detach();
                                break;
          case SERVO_RIGHT_NUM: g_servo_right.detach();
                                break;
          case SERVO_HAND_ROT_NUM: g_servo_hand_rot.detach();
                                  break;
          default: break;
        }  
        return F("[R1:ok]\n");
      }else{ 
        return F("[R2:wrong servo num]\n");
      }
    }else

    //gCrd---------------------------------------------------------------------
    if(cmnd.indexOf(F("gCrd")) >= 0){
      get_current_xyz(&cur_rot, &cur_left, &cur_right, &g_current_x, &g_current_y, &g_current_z, true);
      return "[ok X" + String(g_current_x) + " Y" + String(g_current_y) + " Z" + String(g_current_z) + "]\n";
    }else

    //gAng---------------------------------------------------------------------
    if(cmnd.indexOf(F("gAng")) >= 0){
      get_current_rotleftright();
      return "[ok rot" + String(cur_rot) + " left" + String(cur_left) + " right" + String(cur_right) + "]\n";
    }else

    //gIKX#Y#Z#----------------------------------------------------------------
    if(cmnd.indexOf(F("gIK")) >= 0){
       String IKParameters[] = {F("X"), F("Y"), F("Z")};
       String errorResponse  = isValidCommand(cmnd, IKParameters, 3);
       if(errorResponse.length() > 0) {return errorResponse;}
       
       double values[3];
       getCommandValues(cmnd, IKParameters, 3, values);
       double rot, left, right;
       coordinate_to_angle(values[0], values[1], values[2] , rot, left, right);
       left = left - LEFT_SERVO_OFFSET;//assembling offset
       right = right - RIGHT_SERVO_OFFSET;//assembling offset
       return "[ok rot" + String(rot) + " left" + String(left) + " right" + String(right) + "]\n";
    }else

    //gFKT#L#R#-----------------------------------------------------------------
    // Get Forward Kinematics
    if(cmnd.indexOf(F("gFK")) >= 0){
       String IKParameters[] = {F("T"), F("L"), F("R")};
       String errorResponse  = isValidCommand(cmnd, IKParameters, 3);
       if(errorResponse.length() > 0) {return errorResponse;}
       
       double values[3];
       getCommandValues(cmnd, IKParameters, 3, values);
       double x, y, z;
       //values[1] += LEFT_SERVO_OFFSET;//add the offset
       //values[2] += RIGHT_SERVO_OFFSET;
       get_current_xyz(&values[0], &values[1], &values[2], &x, &y, &z, false);
       return "[ok X" + String(x) + " Y" + String(y) + " Z" + String(z) + "]\n";
    }else

    
    //gMov-----------------------------------------------------------------------
    if(cmnd.indexOf(F("gMov")) >= 0){
      if(available()==false)
      {
        return F("[R1:running]\n");
      }
      else
      {
        return F("[R2:free]\n");
      }

    }else
    
    //gTip-----------------------------------------------------------------------
    if(cmnd.indexOf(F("gTip")) >= 0){
      if(digitalRead(LIMIT_SW)==HIGH)
      {
        return F("[R1:no trigger]\n");
      }
      else
      {
        return F("[R2:triggering]\n");
      }
    }else

    //sBuzzF#T#-------------------------------------------------------------------
    if(cmnd.indexOf(F("sBuzz")) >= 0){
       String buzzerParameters[] = { F("F"),F("T")};
       String errorResponse      = isValidCommand(cmnd, buzzerParameters, 2);
       if(errorResponse.length() > 0) {return errorResponse;}
       
       double values[2];
       getCommandValues(cmnd, buzzerParameters, 2, values);
       if((values[0] < 0)||(values[1] < 0)){
         return F("[R2:no negtive]\n");
       }
       tone(BUZZER, values[0]);
       buzzerStopTime = values[1];
       return F("[R1:OK]\n");
    }
    
     
    if(cmnd.length() > 0){
      return "[ERROR: No such command: " + cmnd + endB;
    }else{
      return F("");
    }
}




void uArmClass::getCommandValues(String cmnd, String parameters[], int parameterCount, double *valueArray){
  unsigned char index[parameterCount],p;
  for(p = 0; p < parameterCount; p++){
    index[p] = cmnd.indexOf(parameters[p]);
  } 
  for(p = 0; p < parameterCount; p++){
    if(p < parameterCount - 1){
      
      valueArray[p] = cmnd.substring(index[p] + 1, index[p + 1]).toFloat();
    }else{
      valueArray[p] = cmnd.substring(index[p] + 1).toFloat();
    }
  }
}




String uArmClass::isValidCommand(String cmnd, String parameters[], int parameterCount){
  int index[parameterCount];
  
  String errorMissingParameter = F("[ERROR: Missing Parameter ");
  String errorWrongOrder       = F("[ERROR: Incorrect Parameter order on parameter ");
  String endingBracket         = F("]\n");
  //  Get all indexes
  for(int p = 0; p < parameterCount; p++){
      index[p] = cmnd.indexOf(parameters[p]);
      if(index[p] == -1){return errorMissingParameter + parameters[p] + F(" ") + cmnd + endingBracket;}
  }
  
  //  Check that the commands are in the correct order
  for(int p = 0; p < parameterCount; p++){
    if(parameterCount == 1){break;}
    
    if(p < parameterCount - 1){
      if(!(index[p] < index[p + 1])){
        return errorWrongOrder + parameters[p] + endingBracket;
      }
    }else if(!(index[p] > index[p-1])){
      return errorWrongOrder + parameters[p] + endingBracket;
    }
  }
  
  //  Check that there is something between each parameter (AKA, the value)
  for(int p = 0; p < parameterCount; p++){   
    if(p < parameterCount - 1){
      if((index[p + 1] - index[p]) == 1){
        return errorMissingParameter + parameters[p] + endingBracket;
      }
    }else if(index[p] == cmnd.length() - 1){
      return errorMissingParameter + parameters[p] + endingBracket;
    }
  }
  
  return F("");
}


//*************************************private functions***************************************//
//**just used by the 512k external eeprom**//
void uArmClass::delay_us(){}

void uArmClass::iic_start()
{
  PORTC |= 0x20;//  SCL=1
  delay_us();
  PORTC |= 0x10;//  SDA = 1;
  delay_us();
  PORTC &= 0xEF;//  SDA = 0;
  delay_us();
  PORTC &= 0xDF;//  SCL=0
  delay_us();
}

void uArmClass::iic_stop()
{
  PORTC &= 0xDF;//  SCL=0
  delay_us();
  PORTC &= 0xEF;//  SDA = 0;
  delay_us();
  PORTC |= 0x20;//  SCL=1
  delay_us();
  PORTC |= 0x10;//  SDA = 1;
  delay_us();
}

//return 0:ACK=0
//return 1:NACK=1
unsigned char uArmClass::read_ack()
{
  unsigned char old_state;
  old_state = DDRC;
  DDRC = DDRC & 0xEF;//SDA INPUT
  PORTC |= 0x10;//  SDA = 1;
  delay_us();
  PORTC |= 0x20;//  SCL=1
  delay_us();
  if((PINC&0x10) == 0x10) // if(SDA)
  {
    PORTC &= 0xDF;//  SCL=0
    iic_stop();
    return 1; 
  }
  else
  {
    PORTC &= 0xDF;//  SCL=0
    
    DDRC = old_state;
    
    return 0;
  }
}

//ack=0:send ack
//ack=1:do not send ack
void uArmClass::send_ack()
{
  unsigned char old_state;
  old_state = DDRC;
  DDRC = DDRC | 0x10;//SDA OUTPUT
  PORTC &= 0xEF;//  SDA = 0;  
  delay_us();
  PORTC |= 0x20;//  SCL=1
  delay_us();
  PORTC &= 0xDF;//  SCL=0
  delay_us();
  DDRC = old_state;
  PORTC |= 0x10;//  SDA = 1;
  delay_us();
}

void uArmClass::iic_sendbyte(unsigned char dat)
{
  unsigned char i;
  for(i = 0;i < 8;i++)
  {
    if(dat & 0x80)
      PORTC |= 0x10;//  SDA = 1;
    else
      PORTC &= 0xEF;//  SDA = 0;
    dat <<= 1;
    delay_us();
    PORTC |= 0x20;//  SCL=1
    delay_us();
    PORTC &= 0xDF;//  SCL=0
  }
}

unsigned char uArmClass::iic_receivebyte()
{
  unsigned char i,byte = 0;
  unsigned char old_state;
  old_state = DDRC;
  DDRC = DDRC & 0xEF;//SDA INPUT
  for(i = 0;i < 8;i++)
  {
    PORTC |= 0x20;//  SCL=1
    delay_us();
    byte <<= 1;
    if((PINC&0x10) == 0x10) // if(SDA)
      byte |= 0x01;
    delay_us();
    PORTC &= 0xDF;//  SCL=0
    DDRC = old_state;
    delay_us();
  }
  return byte;
}

unsigned char uArmClass::iic_writebuf(unsigned char *buf,unsigned char device_addr,unsigned int addr,unsigned char len)
{
  DDRC = DDRC | 0x30;
  PORTC = PORTC | 0x30;
  unsigned char length_of_data=0;//page write
  length_of_data = len;
  iic_start();
  iic_sendbyte(device_addr);
  if(read_ack())return 1;
  iic_sendbyte(addr>>8);
  if(read_ack())return 1;
  iic_sendbyte(addr%256);
  if(read_ack())return 1; 
  while(len != 0)
  {
    iic_sendbyte(*(buf + length_of_data - len));
    len--;
    if(read_ack())return 1;
    delay(5);
  }
  iic_stop();
  
  return 0;
}

unsigned char uArmClass::iic_readbuf(unsigned char *buf,unsigned char device_addr,unsigned int addr,unsigned char len)
{
  DDRC = DDRC | 0x30;
  PORTC = PORTC | 0x30;
  unsigned char length_of_data=0;
  length_of_data = len;
  iic_start();
  iic_sendbyte(device_addr);
  if(read_ack())return 1;
  iic_sendbyte(addr>>8);
  if(read_ack())return 1;
  iic_sendbyte(addr%256);
  if(read_ack())return 1;
  iic_start();
  iic_sendbyte(device_addr+1);
  if(read_ack())return 1;

  while(len != 0)
  {
    *(buf + length_of_data - len) = iic_receivebyte();
    //Serial.println(*(buf + length_of_data - len),DEC);
    len--;
    if(len != 0){
      send_ack();
    }
  }
  iic_stop();
  return 0;
}
//*************************************end***************************************//