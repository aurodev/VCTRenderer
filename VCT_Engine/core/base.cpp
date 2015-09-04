#include "stdafx.h"
#include "base.h"
using namespace VCT_ENGINE;
Base * VCT_ENGINE::Base::coreInstance = nullptr;

// initializes base engine assets and libs
Base::Base()
{
    // initialize external dependencies
    initializer.Initialize();
    // open window and set rendering context
    renderWindow.Open();
    renderWindow.SetAsCurrentContext();
    // initialize context dependant external libs
    initializer.InitializeContextDependant();
    // set interface to current renderwindow
    userInterface.Initialize(renderWindow);
    // load engine demo scene assets
    // assetLoader.LoadDemoScenes();
    assetLoader.LoadShaders();
}


Base::~Base()
{
}

Base * VCT_ENGINE::Base::Instance()
{
    if(!coreInstance)
    {
        return coreInstance = new Base();
    }

    return coreInstance;
}

void VCT_ENGINE::Base::MainLoop()
{
    // gl context handler
    oglplus::Context gl;
    // black screen initiallly
    gl.ClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // render loop
    while(!glfwWindowShouldClose(renderWindow.Handler()))
    {
        glfwPollEvents();
        userInterface.Draw();
        gl.Clear().ColorBuffer().DepthBuffer(); glClear(GL_COLOR_BUFFER_BIT);
        userInterface.Render();
        glfwSwapBuffers(renderWindow.Handler());
    }
}
