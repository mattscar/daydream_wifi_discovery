#include "wifidiscovery_renderer.h"

static const char* TAG = "WiFiDiscovery";
std::vector<std::string> shaderNames = {
  "messages.vert", "messages.frag",
  "spinner.vert", "spinner.frag",
  "pointer.vert", "pointer.frag",
  "host.vert", "host.frag",
  "box.vert", "box.frag",
  "boxtext.vert", "boxtext.frag"};
static const float PLAYER_DEPTH = -5.0f;
static const float CIRCLE_RADIUS = 0.15f;

static const float HOST_WIDTH = 0.3f;
static const float HOST_HEIGHT = 0.3f;
static const float HOST_HORIZ_SPACING = 0.45f;
static const float HOST_VERT_SPACING = 0.75f;

static const float DISPLAY_TEXT_HEIGHT = 0.20f;
static const float DISPLAY_TEXT_SPACING = 0.15f;

static const float BOX_HEIGHT = 0.6f;
static const float BOX_TEXT_HEIGHT = 0.2f;
static const float BORDER_COLOR = 0.6f;
static const float HOST_TEXT_SPACING = -0.08f;
static const float IP_TEXT_SPACING = -0.3f;

// Near and far clipping planes.
static const float near = 1.0f;
static const float far = 100.0f;

// Message vertices
static const GLfloat vertexData[] = {

   // Error message
   1.4f, -0.2f,  0.99f,  0.44f,
  -1.4f, -0.2f,  0.0f,   0.44f,
   1.4f,  0.2f,  0.99f,  0.545f,
  -1.4f,  0.2f,  0.0f,   0.545f,

   // Scanning message
   0.55f, 0.3f,  0.86f,  0.308f,
  -0.4f,  0.3f,  0.255f, 0.308f,
   0.55f, 0.5f,  0.86f,  0.44f,
  -0.4f,  0.5f,  0.255f, 0.44f,

  // Pointer vertices
  0.12f,  0.0f,  0.255f, 0.01f,
  0.00f,  0.0f,  0.0f,   0.01f,
  0.12f,  0.2f,  0.255f, 0.44f,
  0.00f,  0.2f,  0.0f,   0.44f,

  // Host vertices
   HOST_WIDTH/2, -HOST_HEIGHT/2,  0.56f,  0.01f,
  -HOST_WIDTH/2, -HOST_HEIGHT/2,  0.29f,  0.01f,
   HOST_WIDTH/2,  HOST_HEIGHT/2,  0.56f,  0.3f,
  -HOST_WIDTH/2,  HOST_HEIGHT/2,  0.29f,  0.3f};

void handleMessage(GLenum source​, GLenum type​, GLuint id​,
  GLenum severity​, GLsizei length​, const GLchar* msg,
  const void* userData);

WiFiDiscoveryRenderer::WiFiDiscoveryRenderer(
  gvr_context* gvrContext, AAssetManager* assetMgr):
  gvrApi(gvr::GvrApi::WrapNonOwned(gvrContext)),
  buffViewport(gvrApi->CreateBufferViewport()),
  assetManager(assetMgr),
  ready(false),
  firstFrame(true),
  hostReady(false),
  selectedHost(-1),
  numIndices(0),
  numOffsets(0),
  numDisplayChars(0),
  spinnerSegments(2),
  atlas(TextUtils::CreateAtlas()){}

WiFiDiscoveryRenderer::~WiFiDiscoveryRenderer() {
  glDeleteBuffers(NUM_VBOS, vbos);
  glDeleteBuffers(NUM_IBOS, ibos);
  glDeleteBuffers(NUM_UBOS, ubos);
  glDeleteVertexArrays(NUM_VAOS, vaos);
  glDeleteTextures(NUM_TEXTURES, tids);
}

void WiFiDiscoveryRenderer::InitShaders() {

  GLuint vertDescriptor, fragDescriptor;
  std::string vertFile, fragFile;
  unsigned int progIndex;

  // Process each pair of shaders
  for(unsigned int i=0; i<shaderNames.size(); i+=2) {

    // Read and compile vertex shader
    vertDescriptor = glCreateShader(GL_VERTEX_SHADER);
    vertFile = ShaderUtils::ReadFile(assetManager, shaderNames[i].c_str());
    const GLchar* vertSource = vertFile.c_str();
    glShaderSource(vertDescriptor, 1, &vertSource, 0);
    ShaderUtils::CompileShader(vertDescriptor);

    // Read and compile fragment shader
    fragDescriptor = glCreateShader(GL_FRAGMENT_SHADER);
    fragFile = ShaderUtils::ReadFile(assetManager, shaderNames[i+1].c_str());
    const GLchar* fragSource = fragFile.c_str();
    glShaderSource(fragDescriptor, 1, &fragSource, 0);
    ShaderUtils::CompileShader(fragDescriptor);

    // Create program and bind attributes
    progIndex = i/2;
    programs[progIndex] = glCreateProgram();
    glAttachShader(programs[progIndex], vertDescriptor);
    glAttachShader(programs[progIndex], fragDescriptor);
    ShaderUtils::LinkProgram(programs[progIndex]);
  }
}

void WiFiDiscoveryRenderer::InitMessages() {

  // Bind the VAO
  glBindVertexArray(vaos[0]);

  // Configure the VBO
  glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
  GLsizeiptr dataSize = sizeof(vertexData);
  int length = dataSize/sizeof(vertexData[0]);

  glBufferData(GL_ARRAY_BUFFER,
    dataSize, NULL, GL_DYNAMIC_DRAW);
  GLfloat* vertexBuffer = (GLfloat*)glMapBufferRange(
      GL_ARRAY_BUFFER, 0, dataSize, GL_MAP_WRITE_BIT);
  std::copy(vertexData, vertexData + length, vertexBuffer);
  glUnmapBuffer(GL_ARRAY_BUFFER);

  // Associate coordinate data with in_coords
  GLint coordIndex = glGetAttribLocation(programs[0], "in_coords");
  glEnableVertexAttribArray((GLuint)coordIndex);
  glVertexAttribPointer((GLuint)coordIndex, 2,
    GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), 0);

  // Associate color data with in_texcoords
  GLint texcoordIndex = glGetAttribLocation(programs[0], "in_texcoords");
  glEnableVertexAttribArray((GLuint)texcoordIndex);
  glVertexAttribPointer((GLuint)texcoordIndex, 2,
    GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (GLvoid*)8);

  // Unbind the VAO
  glBindVertexArray(0);
}

void WiFiDiscoveryRenderer::InitSpinner() {

  // Set the program and VBO
  glBindVertexArray(vaos[1]);

  // Initialize circle vertices
  float circleVertices[130];
  for(int i=0; i<65; ++i) {
    circleVertices[2*i] = CIRCLE_RADIUS * (float)cos((2.0f * M_PI * i)/64);
    circleVertices[2*i+1] = CIRCLE_RADIUS * (float)sin((2.0f * M_PI * i)/64);
  }

  // Configure the VBO
  glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
  GLsizeiptr dataSize = sizeof(circleVertices);
  int length = dataSize/sizeof(circleVertices[1]);
  glBufferData(GL_ARRAY_BUFFER,
    dataSize, NULL, GL_DYNAMIC_DRAW);
  GLfloat* vertexBuffer = (GLfloat*)glMapBufferRange(
      GL_ARRAY_BUFFER, 0, dataSize, GL_MAP_WRITE_BIT);
  std::copy(circleVertices, circleVertices + length, vertexBuffer);
  glUnmapBuffer(GL_ARRAY_BUFFER);

  // Associate coordinate data with in_coords
  GLint coordIndex = glGetAttribLocation(programs[1], "in_coords");
  glEnableVertexAttribArray((GLuint)coordIndex);
  glVertexAttribPointer((GLuint)coordIndex, 2,
    GL_FLOAT, GL_FALSE, 0, 0);

  // Set the selected button to -1
  glBufferSubData(GL_UNIFORM_BUFFER, sizeof(gvr::Mat4f) + sizeof(target) + sizeof(offsets),
    sizeof(selectedHost), (GLvoid*)&selectedHost);

  // Unbind the VAO
  glBindVertexArray(0);
}

void WiFiDiscoveryRenderer::InitPointer() {

  // Bind the VAO
  glBindVertexArray(vaos[2]);

  // Configure the VBO
  glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);

  // Associate coordinate data with in_coords
  GLint coordIndex = glGetAttribLocation(programs[2], "in_coords");
  glEnableVertexAttribArray((GLuint)coordIndex);
  glVertexAttribPointer((GLuint)coordIndex, 2,
    GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (GLvoid*)(32*sizeof(float)));

  // Associate color data with in_texcoords
  GLint texcoordIndex = glGetAttribLocation(programs[2], "in_texcoords");
  glEnableVertexAttribArray((GLuint)texcoordIndex);
  glVertexAttribPointer((GLuint)texcoordIndex, 2,
    GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (GLvoid*)(34*sizeof(float)));

  // Unbind the VAO
  glBindVertexArray(0);
}

void WiFiDiscoveryRenderer::InitHosts() {

  // Bind the VAO
  glBindVertexArray(vaos[3]);

  // Configure the VBO
  glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);

  // Associate coordinate data with in_coords
  GLint coordIndex = glGetAttribLocation(programs[3], "in_coords");
  glEnableVertexAttribArray((GLuint)coordIndex);
  glVertexAttribPointer((GLuint)coordIndex, 2,
    GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (GLvoid*)(48*sizeof(float)));

  // Associate color data with in_texcoords
  GLint texcoordIndex = glGetAttribLocation(programs[3], "in_texcoords");
  glEnableVertexAttribArray((GLuint)texcoordIndex);
  glVertexAttribPointer((GLuint)texcoordIndex, 2,
    GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (GLvoid*)(50*sizeof(float)));

  // Unbind the VAO
  glBindVertexArray(0);
}

void WiFiDiscoveryRenderer::InitText() {

  // Configure the VAO
  glBindVertexArray(vaos[4]);

  glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
  glBufferStorageEXT(GL_ARRAY_BUFFER, 81920, NULL, GL_MAP_WRITE_BIT);

  // Associate coordinate data with in_coords
  GLint coordIndex = glGetAttribLocation(programs[0], "in_coords");
  glEnableVertexAttribArray((GLuint)coordIndex);
  glVertexAttribPointer((GLuint)coordIndex, 2,
    GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), 0);

  // Associate color data with in_texcoords
  GLint texcoordIndex = glGetAttribLocation(programs[0], "in_texcoords");
  glEnableVertexAttribArray((GLuint)texcoordIndex);
  glVertexAttribPointer((GLuint)texcoordIndex, 2,
    GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (GLvoid*)8);

  // Configure the IBO
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibos[0]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 20480, NULL, GL_DYNAMIC_DRAW);

  // Unbind the VAO
  glBindVertexArray(0);
}

void WiFiDiscoveryRenderer::InitBox() {

  // Configure the VAO
  glBindVertexArray(vaos[5]);

  glBindBuffer(GL_ARRAY_BUFFER, vbos[3]);
  glBufferStorageEXT(GL_ARRAY_BUFFER, 24*sizeof(float), NULL, GL_MAP_WRITE_BIT);

  // Associate coordinate data with in_coords
  GLint coordIndex = glGetAttribLocation(programs[4], "in_coords");
  glEnableVertexAttribArray((GLuint)coordIndex);
  glVertexAttribPointer((GLuint)coordIndex, 2,
    GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), 0);

  // Associate color data with in_texcoords
  GLint texcoordIndex = glGetAttribLocation(programs[4], "in_color");
  glEnableVertexAttribArray((GLuint)texcoordIndex);
  glVertexAttribPointer((GLuint)texcoordIndex, 1,
    GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), (GLvoid*)8);

  // Unbind the VAO
  glBindVertexArray(0);
}

void WiFiDiscoveryRenderer::InitBoxText() {

  // Configure the VAO
  glBindVertexArray(vaos[6]);
  glBindBuffer(GL_ARRAY_BUFFER, vbos[4]);
  glBufferStorageEXT(GL_ARRAY_BUFFER, 8192, NULL,
    GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT_EXT | GL_MAP_COHERENT_BIT_EXT);

  // Associate coordinate data with in_coords
  GLint coordIndex = glGetAttribLocation(programs[5], "in_coords");
  glEnableVertexAttribArray((GLuint)coordIndex);
  glVertexAttribPointer((GLuint)coordIndex, 2,
    GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), 0);

  // Associate color data with in_texcoords
  GLint texcoordIndex = glGetAttribLocation(programs[5], "in_texcoords");
  glEnableVertexAttribArray((GLuint)texcoordIndex);
  glVertexAttribPointer((GLuint)texcoordIndex, 2,
    GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (GLvoid*)8);

  // Configure the IBO
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibos[1]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 2560, NULL, GL_DYNAMIC_DRAW);

  // Unbind the VAO
  glBindVertexArray(0);
}

void WiFiDiscoveryRenderer::InitTextures() {

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tids[0]);

  // Read pixel data and associate it with texture
  AAsset* asset = AAssetManager_open(assetManager, "wifi_discovery.bmp", AASSET_MODE_BUFFER);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, 256, 256, 0,
    GL_RED, GL_UNSIGNED_BYTE, AAsset_getBuffer(asset));

  // Set texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Close asset file
  AAsset_close(asset);
}

void WiFiDiscoveryRenderer::OnSurfaceCreated() {

  // Initialize OpenGL processing
  gvrApi->InitializeGl();
  glEnable(GL_SCISSOR_TEST);
  glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
  glDepthMask(GL_FALSE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_SRC_COLOR);
  glLineWidth(2.0f);
  glBufferStorageEXT =
    (PFNGLBUFFERSTORAGEEXTPROC) eglGetProcAddress("glBufferStorageEXT");

  // Configure event handling
  glEnable(GL_DEBUG_OUTPUT_KHR);
  glDebugMessageCallbackKHR(handleMessage, NULL);

  // Initialize objects and shaders
  glGenVertexArrays(NUM_VAOS, vaos);
  glGenBuffers(NUM_IBOS, ibos);
  glGenBuffers(NUM_VBOS, vbos);
  glGenBuffers(NUM_UBOS, ubos);
  glGenTextures(NUM_TEXTURES, tids);
  InitShaders();

  // Initialize uniform buffer object
  glBindBuffer(GL_UNIFORM_BUFFER, ubos[0]);
  glBufferData(GL_UNIFORM_BUFFER,
    sizeof(gvr::Mat4f) + sizeof(target) + sizeof(offsets) + sizeof(selectedHost),
    NULL, GL_DYNAMIC_DRAW);

  // Associate each program with the UBO
  GLuint uboIndex;
  for(GLuint i=0; i<NUM_PROGRAMS; ++i) {
    uboIndex = glGetUniformBlockIndex(programs[i], "ubo");
    glUniformBlockBinding(programs[i], uboIndex, i);
    glBindBufferBase(GL_UNIFORM_BUFFER, i, ubos[0]);
  }

  // Initialize data
  InitMessages();
  InitSpinner();
  InitTextures();
  InitPointer();
  InitHosts();
  InitText();
  InitBox();
  InitBoxText();

  // Determine the rendering size
  gvr::Sizei maxSize = gvrApi->GetMaximumEffectiveRenderTargetSize();
  renderSize.width = maxSize.width/2;
  renderSize.height = maxSize.height/2;

  // Define a BufferSpec
  std::vector<gvr::BufferSpec> specs;
  specs.push_back(gvrApi->CreateBufferSpec());
  specs[0].SetSize(renderSize);
  specs[0].SetColorFormat(GVR_COLOR_FORMAT_RGBA_8888);
  specs[0].SetDepthStencilFormat(GVR_DEPTH_STENCIL_FORMAT_DEPTH_16);
  specs[0].SetSamples(4);

  // Create the swap chain and viewport list
  swapChain.reset(new gvr::SwapChain(gvrApi->CreateSwapChain(specs)));
  viewports.reset(new gvr::BufferViewportList(
    gvrApi->CreateEmptyBufferViewportList()));

  // Initialize controller processing
  controllerApi.reset(new gvr::ControllerApi);
  controllerApi->Init(GVR_CONTROLLER_ENABLE_ORIENTATION);
  controllerApi->Resume();
  target[0] = 0.0f; target[1] = 0.0f;

  ready = true;
}

void WiFiDiscoveryRenderer::OnDrawFrame() {

  glActiveTexture(GL_TEXTURE0);

  gvr::Sizei size = gvrApi->GetMaximumEffectiveRenderTargetSize();
  size.width = size.width/2;
  size.height = size.height/2;
  if(renderSize.width != size.width ||
     renderSize.height != size.height) {
    swapChain->ResizeBuffer(0, size);
    renderSize = size;
  }

  // Initialize the buffer viewport list
  viewports->SetToRecommendedBufferViewports();

  // Determine the headset's future orientation
  gvr::ClockTimePoint time = gvr::GvrApi::GetTimePointNow();
  time.monotonic_system_time_nanos += 50000000;
  gvr::Mat4f initMatrix =
    gvrApi->GetHeadSpaceFromStartSpaceRotation(time);
  headMatrix = gvrApi->ApplyNeckModel(initMatrix, 1.0);

  // Determine the controller's target if connected
  controllerState.Update(*controllerApi);
  if(!firstFrame && controllerState.GetConnectionState() == GVR_CONTROLLER_CONNECTED) {
    gvr_quatf q = controllerState.GetOrientation();
    float tmp = (q.qw * q.qw) - (q.qx * q.qx) - (q.qy * q.qy) + (q.qz * q.qz);
    target[0] = (2.0f * q.qw * q.qy - 2.0f * q.qx * q.qz) * (PLAYER_DEPTH / tmp);
    target[1] = (-2.0f * q.qw * q.qx - 2.0f * q.qy * q.qz) * (PLAYER_DEPTH / tmp);
  } else {
    target[0] = 0.0f; target[1] = 0.0f;
  }
  if(controllerState.GetRecentered()) {
    target[0] = 0.0f; target[1] = 0.0f;
  }
  glBufferSubData(GL_UNIFORM_BUFFER, sizeof(gvr::Mat4f),
    sizeof(target), (GLvoid*)target);

  // Determine which button is pressed, if any
  bool changed = false;
  int oldSelectedHost = selectedHost;
  selectedHost = -1;
  for(int i=0; i<numOffsets; i+=2) {
    if((target[0] > (offsets[i] - HOST_WIDTH/2)) &&
      (target[0] < (offsets[i] + HOST_WIDTH/2)) &&
      (target[1] > (offsets[i+1] - HOST_HEIGHT)) &&
      (target[1] < offsets[i+1])) {
        selectedHost = i/2;
        if(oldSelectedHost != i/2) {
          changed = true;
        }
        break;
      }
  }

  if(changed) {

    // Update selectedHost uniform
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(gvr::Mat4f) + sizeof(target) + sizeof(offsets),
      sizeof(selectedHost), (GLvoid*)&selectedHost);

    // Update the box VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbos[3]);
    unsigned int len = hosts[selectedHost].box.size();
    GLsizeiptr dataSize = (GLsizeiptr)len * sizeof(float);
    GLfloat* vertexBuffer =
      (GLfloat*)glMapBufferRange(GL_ARRAY_BUFFER, 0, dataSize, GL_MAP_WRITE_BIT);
    std::copy(hosts[selectedHost].box.data(), hosts[selectedHost].box.data() + len, vertexBuffer);
    glUnmapBuffer(GL_ARRAY_BUFFER);

    // Update the box text VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbos[4]);
    len = hosts[selectedHost].text.size();
    dataSize = (GLsizeiptr)len * sizeof(float);
    vertexBuffer = (GLfloat*)glMapBufferRange(GL_ARRAY_BUFFER, 0, dataSize, GL_MAP_WRITE_BIT);
    std::copy(hosts[selectedHost].text.data(), hosts[selectedHost].text.data() + len, vertexBuffer);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    
    // Update the box text IBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibos[1]);
    dataSize = (GLsizeiptr)hosts[selectedHost].textIndices.size();
    GLubyte* indexBuffer =
      (GLubyte*)glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, dataSize, GL_MAP_WRITE_BIT);
    std::copy(hosts[selectedHost].textIndices.data(), hosts[selectedHost].textIndices.data() + dataSize, indexBuffer);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
  }

  // Update host data
  if(hostReady) {

    // Update offsets
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(gvr::Mat4f) + sizeof(target),
      2*(GLsizeiptr)hosts.size()*sizeof(float), (GLvoid*)offsets);

    // Update the text VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
    unsigned int len = textVertices.size();
    GLsizeiptr dataSize = (GLsizeiptr)len * sizeof(textVertices[0]);
    GLfloat* vertexBuffer =
      (GLfloat*)glMapBufferRange(GL_ARRAY_BUFFER, 0, dataSize, GL_MAP_WRITE_BIT);
    std::copy(textVertices.data(), textVertices.data() + len, vertexBuffer);
    glUnmapBuffer(GL_ARRAY_BUFFER);

    // Update the text IBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibos[0]);
    len = textIndices.size();
    dataSize = (GLsizeiptr)len * sizeof(textIndices[0]);
        
    GLushort* indexBuffer =
      (GLushort*)glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, dataSize, GL_MAP_WRITE_BIT);
    std::copy(textIndices.data(), textIndices.data() + dataSize, indexBuffer);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

    hostReady = false;
    state = SCAN_FINISHED;
  }

  // Acquire the frame and bind it
  gvr::Frame frame = swapChain->AcquireFrame();
  frame.BindBuffer(0);

  // Set uniform buffer
  glBindBuffer(GL_UNIFORM_BUFFER, ubos[0]);

  // Set the clear color
  glClearColor(0.0f, 0.30f, 0.25f, 1.0f);
  viewports->GetBufferViewport(0, &buffViewport);
  RenderEye(GVR_LEFT_EYE, buffViewport);
  viewports->GetBufferViewport(1, &buffViewport);
  RenderEye(GVR_RIGHT_EYE, buffViewport);

  glBindVertexArray(0);

  // Unbind the frame
  frame.Unbind();

  // Submit the frame
  frame.Submit(*viewports, headMatrix);
  firstFrame = false;
}

void WiFiDiscoveryRenderer::RenderEye(gvr::Eye eye,
  const gvr::BufferViewport& vport) {

  // Set the viewport and scissor
  const gvr::Rectf& rect = vport.GetSourceUv();
  int left = static_cast<int>(rect.left * renderSize.width);
  int bottom = static_cast<int>(rect.bottom * renderSize.height);
  int width = static_cast<int>((rect.right - rect.left) * renderSize.width);
  int height = static_cast<int>((rect.top - rect.bottom) * renderSize.height);
  glViewport(left, bottom, width, height);
  glScissor(left, bottom, width, height);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glBindTexture(GL_TEXTURE_2D, tids[0]);

  // Compute the MVP matrix
  gvr::Mat4f viewMatrix = MatrixUtils::MultiplyMM(
    gvrApi->GetEyeFromHeadMatrix(eye), headMatrix);
  gvr::Mat4f projMatrix =
    MatrixUtils::Perspective(vport.GetSourceFov(), near, far);
  gvr::Mat4f mvpMatrix =
    MatrixUtils::MultiplyMM(projMatrix, viewMatrix);
  gvr::Mat4f lastMatrix = MatrixUtils::Transpose(mvpMatrix);

  // Pass MVP matrix to shader
  glBufferSubData(GL_UNIFORM_BUFFER, 0,
    sizeof(gvr::Mat4f), (GLvoid*)lastMatrix.m);

  switch(state) {

    case NOT_CONNECTED:

      // Draw not connected message
      glUseProgram(programs[0]);
      glBindVertexArray(vaos[0]);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      glBindVertexArray(0);
      break;

    case SCANNING:

      // Draw scanning process message
      glUseProgram(programs[0]);
      glBindVertexArray(vaos[0]);
      glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
      glBindVertexArray(0);

      // Draw spinner
      glUseProgram(programs[1]);
      glBindVertexArray(vaos[1]);
      glDrawArrays(GL_LINE_STRIP, 0, spinnerSegments);
      glBindVertexArray(0);
      break;

    case SCAN_FINISHED:

      // Draw hosts
      glBindTexture(GL_TEXTURE_2D, tids[0]);
      glUseProgram(programs[3]);
      glBindVertexArray(vaos[3]);
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, hosts.size());
      glBindVertexArray(0);

      // Draw text
      glUseProgram(programs[0]);
      glBindVertexArray(vaos[4]);
      glDrawElements(GL_TRIANGLE_STRIP, numIndices, GL_UNSIGNED_SHORT, (void*)0);
      glBindVertexArray(0);

      if(selectedHost != -1) {

        // Draw box and border
        glUseProgram(programs[4]);
        glBindVertexArray(vaos[5]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDrawArrays(GL_LINE_LOOP, 4, 4);
        glBindVertexArray(0);

        // Draw text
        glUseProgram(programs[5]);
        glBindVertexArray(vaos[6]);
        glDrawElements(GL_TRIANGLE_STRIP, hosts[selectedHost].textIndices.size(), GL_UNSIGNED_BYTE, (void*)0);
        glBindVertexArray(0);
      }

      // Draw pointer
      glUseProgram(programs[2]);
      glBindVertexArray(vaos[2]);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
      glBindVertexArray(0);
      break;
  }
}

void WiFiDiscoveryRenderer::AddHost(std::string hostString) {

  WiFiHost host;
  std::string name;
  TextureChar ch;
  GLuint len;
  float displayScale = DISPLAY_TEXT_HEIGHT/atlas.lineHeight;
  float boxScale = BOX_TEXT_HEIGHT/atlas.lineHeight;

  // Separate host name from IP address
  size_t colonPos = hostString.find(":");
  host.hostName = hostString.substr(0, colonPos);
  host.ipAddr = hostString.substr(colonPos+1, hostString.length() - colonPos);  
  if(host.hostName.length() < 9) {
    host.displayName = host.hostName;
  } else {
    host.displayName = host.hostName.substr(0, 7).append("...");
  }
  numDisplayChars += host.displayName.length();
  host.hostName.insert(0, "Host: ");
  host.ipAddr.insert(0, "IP Address: ");

  // Get display width
  host.displayWidth = 0.0f;
  name = host.displayName;
  len = name.length();
  for(unsigned int i=0; i<len; ++i) {
    ch = atlas.charMap[name[i]];
    host.displayWidth += ch.xAdvance * displayScale;
  }

  // Get hostname width
  host.hostWidth = 0.0f;
  name = host.hostName;
  len = name.length();
  for(unsigned int i=0; i<len; ++i) {
    ch = atlas.charMap[name[i]];
    host.hostWidth += ch.xAdvance * boxScale;
  }

  // Get IP address width
  host.ipWidth = 0.0f;
  name = host.ipAddr;
  len = name.length();
  for(unsigned int i=0; i<len; ++i) {
    ch = atlas.charMap[name[i]];
    host.ipWidth += ch.xAdvance * boxScale;
  }

  // Generate vertices for the box/border
  host.box.reserve(24);
  float maxWidth = std::max(host.hostWidth, host.ipWidth);
  float boxWidth = maxWidth + 0.2f;

  host.box.push_back(boxWidth/2.0f); host.box.push_back(-BOX_HEIGHT); host.box.push_back(0.0f);
  host.box.push_back(-boxWidth/2.0f); host.box.push_back(-BOX_HEIGHT); host.box.push_back(0.0f);
  host.box.push_back(boxWidth/2.0f); host.box.push_back(0.0f); host.box.push_back(0.0f);
  host.box.push_back(-boxWidth/2.0f); host.box.push_back(0.0f); host.box.push_back(0.0f);

  host.box.push_back(boxWidth/2.0f); host.box.push_back(-BOX_HEIGHT); host.box.push_back(BORDER_COLOR);
  host.box.push_back(-boxWidth/2.0f); host.box.push_back(-BOX_HEIGHT); host.box.push_back(BORDER_COLOR);
  host.box.push_back(-boxWidth/2.0f); host.box.push_back(0.0f); host.box.push_back(BORDER_COLOR);
  host.box.push_back(boxWidth/2.0f); host.box.push_back(0.0f); host.box.push_back(BORDER_COLOR);

  // Generate vertices for the host name
  unsigned int numBoxChars = host.hostName.length() + host.ipAddr.length();
  host.text.reserve(16 * numBoxChars);
  float x = -1.0f * maxWidth/2.0f;
  TextUtils::GenerateVertices(host.hostName, host.text, x, HOST_TEXT_SPACING, boxScale, atlas);

  // Generate vertices for the IP address
  x = -1.0f * maxWidth/2.0f;
  TextUtils::GenerateVertices(host.ipAddr, host.text, x, IP_TEXT_SPACING, boxScale, atlas);

  // Generate indices for the box text
  host.textIndices.reserve(5 * numBoxChars);
  for(unsigned int i=0; i<numBoxChars; i++) {
    host.textIndices.push_back((GLubyte)(4*i));
    host.textIndices.push_back((GLubyte)(4*i+1));
    host.textIndices.push_back((GLubyte)(4*i+2));
    host.textIndices.push_back((GLubyte)(4*i+3));
    host.textIndices.push_back(0xff);
  }

  hosts.push_back(host);
}

void WiFiDiscoveryRenderer::SetScanComplete() {

  float x, y, x_limit, y_limit, displayScale;

  // Determine rows and columns
  unsigned int hostsPerRow;
  unsigned int numHosts = hosts.size();
  numOffsets = 2*numHosts;  
  if(numHosts < 40) {
    hostsPerRow = 8;
  } else if(numHosts < 80) {
    hostsPerRow = 10;    
  } else if(numHosts < 160) {
    hostsPerRow = 12;
  } else {
    hostsPerRow = 16;
  }
  unsigned int numRows = numHosts/hostsPerRow;
    
  // Display full rows of hosts
  x_limit = -1.0f * (HOST_WIDTH + HOST_HORIZ_SPACING) * (hostsPerRow - 1.0f)/2.0f;  
  y_limit = (HOST_HEIGHT + HOST_VERT_SPACING) * (numRows - 1.0f)/2.0f;
  for(unsigned int row=0; row<numRows; row++) {
    for(unsigned int col=0; col<hostsPerRow; col++) {
      offsets[2*(row*hostsPerRow + col)] = x_limit + col*(HOST_WIDTH + HOST_HORIZ_SPACING);
      offsets[2*(row*hostsPerRow + col)+1] = y_limit - row*(HOST_HEIGHT + HOST_VERT_SPACING);
    }    
  }

  // Display partial row of hosts
  unsigned int index = numRows*hostsPerRow;
  unsigned int numCols = numHosts - index;
  x_limit = -1.0f * (HOST_WIDTH + HOST_HORIZ_SPACING) * (numCols - 1.0f)/2.0f;  
  for(unsigned int col=0; col<numCols; col++) {
    offsets[2*(index + col)] = x_limit + col*(HOST_WIDTH + HOST_HORIZ_SPACING);
    offsets[2*(index + col)+1] = y_limit - numRows*(HOST_HEIGHT + HOST_VERT_SPACING);
  }
  
  // Set dimensions
  displayScale = DISPLAY_TEXT_HEIGHT/atlas.lineHeight;
  textVertices.reserve(16 * numDisplayChars);
  for(int hostIndex = 0; hostIndex < numHosts; hostIndex++) {
    x = offsets[2*hostIndex] - hosts[hostIndex].displayWidth/2.0f;
    y = offsets[2*hostIndex+1] - DISPLAY_TEXT_SPACING;
    TextUtils::GenerateVertices(hosts[hostIndex].displayName, textVertices, x, y, displayScale, atlas);
  }

  // Set indices
  numIndices = 5*numDisplayChars;
  textIndices.reserve(5 * numDisplayChars);
  for(unsigned short i=0; i<numDisplayChars; i++) {
    textIndices.push_back((GLushort)(4*i));
    textIndices.push_back((GLushort)(4*i+1));
    textIndices.push_back((GLushort)(4*i+2));
    textIndices.push_back((GLushort)(4*i+3));
    textIndices.push_back((GLushort)0xffff);
  }
  hostReady = true;
}

void WiFiDiscoveryRenderer::SetState(int wifiState) {
  state = static_cast<WiFiState>(wifiState);
}

void WiFiDiscoveryRenderer::PublishProgress() {
  spinnerSegments++;
}

void WiFiDiscoveryRenderer::OnPause() {
  if(ready) {
    gvrApi->PauseTracking();
  }
}

void WiFiDiscoveryRenderer::OnResume() {
  if(ready) {
    gvrApi->ResumeTracking();
  }
}

void handleMessage(GLenum source​, GLenum type​, GLuint id​,
  GLenum severity​, GLsizei length​, const GLchar* msg,
  const void* userData) {

  std::string sevMsg, typeMsg;

  // Process severity
  switch(severity​) {
    case GL_DEBUG_SEVERITY_HIGH_KHR:
      sevMsg = "High"; break;
    case GL_DEBUG_SEVERITY_MEDIUM_KHR:
      sevMsg = "Medium"; break;
    case GL_DEBUG_SEVERITY_LOW_KHR:
      sevMsg = "Low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION_KHR:
      sevMsg = "Notification"; break;
    default:
      sevMsg = "Default";
  }

  // Process type
  switch(type​) {
    case GL_DEBUG_TYPE_ERROR_KHR:
      typeMsg = "error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_KHR:
      typeMsg = "deprecated behavior"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_KHR:
      typeMsg = "undefined behavior"; break;
    case GL_DEBUG_TYPE_PORTABILITY_KHR:
      typeMsg = "portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE_KHR:
      typeMsg = "performance"; break;
    case GL_DEBUG_TYPE_MARKER_KHR:
      typeMsg = "marker"; break;
    case GL_DEBUG_TYPE_PUSH_GROUP_KHR:
      typeMsg = "push group"; break;
    case GL_DEBUG_TYPE_POP_GROUP_KHR:
      typeMsg = "pop group"; break;
    default:
      typeMsg = "default";
  }

  // Display the message
  __android_log_print(ANDROID_LOG_INFO, TAG,
    "%s priority %s message: %s (data: %d)",
    sevMsg.data(), typeMsg.data(), msg, (*(int*)userData));
}