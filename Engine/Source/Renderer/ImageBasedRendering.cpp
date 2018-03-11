#include "Renderer/ImageBasedRendering.h"

#include "Framebuffer.h"
#include "Shader.h"
#include "Renderer/RenderState.h"
#include "Texture.h"
#include "TextureUnit.h"

#include <glad/glad.h>
#include <iostream>

namespace Flux
{
    void IrradianceMap::generate(const uint resolution)
    {
        Shader shader;
        shader.loadFromFile("res/Shaders/Quad.vert", "res/Shaders/Irradiance.frag");

        Framebuffer framebuffer;
        framebuffer.create();
        framebuffer.bind();
        framebuffer.addDrawBuffer(GL_COLOR_ATTACHMENT0);

        glClearColor(1, 0, 0, 1);

        shader.bind();

        create();
        bind(TextureUnit::TEXTURE0);
        setWrapping(REPEAT, REPEAT, REPEAT);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        if (skybox) {
            for (int i = 0; i < 6; i++) {
                setFace(i);
                setData(resolution, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            }
            glBindTexture(GL_TEXTURE_2D, 0);
            envMap->bind(TextureUnit::TEXTURE0);
            shader.uniform1i("EnvMap", TextureUnit::TEXTURE0);
        }
        else {
            for (int i = 0; i < 6; i++) {
                setFace(i);
                setData(resolution, GL_RGBA16F, GL_RGBA, GL_FLOAT, nullptr);
            }

            envTex->bind(TextureUnit::TEXTURE0);
            shader.uniform1i("EnvTex", TextureUnit::TEXTURE0);
        }

        shader.uniform1i("Skybox", skybox);
        // Should be resolution of environment map for perfect accuracy, but this is good enough
        shader.uniform1i("textureSize", resolution * 4);

        glViewport(0, 0, resolution, resolution);

        for (int i = 0; i < 6; i++)
        {
            shader.uniform1i("Face", i);
            framebuffer.setCubemap(getHandle(), i, 0);
            framebuffer.validate();

            glBindVertexArray(RenderState::quadVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        framebuffer.release();
        framebuffer.destroy();
    }

    void PrefilterEnvmap::generate(const uint resolution)
    {
        Shader shader;
        shader.loadFromFile("res/Shaders/Quad.vert", "res/Shaders/PrefilterEnvmap.frag");

        Framebuffer framebuffer;
        framebuffer.create();
        framebuffer.bind();
        framebuffer.addDrawBuffer(GL_COLOR_ATTACHMENT0);

        glClearColor(1, 0, 0, 1);

        shader.bind();
        
        create();
        bind(TextureUnit::TEXTURE0);
        setWrapping(REPEAT, REPEAT, REPEAT);

        setMaxMipmapLevel(5);
        setSampling(LINEAR, LINEAR, LINEAR);

        if (skybox) {
            for (int i = 0; i < 6; i++) {
                setFace(i);
                setData(resolution, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            }
            generateMipmaps();
            glBindTexture(GL_TEXTURE_2D, 0);
            envMap->bind(TextureUnit::TEXTURE0);
            shader.uniform1i("EnvMap", TextureUnit::TEXTURE0);
        }
        else {
            for (int i = 0; i < 6; i++) {
                setFace(i);
                setData(resolution, GL_RGBA16F, GL_RGBA, GL_FLOAT, nullptr);
            }
            generateMipmaps();
            envTex->bind(TextureUnit::TEXTURE0);
            shader.uniform1i("EnvTex", TextureUnit::TEXTURE0);
        }

        shader.uniform1i("Skybox", skybox);

        const unsigned int MIP_MAP_LEVELS = 6;
        for (int level = 0; level < MIP_MAP_LEVELS; level++)
        {
            unsigned int mipmapSize = resolution >> level;
            glViewport(0, 0, mipmapSize, mipmapSize);
            float Roughness = (float)level / (MIP_MAP_LEVELS - 1);
            std::cout << "Roughness: " << Roughness << std::endl;
            shader.uniform1f("Roughness", Roughness);

            for (int i = 0; i < 6; i++)
            {
                shader.uniform1i("Face", i);
                framebuffer.setCubemap(getHandle(), i, level);
                framebuffer.validate();

                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }

        framebuffer.release();
    }

    ScaleBiasTexture::ScaleBiasTexture()
    {
        create();

        bind(TextureUnit::TEXTURE0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        setWrapping(CLAMP, CLAMP);

        setData(256, 256, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        release();
    }

    void ScaleBiasTexture::generate()
    {
        Shader shader;
        shader.loadFromFile("res/Shaders/Quad.vert", "res/Shaders/BRDFintegration.frag");

        Framebuffer framebuffer;
        framebuffer.create();
        framebuffer.bind();
        framebuffer.addDrawBuffer(GL_COLOR_ATTACHMENT0);

        glClearColor(1, 0, 0, 1);

        shader.bind();

        glViewport(0, 0, 256, 256);

        framebuffer.setTexture(GL_COLOR_ATTACHMENT0, *this);
        framebuffer.validate();

        glClear(GL_COLOR_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        framebuffer.release();
    }

    void IblSceneInfo::PrecomputeEnvironmentData(const Texture2D& environmentTex) {
        irradianceMap = new IrradianceMap(&environmentTex);
        irradianceMap->generate(32);

        prefilterEnvmap = new PrefilterEnvmap(&environmentTex);
        prefilterEnvmap->generate(512);

        scaleBiasTexture = new ScaleBiasTexture();
        scaleBiasTexture->generate();
    }

    void IblSceneInfo::PrecomputeEnvironmentData(const Skybox& skybox) {
        irradianceMap = new IrradianceMap(&skybox);
        irradianceMap->generate(32);

        prefilterEnvmap = new PrefilterEnvmap(&skybox);
        prefilterEnvmap->generate(512);

        scaleBiasTexture = new ScaleBiasTexture();
        scaleBiasTexture->generate();
    }
}
