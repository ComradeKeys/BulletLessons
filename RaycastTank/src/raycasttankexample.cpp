// Copyright (C) 2014- Danyal Zia
// Copyright (C) 2009-2013 Josiah Hartzell (Skyreign Software)
// This file is part of the "irrBullet" Bullet physics wrapper.
// For conditions of distribution and use, see copyright notice in irrBullet.h
// The above copyright notice and its accompanying information must remain here.

#include <irrlicht.h>
#include <irrBullet.h>
#include "raycasttankexample.h"


using namespace irr;
using namespace core;
using namespace video;
using namespace scene;
using namespace gui;
using namespace io;


/*
NOTE: This example is unfinished, and the tank mechanics are not that great right now.
      Please check it in future releases for a better, more up-to-date version.
This example is moderate skill level, and shows you how to use the rayast vehicle for
vehicles that are different from standard cars/trucks, and more advanced.

In the example you can drive and fire the German Jagdpanther tank destroyer from WWII.
The Jagdpanther model was borrowed from the open source RTS game, Spring 1944.
*/

vector3df spawnPoint(30,5,200);


CRaycastTankExample::CRaycastTankExample()
{
}


bool CRaycastTankExample::OnEvent(const SEvent& event)
{
	if (!device)
		return false;

    switch(event.EventType)
    {
        case EET_MOUSE_INPUT_EVENT:
        {
            if(event.MouseInput.Event==EMIE_RMOUSE_PRESSED_DOWN)
            {
                shootSphere(vector3df(0.2,0.2,0.2), 0.2);
            }
        }
        break;

        case EET_KEY_INPUT_EVENT:
        {
            KeyIsDown[event.KeyInput.Key] = event.KeyInput.PressedDown;
            if(event.KeyInput.Key == KEY_KEY_P && event.KeyInput.PressedDown == false)
            {
                world->pauseSimulation(!world->simulationPaused());
            }

            else
            if(event.KeyInput.Key == KEY_KEY_R && event.KeyInput.PressedDown == false)
            {
                // Re-Spawn our tank at the original location.
                irr::core::matrix4 mat;
                mat.setTranslation(spawnPoint);
                tank->setWorldTransform(mat);
                tank->setAngularVelocity(vector3df(0,0,0));
                tank->setLinearVelocity(vector3df(0,0,0));
            }

            if(event.KeyInput.Key == KEY_SPACE && event.KeyInput.PressedDown == false)
            {
				auto node = static_cast<IAnimatedMeshSceneNode*>(tank->getCollisionShape()->getSceneNode());
                vehicle->getRigidBody()->applyImpulse(vector3df(0,0,-500), node->getJointNode("Muzzle")->getPosition(), ERBTransformSpace::ERBTS_LOCAL);
                createMuzzleFlash(node->getJointNode("Muzzle"));
            }

        }
        break;
        default:
            break;
    }
    return false;


}


void CRaycastTankExample::runExample()
{
    for(s32 i=0; i<KEY_KEY_CODES_COUNT; i++)
        KeyIsDown[i] = false;

    debugDraw = true;
    drawProperties = false;
    drawWireFrame = false;

    leftTrackSpeed = 0.0f;
    rightTrackSpeed = 0.0f;

    device.reset(createDevice( video::EDT_OPENGL, dimension2d<u32>(640, 480), 16,
            false, false, false, this));


    device->setWindowCaption(L"DMUX");

    device->getFileSystem()->addFileArchive("media/");


	auto tankProperties = device->getGUIEnvironment()->addStaticText(L"Tank:",
            rect<s32>(10,10,120,240), false);

    tankProperties->setOverrideColor(SColor(255,255,255,255));


    device->getSceneManager()->addLightSceneNode(0, vector3df(20, 40, -50), SColorf(1.0f, 1.0f, 1.0f), 4000.0f);

    // Create irrBullet World
    world.reset(createIrrBulletWorld(device, true, debugDraw));

	world->setDebugMode(irrPhysicsDebugMode::EPDM_DrawAabb |
		irrPhysicsDebugMode::EPDM_DrawContactPoints);

    world->setGravity(vector3df(0,-10,0));


    createTerrain();


    createTank(L"JagdPanther.b3d", L"JagdPanther.b3d", spawnPoint, 200); // 12
    //    createTank(L"chassis/el-camino-monstertruck.obj", L"el-camino-monstertruck.obj", spawnPoint, 200); // 12

    camera = device->getSceneManager()->addCameraSceneNodeMaya();
	camera->setPosition(vector3df(50,15,200));
	camera->setTarget(vehicle->getRigidBody()->getWorldTransform().getTranslation());


    // Set our delta time and time stamp
    u32 TimeStamp = device->getTimer()->getTime();
    u32 DeltaTime = 0;
    while(device->run())
    {
        device->getVideoDriver()->beginScene(true, true, SColor(255,100,101,140));

        DeltaTime = device->getTimer()->getTime() - TimeStamp;
		TimeStamp = device->getTimer()->getTime();

		// Step the simulation with our delta time
        world->stepSimulation(DeltaTime*0.001f, 120);

        camera->setTarget(vehicle->getRigidBody()->getMotionState()->getWorldTransformationMatrix().getTranslation());

        updateTank();


        stringw string = "Tank:\n";
        string += "LTrack: ";
        string += leftTrackSpeed;
        string += "\n";
        string += "RTrack: ";
        string += rightTrackSpeed;
        string += "\n";
        string += f32(vehicle->getCurrentSpeedKmHour());
        string += "km/h\n";


        tankProperties->setText(string.c_str());


        world->debugDrawWorld(debugDraw);

        // This call will draw the technical properties of the physics simulation
        // to the GUI environment.
        world->debugDrawProperties(drawProperties);


        device->getSceneManager()->drawAll();
        device->getGUIEnvironment()->drawAll();
        device->getVideoDriver()->endScene();
    }
}

void CRaycastTankExample::updateTank()
{
    leftTrackSpeed = 0.0f;
    rightTrackSpeed = 0.0f;

    if(IsKeyDown(KEY_KEY_W)) {
            leftTrackSpeed = 1600.0f;
            rightTrackSpeed = -1600.0f;
    }

    if(IsKeyDown(KEY_KEY_S)) {
            leftTrackSpeed = -1600.0f;
            rightTrackSpeed = 1600.0f;
    }

    //turn left

    if(IsKeyDown(KEY_KEY_A)) {
        leftTrackSpeed = -4600.0f;
        rightTrackSpeed = -4600.0f;
    }

    else
	//turn right
    if(IsKeyDown(KEY_KEY_D)) {
        leftTrackSpeed = 4600.0f;
        rightTrackSpeed = 4600.0f;
    }

    for(u32 i=0; i < vehicle->getNumWheels(); i++)
    {
        bool isLeftWheel = vehicle->getWheelInfo(i).chassisConnectionPointCS.X < 0;

        if(vehicle->getWheelInfo(i+(isLeftWheel ? 4:-4)).raycastInfo.isInContact)
            vehicle->applyEngineForce(isLeftWheel ? leftTrackSpeed:rightTrackSpeed,i);
    }
}


void CRaycastTankExample::createTerrain()
{
    // TERRAIN
	auto Node = device->getSceneManager()->addMeshSceneNode(device->getSceneManager()->getMesh("olivermath/olivermath-1.obj")->getMesh(0));
	Node->setPosition(vector3df(0,0,0));
	Node->setMaterialFlag(video::EMF_LIGHTING, false);

    // For the terrain, instead of adding a cube or sphere shape, we are going to
    // add a BvhTriangleMeshShape. This is the standard trimesh shape
    // for static objects. The first parameter is of course the node to control,
    // the second parameter is the collision mesh, incase you want a low-poly collision mesh,
    // and the third parameter is the mass.
	ICollisionShape *shape = new IBvhTriangleMeshShape(Node, device->getSceneManager()->getMesh("olivermath/olivermath-1.obj"), 0.0);

	shape->setMargin(0.07);

    // The rigid body will be placed at the origin of the node that the collision shape is controlling,
    // so we do not need to set the position after positioning the node.
	auto terrain = world->addRigidBody(shape);


    shape->setLocalScaling(vector3df(4,4,4), EScalingPair::ESP_BOTH);

	// When setting a rigid body to a static object, please be sure that you have
	// that object's mass set to 0.0. Otherwise, undesired results will occur.
	terrain->setCollisionFlags(ECollisionFlag::ECF_STATIC_OBJECT);
}


void CRaycastTankExample::createTank(const stringw file, const stringw collFile, const vector3df &pos, const f32 mass)
{
	auto Node = device->getSceneManager()->addAnimatedMeshSceneNode(
        device->getSceneManager()->getMesh(file.c_str()));
	Node->setPosition(pos);
	Node->setMaterialFlag(video::EMF_LIGHTING, true);

	IGImpactMeshShape *shape = new IGImpactMeshShape(Node, device->getSceneManager()->getMesh(collFile.c_str()), mass);


	tank = world->addRigidBody(shape);

    // When using a raycast vehicle, we don't want this rigid body to deactivate.
	tank->setActivationState(EActivationState::EAS_DISABLE_DEACTIVATION);

    // Set some damping on the rigid body because the raycast vehicles tend to bounce a lot without a lot of tweaking.
    // (cheap fix for the example only)
	tank->setDamping(0.4, 0.4);

    // We create our vehicle, passing our newly created rigid body as a parameter.
	vehicle = world->addRaycastVehicle(tank);


    // Set up our wheel construction info. These values can be changed for each wheel,
    // and the values that you want to keep will stay the same, that way
    // all parameters for each wheel can stay the same for what needs to remain equal,
    // such as directions and suspension rest length.
    SWheelInfoConstructionInfo wheel;
    wheel.chassisConnectionPointCS = vector3df(0.0,-0.88,4.0);
    wheel.wheelDirectionCS = vector3df(0.0,-0.1,0.0);
    wheel.wheelAxleCS = vector3df(-0.5,0.0,0.0);
    wheel.suspensionRestLength = 1.6;
    wheel.wheelRadius = 8.0;
    wheel.isFrontWheel = true;

    // The bones are in the center of the mesh on the X axis, so we just set the width ourselves.
    // Do the left row of wheels.
    for(u32 i=0; i < Node->getJointCount(); i++)
    {
        // The bones that we need in this mesh are all named "RoadWheels" with a numerical suffix.
        // So we do a quick check to make sure that no unwanted bones get through as wheels.
        if(Node->getJointNode(i)->getName()[0] == 'R') {


            wheel.chassisConnectionPointCS = vector3df(-4, Node->getJointNode(i)->getPosition().Y,Node->getJointNode(i)->getPosition().Z);
            vehicle->addWheel(wheel);

        }
    }

    wheel.wheelAxleCS = vector3df(0.5,0.0,0.0);

    // Do the right row of wheels.
    for(u32 i=0; i < Node->getJointCount(); i++)
    {
        if(Node->getJointNode(i)->getName()[0] == 'R')
        {
            wheel.chassisConnectionPointCS = vector3df(4, Node->getJointNode(i)->getPosition().Y,Node->getJointNode(i)->getPosition().Z);
            vehicle->addWheel(wheel);
        }
    }


	for (u32 i=0;i<vehicle->getNumWheels();i++)
    {
        SWheelInfo &info = vehicle->getWheelInfo(i);

        info.suspensionStiffness = 0.08f;
        info.wheelDampingRelaxation = 20.0f;
        info.wheelDampingCompression = 20.0f;
        info.frictionSlip = 1000;
        info.rollInfluence = 0.1f;


        // We call updateWheel, which takes SWheelInfo as the first parameter,
        // and the ID of the wheel to apply that info to. This must
        // be called after any changes in order for the changes to actually take effect.
        vehicle->updateWheelInfo(i);
    }
}


void CRaycastTankExample::createMuzzleFlash(irr::scene::IBoneSceneNode *node)
{
    IBillboardSceneNode *muzzleFlash = device->getSceneManager()->addBillboardSceneNode(0,
        dimension2d<f32>(30.0f, 30.0f), node->getAbsoluteTransformation().getTranslation());

    muzzleFlash->setMaterialFlag(EMF_LIGHTING, false);
    muzzleFlash->setMaterialTexture(0,device->getVideoDriver()->getTexture("particlewhite.bmp"));
    muzzleFlash->setMaterialType(EMT_TRANSPARENT_ADD_COLOR);


    ISceneNodeAnimator* deleteAnim = device->getSceneManager()->createDeleteAnimator(100);
    muzzleFlash->addAnimator(deleteAnim);
    deleteAnim->drop();
}


CRaycastTankExample::~CRaycastTankExample()
{
}
