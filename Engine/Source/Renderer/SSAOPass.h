#pragma
#ifndef SSAO_PASS_H
#define SSAO_PASS_H

#include <Engine/Source/RenderPhase.h>

#include <Engine/Source/Shader.h>
#include <Engine/Source/DeferredRenderer.h>
#include <Engine/Source/Renderer/SSAORendering.h>

#include <memory>

namespace Flux
{
    class Texture;

    class SSAOPass : public RenderPhase
    {
    public:
        SSAOPass();

        void SetGBuffer(const GBuffer* gBuffer);
        void SetSsaoInfo(SsaoInfo* ssaoInfo);
        void SetWindowSize(const Size* size);
        void SetCamera(Entity* camera);

        void render();

    private:
        std::unique_ptr<Shader> ssaoShader;
        std::unique_ptr<Shader> blurShader;

        const GBuffer* gBuffer;
        SsaoInfo* ssaoInfo;
        const Size* windowSize;
        Entity* camera;
    };
}

#endif /* SSAO_PASS_H */