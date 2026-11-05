#include "Display/ShadowMapper.hpp"

#include <cstdio>

ShadowMapper::ShadowMapper()
{
}

ShadowMapper::~ShadowMapper()
{
    Shutdown();
}

bool ShadowMapper::Initialize(int width, int height)
{
    if (mInitialized)
        return true;

    mWidth = width > 0 ? width : 1024;
    mHeight = height > 0 ? height : 1024;

    // 建立深度貼圖
    glGenTextures(1, &mDepthTexture);
    glBindTexture(GL_TEXTURE_2D, mDepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, mWidth, mHeight, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 建立 FBO 並附加深度貼圖
    glGenFramebuffers(1, &mFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           mDepthTexture, 0);

    // 不繪製顏色，只寫入深度
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        std::fprintf(stderr,
                     "ShadowMapper: FBO incomplete (status=0x%x)\n", (unsigned)status);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    mInitialized = true;
    return true;
}

void ShadowMapper::BeginShadowPass()
{
    if (!mInitialized || !mFBO)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glViewport(0, 0, mWidth, mHeight);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
}

void ShadowMapper::EndShadowPass()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowMapper::BindForReading(GLuint textureUnit)
{
    if (!mInitialized || !mDepthTexture)
        return;

    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, mDepthTexture);
}

void ShadowMapper::Shutdown()
{
    if (mFBO)
    {
        glDeleteFramebuffers(1, &mFBO);
        mFBO = 0;
    }
    if (mDepthTexture)
    {
        glDeleteTextures(1, &mDepthTexture);
        mDepthTexture = 0;
    }
    mInitialized = false;
}
