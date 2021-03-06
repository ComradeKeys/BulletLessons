/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletDynamics/MLCPSolvers/btDantzigSolver.h>
#include <BulletDynamics/MLCPSolvers/btSolveProjectedGaussSeidel.h>
#include <BulletDynamics/MLCPSolvers/btMLCPSolver.h>
#include <BulletDynamics/ConstraintSolver/btHingeConstraint.h>
#include <BulletDynamics/ConstraintSolver/btSliderConstraint.h>
#include <LinearMath/btAlignedObjectArray.h>
#include <btBulletCollisionCommon.h>
#include <stdio.h>
#include "CarHandlingDemo.hpp"
#include "Globals.hpp"

CarHandlingDemo::CarHandlingDemo()
{
}

void CarHandlingDemo::initPhysics()
{

	world->setGravity(btVector3(0, -10, 0));

	//keep track of the shapes, we release memory at exit.
	//make sure to re-use collision shapes among rigid bodies whenever possible!
	{
		//Creates the ground shape
		btCollisionShape* groundShape = new btBoxShape(btVector3(100, 1, 100));

		//Stores on an array for reusing
		collisionShapes.push_back(groundShape);

		//Creates the ground rigidbody
		btRigidBody* groundRigidBody = this->createGroundRigidBodyFromShape(groundShape);

		//Adds it to the world
		world->addRigidBody(groundRigidBody);
	}

	{
		//the dimensions for the boxShape are half extents, so 0,5 equals to 
		btVector3 halfExtends(1, btScalar(0.5), 2);

		//The btBoxShape is centered at the origin
		btCollisionShape* chassisShape = new btBoxShape(halfExtends);

		//Stores on an array for reusing
		collisionShapes.push_back(chassisShape);

		//A compound shape is used so we can easily shift the center of gravity of our vehicle to its bottom
		//This is needed to make our vehicle more stable
		btCompoundShape* compound = new btCompoundShape();

		collisionShapes.push_back(compound);

		btTransform localTransform;
		localTransform.setIdentity();
		localTransform.setOrigin(btVector3(0, 1, 0));

		//The center of gravity of the compound shape is the origin. When we add a rigidbody to the compound shape
		//it's center of gravity does not change. This way we can add the chassis rigidbody one unit above our center of gravity
		//keeping it under our chassis, and not in the middle of it
		compound->addChildShape(localTransform, chassisShape);

		//Creates a rigid body
		btRigidBody* chassisRigidBody = this->createChassisRigidBodyFromShape(compound);

		//Adds the vehicle chassis to the world
		world->addRigidBody(chassisRigidBody);

		btVehicleRaycaster* vehicleRayCaster = new btDefaultVehicleRaycaster(world);

		btRaycastVehicle::btVehicleTuning tuning;

		//Creates a new instance of the raycast vehicle
		this->vehicle = new btRaycastVehicle(tuning, chassisRigidBody, vehicleRayCaster);

		//Never deactivate the vehicle
		chassisRigidBody->setActivationState(DISABLE_DEACTIVATION);

		//Adds the vehicle to the world
		world->addVehicle(this->vehicle);

		//Adds the wheels to the vehicle
		this->addWheels(&halfExtends, this->vehicle, tuning);
	}

}

btRigidBody* CarHandlingDemo::createChassisRigidBodyFromShape(btCollisionShape* chassisShape)
{
    //Visualisation for the rigid body
    irr::scene::ISceneNode *node = smgr->addCubeSceneNode(1.0f);
    node->setMaterialFlag(irr::video::EMF_LIGHTING, 1);
    node->setMaterialTexture(0, driver->getTexture("assets/cube.png"));


    btTransform chassisTransform;
    chassisTransform.setIdentity();
    chassisTransform.setOrigin(btVector3(0, 1, 0));

    {
	//chassis mass 
	btScalar mass(1200);

	//since it is dynamic, we calculate its local inertia
	btVector3 localInertia(0, 0, 0);
	chassisShape->calculateLocalInertia(mass, localInertia);

	//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
	btDefaultMotionState* groundMotionState = new btDefaultMotionState(chassisTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, groundMotionState, chassisShape, localInertia);

    // Store a pointer to the irrlicht node so we can update it later
    btRigidBody *rigidBody =  new btRigidBody(rbInfo);

    rigidBody->setUserPointer((void *)(node));
    worldObjs.push_back(rigidBody);


	return rigidBody;
    }
}

btRigidBody* CarHandlingDemo::createGroundRigidBodyFromShape(btCollisionShape* groundShape)
{
	btTransform groundTransform;
	groundTransform.setIdentity();
	groundTransform.setOrigin(btVector3(0, -1, 0));

	{
		//The ground is not dynamic, we set its mass to 0
		btScalar mass(0);

		//No need for calculating the inertia on a static object
		btVector3 localInertia(0, 0, 0);

		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* groundMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, groundMotionState, groundShape, localInertia);

		return new btRigidBody(rbInfo);
	}
}

void CarHandlingDemo::addWheels(
	btVector3* halfExtents,
	btRaycastVehicle* vehicle,
	btRaycastVehicle::btVehicleTuning tuning)
{
	//The direction of the raycast, the btRaycastVehicle uses raycasts instead of simiulating the wheels with rigid bodies
	btVector3 wheelDirectionCS0(0, -1, 0);

	//The axis which the wheel rotates arround
	btVector3 wheelAxleCS(-1, 0, 0);

	btScalar suspensionRestLength(0.7);

	btScalar wheelWidth(0.4);

	btScalar wheelRadius(0.5);

	//The height where the wheels are connected to the chassis
	btScalar connectionHeight(1.2);

	//All the wheel configuration assumes the vehicle is centered at the origin and a right handed coordinate system is used
	btVector3 wheelConnectionPoint(halfExtents->x() - wheelRadius, connectionHeight, halfExtents->z() - wheelWidth);

	//Adds the front wheels
	vehicle->addWheel(wheelConnectionPoint, wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, true);

	vehicle->addWheel(wheelConnectionPoint * btVector3(-1, 1, 1), wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, true);

	//Adds the rear wheels
	vehicle->addWheel(wheelConnectionPoint* btVector3(1, 1, -1), wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, false);

	vehicle->addWheel(wheelConnectionPoint * btVector3(-1, 1, -1), wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, false);

	//Configures each wheel of our vehicle, setting its friction, damping compression, etc.
	//For more details on what each parameter does, refer to the docs
	for (int i = 0; i < vehicle->getNumWheels(); i++)
	{
		btWheelInfo& wheel = vehicle->getWheelInfo(i);
		wheel.m_suspensionStiffness = 50;
		wheel.m_wheelsDampingCompression = btScalar(0.3) * 2 * btSqrt(wheel.m_suspensionStiffness);//btScalar(0.8);
		wheel.m_wheelsDampingRelaxation = btScalar(0.5) * 2 * btSqrt(wheel.m_suspensionStiffness);//1;
		//Larger friction slips will result in better handling
		wheel.m_frictionSlip = btScalar(1.2);
		wheel.m_rollInfluence = 1;
	}
}

void CarHandlingDemo::exitPhysics()
{
	for (int i = world->getNumCollisionObjects() - 1; i >= 0; i--)
	{
		btCollisionObject* obj = world->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body && body->getMotionState())
		{
			while (body->getNumConstraintRefs())
			{
				btTypedConstraint* constraint = body->getConstraintRef(0);
				world->removeConstraint(constraint);
				delete constraint;
			}
			delete body->getMotionState();
			world->removeRigidBody(body);
		}
		else
		{
			world->removeCollisionObject(obj);
		}
		delete obj;
	}

	//delete collision shapes
	for (int j = 0; j< collisionShapes.size(); j++)
	{
		btCollisionShape* shape = collisionShapes[j];
		delete shape;
	}
	collisionShapes.clear();

	btConstraintSolver* constraintSolver = world->getConstraintSolver();
	btBroadphaseInterface* broadphaseInterface = world->getBroadphase();
	btDispatcher* collisionDispatcher = world->getDispatcher();

	delete world;
	delete this->vehicle;

	delete constraintSolver;
	delete broadphaseInterface;
	delete collisionDispatcher;
}

CarHandlingDemo::~CarHandlingDemo()
{
}

void CarHandlingDemo::physicsDebugDraw(int debugFlags)
{
	if (world && world->getDebugDrawer())
	{
		world->getDebugDrawer()->setDebugMode(debugFlags);
		world->debugDrawWorld();
	}
}

void CarHandlingDemo::stepSimulation(float deltaTime)
{
	world->stepSimulation(deltaTime, 2);
}

