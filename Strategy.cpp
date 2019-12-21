// Strategy.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "Strategy.h"
#include "winDebugger/Client.h"

#include <math.h>
#include <string>

using namespace std;

BOOL APIENTRY DllMain(HANDLE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


const double PI = 3.1415923;
char myMessage[200]; //big enough???

void PredictBall(Environment* env);
void NearBound2(Robot* robot, double vl, double vr, Environment* env);
void Attack2(Robot* robot, Environment* env);

// by moon at 9/2/2002
void MoonAttack(Robot* robot, Environment* env);
// just for testing to check whether the &env->opponent works or not
void MoonFollowOpponent(Robot* robot, OpponentRobot* opponent);
void Velocity(Robot* robot, int vl, int vr);
void Angle(Robot* robot, int desired_angle);

enum SIDE {
	BLUE = 1,
	YELLOW = -1,
	NONE = 0
};

Vector3D CENTER;
SIDE myTeam = NONE;

// strategy related
void ChoseStrategy(Environment* env);
void Obrona(Environment* env);
void Atak(Environment* env);
void Bramkarz(Environment* pEnv);
void Wybijacz(Environment* env);

// helpers
void Init();
void SetTeam(Environment* env);
SIDE WhichSide(Vector3D* position);
double Length(Vector3D* p1, Vector3D* p2);

void Position(Environment* pEnv, int id, double x, double y);
void Position(Robot* robot, double x, double y);
void MyDefend(Environment* env, int id, double low, double high);


extern "C" STRATEGY_API void Create(Environment* env)
{
	// allocate user data and assign to env->userData
	// eg. env->userData = ( void * ) new MyVariables ();

	Init();
}

extern "C" STRATEGY_API void Destroy(Environment* env)
{
	// free any user data created in Create ( Environment * )

	// eg. if ( env->userData != NULL ) delete ( MyVariables * ) env->userData;
}

extern "C" STRATEGY_API void Strategy(Environment* env)
{
	// not supported env->gameState

	// check is yello or blue
	SetTeam(env);

	// chose atack or defense
	ChoseStrategy(env);
}

void Init()
{
	CENTER.x = (GLEFT + GRIGHT) / 2.0;
	CENTER.y = (FBOT + FTOP) / 2.0;
}

void SetTeam(Environment* env)
{
	if (myTeam == NONE) {
		myTeam = WhichSide(&env->home[0].pos);
	}
}

SIDE WhichSide(Vector3D* position)
{
	return (position->x < CENTER.x) ? YELLOW : BLUE;
}

void ChoseStrategy(Environment* env)
{
	if (WhichSide(&env->currentBall.pos) == myTeam) {
		Obrona(env);
	}

	else {
		Atak(env);
	}
}

void Obrona(Environment* env)
{
	Bramkarz(env);
	Wybijacz(env);
}

void Atak(Environment* env)
{
	Bramkarz(env);
}

void Bramkarz(Environment* env)
{
	MyDefend(env, 0, 36.7, 47.5);
}

void Wybijacz(Environment* env)
{
	double tmpLenght = 0;
	int wybijaczId = -1;
	Vector3D* bPos = &env->currentBall.pos;

	for (int i = 1; i < 5; ++i) // without bramkarz
	{
		Vector3D* rPos = &env->home[i].pos;

		// before the ball
		if ((rPos->x - bPos->x) * myTeam > 0) {
			double l = Length(rPos, bPos);

			if (wybijaczId < 0 || l < tmpLenght)
			{
				wybijaczId = i;
				tmpLenght = l;
			}
		}		
	}

	// wybijacz before the ball found
	if (wybijaczId > 0) {
		;
	}
}

/// Lenght between two points
double Length(Vector3D* p1, Vector3D* p2)
{
	return sqrt(pow(fabs(p1->x - p2->x), 2) + pow(fabs(p1->y - p2->y), 2));
}

void Position(Environment* pEnv, int id, double x, double y)
{
	Position(&(pEnv->home[id]), x, y);
}

void Position(Robot* robot, double x, double y)
{
	int desired_angle = 0, theta_e = 0, d_angle = 0, vl, vr, vc = 70;

	double dx, dy, d_e, Ka = 10.0 / 90.0;
	dx = x - robot->pos.x;
	dy = y - robot->pos.y;

	d_e = sqrt(dx * dx + dy * dy);
	if (dx == 0 && dy == 0)
		desired_angle = 90;
	else
		desired_angle = (int)(180. / PI * atan2((double)(dy), (double)(dx)));
	theta_e = desired_angle - (int)robot->rotation;

	while (theta_e > 180) theta_e -= 360;
	while (theta_e < -180) theta_e += 360;

	if (d_e > 100.)
		Ka = 17. / 90.;
	else if (d_e > 50)
		Ka = 19. / 90.;
	else if (d_e > 30)
		Ka = 21. / 90.;
	else if (d_e > 20)
		Ka = 23. / 90.;
	else
		Ka = 25. / 90.;

	if (theta_e > 95 || theta_e < -95)
	{
		theta_e += 180;

		if (theta_e > 180)
			theta_e -= 360;
		if (theta_e > 80)
			theta_e = 80;
		if (theta_e < -80)
			theta_e = -80;
		if (d_e < 5.0 && abs(theta_e) < 40)
			Ka = 0.1;
		vr = (int)(-vc * (1.0 / (1.0 + exp(-3.0 * d_e)) - 0.3) + Ka * theta_e);
		vl = (int)(-vc * (1.0 / (1.0 + exp(-3.0 * d_e)) - 0.3) - Ka * theta_e);
	}

	else if (theta_e < 85 && theta_e > -85)
	{
		if (d_e < 5.0 && abs(theta_e) < 40)
			Ka = 0.1;
		vr = (int)(vc * (1.0 / (1.0 + exp(-3.0 * d_e)) - 0.3) + Ka * theta_e);
		vl = (int)(vc * (1.0 / (1.0 + exp(-3.0 * d_e)) - 0.3) - Ka * theta_e);
	}

	else
	{
		vr = (int)(+.17 * theta_e);
		vl = (int)(-.17 * theta_e);
	}

	Velocity(robot, vl, vr);
}

// Align robot at 90 or 270 degree
// oscilate on y axis following the ball
// in low high range
void MyDefend(Environment* env, int id, double low, double high)
{
	// params for tune algorith
	double initialAlignDelta = 2.5;
	double oscylation = 0.5;

	Robot* robot = &env->home[id];
	Vector3D* rPos = &robot->pos;
	Vector3D* bPos = &env->currentBall.pos;

	double rRot = robot->rotation + 180.0; // rotation range [-180,180] +180.0 to work with positiv numbers
	double angle = rRot < 180.0 ? 90.0 : 270.0; // closer for robot

	// Robot is initialy align - oscylate base on ball
	if (fabs(rRot - angle) <= initialAlignDelta)
	{
		double direction = angle == 90.0 ? -1 : 1; // up or down
		double vr = 0;
		double vl = 0;

		if (rPos->y > bPos->y + oscylation && rPos->y > low)
		{
			vr = -100;
			vl = -100;
		}

		if (rPos->y < bPos->y - oscylation && rPos->y < high)
		{
			vr = 100;
			vl = 100;
		}

		if (rPos->y > high)
		{
			vr = -100;
			vl = -100;
		}

		if (rPos->y < low)
		{
			vr = 100;
			vl = 100;
		}

		robot->velocityRight = direction * vr;
		robot->velocityLeft = direction * vl;
	}

	// Align robot
	else
	{
		double turnDirection = rRot < angle ? 1 : -1; // 1 turn left, -1 turn right

		robot->velocityRight = turnDirection * fabs(angle - rRot);
		robot->velocityLeft = -(turnDirection * fabs(angle - rRot));
	}

	char debug[250];
	sprintf(debug, "rot: %lf, x: %lf, y: %lf", rRot, rPos->x, rPos->y);
	Client::debugClient->SendMessages(debug);
}

//
// --
//

void MoonAttack(Robot* robot, Environment* env)
{
	//Velocity (robot, 127, 127);
	//Angle (robot, 45);
	PredictBall(env);
	Position(robot, env->predictedBall.pos.x, env->predictedBall.pos.y);
	// Position(robot, 0.0, 0.0);
}

void MoonFollowOpponent(Robot* robot, OpponentRobot* opponent)
{
	Position(robot, opponent->pos.x, opponent->pos.y);
}

void Velocity(Robot* robot, int vl, int vr)
{
	robot->velocityLeft = vl;
	robot->velocityRight = vr;
}

void PredictBall(Environment* env)
{
	double dx = env->currentBall.pos.x - env->lastBall.pos.x;
	double dy = env->currentBall.pos.y - env->lastBall.pos.y;
	env->predictedBall.pos.x = env->currentBall.pos.x + dx;
	env->predictedBall.pos.y = env->currentBall.pos.y + dy;

}

void Attack2(Robot* robot, Environment* env)
{
	Vector3D t = env->currentBall.pos;
	double r = robot->rotation;
	if (r < 0) r += 360;
	if (r > 360) r -= 360;
	double vl = 0, vr = 0;

	if (t.y > env->fieldBounds.top - 2.5) t.y = env->fieldBounds.top - 2.5;
	if (t.y < env->fieldBounds.bottom + 2.5) t.y = env->fieldBounds.bottom + 2.5;
	if (t.x > env->fieldBounds.right - 3) t.x = env->fieldBounds.right - 3;
	if (t.x < env->fieldBounds.left + 3) t.x = env->fieldBounds.left + 3;

	double dx = robot->pos.x - t.x;
	double dy = robot->pos.y - t.y;

	double dxAdjusted = dx;
	double angleToPoint = 0;

	if (fabs(robot->pos.y - t.y) > 7 || t.x > robot->pos.x)
		dxAdjusted -= 5;

	if (dxAdjusted == 0)
	{
		if (dy > 0)
			angleToPoint = 270;
		else
			angleToPoint = 90;
	}
	else if (dy == 0)
	{
		if (dxAdjusted > 0)
			angleToPoint = 360;
		else
			angleToPoint = 180;

	}
	else
		angleToPoint = atan(fabs(dy / dx)) * 180.0 / PI;

	if (dxAdjusted > 0)
	{
		if (dy > 0)
			angleToPoint -= 180;
		else if (dy < 0)
			angleToPoint = 180 - angleToPoint;
	}
	if (dxAdjusted < 0)
	{
		if (dy > 0)
			angleToPoint = -angleToPoint;
		else if (dy < 0)
			angleToPoint = 90 - angleToPoint;
	}

	if (angleToPoint < 0) angleToPoint = angleToPoint + 360;
	if (angleToPoint > 360) angleToPoint = angleToPoint - 360;
	if (angleToPoint > 360) angleToPoint = angleToPoint - 360;

	double c = r;

	double angleDiff = fabs(r - angleToPoint);

	if (angleDiff < 40)
	{
		vl = 100;
		vr = 100;
		if (c > angleToPoint)
			vl -= 10;
		if (c < angleToPoint)
			vr -= 10;
	}
	else
	{
		if (r > angleToPoint)
		{
			if (angleDiff > 180)
				vl += 360 - angleDiff;
			else
				vr += angleDiff;
		}
		if (r < angleToPoint)
		{
			if (angleDiff > 180)
				vr += 360 - angleDiff;
			else
				vl += angleDiff;
		}
	}

	NearBound2(robot, vl, vr, env);
}

void NearBound2(Robot* robot, double vl, double vr, Environment* env)
{
	//Vector3D t = env->currentBall.pos;

	Vector3D a = robot->pos;
	double r = robot->rotation;

	if (a.y > FTOP - 15 && r > 45 && r < 130)
	{
		if (vl > 0)
			vl /= 3;
		if (vr > 0)
			vr /= 3;
	}

	if (a.y < FBOT + 15 && r < -45 && r > -130)
	{
		if (vl > 0) vl /= 3;
		if (vr > 0) vr /= 3;
	}

	if (a.x > FRIGHTX - 10)
	{
		if (vl > 0)
			vl /= 2;
		if (vr > 0)
			vr /= 2;
	}

	if (a.x < FLEFTX + 10)
	{
		if (vl > 0)
			vl /= 2;
		if (vr > 0)
			vr /= 2;
	}

	robot->velocityLeft = -vl;
	robot->velocityRight = -vr;
}