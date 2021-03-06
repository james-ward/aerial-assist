#include <math.h>

#include "Chassis.h"
#include "../Robotmap.h"
#include "../commands/MeccanumDrive.h"
#include "../SpeedEncoder.h"

Chassis::Chassis():Subsystem("Chassis"), gyro(new Gyro(GYRO_PORT)){
    driveMotorA = new Victor(MOTOR_A_PWM);
    driveMotorB = new Victor(MOTOR_B_PWM);
    driveMotorC = new Victor(MOTOR_C_PWM);
    driveMotorD = new Victor(MOTOR_D_PWM);
    
    encoderA = new SpeedEncoder(ENCODER_A_1, ENCODER_A_2, ENCODER_A_REV);
	encoderB = new SpeedEncoder(ENCODER_B_1, ENCODER_B_2, ENCODER_B_REV); 
	encoderC = new SpeedEncoder(ENCODER_C_1, ENCODER_C_2, ENCODER_C_REV); 
	encoderD = new SpeedEncoder(ENCODER_D_1, ENCODER_D_2, ENCODER_D_REV);
	
	encoderA->Reset();
    encoderB->Reset();
    encoderC->Reset();
    encoderD->Reset();
    
    encoderA->Start();
    encoderB->Start();
    encoderC->Start();
    encoderD->Start();
    
    gyro->SetSensitivity(-Gyro::kDefaultVoltsPerDegreePerSecond);
    gyro->SetPIDSourceParameter(Gyro :: kAngle);
    correction = new GyroCorrection();
    gyro_pid = new PIDController(-0.05,0,0,gyro,correction);
    gyro_pid->SetOutputRange(-360, 360);
    gyro_pid->Enable();
    
    SetHeading = 0.0;
    
    //mpu = new lib4774::MPU6050(MPU6050_ADDRESS_AD0_LOW); // GY-521 has MPU-6050 set to AD0 low.
    
    //mpu->dmpInitialize();
    
    /*mpu->setXGyroOffset(5000);//45.5);
    mpu->setYGyroOffset(-22.5);//-22.5;
    mpu->setZGyroOffset(43);
    mpu->setXAccelOffset(5000);//-157.79;
    mpu->setYAccelOffset(14.74);
    mpu->setZAccelOffset(264.6);*/
    
    loopcounter = 0;
    gyroX = 0;
    gyroY = 0;
    gyroZ = 0;
    accX = 0;
    accY = 0;
    accZ = 0;
    
    weBePimpin = false;
    
    // Add to Live Window
    liveWindow();
}

Chassis::~Chassis() {
	delete driveMotorA;
	delete driveMotorB;
	delete driveMotorC;
	delete driveMotorD;
	
	delete encoderA;
	delete encoderB;
	delete encoderC;
	delete encoderD;
	delete gyro;
}

void Chassis::drive(double vX, double vY, double vZ, double throttle) {
	double vMotor[4];
	
	//this maps the body co-ordinates to the absolute field co-ordinates
	if(weBePimpin){
		double heading = gyro->GetAngle()*3.14159/180.0;
		double vXpimp = vX*cos(heading)+vY*sin(heading);
		double vYpimp = -vX*sin(heading)+vY*cos(heading);
		vX = vXpimp;
		vY = vYpimp;
	}
	
	double ax = log(JOYSTICK_X_EXPONENTIAL+1);
	double ay = log(JOYSTICK_Y_EXPONENTIAL+1);
	double az = log(JOYSTICK_Z_EXPONENTIAL+1);

	
	if (vX > 0) {
		vX = (exp(ax*vX)-1)/JOYSTICK_X_EXPONENTIAL;
	} else {
		vX = -(exp(ax*-vX)-1)/JOYSTICK_X_EXPONENTIAL; 
		}
		
	if (vY > 0) {
		vY =(exp(ay*vY)-1)/JOYSTICK_Y_EXPONENTIAL;
	} else {
		vY = -(exp(ay*-vY)-1)/JOYSTICK_Y_EXPONENTIAL; 
	}
		
	if (vZ > 0) {
		vZ = (exp(az*vZ)-1)/JOYSTICK_Z_EXPONENTIAL;
	} else {
		vZ = -(exp(az*-vZ)-1)/JOYSTICK_Z_EXPONENTIAL; 
    }
	
    if (weBePimpin){
        if (vZ != 0 || (abs(gyro->GetRate()) > 2 && !gyro_pid->IsEnabled())) {
        
            SetHeading = gyro->GetAngle();
            gyro_pid->Reset();
            gyro_pid->SetSetpoint(SetHeading);
            gyro_pid->Enable();
            correction->correction = 0;
    } else {
            vZ = correction->correction;
        }
    }


    
	vMotor[0] = vX - vY - vZ;
	vMotor[1] = vX + vY - vZ;
	vMotor[2] = -vX + vY - vZ;
	vMotor[3] = -vX - vY - vZ;
	
	double vmax = 1.0;
	for(int i = 0; i < 4; ++i){
		if(abs(vMotor[i]) >vmax ) {
			vmax =abs (vMotor [i]);
		}
	}
	for (int i = 0; i < 4; ++i){
		//vMotor[i] = vMotor[i]/vmax*throttle*VMAX; //This is the set point in counts/sec
		vMotor[i] = vMotor[i]/vmax*throttle; //This is the set point in counts/sec
	}
	
	driveMotorA->Set(vMotor[0]);
    driveMotorB->Set(vMotor[1]);
    driveMotorC->Set(vMotor[2]);
    driveMotorD->Set(vMotor[3]);
    
    // Put the values onto the SmartDashboard
    SmartDashboard::PutNumber("Motor A", vMotor[0]);
    SmartDashboard::PutNumber("Motor B", vMotor[1]);
    SmartDashboard::PutNumber("Motor C", vMotor[2]);
    SmartDashboard::PutNumber("Motor D", vMotor[3]);
    SmartDashboard::PutNumber("Gyro(deg)", gyro->GetAngle());
    SmartDashboard::PutNumber("Gyro(rate)", gyro->GetRate());
    SmartDashboard::PutNumber("EncoderA(counts)", encoderA->Get());
	SmartDashboard::PutNumber("EncoderB(counts)", encoderB->Get());
	SmartDashboard::PutNumber("EncoderC(counts)", encoderC->Get());
	SmartDashboard::PutNumber("EncoderD(counts)", encoderD->Get());
	SmartDashboard::PutNumber("EncoderA(speed)", encoderA->GetRate());
	SmartDashboard::PutNumber("EncoderB(speed)", encoderB->GetRate());
	SmartDashboard::PutNumber("EncoderC(speed)", encoderC->GetRate());
	SmartDashboard::PutNumber("EncoderD(speed)", encoderD->GetRate());
    //SmartDashboard::PutNumber("MPU6050_frc(deg)", mpu->getRotationZ());
    SmartDashboard::PutNumber("GyroPID", correction->correction);
    //SmartDashboard::PutNumber("X gyro offset", mpu->getXGyroOffset());
    //SmartDashboard::PutNumber("Get Y gyro offset", mpu->getYGyroOffset());
    //SmartDashboard::PutNumber("Get Z gyro offset", mpu->getZGyroOffset());
    //SmartDashboard::PutNumber("Get X accel offset", mpu->getXAccelOffset());
    //SmartDashboard::PutNumber("Get Y accel offset", mpu->getYAccelOffset());
    //SmartDashboard::PutNumber("Get Z accel offset", mpu->getZAccelOffset());
    SmartDashboard::PutNumber("gyro Heading (deg)", gyro->GetAngle());
    SmartDashboard::PutNumber("gyro SetPoint (deg)", SetHeading);
    SmartDashboard::PutNumber("control loop output", correction->correction);
    SmartDashboard::PutBoolean("are we Pimpin", weBePimpin);
    
    loopcounter ++;
    
    /*if (loopcounter == 20){
        SmartDashboard::PutNumber("X Rotation", gyroX/loopcounter);
        SmartDashboard::PutNumber("Y Rotation", gyroY/loopcounter);
        SmartDashboard::PutNumber("Z Rotation", gyroZ/loopcounter);
        SmartDashboard::PutNumber("X Acceleration", accX/loopcounter);
        SmartDashboard::PutNumber("Y Acceleration", accY/loopcounter);
        SmartDashboard::PutNumber("Z Acceleration", accZ/loopcounter);
        
        loopcounter = 0;
        gyroX = gyroY = gyroZ = accX = accY = accZ = 0;
    }   else {
        gyroX += mpu->getRotationX() + 22.75;
        gyroY += mpu->getRotationY() - 22.5;
        gyroZ += mpu->getRotationZ() + 43;
        accX += mpu->getAccelerationX() - 157.79;
        accY += mpu->getAccelerationY() + 14.74;
        accZ += mpu->getAccelerationZ() + 264.6;
    }*/
}

void Chassis::InitDefaultCommand() {
    SetDefaultCommand(new MeccanumDrive());
}
    
void Chassis::liveWindow() {
	// Add all off the components to the Live Window for testing
	LiveWindow* lw = LiveWindow::GetInstance();
	lw->AddActuator("Chassis", "Motor A", driveMotorA);
	lw->AddActuator("Chassis", "Motor B", driveMotorB);
	lw->AddActuator("Chassis", "Motor C", driveMotorC);
	lw->AddActuator("Chassis", "Motor D", driveMotorD);
	lw->AddSensor("Chassis", "Gyro(deg)", gyro);
	lw->AddSensor("Chassis", "EncoderA",  encoderA);
	lw->AddSensor("Chassis", "EncoderB",  encoderB);
	//lw->AddSensor("Chassis", "EncoderC",  encoderC);
	//lw->AddSensor("Chassis", "EncoderD",  encoderD);
}
