/****************************************************************************
Copyright (c) 2013-2014 Auticiel SAS

http://www.fennex.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************///

#include "SceneSwitcher.h"
#include "Shorteners.h"
#include "AppMacros.h"
#include "SynchronousReleaser.h"
#include "FenneXCCBLoader.h"
#include "InputLabel.h"
#include "FileLogger.h"

NS_FENNEX_BEGIN
// singleton stuff
static SceneSwitcher *s_SharedSwitcher = nullptr;

SceneSwitcher* SceneSwitcher::sharedSwitcher(void)
{
    if (!s_SharedSwitcher)
    {
        s_SharedSwitcher = new SceneSwitcher();
        s_SharedSwitcher->init();
    }
    
    return s_SharedSwitcher;
}

SceneSwitcher::~SceneSwitcher()
{
    Director::getInstance()->getEventDispatcher()->removeEventListener(planSceneSwitchListener);
    s_SharedSwitcher = nullptr;
}

void SceneSwitcher::init()
{
    currentScene = nullptr;
    sceneSwitchCancelled = false;
    currentSceneName = None;
    nextSceneParam = ValueMap();
    isEventFired = false;
    keyboardLock = -1;
    delayReplace = 0;
    Director::getInstance()->setNotificationNode(Node::create());
    planSceneSwitchListener = Director::getInstance()->getEventDispatcher()->addCustomEventListener("PlanSceneSwitch", std::bind(&SceneSwitcher::planSceneSwitch, this, std::placeholders::_1));
}

void SceneSwitcher::initWithScene(SceneName nextSceneType, ValueMap param)
{
    CCAssert(currentSceneName == None, "in initWithScene in SceneSwitcher : cannot init : a scene already exist");
    CCAssert(nextSceneType != None, "in initWithScene in SceneSwitcher : cannot init to None scene");
    nextScene = nextSceneType;
    frameDelay = true;
    nextSceneParam = param;
    processingSwitch = true;
    this->trySceneSwitch();
}

void SceneSwitcher::trySceneSwitch(float deltaTime)
{
    if(nextScene != None && !isEventFired)
    {
#if VERBOSE_GENERAL_INFO
        log("next scene : %s, current scene : %s", formatSceneToString(nextScene).c_str(), formatSceneToString(currentSceneName).c_str());
#endif
        FileLogger::info("SceneSwitcher", "Going from " + formatSceneToString(currentSceneName) + " to " + formatSceneToString(nextScene));
        if(nextScene != currentSceneName || reloadAllowed)
        {
            //TODO : add delay : requires GraphicLayer : implementation of closePanel needed
            float sceneSwitchDelay = 0;
            if(sceneSwitchDelay != 0)
            {
                delayReplace = sceneSwitchDelay + SCENE_SWITCH_OFFSET;
                isEventFired = true;
                frameDelay = true;
            }
            else if(!frameDelay)
            {
                frameDelay = true;
            }
            else
            {
                this->replaceScene();
            }
        }
        else
        {
            this->takeQueuedScene();
        }
    }
    else if(nextScene != None && isEventFired)
    {
        delayReplace -= deltaTime;
#if VERBOSE_GENERAL_INFO
        log("event fired, remaining delay : %f", delayReplace);
#endif
        if(delayReplace <= 0)
        {
            delayReplace = 0;
            this->replaceScene();
        }
    }
    else if(nextScene == None)
    {
        this->takeQueuedScene();
    }
    processingSwitch = false;
    if(keyboardLock >= 0)
    {
        InputLabel::releaseAllKeyboardLocks();
        keyboardLock = -1;
    }
}

void SceneSwitcher::takeQueuedScene()
{
    if(nextScene != queuedScene)
    {
#if VERBOSE_GENERAL_INFO
        log("Taking queued scene : %s", formatSceneToString(queuedScene).c_str());
#endif
        nextScene = queuedScene;
        frameDelay = false;
        nextSceneParam = queuedParam;
    }
    else
    {
#if VERBOSE_GENERAL_INFO
        log("Queued scene is the same as current : %s", formatSceneToString(queuedScene).c_str());
#endif
        nextScene = None;
        queuedParam.clear();
    }
    queuedScene = None;
    queuedParam.clear();
}

void SceneSwitcher::planSceneSwitch(EventCustom* event)
{
    ValueMap infos = ((Value*)event->getUserData())->asValueMap();
    if(!processingSwitch && nextScene == None)
    {
        //Unbind all async texture load: since the scene will be replaced, the image won't need their new texture
        Director::getInstance()->getTextureCache()->unbindAllImageAsync();
        nextScene = (SceneName) infos["Scene"].asInt();
#if VERBOSE_GENERAL_INFO
        log("Planning Scene Switch to %s", formatSceneToString(nextScene).c_str());
#endif
        frameDelay = false;
        nextSceneParam = ValueMap(infos);
        processingSwitch = true;
        isEventFired = false;
        keyboardLock = InputLabel::preventKeyboardOpen();
    }
    else
    {
        queuedScene = (SceneName) infos["Scene"].asInt();
        queuedParam = ValueMap(infos);
#if VERBOSE_GENERAL_INFO
        log("Queuing scene change as a scene switch is already happening");
#endif
    }
}

void SceneSwitcher::cancelSceneSwitch()
{
    sceneSwitchCancelled = true;
#if VERBOSE_GENERAL_INFO
    log("Scene switch cancelled");
#endif
}

bool SceneSwitcher::isSwitching()
{
    return processingSwitch || isEventFired;
}

void SceneSwitcher::allowReload()
{
    reloadAllowed = true;
}

void SceneSwitcher::replaceScene()
{
#if VERBOSE_PERFORMANCE_TIME
    timeval startTime;
    gettimeofday(&startTime, nullptr);
#endif
#if VERBOSE_GENERAL_INFO
    log("Starting replace Scene");
#endif
    if(sceneSwitchCancelled)
    {
#if VERBOSE_GENERAL_INFO
        log("Ignoring scene %s as it was cancelled", formatSceneToString(currentSceneName).c_str());
#endif
        this->takeQueuedScene();
        sceneSwitchCancelled = false;
    }
    if((currentSceneName != nextScene && nextScene != None) || reloadAllowed)
    {
        if(currentSceneName != None)
        {
            SynchronousReleaser::sharedReleaser()->addObjectToReleasePool(currentScene);
            CCAssert(currentScene != nullptr, "in replaceScene in SceneSwitcher currentScene is nil while not being None");
            currentScene->stop();
            currentScene = nullptr;
        }
        if(currentScene != nullptr)
        {
            log("Warning : current scene should be nullptr already, stopping it anyway (probably caused by a cancelSceneSwitch");
            currentScene->stop();
        }
        CCAssert(nextScene != None, "in replaceScene in SceneSwitcher cannot go to scene None");
        currentScene = Scene::createScene(nextScene, nextSceneParam);
        if(Director::getInstance()->getRunningScene() == nullptr)
        {
            Director::getInstance()->runWithScene(currentScene->getCocosScene());
        }
        else
        {
            Director::getInstance()->replaceScene(currentScene->getCocosScene());
        }
        nextSceneParam.clear();
        currentSceneName = nextScene;
        this->takeQueuedScene();
        Value infos = Value(ValueMap({{"Scene", Value(currentSceneName)}}));
        Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("SceneSwitched", &infos);
    }
    else
    {
        log("Warning : in replaceScene in SceneSwitcher : same next scene");
    }
    isEventFired = false;
    reloadAllowed = false;
    
#if VERBOSE_PERFORMANCE_TIME
    timeval endTime;
    gettimeofday(&endTime, nullptr);
    log("Replace Scene ended in %f ms",  getTimeDifferenceMS(startTime, endTime));
#endif
}
NS_FENNEX_END
