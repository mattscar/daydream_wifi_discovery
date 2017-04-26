#ifndef MEDIA_PLAYER_RENDERER_H_
#define MEDIA_PLAYER_RENDERER_H_

#include <jni.h>

#include <algorithm>
#include <string>

#include <android/asset_manager_jni.h>
#include <android/log.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

#include "vr/gvr/capi/include/gvr.h"
#include "vr/gvr/capi/include/gvr_controller.h"

#include "matrixutils.h"
#include "shaderutils.h"
#include "textutils.h"

#define NUM_VAOS 7
#define NUM_VBOS 5
#define NUM_IBOS 2
#define NUM_UBOS 1
#define NUM_TEXTURES 1
#define NUM_PROGRAMS 6

class WiFiDiscoveryRenderer {

  public:
    WiFiDiscoveryRenderer(gvr_context_* gvrContext, AAssetManager* assetMgr);
    ~WiFiDiscoveryRenderer();

    void OnSurfaceCreated();
    void OnDrawFrame();
    void OnPause();
    void OnResume();
    void InitShaders();
    void InitSpinner();
    void SetState(int state);
    void AddHost(std::string host);
    void SetScanComplete();
    void InitMessages();
    void InitPointer();
    void InitTextures();
    void InitHosts();
    void InitText();
    void InitBox();
    void InitBoxText();
    void RenderEye(gvr::Eye eye, const gvr::BufferViewport& viewport);
    void PublishProgress();

  private:
    enum WiFiState { NOT_CONNECTED = 0, SCANNING = 1, SCAN_FINISHED = 2};
    WiFiState state;

    typedef struct {
      std::string displayName, hostName, ipAddr;
      float displayWidth, hostWidth, ipWidth, maxWidth;
      std::vector<GLfloat> box;
      std::vector<GLfloat> text;      
      std::vector<GLubyte> textIndices;
    } WiFiHost;
    std::vector<WiFiHost> hosts;
    float offsets[256];

    // Buffer descriptors
    GLuint vaos[NUM_VAOS], vbos[NUM_VBOS], ibos[NUM_IBOS],
      ubos[NUM_UBOS], tids[NUM_TEXTURES], programs[NUM_PROGRAMS];

    AAssetManager* assetManager;
    std::unique_ptr<gvr::GvrApi> gvrApi;
    std::unique_ptr<gvr::SwapChain> swapChain;
    std::unique_ptr<gvr::BufferViewportList> viewports;
    gvr::BufferViewport buffViewport;
    gvr::Sizei renderSize;
    gvr::Mat4f headMatrix;
    int selectedHost, spinnerSegments;
    bool ready, firstFrame, hostReady;

    // Extension function
    PFNGLBUFFERSTORAGEEXTPROC glBufferStorageEXT;

    // Controller
    std::unique_ptr<gvr::ControllerApi> controllerApi;
    gvr::ControllerState controllerState;
    float target[2];

    // Text
    TextureAtlas atlas;
    std::vector<GLfloat> textVertices;
    std::vector<GLushort> textIndices;
    GLuint numIndices, numDisplayChars;
    unsigned int numOffsets;
};

#endif  // MEDIA_PLAYER_RENDERER_H_