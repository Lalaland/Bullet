#include "client.h"

#include <OGRE/Ogre.h>
#include <OIS/OIS.h>
#include "../utils/BulletXML.h"
#include "../utils/DotSceneLoader.h"

#include <iostream>

using namespace std;

bool Client::go()
{
   #ifdef _DEBUG
   mResourcesCfg = "resources_d.cfg";
   mPluginsCfg = "plugins_d.cfg";
   #else
   mResourcesCfg = "resources.cfg";
   mPluginsCfg = "plugins.cfg";
   #endif

   //Create Root
   mRoot = new Ogre::Root(mPluginsCfg);
  
   //Parse resources.cfg
   Ogre::ConfigFile cf;
   cf.load(mResourcesCfg);
   Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();
   Ogre::String secName, typeName, archName;
   while (seci.hasMoreElements())
   {
      secName = seci.peekNextKey();
      Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
      Ogre::ConfigFile::SettingsMultiMap::iterator i;
      for (i = settings->begin(); i != settings->end(); ++i)    
      {
	 typeName = i->first;
	 archName = i->second;
	 Ogre::ResourceGroupManager::getSingleton().addResourceLocation( archName, typeName, secName);
      }
   }
   

   //Create ogre.cfg
   if(!(mRoot->restoreConfig() || mRoot->showConfigDialog()))
   {
      return false;
   }

   //Create window
   mWindow = mRoot->initialise(true, "BasicTutorial6 Render Window");
   
   //Load textures
   Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);
   Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
    
   //Create Scene Manager
   mSceneMgr = mRoot->createSceneManager("DefaultSceneManager");
   mSceneMgr->setSkyDome(true,"SkyDome");
  
   //Create Camera
   mCamera = mSceneMgr->createCamera("PlayerCam");
   mCamera->setNearClipDistance(5);

   //Create Viewport
   Ogre::Viewport* vp = mWindow->addViewport(mCamera);
   vp->setBackgroundColour(Ogre::ColourValue(0,0,0));
   mCamera->setAspectRatio(Ogre::Real(vp->getActualWidth()) / Ogre::Real(vp->getActualHeight()));

   //Create Scene looks
   Ogre::Entity* ogreHead = mSceneMgr->createEntity("", "Cone.mesh");
   
   mConeNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
   mConeNode->attachObject(ogreHead);
   mConeNode->setScale(5,5,5);
   
   
   mPlayerNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
   mPlayerNode->setPosition(0,20,0);
   
   mCameraYawNode = mPlayerNode->createChildSceneNode();
   mCameraPitchNode = mCameraYawNode->createChildSceneNode();
   mCameraPitchNode->attachObject(mCamera);

   // Create a light
   Ogre::Light* l = mSceneMgr->createLight("MainLight");
   l->setPosition(20,80,50);

   //Create OIS 
   Ogre::LogManager::getSingletonPtr()->logMessage("*** Initializing OIS ***");
   OIS::ParamList pl;
   size_t windowHnd = 0;
   std::ostringstream windowHndStr;
   mWindow->getCustomAttribute("WINDOW", &windowHnd);
   windowHndStr << windowHnd;
   pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));
   pl.insert(std::make_pair(std::string("x11_mouse_grab"), "false"));
   //pl.insert(std::make_pair(std::string("x11_mouse_hide"), "false"));
   pl.insert(std::make_pair(std::string("x11_keyboard_grab"), "false"));
     
   mInputManager = OIS::InputManager::createInputSystem( pl );
   
   mKeyboard = static_cast<OIS::Keyboard*>(mInputManager->createInputObject( OIS::OISKeyboard, true ));
   mMouse = static_cast<OIS::Mouse*>(mInputManager->createInputObject( OIS::OISMouse, false ));
   windowResized(mWindow);

   mKeyboard->setEventCallback(this);

   //Add this class as a listener
   Ogre::WindowEventUtilities::addWindowEventListener(mWindow, this);
   mRoot->addFrameListener(this);

   //Show Overlay
   mOverlayManager = Ogre::OverlayManager::getSingletonPtr();

   mFirstOverlay = mOverlayManager->getByName("Test");
   mFirstOverlay->show();

   mDebugOverlay = mOverlayManager->getByName("Core/DebugOverlay");
   mDebugOverlay->show();
   
   //Load scene
   //DotSceneLoader loader;
   //loader.parseDotScene("lol.scene","General", mSceneMgr,&mMeshes);
   
   //Activate physics
   loadPhx(); 
   //BulletXML test("lol.xml",mWorld,mSceneMgr,&mCopyData,&mMeshes,&mStore);
   //test.parse();

   start("127.0.0.1");

   parseObjectData();
   startGame();

   //Need to see stuff
   mSceneMgr->setAmbientLight(Ogre::ColourValue(0.5, 0.5, 0.5));
   
   mPlayerNode->setPosition(0,20,0);
   mRoot->startRendering();
   
   return true;
}

Client::Client() : mRoot(0), mPluginsCfg(Ogre::StringUtil::BLANK) 
{
}

Client::~Client()
{
   Ogre::WindowEventUtilities::removeWindowEventListener(mWindow, this);
   windowClosed(mWindow);
   delete mRoot;
}

bool Client::movePlayer(Ogre::Real time)
{
   Ogre::Vector3 moveVec(0,0,0);

   mKeyboard->capture();
   mMouse->capture();

   OIS::MouseState state = mMouse->getMouseState();
   
   Ogre::Quaternion test(Ogre::Degree(state.X.rel * time * -20),Ogre::Vector3::UNIT_Y);
   Ogre::Quaternion test2(Ogre::Degree(state.Y.rel * time * -20),Ogre::Vector3::UNIT_X);

   moveQuat = moveQuat * test;
   moveQuat.normalise();

   cameraQuat = cameraQuat * test2;
   cameraQuat.normalise();
   
   mCameraYawNode->rotate(test);
   mCameraPitchNode->rotate(test2);
   

   if (mKeyboard->isKeyDown(OIS::KC_ESCAPE))
      return false;

   if (mKeyboard->isKeyDown(OIS::KC_A))
      moveVec.x += -20;

   if (mKeyboard->isKeyDown(OIS::KC_S))
      moveVec.x += 20;
   
   if (mKeyboard->isKeyDown(OIS::KC_W))
      moveVec.z += -20;
   
   if (mKeyboard->isKeyDown(OIS::KC_R))
      moveVec.z += 20;

   if(mKeyboard->isKeyDown(OIS::KC_SPACE))
      moveVec.y += 20;

   if(mKeyboard->isKeyDown(OIS::KC_F))
      moveVec.y += -20;
   
   mPlayerNode->translate(moveQuat * (moveVec * time));
   
   return true;
}
  
void Client::updateStats(void)
{
   static Ogre::String currFps = "Current FPS: ";
   static Ogre::String avgFps = "Average FPS: ";
   static Ogre::String bestFps = "Best FPS: ";
   static Ogre::String worstFps = "Worst FPS: ";
   static Ogre::String tris = "Triangle Count: ";
   static Ogre::String batches = "Batch Count: ";

   // update stats when necessary
   try {
      Ogre::OverlayElement* guiAvg = mOverlayManager->getOverlayElement("Core/AverageFps");
      Ogre::OverlayElement* guiCurr = mOverlayManager->getOverlayElement("Core/CurrFps");
      Ogre::OverlayElement* guiBest = mOverlayManager->getOverlayElement("Core/BestFps");
      Ogre::OverlayElement* guiWorst = mOverlayManager->getOverlayElement("Core/WorstFps");

      const Ogre::RenderTarget::FrameStats& stats = mWindow->getStatistics();
      guiAvg->setCaption(avgFps + Ogre::StringConverter::toString(stats.avgFPS));
      guiCurr->setCaption(currFps + Ogre::StringConverter::toString(stats.lastFPS));
      guiBest->setCaption(bestFps + Ogre::StringConverter::toString(stats.bestFPS) +" "+Ogre::StringConverter::toString(stats.bestFrameTime)+" ms");
      guiWorst->setCaption(worstFps + Ogre::StringConverter::toString(stats.worstFPS) +" "+Ogre::StringConverter::toString(stats.worstFrameTime)+" ms");

      Ogre::OverlayElement* guiTris = mOverlayManager->getOverlayElement("Core/NumTris");
      guiTris->setCaption(tris + Ogre::StringConverter::toString(stats.triangleCount));

      Ogre::OverlayElement* guiBatches = mOverlayManager->getOverlayElement("Core/NumBatches");
      guiBatches->setCaption(batches + Ogre::StringConverter::toString(stats.batchCount));

      Ogre::OverlayElement* guiDbg = mOverlayManager->getOverlayElement("Core/DebugText");
      guiDbg->setCaption("Debug something here");
   }
   catch(...) { /* ignore */ }
}

bool Client::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
   Ogre::Real time = evt.timeSinceLastFrame;

   if (mWindow->isClosed())
      return false;

   if (!movePlayer(time))
      return false;
   
   while (!mUpdateQueue.empty())
   {
      t_updatePacket &temp = mUpdateQueue.front();

      mStore[temp.name].node->setPosition((Ogre::Vector3) temp.position);
      mStore[temp.name].node->setOrientation((Ogre::Quaternion) temp.orientation);

      mUpdateQueue.pop();
   }

   updateStats();
   return true;
}

bool Client::frameStarted(const Ogre::FrameEvent &evt)
{
static int lol = 0;

   if (mKeyboard->isKeyDown(OIS::KC_D))
   {
      Ogre::Vector3 Loc = mPlayerNode->getPosition();
      btVector3 from = BtOgre::Convert::toBullet(mPlayerNode->convertLocalToWorldPosition(Ogre::Vector3(0,0,0)));
      btVector3 to = BtOgre::Convert::toBullet(mPlayerNode->convertLocalToWorldPosition(moveQuat * cameraQuat * Ogre::Quaternion(Ogre::Degree(-90), Ogre::Vector3::UNIT_Y)* Ogre::Vector3(-100,0,0)));
      mConeNode->setPosition(mPlayerNode->convertLocalToWorldPosition(moveQuat * cameraQuat * Ogre::Quaternion(Ogre::Degree(-90), Ogre::Vector3::UNIT_Y)* Ogre::Vector3(-100,0,0)));

      RayCall lol(from,to);
      mWorld->rayTest(from,to,lol);
   }
   
   if (lol<5 && mKeyboard->isKeyDown(OIS::KC_T))
   {
      lol++;
      mStore["Box"].body->activate();
      mStore["Box"].body->applyCentralForce(btVector3(0,2500,1000));
   }

   if (mKeyboard->isKeyDown(OIS::KC_B))
      cout<<mCamera->getRealPosition().x<<' '<<mCamera->getRealPosition().z<<endl;
   
   mWorld->stepSimulation(evt.timeSinceLastFrame, 10);
   mWorld->debugDrawWorld();

   dbgdraw->setDebugMode(1);
   dbgdraw->step();
   return true;
}

void Client::windowResized(Ogre::RenderWindow* rw)
{
    unsigned int width, height, depth;
    int left, top;
    rw->getMetrics(width, height, depth, left, top);
 
    const OIS::MouseState &ms = mMouse->getMouseState();
    ms.width = width;
    ms.height = height;

}
 
//Unattach OIS before window shutdown (very important under Linux)
void Client::windowClosed(Ogre::RenderWindow* rw)
{
    //Only close for window that created OIS (the main window in these demos)
    if( rw == mWindow )
    {
        if( mInputManager )
        {
            mInputManager->destroyInputObject( mMouse );
            mInputManager->destroyInputObject( mKeyboard );
 
            OIS::InputManager::destroyInputSystem(mInputManager);
            mInputManager = 0;
        }
    }
}

void Client::loadPhx()
{
   mBroadphase = new btAxisSweep3(btVector3(-10000,-10000,-10000), btVector3(10000,10000,10000), 1024);
   mCollisionConfig = new btDefaultCollisionConfiguration();
   mDispatcher = new btCollisionDispatcher(mCollisionConfig);
   mSolver = new btSequentialImpulseConstraintSolver();

   mWorld = new btDiscreteDynamicsWorld(mDispatcher, mBroadphase, mSolver, mCollisionConfig);
   mWorld->setGravity(btVector3(0,-9.8,0));

   dbgdraw = new BtOgre::DebugDrawer(mSceneMgr->getRootSceneNode(), mWorld);
   mWorld->setDebugDrawer(dbgdraw);
}

void Client::parseObjectData(void)
{
   for (auto iter = mObjectData.begin();iter != mObjectData.end();++iter)
   {
      parseObject(iter->second);
   }
}
         

void Client::parseObject(const t_objectData &ObjectData)
{
   t_Store temp;

   temp.entity = mSceneMgr->createEntity(ObjectData.entName, ObjectData.meshName);
   temp.node = mSceneMgr->getRootSceneNode()->createChildSceneNode(ObjectData.sceneName,ObjectData.position,ObjectData.orientation); 

   temp.node->attachObject(temp.entity);

   temp.node->setScale(ObjectData.scale);

   //Create shape.
   BtOgre::StaticMeshToShapeConverter converter(temp.entity);
   switch (ObjectData.type)
   {
      case 0:
         temp.shape = converter.createBox();
         break;

      case 1:
         temp.shape = converter.createTrimesh();
         break;
   }

   //Calculate inertia.
   btScalar mass = ObjectData.mass;
   btVector3 inertia;
   temp.shape->calculateLocalInertia(mass, inertia);

   //Create BtOgre MotionState (connects Ogre and Bullet).
   BtOgre::RigidBodyState *mState = new BtOgre::RigidBodyState(temp.node);

   //Create the Body.
   btRigidBody::btRigidBodyConstructionInfo mRigidInfo(mass, mState, temp.shape, inertia);
   mRigidInfo.m_friction = ObjectData.friction;

   temp.body = new btRigidBody(mRigidInfo);
   mWorld->addRigidBody(temp.body);

   mStore[ObjectData.name] = temp;
}
   

/*
   Ogre::SceneNode *mGroundNode = mSceneMgr->getSceneNode("Plane");
   Ogre::Entity *mGroundEnt = mSceneMgr->getEntity("Plane");

   Ogre::SceneNode *mBoxNode = mSceneMgr->getSceneNode("Cube");
   Ogre::Entity *mBoxEnt = mSceneMgr->getEntity("Cube");
   
   //Create shape.
   BtOgre::StaticMeshToShapeConverter converter(mBoxEnt);
   mBoxShape = converter.createBox();

   //Calculate inertia.
   btScalar mass = 5;
   btVector3 inertia;
   mBoxShape->calculateLocalInertia(mass, inertia);

   //Create BtOgre MotionState (connects Ogre and Bullet).
   BtOgre::RigidBodyState *mBoxState = new BtOgre::RigidBodyState(mBoxNode);

   //Create the Body.
   btRigidBody::btRigidBodyConstructionInfo mBoxRigidInfo(mass, mBoxState, mBoxShape, inertia);
   mBoxRigidInfo.m_friction = 1;

   mBoxBody = new btRigidBody(mBoxRigidInfo);
   mWorld->addRigidBody(mBoxBody);


   //Create the ground shape.
   BtOgre::StaticMeshToShapeConverter converter2(mGroundEnt);
   mGroundShape = converter2.createTrimesh();

   //Calculate inertia.
   mass = 0;
   inertia = btVector3(0,0,0);
   mGroundShape->calculateLocalInertia(mass, inertia);

   //Create MotionState (no need for BtOgre here, you can use it if you want to though).
   BtOgre::RigidBodyState *mGroundState = new BtOgre::RigidBodyState(mGroundNode);//btTransform(btQuaternion(0,0,0,1),btVector3(0,0,0)));

   //Create the Body.
   btRigidBody::btRigidBodyConstructionInfo mGroundRigidInfo(mass, mGroundState, mGroundShape, inertia);
   mGroundRigidInfo.m_friction = 1;

   mGroundBody = new btRigidBody(mGroundRigidInfo);
   mWorld->addRigidBody(mGroundBody); 
   
   //addCylinder("Cylinder.004",mCyl1Shape,mCyl1Body,1.9,-1.5,.5);
   //addCylinder("Cylinder",mCyl2Shape,mCyl2Body,1.9,1.5,.5);
   //addCylinder("Cylinder.006",mCyl3Shape,mCyl3Body,-1.9,-1.5,-.5);
   //addCylinder("Cylinder.007",mCyl4Shape,mCyl4Body,-1.9,1.5,-.5);
}
*/
/*
void Client::addCylinder(const char* name,btCollisionShape *mCylShape,btRigidBody *mCylBody,float bodyx, float bodyz,float wheelx)
{
   Ogre::SceneNode *mCylNode = mSceneMgr->getSceneNode(name);
   Ogre::Entity *mCylEnt = mSceneMgr->getEntity(name);
   
   //Create shape.
   BtOgre::StaticMeshToShapeConverter converter3(mCylEnt);
   mCylShape = converter3.createConvex();

   //Calculate inertia.
   btScalar mass = 1;
   btVector3 inertia;
   mCylShape->calculateLocalInertia(mass, inertia);

   //Create BtOgre MotionState (connects Ogre and Bullet).
   BtOgre::RigidBodyState *mCylState = new BtOgre::RigidBodyState(mCylNode);

   //Create the Body.
   btRigidBody::btRigidBodyConstructionInfo mCylRigidInfo(mass, mCylState, mCylShape, inertia);
   mCylRigidInfo.m_friction = 1;

   mCylBody = new btRigidBody(mCylRigidInfo);
   mCylBody->setActivationState(DISABLE_DEACTIVATION);
   mWorld->addRigidBody(mCylBody);

   btVector3 axisA(0,1,0), axisB(0,0,0);
   btHingeConstraint *Constraint = new btHingeConstraint(*mCylBody,*mBoxBody,btVector3(0,wheelx,0),btVector3(bodyx,-.35,bodyz),axisA,axisB);
   mWorld->addConstraint(Constraint,true);
   //mCylBody->setAngularVelocity(axisB);
}
*/
